#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include "const.h"


long player_id;


void disconnect() {
    printf("Disconnecting...\n");
    int fifo = open(DISCONNECT_FIFO, O_WRONLY);
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


void signal_handler(int signo) {
    disconnect();
    player_id = -1;
}


void wait_challenge(char *challenge, int size) {
    int challenge_fifo = open(CHALLENGE_FIFO, O_RDONLY);
    int ret = read(challenge_fifo, challenge, size);
    close(challenge_fifo);

    printf("wait - ret %d errno %d\n", ret, errno);

    if(ret < 0) {
        perror("read");
        exit(-1);
    }

    return;
}


int main(int argc, char *argv[]) {
    char buffer[CHALLENGE_LENGTH];

    if(signal(SIGINT, signal_handler) == SIG_ERR) {
        perror("signal");
        exit(-1);
    }

    connect();
    while(player_id >= 0) {
        printf("Waiting for challenge...\n");
        wait_challenge(buffer, sizeof(buffer));
        printf("Challenge received: %s\n", buffer);
    }

    return 0;
}
