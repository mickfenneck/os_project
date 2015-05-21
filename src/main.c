#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "const.h"


extern int server_main(int, int);
extern int client_main();


void print_usage(char *argv[]) {
    printf("Usage: %s <server|client> args...\n\n", argv[0]);
    printf("Server args: --max/-m max number of players (1 - 10)\n");
    printf("             --win/-w score to reach for victory (10 - 100)\n");
}


int launch_server(int argc, char *args[]) {
    int max_players = 10,
        win_score = 20, i;

    for(i = 0; i < argc; i++) {
        if(strcmp("--max", args[i]) == 0 || strcmp("-m", args[i]) == 0) {
            i += 1;
            if(i >= argc) {
                printf("Need 0 < number < 11 after %s\n", args[i - 1]);
                exit(-1);
            }
            max_players = atoi(args[i]);
            if(max_players < 1 || max_players > 10) {
                printf("Need 0 < number < 11 after %s\n", args[i - 1]);
                exit(-1);
            }
        }
        else if(strcmp("--win", args[i]) == 0 || strcmp("-w", args[i]) == 0) {
            i += 1;
            if(i >= argc) {
                printf("Need 9 < number < 101 after %s\n", args[i - 1]);
                exit(-1);
            }
            win_score = atoi(args[i]);
            if(win_score < 10 || win_score > 100) {
                printf("Need 9 < number < 101 after %s\n", args[i - 1]);
                exit(-1);
            }
        }
    }

    return server_main(max_players, win_score);
}


int launch_client(int argc, char *args[]) {
    return client_main();
}


int main(int argc, char *argv[]) {
	#ifdef DEBUG
	debug_f = stderr;
	#else
	debug_f = fopen("/dev/null", "w");
	#endif

    if(argc < 2) {
        print_usage(argv);
        exit(-1);
    }
    else if(strcmp("server", argv[1]) == 0) {
        return launch_server(argc - 2, argv + 2);
    }
    else if(strcmp("client", argv[1]) == 0) {
        return launch_client(argc - 2, argv + 2);
    }
    else {
        print_usage(argv);
        exit(-1);
    }
}
