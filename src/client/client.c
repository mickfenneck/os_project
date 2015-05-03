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
    printf("\nDisconnecting...\n");
    int fifo = open(DISCONNECT_FIFO, O_WRONLY);
    write(fifo, &player_id, sizeof(player_id));
    close(fifo);
}


long connect() {
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    player_id = tp.tv_nsec;
    printf("Our player id is %lu\n", player_id);

    int fifo;
    printf("Connecting to server...\n");
    if((fifo = open(CONNECT_FIFO, O_WRONLY)) < 0) {
        perror("open");
        printf("No server running?\n");
        exit(-1);
    }
    write(fifo, &player_id, sizeof(player_id));
    close(fifo);
    printf("Connected!\n");

    return player_id;
}


void signal_handler(int signo) {
    disconnect();
    exit(-1);
}


void wait_challenge(challenge_pack_t *challenge) {
    int fifo = open(CHALLENGE_FIFO, O_RDONLY);
    if(fifo < 0) {
        perror("open");
        exit(-1);
    }

    int ret = read(fifo, challenge, sizeof(challenge_pack_t));
    close(fifo);

    debug("ret %d errno %d\n", ret, errno);

    if(ret < 0) {
        perror("read");
        exit(-1);
    }

    return;
}


void send_answer(int answer) {
    int fifo = open(ANSWER_FIFO, O_WRONLY);
    if(fifo < 0) {
        perror("open");
        exit(-1);
    }

    answer_pack_t pack;
    pack.answer = answer;
    pack.player_id = player_id;
    write(fifo, &pack, sizeof(pack));
    close(fifo);
}


int main(int argc, char *argv[]) {
    if(signal(SIGINT, signal_handler) == SIG_ERR) {
        perror("signal");
        exit(-1);
    }

    connect();
    while(player_id >= 0) {
        challenge_pack_t challenge;

        printf("Waiting for challenge...\n");
        wait_challenge(&challenge);
        printf("Challenge received: %d + %d\n", challenge.x, challenge.y);

        int answer;
        printf("Enter answer: ");
        scanf("%d", &answer);
        send_answer(answer);
    }

    return 0;
}
