#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include "threads.c"


extern void make_threads();
extern void destroy_threads();


void shutdown() {
    pthread_mutex_lock(&shared->mutex);
    shared->global_stop = 1;
    pthread_mutex_unlock(&shared->mutex);

    destroy_threads();

    printf("Destroying FIFOs...\n");
    unlink(CHALLENGE_FIFO);
    unlink(ANSWER_FIFO);
    unlink(CONNECT_FIFO);
    unlink(DISCONNECT_FIFO);

    printf("Finalizing...\n");
    free(shared);
    exit(0);
}


void init_fifo() {
    int MODE = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    if(mkfifo(CHALLENGE_FIFO, MODE) != 0 ||
            mkfifo(ANSWER_FIFO, MODE) != 0 ||
            mkfifo(CONNECT_FIFO, MODE) != 0 ||
            mkfifo(DISCONNECT_FIFO, MODE) != 0) {

        perror("mkfifo");
        exit(-1);
    }
}


void signal_handler(int signo) {
    printf("Shutting down\n");
    shutdown();  // not polite
}


void send_challenge(const char *challenge, int size, const player_info_t *players) {
    int fifo = open(CHALLENGE_FIFO, O_WRONLY | O_NONBLOCK);

    pthread_mutex_lock(&shared->mutex);
    while(players) {
        write(fifo, challenge, size);
        players = players->next;
        printf(".");
    }
    pthread_mutex_unlock(&shared->mutex);

    close(fifo);
}


void play() {
    char *msg = "CIAO";
    int connect_fifo = open(CONNECT_FIFO, O_RDONLY | O_NONBLOCK);

    while(!shared->global_stop && shared->players != NULL) {
        send_challenge(msg, strlen(msg) + 1, shared->players);

        printf("Challenge sent, sleeping...\n");
        sleep(5);
    }

    close(connect_fifo);
}

void wait_for_players() {
    int count, stop;
    do {
        pthread_mutex_lock(&shared->mutex);
        count = shared->player_count;
        stop = shared->global_stop;
        pthread_mutex_unlock(&shared->mutex);

        printf("Waiting for players, minimum 2, connected %d...\n", count);
        sleep(5);
    } while(count < 2 && !stop);
}


int main(int argc, char *argv[]) {
    if(signal(SIGINT, signal_handler) == SIG_ERR ||
            signal(SIGSEGV, signal_handler) == SIG_ERR) {  // happens too..
        perror("signal");
        exit(-1);
    }

    shared = malloc(sizeof(server_shared_t));
    if(!shared) {
        printf("[server_shared] malloc\n");
        exit(-1);
    }

    shared->players = NULL;
    shared->answers = NULL;
    shared->player_count = shared->global_stop = 0;

    init_fifo();
    make_threads();
    wait_for_players();
    play();
    shutdown();

    return 0;
}
