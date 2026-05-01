#include "ui.h"
#include "chart.h"
#include<ncurses.h>

const char* chart_bars[] = {" ", "▂", "▃", "▄", "▅", "▆", "▇", "█"};

void draw_layout(int max_y, int max_x)
{
	box(stdscr, 0, 0);

	int divider_x = max_x / 3;
	mvvline(1, divider_x, ACS_VLINE, max_y - 2);

	mvhline(2, 1, ACS_HLINE, max_x - 2);

	mvaddch(2, 0, ACS_LTEE);
	mvaddch(2, max_x - 1, ACS_RTEE);
	mvaddch(0, divider_x, ACS_TTEE);
	mvaddch(max_y - 1, divider_x, ACS_BTEE);
	mvaddch(2, divider_x, ACS_PLUS);
}

void draw_ascii_chart(int y, int x, float* open_data, float* close_data) {
    if (close_data[0] == 0.0) {
        mvprintw(y, x, "Loading Chart Data...");
        return;
    }

    float min_val = close_data[0], max_val = close_data[0];
    for (int i = 1; i < CHART_POINTS; i++) {
        if (close_data[i] < min_val) min_val = close_data[i];
        if (close_data[i] > max_val) max_val = close_data[i];
    }

    float range = max_val - min_val;
    if (range == 0) range = 1.0; 

    mvprintw(y, x, "24H Chart: ");
    int start_x = x + 11;

    for (int i = 0; i < CHART_POINTS; i++) {
        float normalized = (close_data[i] - min_val) / range;
        int level = (int)(normalized * 7.99); // 0 ~ 7 인덱스 생성
        if (level < 0) level = 0;
        if (level > 7) level = 7;

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
    
    mvprintw(y + 2, x, "High: %.2f  |  Low: %.2f", max_val, min_val);
}
void draw_braille_chart(int start_y, int start_x, int width, int height, float* prices, int num_points) {
    if (prices[0] == 0.0) {
        mvprintw(start_y, start_x, "Loading Chart Data...");
        return;
    }

    // 1. 도화지(Canvas) 생성 (터미널 가로/세로 칸 수 기준)
    Canvas* canvas = create_canvas(width, height);

    // 2. 매핑된 좌표를 담을 배열 생성
    Point2D points[CHART_POINTS];

    // 3. 가격 데이터 -> 픽셀 좌표로 변환 (Phase 2)
    map_coordinates(prices, num_points, canvas->px_w, canvas->px_h, points);

    // 4. 점과 점 사이를 부드러운 선으로 연결 (Phase 3)
    draw_chart_lines(canvas, points, num_points);

    // 5. 완성된 도화지를 점자로 인코딩하여 화면에 쫙 뿌림! (Phase 4)
    draw_canvas_to_screen(canvas, start_y, start_x);

    // 6. 메모리 누수 방지 (가장 중요 🚨)
    // 매 프레임마다 그리기 때문에, 다 그렸으면 반드시 도화지를 찢어(free) 없애야 해!
    free_canvas(canvas);

    // 7. (디테일) 최고가/최저가 라벨 표시
    float min_val = prices[0], max_val = prices[0];
    for (int i = 1; i < num_points; i++) {
        if (prices[i] < min_val) min_val = prices[i];
        if (prices[i] > max_val) max_val = prices[i];
    }
    // 차트 맨 윗줄 오른쪽 끝에 최고가, 맨 아랫줄 오른쪽 끝에 최저가
    mvprintw(start_y, start_x + width + 2, "Max: %.2f", max_val);
    mvprintw(start_y + height - 1, start_x + width + 2, "Min: %.2f", min_val);
}
// 시가와 종가를 비교해 색상 있는 캔들(기둥)을 그리는 함수
void draw_candlestick_chart(int start_y, int start_x, int width, int height, float* open_data, float* close_data, int num_points) {
    if (num_points <= 0 || close_data[0] == 0.0) return;

    // 1. 스케일링을 위한 최고가/최저가 탐색 (시가, 종가 모두 검사)
    float min_val = open_data[0], max_val = open_data[0];
    for (int i = 0; i < num_points; i++) {
        if (open_data[i] < min_val) min_val = open_data[i];
        if (close_data[i] < min_val) min_val = close_data[i];
        if (open_data[i] > max_val) max_val = open_data[i];
        if (close_data[i] > max_val) max_val = close_data[i];
    }
    float range = max_val - min_val;
    if (range == 0.0f) range = 1.0f;

    float x_step = (float)(width - 1) / (num_points > 1 ? num_points - 1 : 1);

    // 2. 데이터 순회하며 캔들 기둥 세우기
    for (int i = 0; i < num_points; i++) {
        int x = start_x + (int)(i * x_step);
        
        // 시가와 종가의 Y 픽셀 좌표 계산
        int y_open = start_y + (int)((1.0f - (open_data[i] - min_val) / range) * (height - 1));
        int y_close = start_y + (int)((1.0f - (close_data[i] - min_val) / range) * (height - 1));

        // 💡 핵심: 양봉(Green)인지 음봉(Red)인지 판별! (코인판은 상승이 초록, 하락이 빨강)
        int color_pair = (close_data[i] >= open_data[i]) ? 1 : 2;

        // 위에서 아래로 기둥을 칠하기 위해 top과 bottom 정렬
        int top_y = (y_open < y_close) ? y_open : y_close;
        int bottom_y = (y_open > y_close) ? y_open : y_close;

        // 색깔 장전!
        attron(COLOR_PAIR(color_pair));
        
        // top부터 bottom까지 꽉 찬 블록(█)으로 기둥을 그림
        for (int y = top_y; y <= bottom_y; y++) {
            mvprintw(y, x, "█"); 
        }
        
        attroff(COLOR_PAIR(color_pair));
    }

    // 3. 최저/최고가 라벨 (색깔 빼고 하얀색으로)
    mvprintw(start_y, start_x + width + 2, "Max: %.2f", max_val);
    mvprintw(start_y + height - 1, start_x + width + 2, "Min: %.2f", min_val);
}