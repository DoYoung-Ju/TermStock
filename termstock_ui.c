#define _XOPEN_SOURCE_EXTENDED 1  
#include <ncurses.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <pthread.h>
#include <locale.h>
#include <time.h>
#include <stdarg.h>


#define MAX_SYMBOLS 15
#define SYMBOL_LEN 20
#define CHART_POINTS 20

typedef struct {
    char symbol[SYMBOL_LEN];
    char price[50];
	float open_data[CHART_POINTS];
	float close_data[CHART_POINTS];
} WatchlistItem;

//시그널 핸들러 안에서 안전하게 값을 바꿀 수 있는 타입
volatile sig_atomic_t is_running = 1;

WatchlistItem watchlist[MAX_SYMBOLS];
int num_symbols = 0;

//데이터를 보호할 자물쇠(Mutex) 생성
pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;

//로그 쓰기 전용 자물쇠
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

const char* chart_bars[] = {" ", "▂", "▃", "▄", "▅", "▆", "▇", "█"};

//현재 커서가 위치한 종목 인덱스
int selected_idx = 0;
char news_headlines[3][256] = {"Loading breaking news...", "", ""};

// 뉴스를 중복해서 가져오지 않도록 기억하는 변수
int last_fetched_news_idx = -1;

// 뉴스 전용 자물쇠
pthread_mutex_t news_mutex = PTHREAD_MUTEX_INITIALIZER;

void handle_sigint(int sig)
{
	is_running = 0;
}

//시스템 로그 기록 함수
void write_log(const char* format, ...) {
    pthread_mutex_lock(&log_mutex); // 로그 파일에 동시 접근 방지
    
    // O_APPEND: 파일 맨 끝에 이어서 씀
    int fd = open("termstock.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd != -1) {
        // 1. 현재 시간 구하기
        time_t now;
        time(&now);
        struct tm* local = localtime(&now);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "[%Y-%m-%d %H:%M:%S] ", local);
        write(fd, time_str, strlen(time_str)); // 시간 기록

        // 2. 가변 인자를 이용해 메시지 포맷팅 (printf 처럼 작동)
        char msg[512];
        va_list args;
        va_start(args, format);
        vsnprintf(msg, sizeof(msg), format, args);
        va_end(args);

        // 3. 메시지 쓰고 줄바꿈
        write(fd, msg, strlen(msg));
        write(fd, "\n", 1);
        close(fd);
    }
    
    pthread_mutex_unlock(&log_mutex);
}

//리스트를 파일에 덮어서 저장하는 함수
void save_watchlist() {
    // O_TRUNC: 기존 파일 내용을 싹 지우고 새로 씀
    // O_CREAT: 파일이 없으면 만듦 (권한 0644)
    int fd = open("watchlist.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd != -1) {
        for (int i = 0; i < num_symbols; i++) {
            write(fd, watchlist[i].symbol, strlen(watchlist[i].symbol));
            write(fd, "\n", 1);
        }
        close(fd);
    }
}

void load_watchlist()
{
	int fd = open("watchlist.txt", O_RDONLY);
	if (fd == -1)
	{
		strcpy(watchlist[0].symbol, "BTCUSDT");
        strcpy(watchlist[0].price, "Loading...");
        num_symbols = 1;
		write_log("System started. No watchlist.txt found. Defaulted to BTCUSDT.");
        return;
	}

	char buffer[1024] = {0};

	//파일 전체 읽기
	ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
	close(fd);

	if (bytes_read > 0)
	{
		//줄바꿈을 기준으로 문자열 쪼개기
		char* token = strtok(buffer, "\n\r ");
		while (token != NULL && num_symbols < MAX_SYMBOLS)
		{
			strncpy(watchlist[num_symbols].symbol, token, SYMBOL_LEN - 1);
            watchlist[num_symbols].symbol[SYMBOL_LEN - 1] = '\0';
            strcpy(watchlist[num_symbols].price, "Loading...");
			for(int i=0; i<CHART_POINTS; i++)
			{
				watchlist[num_symbols].open_data[i] = 0.0;
				watchlist[num_symbols].close_data[i] = 0.0;
			}
            num_symbols++;
            token = strtok(NULL, "\n\r ");
		}
		write_log("System started. Loaded %d symbols from watchlist.txt.", num_symbols);
	}
}

