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

#define MIN_PLAYERS 1  // minimum number of players needed to start the game
#define MAX_PLAYERS 10  // maximum number of connected players


struct p_info_t {
    long int player_id;
    int score;
    int answer;
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


void init_fifo(int *connect_fifo, int *disconnect_fifo, int *answer_fifo) {
    int MODE = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    if(mkfifo(CHALLENGE_FIFO, MODE) != 0 ||
            mkfifo(ANSWER_FIFO, MODE) != 0 ||
            mkfifo(CONNECT_FIFO, MODE) != 0 ||
            mkfifo(DISCONNECT_FIFO, MODE) != 0) {

        perror("mkfifo");
        printf("Another server running?\n");
        exit(-1);
    }

    if((*connect_fifo = open(CONNECT_FIFO, O_RDONLY | O_NONBLOCK)) < 0 ||
        (*disconnect_fifo = open(DISCONNECT_FIFO, O_RDONLY | O_NONBLOCK)) < 0 ||
        (*answer_fifo = open(ANSWER_FIFO, O_RDONLY | O_NONBLOCK)) < 0) {

        perror("open");
        shutdown();
        exit(-1);
    }
}


void accept_connection(player_info_t *players, int *player_count, int fifo) {
    int ret;
    long player_id;

    while((ret = read(fifo, &player_id, sizeof(player_id))) > 0) {
        debug("ret %d errno %d\n", ret, errno);

        if(*player_count < MAX_PLAYERS) {
            players[*player_count].score = 0;
            players[*player_count].player_id = player_id;
            *player_count += 1;

            printf("Player %lu connected\n", player_id);
        }
        else {
            printf("Rejecting connection from player %lu (too many connected players)\n",
                   player_id);
        }
    }
}


void accept_disconnection(player_info_t *players, int *player_count, int fifo) {
    int ret;
    long player_id;

    while((ret = read(fifo, &player_id, sizeof(player_id))) > 0) {
        debug("ret %d errno %d\n", ret, errno);

        // find player who disconnected
        int i = 0;
        while(i < *player_count && players[i].player_id != player_id)
            i += 1;

        if(i < *player_count) {
            // delete it by sliding next players
            while(i < *player_count) {
                players[i] = players[i + 1];
                i += 1;
            }
            *player_count -= 1;
        }
        else {
            printf("disconnect - received disconnection from unknown player: %lu\n",
                   player_id);
        }
    }
}


void wait_for_players(player_info_t *players, int *player_count, int connect_fifo,
    int disconnect_fifo)
{
    do {
        printf("Waiting for players, minimum %d, connected %d...\n",
               MIN_PLAYERS, *player_count);
        sleep(5);

        accept_connection(players, player_count, connect_fifo);
        accept_disconnection(players, player_count, disconnect_fifo);
    } while(*player_count < MIN_PLAYERS);
}


void send_challenge(const char *challenge, int size, player_info_t *players,
                    int player_count)
{
    int i, fifo = open(CHALLENGE_FIFO, O_WRONLY);

    for(i = 0; i < player_count; i++) {
        write(fifo, challenge, size);
        printf(".");
    }

    close(fifo);
}


int accept_answer(player_info_t *players,  int *player_count, int fifo) {
    int ret, count = 0;
    answer_pack_t pack;

    while((ret = read(fifo, &pack, sizeof(pack))) > 0) {
        debug("ret %d errno %d\n", ret, errno);

        int i = 0;
        while(i < *player_count && players[i].player_id != pack.player_id)
            i += 1;

        if(i < *player_count) {
            players[i].answer = pack.answer;
            printf("Received answer %d from player %lu\n", pack.answer,
                   pack.player_id);
            count += 1;
        }
        else printf("Received answer from unknown player %lu\n", pack.player_id);
    }

    return count;
}

player_info_t *play(player_info_t *players, int *player_count, int connection_fifo,
                    int disconnection_fifo, int answer_fifo)
{
    char challenge[10];
    int i = 0;

    do {
        sprintf(challenge, "Challenge number %d", i++);
        send_challenge(challenge, strlen(challenge) + 1, players, *player_count);
        printf("Challenge sent, waiting for answers...\n");

        // wait until we received an answer from everyone (and update who "everyone" is)
        int answer_count = 0, needed_answers = *player_count;
        while(answer_count < needed_answers && *player_count >= MIN_PLAYERS) {
            answer_count += accept_answer(players, player_count, answer_fifo);

            // players who connect now will not receive this challenge
            accept_connection(players, player_count, connection_fifo);

            int connected = *player_count;
            accept_disconnection(players, player_count, disconnection_fifo);
            int disconnected = connected - *player_count;

            needed_answers -= disconnected;  // don't wait answers for disconnected players

            sleep(1);
        }
    } while(*player_count >= MIN_PLAYERS);

    return players;
}


int main(int argc, char *argv[]) {
    if(signal(SIGINT, signal_handler) == SIG_ERR) {
        perror("signal");
        exit(-1);
    }

    player_info_t players[MAX_PLAYERS];
    int connect_fifo, disconnect_fifo, answer_fifo, player_count = 0;

    init_fifo(&connect_fifo, &disconnect_fifo, &answer_fifo);
    wait_for_players(players, &player_count, connect_fifo, disconnect_fifo);
    play(players, &player_count, connect_fifo, disconnect_fifo, answer_fifo);
    shutdown();

    return 0;
}
