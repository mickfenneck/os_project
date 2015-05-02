#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include "const.h"


struct p_info_t {
    int score;
    long int player_id;

    struct p_info_t *next;
    struct p_info_t *prev;
};
typedef struct p_info_t player_info_t;


void shutdown() {
    printf("Destroying FIFOs...\n");
    unlink(CHALLENGE_FIFO);
    unlink(ANSWER_FIFO);
    unlink(CONNECT_FIFO);
    unlink(DISCONNECT_FIFO);

    exit(0);
}


void signal_handler(int signo) {
    printf("Shutting down\n");
    shutdown();
}


void init_fifo(int *connect_fifo, int *disconnect_fifo) {
    int MODE = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    if(mkfifo(CHALLENGE_FIFO, MODE) != 0 ||
            mkfifo(ANSWER_FIFO, MODE) != 0 ||
            mkfifo(CONNECT_FIFO, MODE) != 0 ||
            mkfifo(DISCONNECT_FIFO, MODE) != 0) {

        perror("mkfifo");
        exit(-1);
    }

    if((*connect_fifo = open(CONNECT_FIFO, O_RDONLY | O_NONBLOCK)) < 0 ||
        (*disconnect_fifo = open(DISCONNECT_FIFO, O_RDONLY | O_NONBLOCK)) < 0) {

        perror("open");
        shutdown();
        exit(-1);
    }
}


player_info_t *accept_connection(player_info_t *players, int fifo) {
    int ret;
    long player_id;

    while((ret = read(fifo, &player_id, sizeof(player_id))) > 0) {
        printf("connect - ret %d errno %d\n", ret, errno);

        player_info_t *info = malloc(sizeof(player_info_t));
        info->score = 0;
        info->player_id = player_id;
        info->next = players;
        if(players)
            players->prev = info;
        players = info;

        printf("Player %lu connected\n", player_id);
    }

    return players;
}


player_info_t *accept_disconnection(player_info_t *players, int fifo) {
    int ret;
    long player_id;

    while((ret = read(fifo, &player_id, sizeof(player_id))) > 0) {
        printf("disconnect - ret %d errno %d\n", ret, errno);

        player_info_t *cursor = players;
        while(cursor && cursor->player_id != player_id)
            cursor = cursor->next;

        if(cursor) {
            if(cursor->prev)
                cursor->prev->next = cursor->next;

            if(cursor->next)
                cursor->next->prev = cursor->prev;

            if(cursor == players)
                players = cursor->next;

            free(cursor);
            printf("Player %lu disconnected\n", player_id);
        }
        else {
            printf("disconnect - received disconnection from unknown player: %lu\n",
                   player_id);
        }
            
    }

    return players;
}


player_info_t *wait_for_players(player_info_t *players, int connect_fifo,
    int disconnect_fifo)
{
    int count;

    do {
        printf("Waiting for players, minimum 2, connected %d...\n", count);
        sleep(5);

        players = accept_connection(players, connect_fifo);
        players = accept_disconnection(players, disconnect_fifo);

        count = 0;
        player_info_t *cursor = players;
        while(cursor) {
            count += 1;
            cursor = cursor->next;
        }
    } while(count < 1);

    return players;
}


void send_challenge(const char *challenge, int size, const player_info_t *players) {
    int fifo = open(CHALLENGE_FIFO, O_WRONLY);

    while(players) {
        write(fifo, challenge, size);
        players = players->next;
        printf(".");
    }

    close(fifo);
}


player_info_t *play(player_info_t *players, int connection_fifo, int disconnection_fifo) {
    char *msg = "CIAO";

    while(1) {
        send_challenge(msg, strlen(msg) + 1, players);

        printf("Challenge sent, sleeping...\n");
        sleep(5);

        players = accept_connection(players, connection_fifo);
        players = accept_disconnection(players, disconnection_fifo);
    }

    return players;
}


int main(int argc, char *argv[]) {
    if(signal(SIGINT, signal_handler) == SIG_ERR) {
        perror("signal");
        exit(-1);
    }

    player_info_t *players = NULL;
    int connect_fifo, disconnect_fifo;

    init_fifo(&connect_fifo, &disconnect_fifo);
    players = wait_for_players(players, connect_fifo, disconnect_fifo);
    play(players, connect_fifo, disconnect_fifo);
    shutdown();

    return 0;
}
