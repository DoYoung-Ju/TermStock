#ifndef CHART_H
#define CHART_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>


// C++의 Class 속성을 모아둔 구조체
typedef struct {
    int term_w;    // 터미널 가로 칸 수
    int term_h;    // 터미널 세로 줄 수
    int px_w;      // 픽셀 가로 크기 (term_w * 2)
    int px_h;      // 픽셀 세로 크기 (term_h * 4)
    uint8_t* grid; // 1차원 픽셀 배열 (동적 할당용)
} Canvas;

// 점의 좌표를 기억할 구조체
typedef struct {
    int x;
    int y;
} Point2D;


// C++의 메서드들을 C 함수로 구현
Canvas* create_canvas(int term_w, int term_h);
void clear_canvas(Canvas* canvas);
void set_pixel(Canvas* canvas, int x, int y);
uint8_t get_pixel(Canvas* canvas, int x, int y);
void free_canvas(Canvas* canvas);

// 함수 원형 (메뉴판에 등록)
void map_coordinates(float* prices, int num_points, int px_w, int px_h, Point2D* out_points);

// 브레젠험 알고리즘으로 두 점 (x0, y0)와 (x1, y1) 사이에 선을 긋는 함수
void draw_line(Canvas* canvas, int x0, int y0, int x1, int y1);

// 배열에 담긴 20개의 좌표를 순서대로 이어주는 마스터 함수
void draw_chart_lines(Canvas* canvas, Point2D* points, int num_points);

// 8개의 픽셀 상태를 압축한 숫자(v)를 UTF-8 문자열로 변환하는 함수
void get_braille_char(uint8_t v, char* out_str);

// 압축된 캔버스 전체를 터미널 화면에 뿌려주는 최종 엔진 함수
void draw_canvas_to_screen(Canvas* canvas, int start_y, int start_x);

#endif