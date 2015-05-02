#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include "const.h"
#include "client.h"


long player_id;


void signal_handler(int signo) {
    shared->global_stop = 1;

    int fifo = open(DISCONNECT_FIFO, O_WRONLY | O_NONBLOCK);
    write(fifo, &player_id, sizeof(player_id));
    close(fifo);
}


long connect() {
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    player_id = tp.tv_nsec;
    printf("Our player id is %lu\n", player_id);

    printf("Connecting to server...\n");
    int connect_fifo = open(CONNECT_FIFO, O_WRONLY);
    write(connect_fifo, &player_id, sizeof(player_id));
    printf("Connected!\n");

    return player_id;
}

void wait_challenge(char *challenge, int size) {
    int challenge_fifo = open(CHALLENGE_FIFO, O_RDONLY);
    int ret = read(challenge_fifo, challenge, size);
    close(challenge_fifo);

    if(ret < 0) {
        perror("read");
        exit(-1);
    }

    return;
}


int main(int argc, char *argv[]) {
    if(signal(SIGINT, signal_handler) == SIG_ERR ||
            signal(SIGSEGV, signal_handler) == SIG_ERR) {  // happens too..
        perror("signal");
        exit(-1);
    }

    connect();

    char buffer[CHALLENGE_LENGTH];
    while(!shared->global_stop) {
        printf("Waiting for challenge...\n");
        wait_challenge(buffer, sizeof(buffer));
        printf("Challenge received: %s\n", buffer);
    }

    return 0;
}
