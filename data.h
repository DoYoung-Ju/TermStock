#ifndef DATA_H
#define DATA_H

#include <pthread.h>
#include <signal.h>

#define MAX_SYMBOLS 15
#define SYMBOL_LEN 20
#define CHART_POINTS 20

typedef struct {
    char symbol[SYMBOL_LEN];
    char price[50];
    float open_data[CHART_POINTS];
    float close_data[CHART_POINTS];
} WatchlistItem;

//extern을 써서 다른 파일과 공유할 전역 변수들 선언
extern volatile sig_atomic_t is_running;
extern WatchlistItem watchlist[MAX_SYMBOLS];
extern int num_symbols;
extern int selected_idx;
extern char news_headlines[3][256];
extern int last_fetched_news_idx;

// 뮤텍스(자물쇠) 공유
extern pthread_mutex_t data_mutex;
extern pthread_mutex_t news_mutex;
extern pthread_mutex_t log_mutex;

// 파일 I/O 및 로깅 함수 원형
void load_watchlist();
void save_watchlist();
void write_log(const char* format, ...);

#endif