//테두리와 화면 분할 선을 그리는 함수
void draw_layout(int max_y, int max_x)
{
	//1. 전체 테두리 그리기
	box(stdscr, 0, 0);

	//2. 화면 세로로 나누기 (왼쪽 1/3 지점에 선 긋기)
	int divider_x = max_x / 3;
	mvvline(1, divider_x, ACS_VLINE, max_y - 2);

	//3. 상단에 가로선 하나 긋기(타이틀과 내용 분리)
	mvhline(2, 1, ACS_HLINE, max_x - 2);

	//4. 교차점 다듬기
	mvaddch(2, 0, ACS_LTEE);
	mvaddch(2, max_x - 1, ACS_RTEE);
	mvaddch(0, divider_x, ACS_TTEE);
	mvaddch(max_y - 1, divider_x, ACS_BTEE);
	mvaddch(2, divider_x, ACS_PLUS);
}

//바이낸스 Klines API 파싱 함수
void parse_klines(char* buffer, float* open_out, float* close_out) {
    char* ptr = buffer;
    for(int i = 0; i < CHART_POINTS; i++) {
        ptr = strchr(ptr, '['); 
        if(!ptr) break;
        ptr++;

		ptr = strchr(ptr, ',');
		if(ptr)
		{
			ptr += 2;
			open_out[i] = atof(ptr);
		}

        // 종가(Close Price)는 4번째 쉼표 뒤에 위치함
        for(int c = 0; c < 3; c++) {
            ptr = strchr(ptr, ',');
            if(ptr) ptr++;
        }
        if(ptr) {
            ptr++; // 큰따옴표 건너뛰기
            close_out[i] = atof(ptr); // 문자열을 float으로 변환
        }
    }
}

//API에서 가격을 긁어오는 함수
void fetch_price(const char* symbol, char* price_out, float* open_out, float* close_out)
{
	//파이프 생성
	int pipe_fd[2];
	if (pipe(pipe_fd) == -1) return;

	//프로세스 복제
	pid_t pid = fork();

	if(pid == 0)
	{
		//자식 프로세스
		//읽기 구멍은 닫기
		close(pipe_fd[0]);

		//표준 출력을 파이프의 쓰기 구멍으로 연결
		dup2(pipe_fd[1], STDOUT_FILENO);
		close(pipe_fd[1]);

		char url[256];
		sprintf(url, "https://api.binance.com/api/v3/klines?symbol=%s&interval=1h&limit=%d", symbol, CHART_POINTS);

		execlp("curl", "curl", "-s", url, NULL);
		exit(1);
	}
	else if(pid > 0)
	{
		//부모 프로세스
		//쓰기 구멍은 닫기
		close(pipe_fd[1]);

		//파이프에서 데이터 읽기
		char buffer[4096] = {0};
		int n = read(pipe_fd[0], buffer, sizeof(buffer) - 1);
		close(pipe_fd[0]);

		waitpid(pid, NULL, 0);

		//JSON 파서가 없으므로 문자열 검색으로 가격만 파싱
		if(n > 0)
		{
			parse_klines(buffer, open_out, close_out);

			// 마지막 데이터(가장 최근 종가)를 문자열 가격으로 포맷팅
			if (close_out[CHART_POINTS-1] > 0)
			{
				sprintf(price_out, "$ %.2f", close_out[CHART_POINTS-1]);
			}
		}
	}
}

//리눅스 쉘(sh) 명령어를 직접 실행해서 뉴스 제목만 파싱해오는 함수
void fetch_news(char result[3][256]) {
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) return;
    
    pid_t pid = fork();
    if (pid == 0) {
        close(pipe_fd[0]);
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[1]);
        
        // CryptoCompare 무료 API 호출 후 -> 제목만 grep -> 3줄 자르기 -> 깔끔하게 문자열 다듬기
        // (OS 명령어 파이프라인의 활용을 보여줌)
        char* cmd = "curl -s 'https://api.rss2json.com/v1/api.json?rss_url=https://cointelegraph.com/rss' | "
                    "grep -o '\"title\":\"[^\"]*\"' | head -n 3 | "
                    "awk -F'\":\"' '{print $2}' | tr -d '\"'";
        
        execlp("sh", "sh", "-c", cmd, NULL);
        exit(1);
    } else if (pid > 0) {
        close(pipe_fd[1]);
        char buffer[2048] = {0};
        read(pipe_fd[0], buffer, sizeof(buffer) - 1);
        close(pipe_fd[0]);
        waitpid(pid, NULL, 0);

        // 엔터(\n)를 기준으로 문자열 3줄을 쪼개서 배열에 저장
        char* token = strtok(buffer, "\n");
        for(int i = 0; i < 3 && token != NULL; i++) {
            strncpy(result[i], token, 255);
            token = strtok(NULL, "\n");
        }
    }
}

