#define _XOPEN_SOURCE_EXTENDED 1
#include <ncurses.h>
#include <unistd.h>
#include <locale.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <ctype.h>

#include "data.h"
#include "network.h"
#include "ui.h"

void handle_sigint(int sig)
{
	is_running = 0;
}

int main()
{
	setlocale(LC_ALL, "");
	signal(SIGINT, handle_sigint);
	load_watchlist();
	initscr();
	start_color();
	use_default_colors();
	init_pair(1, COLOR_GREEN, -1);
	init_pair(2, COLOR_RED, -1);
	
    noecho();
	cbreak();
	curs_set(0);
	timeout(100);
	keypad(stdscr, TRUE);

	pthread_t fetch_tid, news_tid;
	pthread_create(&fetch_tid, NULL, fetch_worker, NULL);
	pthread_create(&news_tid, NULL, news_worker, NULL);

	int max_y = 0, max_x = 0;
	int old_max_y = 0, old_max_x = 0;

    while(is_running)
    {
        getmaxyx(stdscr, max_y, max_x);
        
        if (max_y != old_max_y || max_x != old_max_x) {
            clear(); 
            old_max_y = max_y;
            old_max_x = max_x;
        } else {
            erase();
        }
		draw_layout(max_y, max_x);

		pthread_mutex_lock(&data_mutex);

		mvprintw(1, 2, "[ Watchlist ]");
		for(int i = 0; i < num_symbols; i++)
		{
			if (i == selected_idx) {
                attron(A_REVERSE);
                mvprintw(4 + i, 2, " > %-9s | %s ", watchlist[i].symbol, watchlist[i].price);
                attroff(A_REVERSE);
            }
			else
			{
				mvprintw(4 + i, 2, "%-9s | %s", watchlist[i].symbol, watchlist[i].price);
			}
		}

		int right_start = (max_x / 3) + 2;
		mvprintw(1, right_start, "[ Detailed Info ]");
		if (num_symbols > 0)
		{
			mvprintw(4, right_start, "Symbol: %s", watchlist[selected_idx].symbol);
			mvprintw(5, right_start, "Price : %s", watchlist[selected_idx].price);

            int chart_w = 50;
            int chart_h = 15;
			//draw_ascii_chart(7, right_start, watchlist[selected_idx].open_data, watchlist[selected_idx].close_data);
            //draw_braille_chart(6, right_start, chart_w, chart_h, watchlist[selected_idx].close_data, CHART_POINTS);
            draw_candlestick_chart(6, right_start, chart_w, chart_h, 
                                   watchlist[selected_idx].open_data,  // 시가 추가
                                   watchlist[selected_idx].close_data, // 종가
                                   CHART_POINTS);
        }

		pthread_mutex_unlock(&data_mutex);

        //int news_y = 12;
        int news_y = max_y - 6;
        
        mvhline(news_y, right_start - 1, ACS_HLINE, max_x - right_start);
        mvprintw(news_y + 1, right_start, "◆ [ Global Crypto News ] ◆");
		
		pthread_mutex_lock(&news_mutex);
        for(int i = 0; i < 3; i++) {
            if(strlen(news_headlines[i]) > 0) {
                mvprintw(news_y + 2 + i, right_start, "▪ %.60s...", news_headlines[i]);
            }
        }
        pthread_mutex_unlock(&news_mutex);

		mvprintw(max_y - 1, 2, " TermStock v0.7 | [a]dd  [d]elete  [q]uit | Logs active ");

		refresh();

		int ch = getch();

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
                
                echo();
                curs_set(1);
                timeout(-1);
                
                getnstr(new_sym, SYMBOL_LEN - 1);
                
                for(int i = 0; new_sym[i]; i++){
                    new_sym[i] = toupper(new_sym[i]);
                }
                
                if(strlen(new_sym) > 0) {
					pthread_mutex_lock(&data_mutex);

                    strcpy(watchlist[num_symbols].symbol, new_sym);
                    strcpy(watchlist[num_symbols].price, "Loading...");
					for(int c=0; c<CHART_POINTS; c++) watchlist[num_symbols].open_data[c] = 0.0;
                    num_symbols++;
                    save_watchlist();

					pthread_mutex_unlock(&data_mutex);
					write_log("User Event: Added symbol '%s'", new_sym);
                }
                
                noecho();      
                curs_set(0);   
                timeout(100);  
            }
        }

		else if (ch == 'd' || ch == 'D') {
            if (num_symbols > 0) {
                char del_sym[SYMBOL_LEN];
                mvprintw(max_y - 2, 2, "Enter symbol to delete: ");
                
                echo(); curs_set(1); timeout(-1);
                getnstr(del_sym, SYMBOL_LEN - 1);
                for(int i = 0; del_sym[i]; i++) del_sym[i] = toupper(del_sym[i]);
                
				pthread_mutex_lock(&data_mutex);

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
                    save_watchlist();

                    if (selected_idx >= num_symbols) {
                        if (num_symbols > 0) {
                            // 목록이 남아있으면 맨 마지막 종목으로 커서 이동
                            selected_idx = num_symbols - 1; 
                        } else {
                            // 목록이 텅 비어버렸으면 0으로 초기화
                            selected_idx = 0; 
                        }
                    }

					write_log("User Event: Deleted symbol '%s'", del_sym);
                }
                pthread_mutex_unlock(&data_mutex);
                noecho(); curs_set(0); timeout(100);
            }
        }
        
        else if (ch == 'q' || ch == 'Q') {
            is_running = 0;

			write_log("User Event: 'q' pressed. Initiating shutdown.");
        }

		usleep(100000);
	}

	pthread_join(fetch_tid, NULL);
	pthread_join(news_tid, NULL);
	pthread_mutex_destroy(&data_mutex);
    pthread_mutex_destroy(&news_mutex);
	pthread_mutex_destroy(&log_mutex);

	endwin();

	while(waitpid(-1, NULL, WNOHANG) > 0);

	printf("TermStock safely terminated. Check 'termstock.log' for details.\n");

	return 0;
}