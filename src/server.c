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


typedef struct {
    long player_id;
    int score;
    int fifo_rd;
    int fifo_wr;
} player_info_t;


challenge_pack_t SERVER_QUIT_PACK = { .x = -1, .y = -1 };
challenge_pack_t SERVER_VICTORY_PACK = { .x = -2, .y = -2 };


static void shutdown();
static void signal_handler(int);
static void init_fifo(int *, int *, int *);
static void accept_connection(player_info_t *, int *, int, int);
static void accept_disconnection(player_info_t *, int *, int);
static void wait_for_players(player_info_t *, int *, int, int, int);
static void send_challenge(challenge_pack_t *, int);
static int accept_answer(player_info_t *, int, int);
static void get_challenge(challenge_pack_t *, player_info_t *, int);
static void assign_points(player_info_t *, int, int);
static int print_ranking(player_info_t *, int);
static void play(player_info_t *players, int *, int, int, int, int, int);


static void shutdown() {
    send_challenge(&SERVER_QUIT_PACK, MAX_PLAYERS);

    printf("Destroying FIFOs...\n");
    unlink(STATUS_FIFO);
    unlink(CONNECT_FIFO);

    exit(0);
}


static void signal_handler(int signo) {
    printf("Shutting down\n");
    shutdown();
}


static void init_fifo(int *connect_fifo, int *status_fifo, int *answer_fifo) {
    int MODE = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    if(mkfifo(STATUS_FIFO, MODE) != 0 ||
        mkfifo(CONNECT_FIFO, MODE) != 0 ||
        mkfifo(ANSWER_FIFO, MODE) != 0) {

        perror("mkfifo");
        printf("Another server running?\n");
        exit(-1);
    }

    if((*connect_fifo = open(CONNECT_FIFO, O_RDONLY | O_NONBLOCK)) < 0 ||
        (*answer_fifo = open(ANSWER_FIFO, O_RDONLY | O_NONBLOCK)) < 0 || 
        (*status_fifo = open(STATUS_FIFO, O_RDONLY | O_NONBLOCK)) < 0) {

        perror("open");
        shutdown();
        exit(-1);
    }
}


// accept all pending connections
static void accept_connection(player_info_t *players, int *player_count,
    int fifo, int max_players)
{
    int ret, wf, rf;
    long player_id;
    connection_pack_t pack;

    while((ret = read(fifo, &pack, sizeof(pack))) > 0) {
        debug("ret %d errno %d\n", ret, errno);

        if(*player_count < max_players) {
            players[*player_count].player_id = pack.player_id;
            players[*player_count].score = max_players - *player_count - 1;

            rf = open(pack.fifo_rd, O_RDONLY | O_NONBLOCK);
            if(rf < 0) {
                debug("cannot open read fifo %s for player %lu\n",
                    pack.fifo_rd, pack.player_id);
                printf("Error while a player was connecting.\n");
                continue;
            }
            else players[*player_count].fifo_rd = rf;

            wf = open(pack.fifo_rw, O_WRONLY | O_NONBLOCK);
            if(wf < 0) {
                debug("cannot open write fifo %s for player %lu\n",
                    pack.fifo_wr, pack.player_if);
                printf("Error while a player was connecting.\n");
                close(rf);
                continue;
            }
            else players[*player_count].fifo_wr = wf;

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


// handle a disconnection from a player
static void handle_disconnection(player_info_t *players, int *player_count,
    int player_index)
{
    printf("Player %lu disconnected.\n", players[player_index].player_id;

    int i;
    for(i = player_index; i < *player_count; i++)
        players[i] = players[i + 1]
    *player_count -= 1;
}


static void wait_for_players(player_info_t *players, int *player_count, int connect_fifo,
    int disconnect_fifo, int max_players)
{
    int i, ret;
    message_pack_t msg;

    do {
        printf("Waiting for players, minimum %d, connected %d...\n",
               MIN_PLAYERS, *player_count);
        sleep(5);

        accept_connection(players, player_count, connect_fifo, max_players);

        // loop for disconnections
        for(i = 0; i < *player_count; i++) {
            if((ret = read(players[i].read_fifo, &msg, sizeof(msg))) > 0) {
                if(msg.message_type == MESSAGE_DISCONNECTION)
                    handle_disconnection(players, player_count, i)
                else {
                    debug("received unexpected message from player %lu: %d\n",
                        players[i].player_id, msg.message_type);
                }
            }
        }
    } while(*player_count < MIN_PLAYERS);
}


static void send_challenge(challenge_pack_t *challenge, int player_count) {
    int i, fifo = open(CHALLENGE_FIFO, O_WRONLY | O_NONBLOCK);
    for(i = 0; i < player_count; i++) {
        write(fifo, challenge, sizeof(challenge_pack_t));
        debug("sent pack #%d\n", i);
    }
    close(fifo);
}


static int accept_answer(player_info_t *players, int player_count, int fifo) {
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


static void get_challenge(challenge_pack_t *challenge, player_info_t *players,
    int player_num)
{
    challenge->x = rand() % 100;
    challenge->y = rand() % 100;
    challenge->player_num = player_num;
    memcpy(challenge->ranking, players, sizeof(player_info_t) * player_num);

    debug("challenge is %d + %d\n", challenge->x, challenge->y);
}


static int print_ranking(player_info_t *players, int player_count) {
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


static void play(player_info_t *players, int *player_count, int connection_fifo, 
    int max_players, int win_score)
{
    // players are kept sorted by score

    int i, high_score;
    challenge_pack_t challenge;

    do {
        get_challenge(&challenge, players, *player_count);
        send_challenge(&challenge, *player_count);
        printf("Challenge sent, waiting for answers...\n");

        int round_end, correct = challenge.x + challenge.y;
        do {
            for(i = 0; i < *player_count; i++) {
                int ret;
                message_pack_t message;

                if((ret = read(players[i].fifo_rd, &message, sizeof(message))) > 0)
                    continue;

                if(message.message_type == MESSAGE_DISCONNECTION) {
                    handle_disconnection(player_info_t *players, player_count, i);
                }
                else if(message.message_type == MESSAGE_ANSWER) {
                    
                }
            }

            sleep(1);
        } while(!round_end);
}


int server_main(int max_players, int win_score) {
    player_info_t players[max_players];
    int connect_fifo, status_fifo, answer_fifo, player_count = 0;

    printf("Accepting at most %d players, victory score is %d\n",
           max_players, win_score);

    if(signal(SIGINT, signal_handler) == SIG_ERR) {
        perror("signal");
        exit(-1);
    }

    srand(time(NULL));

    init_fifo(&connect_fifo, &status_fifo, &answer_fifo);
    wait_for_players(players, &player_count, connect_fifo, disconnect_fifo, max_players);

    /*
    play(players, &player_count, connect_fifo, disconnect_fifo, answer_fifo,
         max_players, win_score);
    */

    close(connect_fifo); close(status_fifo);
    shutdown();

    return 0;
}