//배열 데이터를 정규화해서 터미널에 차트로 그리는 엔진
void draw_ascii_chart(int y, int x, float* open_data, float* close_data) {
    if (close_data[0] == 0.0) {
        mvprintw(y, x, "Loading Chart Data...");
        return;
    }

    // 1. 최소/최대값 찾기
    float min_val = close_data[0], max_val = close_data[0];
    for (int i = 1; i < CHART_POINTS; i++) {
        if (close_data[i] < min_val) min_val = close_data[i];
        if (close_data[i] > max_val) max_val = close_data[i];
    }

    float range = max_val - min_val;
    if (range == 0) range = 1.0; 

    // 2. 차트 렌더링
    mvprintw(y, x, "24H Chart: ");
    int start_x = x + 11;

    for (int i = 0; i < CHART_POINTS; i++) {
        float normalized = (close_data[i] - min_val) / range;
        int level = (int)(normalized * 7.99); // 0 ~ 7 인덱스 생성
        if (level < 0) level = 0;
        if (level > 7) level = 7;

		//양봉/음봉 판단 로직
		if (close_data[i] >= open_data[i]) {
            attron(COLOR_PAIR(1)); // 초록색 ON
            mvprintw(y, start_x + (i * 2), "%s", chart_bars[level]);
            attroff(COLOR_PAIR(1)); // 초록색 OFF
        } else {
            attron(COLOR_PAIR(2)); // 빨간색 ON
            mvprintw(y, start_x + (i * 2), "%s", chart_bars[level]);
            attroff(COLOR_PAIR(2)); // 빨간색 OFF
        }
	}
    
    // 3. 디테일 추가: 최고점/최저점 수치 출력
    mvprintw(y + 2, x, "High: %.2f  |  Low: %.2f", max_val, min_val);
}

//백그라운드에서 계속 가격만 알아오는 스레드 함수
void* fetch_worker(void* arg) {
    while(is_running) {
        for (int i = 0; i < num_symbols; i++) {
            char temp_symbol[SYMBOL_LEN];
            char temp_price[50] = "Error...";
			float temp_open[CHART_POINTS] = {0.0};
			float temp_close[CHART_POINTS] = {0.0};

            // 1. 읽을 때 자물쇠 잠그고 심볼 복사 (UI 스레드가 못 건드리게)
            pthread_mutex_lock(&data_mutex);
            if (i < num_symbols) strcpy(temp_symbol, watchlist[i].symbol);
            pthread_mutex_unlock(&data_mutex);

            // 2. 네트워크 I/O (가장 오래 걸리는 작업이므로 자물쇠 푼 상태로 실행!)
            if (strlen(temp_symbol) > 0) {
                fetch_price(temp_symbol, temp_price, temp_open, temp_close);

                // 3. 쓸 때 다시 자물쇠 잠그고 가격 업데이트
                pthread_mutex_lock(&data_mutex);
                if (i < num_symbols && strcmp(watchlist[i].symbol, temp_symbol) == 0) {
                    strcpy(watchlist[i].price, temp_price);
					for(int c=0; c<CHART_POINTS; c++) {
                        watchlist[i].open_data[c] = temp_open[c];
                        watchlist[i].close_data[c] = temp_close[c];
					}
                }
                pthread_mutex_unlock(&data_mutex);

				//통신 성공 시 백그라운드 로깅
				if (temp_close[CHART_POINTS-1] > 0) {
                    write_log("Fetch Success: %s -> %s", temp_symbol, temp_price);
                } else {
                    write_log("Fetch Failed: API error for %s", temp_symbol);
                }
            }
        }
        sleep(5);
    }
	write_log("Worker thread terminated.");
    return NULL;
}

