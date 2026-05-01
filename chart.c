#include <ncurses.h>
#include "chart.h"

// 1. 캔버스 생성자 (Constructor 역할)
Canvas* create_canvas(int term_w, int term_h) {
    Canvas* canvas = (Canvas*)malloc(sizeof(Canvas));
    canvas->term_w = term_w;
    canvas->term_h = term_h;
    canvas->px_w = term_w * 2;
    canvas->px_h = term_h * 4;
    
    // 픽셀 개수만큼 메모리 할당 후 0으로 초기화 (calloc)
    canvas->grid = (uint8_t*)calloc(canvas->px_w * canvas->px_h, sizeof(uint8_t));
    return canvas;
}

// 2. 도화지 초기화
void clear_canvas(Canvas* canvas) {
    if (canvas && canvas->grid) {
        memset(canvas->grid, 0, canvas->px_w * canvas->px_h * sizeof(uint8_t));
    }
}

// 3. 점 찍기 로직 (핵심)
void set_pixel(Canvas* canvas, int x, int y) {
    if (!canvas || !canvas->grid) return;
    
    // 안전 장치: 좌표가 캔버스 범위를 벗어나면 점을 찍지 않음
    if (x < 0 || x >= canvas->px_w || y < 0 || y >= canvas->px_h) {
        return; 
    }
    
    // (y * 너비) + x 공식으로 1차원 배열 인덱스 접근
    canvas->grid[y * canvas->px_w + x] = 1;
}

// 4. 특정 좌표의 픽셀 상태 읽기
uint8_t get_pixel(Canvas* canvas, int x, int y) {
    if (!canvas || !canvas->grid) return 0;
    if (x < 0 || x >= canvas->px_w || y < 0 || y >= canvas->px_h) return 0;
    
    return canvas->grid[y * canvas->px_w + x];
}

// 5. 소멸자 (Destructor 역할)
void free_canvas(Canvas* canvas) {
    if (canvas) {
        if (canvas->grid) free(canvas->grid);
        free(canvas);
    }
}

// 데이터 배열을 받아서 Point2D 배열(out_points)에 좌표를 꽉꽉 채워주는 함수
void map_coordinates(float* prices, int num_points, int px_w, int px_h, Point2D* out_points) {
    if (num_points <= 0 || px_w == 0 || px_h == 0) return;

    // 1. 최고가, 최저가 찾기
    float min_price = prices[0];
    float max_price = prices[0];
    for (int i = 1; i < num_points; i++) {
        if (prices[i] < min_price) min_price = prices[i];
        if (prices[i] > max_price) max_price = prices[i];
    }
    
    float range = max_price - min_price;
    if (range == 0.0f) range = 1.0f;

    // X축 픽셀 이동 간격
    float x_step = (float)(px_w - 1) / (num_points > 1 ? num_points - 1 : 1);

    // 2. 픽셀 좌표 매핑
    for (int i = 0; i < num_points; i++) {
        // 반올림을 위해 +0.5f 추가
        int px_x = (int)(i * x_step + 0.5f); 
        
        float normalized_y = (prices[i] - min_price) / range;
        
        // 💡 y=0이 화면 꼭대기이므로 뒤집기
        int px_y = (int)((1.0f - normalized_y) * (px_h - 1) + 0.5f);

        // 안전 장치
        if (px_x >= px_w) px_x = px_w - 1;
        if (px_y >= px_h) px_y = px_h - 1;
        if (px_y < 0) px_y = 0;

        out_points[i].x = px_x;
        out_points[i].y = px_y;
    }
}


// 1. 브레젠험 직선 알고리즘 (코어 엔진)
void draw_line(Canvas* canvas, int x0, int y0, int x1, int y1) {
    if (!canvas) return;

    // x와 y가 이동해야 할 절대 거리
    int dx = abs(x1 - x0);
    int dy = -abs(y1 - y0);
    
    // 이동 방향 (오른쪽/왼쪽, 아래/위)
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    
    // 누적 오차값
    int err = dx + dy;
    int e2;

    while (1) {
        // 현재 위치에 점 찍기 (Phase 1에서 만든 함수 재활용!)
        set_pixel(canvas, x0, y0);

        // 목표 지점에 도달하면 루프 탈출
        if (x0 == x1 && y0 == y1) break;

        e2 = 2 * err;

        // X축 이동이 필요할 때
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        // Y축 이동이 필요할 때
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// 2. 20개의 점을 순서대로 이어주는 래퍼(Wrapper) 함수
void draw_chart_lines(Canvas* canvas, Point2D* points, int num_points) {
    if (!canvas || !points || num_points < 2) return;

    // 0번 점과 1번 점, 1번 점과 2번 점... 순서대로 선을 그음
    for (int i = 0; i < num_points - 1; i++) {
        draw_line(canvas, points[i].x, points[i].y, points[i+1].x, points[i+1].y);
    }
}
// 1. 비트 마스킹 결과를 3바이트 UTF-8 점자 문자로 인코딩하는 마법의 공식
void get_braille_char(uint8_t v, char* out_str) {
    // 점자 유니코드 시작점 U+2800을 UTF-8 3바이트로 분할하여 v값을 더함
    out_str[0] = 0xE2; 
    out_str[1] = 0xA0 | (v >> 6); 
    out_str[2] = 0x80 | (v & 0x3F); 
    out_str[3] = '\0'; // 문자열 끝
}

// 2. 캔버스를 2x4 블록으로 스캔하며 터미널에 점자를 찍는 최종 엔진
void draw_canvas_to_screen(Canvas* canvas, int start_y, int start_x) {
    if (!canvas) return;

    char braille[4]; // 3바이트 문자 + NULL 방 1개

    // 가로/세로 픽셀이 아닌 '터미널의 칸 수' 단위로 루프를 돎
    for (int ty = 0; ty < canvas->term_h; ty++) {
        for (int tx = 0; tx < canvas->term_w; tx++) {
            
            // 터미널 1칸 = 가상 캔버스에서는 가로 2 x 세로 4 픽셀
            int px = tx * 2;
            int py = ty * 4;
            uint8_t v = 0; // 8개 점의 상태를 담을 바구니 (0으로 초기화)

            // 2x4 범위의 픽셀을 검사해서 켜져(1) 있으면 정해진 비트값을 합침(|)
            if (get_pixel(canvas, px, py))         v |= 0x01; // 점 1
            if (get_pixel(canvas, px, py + 1))     v |= 0x02; // 점 2
            if (get_pixel(canvas, px, py + 2))     v |= 0x04; // 점 3
            if (get_pixel(canvas, px + 1, py))     v |= 0x08; // 점 4
            if (get_pixel(canvas, px + 1, py + 1)) v |= 0x10; // 점 5
            if (get_pixel(canvas, px + 1, py + 2)) v |= 0x20; // 점 6
            if (get_pixel(canvas, px, py + 3))     v |= 0x40; // 점 7
            if (get_pixel(canvas, px + 1, py + 3)) v |= 0x80; // 점 8

            // 만들어진 비트값을 점자 문자열(예: "⣯")로 변환
            get_braille_char(v, braille);

            // 터미널의 (start_y + ty, start_x + tx) 좌표에 출력!
            // btop처럼 초록색을 입히고 싶다면 attron/attroff를 써도 좋아.
            attron(COLOR_PAIR(1));
            mvprintw(start_y + ty, start_x + tx, "%s", braille);
            attroff(COLOR_PAIR(1));
        }
    }
}