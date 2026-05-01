#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <fcntl.h>

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

        for(int c = 0; c < 3; c++) {
            ptr = strchr(ptr, ',');
            if(ptr) ptr++;
        }
        if(ptr) {
            ptr++;
            close_out[i] = atof(ptr);
        }
    }
}
void fetch_price(const char* symbol, char* price_out, float* open_out, float* close_out)
{
	int pipe_fd[2];
	if (pipe(pipe_fd) == -1) return;

	pid_t pid = fork();

	if(pid == 0)
	{
		close(pipe_fd[0]);

		dup2(pipe_fd[1], STDOUT_FILENO);
		close(pipe_fd[1]);

		char url[256];
		sprintf(url, "https://api.binance.com/api/v3/klines?symbol=%s&interval=1h&limit=%d", symbol, CHART_POINTS);

		execlp("curl", "curl", "-s", url, NULL);
		exit(1);
	}
	else if(pid > 0)
	{
		close(pipe_fd[1]);

		char buffer[4096] = {0};
		int n = read(pipe_fd[0], buffer, sizeof(buffer) - 1);
		close(pipe_fd[0]);

		waitpid(pid, NULL, 0);

		if(n > 0)
		{
			parse_klines(buffer, open_out, close_out);

			if (close_out[CHART_POINTS-1] > 0)
			{
				sprintf(price_out, "$ %.2f", close_out[CHART_POINTS-1]);
			}
		}
	}
}
void fetch_news(char result[3][256]) {
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) return;
    
    pid_t pid = fork();
    if (pid == 0) {
        close(pipe_fd[0]);
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[1]);
        
        char* cmd = "curl -s 'https://api.rss2json.com/v1/api.json?rss_url=https://cointelegraph.com/rss' | "
                    "grep -o '\"title\":\"[^\"]*\"' | tail -n +2 | head -n 3 | "
                    "awk -F'\":\"' '{print $2}' | tr -d '\"'";
                    
        execlp("sh", "sh", "-c", cmd, NULL);
        exit(1);
    }
    else if (pid > 0) {
        close(pipe_fd[1]);
        char buffer[2048] = {0};
        if (read(pipe_fd[0], buffer, sizeof(buffer) - 1) < 0) { 
            // 경고 무시용 
        }
        close(pipe_fd[0]);
        waitpid(pid, NULL, 0);

        char* token = strtok(buffer, "\n");
        for(int i = 0; i < 3 && token != NULL; i++) {
            strncpy(result[i], token, 255);
            token = strtok(NULL, "\n");
        }
    }
}
void* fetch_worker(void* arg) {
    while(is_running) {
        for (int i = 0; i < num_symbols; i++) {
            char temp_symbol[SYMBOL_LEN];
            char temp_price[50] = "Error...";
			float temp_open[CHART_POINTS] = {0.0};
			float temp_close[CHART_POINTS] = {0.0};

            pthread_mutex_lock(&data_mutex);
            if (i < num_symbols) strcpy(temp_symbol, watchlist[i].symbol);
            pthread_mutex_unlock(&data_mutex);

            if (strlen(temp_symbol) > 0) {
                fetch_price(temp_symbol, temp_price, temp_open, temp_close);

                pthread_mutex_lock(&data_mutex);
                if (i < num_symbols && strcmp(watchlist[i].symbol, temp_symbol) == 0) {
                    strcpy(watchlist[i].price, temp_price);
					for(int c=0; c<CHART_POINTS; c++) {
                        watchlist[i].open_data[c] = temp_open[c];
                        watchlist[i].close_data[c] = temp_close[c];
					}
                }
                pthread_mutex_unlock(&data_mutex);

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
void* news_worker(void* arg) {
    while(is_running) {
        int current_idx;
        pthread_mutex_lock(&data_mutex);
        current_idx = selected_idx;
        pthread_mutex_unlock(&data_mutex);

        if (current_idx != last_fetched_news_idx && num_symbols > 0) {
            char temp_news[3][256] = {"No recent news.", "", ""};
            
            fetch_news(temp_news);

            pthread_mutex_lock(&news_mutex);
            for(int i=0; i<3; i++) strcpy(news_headlines[i], temp_news[i]);
            last_fetched_news_idx = current_idx;
            pthread_mutex_unlock(&news_mutex);
            
            write_log("News updated for list index %d", current_idx);
        }
        usleep(300000);
    }
    return NULL;
}