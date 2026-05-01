#ifndef NETWORK_H
#define NETWORK_H

#include "data.h"

void parse_klines(char* buffer, float* open_out, float* close_out);
void fetch_price(const char* symbol, char* price_out, float* open_out, float* close_out);
void fetch_news(char result[3][256]);
void* fetch_worker(void* arg);
void* news_worker(void* arg);

#endif