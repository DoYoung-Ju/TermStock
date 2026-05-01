#ifndef UI_H
#define UI_H

#include <ncurses.h>
#include "data.h"

void draw_layout(int max_y, int max_x);
void draw_ascii_chart(int y, int x, float* open_data, float* close_data);
void draw_braille_chart(int start_y, int start_x, int width, int height, float* prices, int num_points);
void draw_candlestick_chart(int start_y, int start_x, int width, int height, float* open_data, float* close_data, int num_points);

#endif