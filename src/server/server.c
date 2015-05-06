#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include "const.h"

#define MIN_PLAYERS 1  // minimum number of players needed to start the game
#define MAX_PLAYERS 10  // maximum number of connected players
#define WIN_SCORE 15

typedef struct p_info_t {
    long int player_id;
    int score;
    int answer;
    int has_answered;
} player_info_t;


void shutdown();
void signal_handler(int);
void init_fifo(int *, int *, int *);
void accept_connection(player_info_t *, int *, int, int);
void accept_disconnection(player_info_t *, int *, int);
void wait_for_players(player_info_t *, int *, int, int, int);
void send_challenge(challenge_pack_t *, int);
int accept_answer(player_info_t *, int, int);
void get_challenge(challenge_pack_t *);
void assign_points(player_info_t *, int, int);
int print_ranking(player_info_t *, int);
void play(player_info_t *players, int *, int, int, int, int, int);


void shutdown() {
    send_challenge(&SERVER_QUIT_PACK, MAX_PLAYERS);

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


void accept_connection(player_info_t *players, int *player_count, int fifo, int max_players) {
    int ret;
    long player_id;

    while((ret = read(fifo, &player_id, sizeof(player_id))) > 0) {
        debug("ret %d errno %d\n", ret, errno);

        if(*player_count < max_players) {
            players[*player_count].player_id = player_id;
            players[*player_count].score = max_players - *player_count - 1;

            // don't wait for an answer if player has connected when a challenge
            // was already sent
            players[*player_count].has_answered = 1;
            *player_count += 1;

            printf("Player %lu connected (%d players connected)\n", player_id,
                   *player_count);
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

        int i = 0;
        while(i < *player_count && players[i].player_id != player_id)
            i += 1;

        if(i < *player_count) {
            while(i < *player_count) {
                players[i] = players[i + 1];
                i += 1;
            }
            *player_count -= 1;
            printf("Player %lu disconnected (%d players conneceted)\n", player_id,
                   *player_count);
        }
        else {
            printf("disconnect - received disconnection from unknown player: %lu\n",
                   player_id);
        }
    }
}


void wait_for_players(player_info_t *players, int *player_count, int connect_fifo,
    int disconnect_fifo, int max_players)
{
    do {
        printf("Waiting for players, minimum %d, connected %d...\n",
               MIN_PLAYERS, *player_count);
        sleep(5);

        accept_connection(players, player_count, connect_fifo, max_players);
        accept_disconnection(players, player_count, disconnect_fifo);
    } while(*player_count < MIN_PLAYERS);
}


void send_challenge(challenge_pack_t *challenge, int player_count) {
    int i, fifo = open(CHALLENGE_FIFO, O_WRONLY | O_NONBLOCK);
    for(i = 0; i < player_count; i++) {
        write(fifo, challenge, sizeof(challenge_pack_t));
        debug("sent pack #%d\n", i);
    }
    close(fifo);
}


int accept_answer(player_info_t *players, int player_count, int fifo) {
    int ret, count = 0;
    answer_pack_t pack;

    while((ret = read(fifo, &pack, sizeof(pack))) > 0) {
        debug("ret %d errno %d\n", ret, errno);

        int i = 0;
        while(i < player_count && players[i].player_id != pack.player_id)
            i += 1;

        if(i < player_count) {
            players[i].answer = pack.answer;
            players[i].has_answered = 1;
            count += 1;

            printf("Received answer %d from player %lu\n", pack.answer,
                   pack.player_id);
        }
        else printf("Received answer from unknown player %lu\n", pack.player_id);
    }

    return count;
}


void get_challenge(challenge_pack_t *challenge) {
    challenge->x = rand() % 100;
    challenge->y = rand() % 100;
    debug("challenge is %d + %d\n", challenge->x, challenge->y);
}


void assign_points(player_info_t *players, int player_count, int correct) {
    int i;
    for(i = 0; i < player_count; i++) {
        if(players[i].answer == correct)
            players[i].score += 1;
        else
            players[i].score -= 1;
    }
}


int print_ranking(player_info_t *players, int player_count) {
    if(player_count == 0)
        return -1;

    // sort players by score
    int i, j;
    player_info_t temp;
    for(i = 0; i < player_count; i++) {
        for(j = i + 1; j < player_count; j++) {
            if(players[i].score < players[j].score) {
                temp = players[i];
                players[i] = players[j];
                players[j] = temp;
            }
        }
    }

    printf("***  Updated ranking  ***\n");
    for(i = 0; i < player_count; i++)
        printf("\tPlayer %lu - Score %d\n", players[i].player_id, players[i].score);

    return players[0].score;
}


void play(player_info_t *players, int *player_count, int connection_fifo,
          int disconnection_fifo, int answer_fifo, int max_players, int win_score)
{
    int i, high_score;
    do {
        for(i = 0; i < *player_count; i++)
            players[i].has_answered = 0;

        challenge_pack_t challenge;

        get_challenge(&challenge);
        send_challenge(&challenge, *player_count);
        printf("Challenge sent, waiting for answers...\n");

        // wait until we received an answer from everyone (and update who "everyone" is)
        int finished;
        do {
            accept_answer(players, *player_count, answer_fifo);

            finished = 1;
            for(i = 0; i < *player_count; i++)
                finished &= players[i].has_answered;

            // players who connect now will not receive this challenge
            accept_connection(players, player_count, connection_fifo, max_players);
            accept_disconnection(players, player_count, disconnection_fifo);

            sleep(1);
        } while(!finished);

        int correct = challenge.x + challenge.y;
        assign_points(players, *player_count, correct);
        high_score = print_ranking(players, *player_count);
    } while(*player_count >= MIN_PLAYERS && high_score < win_score);
}


int server_main(int max_players, int win_score) {
    if(signal(SIGINT, signal_handler) == SIG_ERR) {
        perror("signal");
        exit(-1);
    }

    player_info_t players[max_players];
    int connect_fifo, disconnect_fifo, answer_fifo, player_count = 0;

    srand(time(NULL));

    init_fifo(&connect_fifo, &disconnect_fifo, &answer_fifo);
    wait_for_players(players, &player_count, connect_fifo, disconnect_fifo, max_players);
    play(players, &player_count, connect_fifo, disconnect_fifo, answer_fifo,
         max_players, win_score);

    close(connect_fifo); close(disconnect_fifo); close(answer_fifo);
    shutdown();

    return 0;
}


int main(int argc, char *argv[]) {
    server_main(MAX_PLAYERS, WIN_SCORE);
}
