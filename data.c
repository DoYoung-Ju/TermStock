#include "data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <stdarg.h>

//extern 없이 진짜 변수를 만들고 초기값을 넣음!
volatile sig_atomic_t is_running = 1;
WatchlistItem watchlist[MAX_SYMBOLS];
int num_symbols = 0;
int selected_idx = 0;
char news_headlines[3][256] = {"Loading breaking news...", "", ""};
int last_fetched_news_idx = -1;

pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t news_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void write_log(const char* format, ...) {
    pthread_mutex_lock(&log_mutex); 
    int fd = open("termstock.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd != -1) {
        time_t now;
        time(&now);
        struct tm* local = localtime(&now);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "[%Y-%m-%d %H:%M:%S] ", local);
        if (write(fd, time_str, strlen(time_str)) < 0) {}
        
        char msg[512];
        va_list args;
        va_start(args, format);
        vsnprintf(msg, sizeof(msg), format, args);
        va_end(args);

        if (write(fd, msg, strlen(msg)) < 0) {}
        if (write(fd, "\n", 1) < 0) {}
        close(fd);
    }
    pthread_mutex_unlock(&log_mutex);
}

void save_watchlist() {
    int fd = open("watchlist.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd != -1) {
        for (int i = 0; i < num_symbols; i++) {
            if (write(fd, watchlist[i].symbol, strlen(watchlist[i].symbol)) < 0) { /* 경고 무시 */ }
            if (write(fd, "\n", 1) < 0) { /* 경고 무시 */ }
        }
        close(fd);
    }
}

void load_watchlist() {
    int fd = open("watchlist.txt", O_RDONLY);
    if (fd == -1) {
        strcpy(watchlist[0].symbol, "BTCUSDT");
        strcpy(watchlist[0].price, "Loading...");
        num_symbols = 1;
        write_log("System started. No watchlist.txt found. Defaulted to BTCUSDT.");
        return;
    }
    char buffer[1024] = {0};
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);
    if (bytes_read > 0) {
        char* token = strtok(buffer, "\n\r ");
        while (token != NULL && num_symbols < MAX_SYMBOLS) {
            strncpy(watchlist[num_symbols].symbol, token, SYMBOL_LEN - 1);
            watchlist[num_symbols].symbol[SYMBOL_LEN - 1] = '\0';
            strcpy(watchlist[num_symbols].price, "Loading...");
            for(int i=0; i<CHART_POINTS; i++) {
                watchlist[num_symbols].open_data[i] = 0.0;
                watchlist[num_symbols].close_data[i] = 0.0;
            }
            num_symbols++;
            token = strtok(NULL, "\n\r ");
        }
        write_log("System started. Loaded %d symbols.", num_symbols);
    }
}