//커서가 움직일 때마다 뉴스를 갱신하는 백그라운드 스레드
void* news_worker(void* arg) {
    while(is_running) {
        int current_idx;
        pthread_mutex_lock(&data_mutex);
        current_idx = selected_idx; // UI 스레드에서 변경한 커서 위치를 읽어옴
        pthread_mutex_unlock(&data_mutex);

        // 커서 위치가 바뀌었을 때만 API를 새로 찔러서 뉴스를 갱신
        if (current_idx != last_fetched_news_idx && num_symbols > 0) {
            char temp_news[3][256] = {"No recent news.", "", ""};
            
            fetch_news(temp_news); // 네트워크 통신 (시간 소요)

            pthread_mutex_lock(&news_mutex);
            for(int i=0; i<3; i++) strcpy(news_headlines[i], temp_news[i]);
            last_fetched_news_idx = current_idx;
            pthread_mutex_unlock(&news_mutex);
            
            write_log("News updated for list index %d", current_idx);
        }
        usleep(300000); // 0.3초마다 커서가 움직였는지 확인
    }
    return NULL;
}

int main()
{
	//차트 블록 문자 안 깨지게 해주는 시스템 로케일
	setlocale(LC_ALL, "");

	signal(SIGINT, handle_sigint);
	load_watchlist();

	//1. ncurses 초기화
	initscr();

	start_color();
	use_default_colors();
	init_pair(1, COLOR_GREEN, -1);
	init_pair(2, COLOR_RED, -1);

	noecho();
	cbreak();
	curs_set(0);
	timeout(100);

	//방향키를 인식하게 설정
	keypad(stdscr, TRUE);

	//백그라운드 스레드 생성
	pthread_t fetch_tid, news_tid;
	pthread_create(&fetch_tid, NULL, fetch_worker, NULL);
	pthread_create(&news_tid, NULL, news_worker, NULL);

	int max_y = 0, max_x = 0;

	//2. 메인 렌더링 루프
	while(is_running)
	{
		getmaxyx(stdscr, max_y, max_x);
		clear();
		draw_layout(max_y, max_x);

		//fetch_price를 부를 필요 없이 스레드가 물어온 데이터를 화면에 그리기만 함
		pthread_mutex_lock(&data_mutex);

		//왼쪽 패널: 관심종목
		mvprintw(1, 2, "[ Watchlist ]");
		for(int i = 0; i < num_symbols; i++)
		{
			if (i == selected_idx) {
                attron(A_REVERSE); //커서가 있는 줄은 색상을 반전(하이라이트)
                mvprintw(4 + i, 2, " > %-9s | %s ", watchlist[i].symbol, watchlist[i].price);
                attroff(A_REVERSE);
            }
			else
			{
				mvprintw(4 + i, 2, "%-9s | %s", watchlist[i].symbol, watchlist[i].price);
			}
		}

		//오른쪽 패널: 상세차트
		int right_start = (max_x / 3) + 2;
		mvprintw(1, right_start, "[ Detailed Info ]");
		if (num_symbols > 0)
		{
			mvprintw(4, right_start, "Symbol: %s", watchlist[selected_idx].symbol);
			mvprintw(5, right_start, "Price : %s", watchlist[selected_idx].price);

			//첫 번째 종목의 차트를 화면 오른쪽에 그리기
			draw_ascii_chart(7, right_start, watchlist[selected_idx].open_data, watchlist[selected_idx].close_data);
		}

		//다 그렸으니 잠금 해제
		pthread_mutex_unlock(&data_mutex);

		// --- 오른쪽 패널 하단 (뉴스) ---
        int news_y = 12; // 차트 아래쪽에 가로선 긋기
        mvhline(news_y, right_start - 1, ACS_HLINE, max_x - right_start);
        mvprintw(news_y + 1, right_start, "◆ [ Global Crypto News ] ◆");
		
		pthread_mutex_lock(&news_mutex);
        for(int i = 0; i < 3; i++) {
            if(strlen(news_headlines[i]) > 0) {
                // 긴 뉴스가 화면을 뚫고 나가지 않게 글자수 제한(%.60s) 적용
                mvprintw(news_y + 3 + (i * 2), right_start, "▪ %.60s...", news_headlines[i]);
            }
        }
        pthread_mutex_unlock(&news_mutex);

		//하단 안내문
		mvprintw(max_y - 1, 2, " TermStock v0.7 | [a]dd  [d]elete  [q]uit | Logs active ");

		refresh();

		//키 입력 처리
		int ch = getch();

		//위/아래 방향키를 누르면 인덱스 증감
		if (ch == KEY_UP && selected_idx > 0) {
            selected_idx--;
        }
        else if (ch == KEY_DOWN && selected_idx < num_symbols - 1) {
            selected_idx++;
        }

		if (ch == 'a' || ch == 'A') {
            if (num_symbols < MAX_SYMBOLS) {
                char new_sym[SYMBOL_LEN];
                mvprintw(max_y - 2, 2, "Enter new symbol (e.g., XRPUSDT): ");
                
                // 터미널 입력 모드 전환
                echo();
                curs_set(1);   // 커서 깜빡임 켬
                timeout(-1);   // 엔터 칠 때까지 무한 대기 (화면 멈춤)
                
                getnstr(new_sym, SYMBOL_LEN - 1); // 문자열 입력 받기
                
                // 대문자로 변환
                for(int i = 0; new_sym[i]; i++){
                    new_sym[i] = toupper(new_sym[i]);
                }
                
                // 구조체 배열에 새 종목 추가
                if(strlen(new_sym) > 0) {
					//구조체 건드릴 땐 무조건 잠금
					pthread_mutex_lock(&data_mutex);

                    strcpy(watchlist[num_symbols].symbol, new_sym);
                    strcpy(watchlist[num_symbols].price, "Loading...");
					for(int c=0; c<CHART_POINTS; c++) watchlist[num_symbols].open_data[c] = 0.0;
                    num_symbols++;
                    save_watchlist(); // 변경 즉시 파일에 저장!

					pthread_mutex_unlock(&data_mutex);
					write_log("User Event: Added symbol '%s'", new_sym);
                }
                
                // 원래 UI 모드로 복귀
                noecho();      
                curs_set(0);   
                timeout(100);  
            }
        }

		//종목 삭제 로직
		else if (ch == 'd' || ch == 'D') {
            if (num_symbols > 0) {
                char del_sym[SYMBOL_LEN];
                mvprintw(max_y - 2, 2, "Enter symbol to delete: ");
                
                echo(); curs_set(1); timeout(-1);
                getnstr(del_sym, SYMBOL_LEN - 1);
                for(int i = 0; del_sym[i]; i++) del_sym[i] = toupper(del_sym[i]);
                
				pthread_mutex_lock(&data_mutex);

                // 배열에서 종목 찾아 지우고 앞으로 한 칸씩 당기기
                int target_idx = -1;
                for(int i = 0; i < num_symbols; i++) {
                    if(strcmp(watchlist[i].symbol, del_sym) == 0) {
                        target_idx = i; break;
                    }
                }
                
                if(target_idx != -1) {
                    for(int i = target_idx; i < num_symbols - 1; i++) {
                        watchlist[i] = watchlist[i+1];
                    }
                    num_symbols--;
                    save_watchlist(); // 변경 즉시 파일에 저장

					write_log("User Event: Deleted symbol '%s'", del_sym);
                }
                pthread_mutex_unlock(&data_mutex);
                noecho(); curs_set(0); timeout(100);
            }
        }
        
        else if (ch == 'q' || ch == 'Q') {
            is_running = 0;

			//종료 로깅
			write_log("User Event: 'q' pressed. Initiating shutdown.");
        }

		usleep(100000);
	}

	//종료 처리: 메인 루프가 끝나면 스레드도 안전하게 종료될 때까지 기다려줌
	pthread_join(fetch_tid, NULL);
	pthread_join(news_tid, NULL);
	pthread_mutex_destroy(&data_mutex); // 자물쇠 반납
    pthread_mutex_destroy(&news_mutex);
	pthread_mutex_destroy(&log_mutex); // 자물쇠 파괴

	endwin();

	//백그라운드에 남아있는 자식 프로세스 모두 수거
	while(waitpid(-1, NULL, WNOHANG) > 0);

	printf("TermStock safely terminated. Check 'termstock.log' for details.\n");

	return 0;
}