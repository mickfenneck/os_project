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
#include "fifos.c"
#include "const.h"
#include "server.h"


static player_info_t players[MAX_PLAYERS];
static int player_count = 0;


static void shutdown() {
    int i, ret;
    message_pack_t message = { .type = MESSAGE_SERVER_QUIT, .player_id = 0 };
    for(i = 0; i < player_count; i++) {
        ret = write(players[i].fifo, &message, sizeof(message));
        debug("sent quit message to player %d, ret %d errno %d\n", i, ret, errno);
    }
    fifo_destroy();
    exit(0);
}


static void signal_handler(int signo) {
    printf("Shutting down\n");
    shutdown();
}


// wait until the minimum number of players joined the match
static void wait_for_players(int max_players) {
    do {
        printf("Waiting for players, minimum %d, connected %d...\n",
               MIN_PLAYERS, player_count);
        sleep(2);

        accept_connections(players, &player_count, max_players);
        accept_disconnections(players, &player_count);
    } while(player_count < MIN_PLAYERS);
}

// send the challenge to all players
static void send_challenge(int x, int y) {
    int i, ret;
    message_pack_t message = { .type = MESSAGE_CHALLENGE, .x = x, .y = y };

    debug("sending challenge (%d, %d)\n", x, y);
    for(i = 0; i < player_count; i++) {
        debug("sending to fd %d, sizeof(message) = %lu\n", players[i].fifo, sizeof(message));
        ret = write(players[i].fifo, &message, sizeof(message));
        debug("sent challenge to player %d, ret %d errno %d\n", i, ret, errno);
    }
}


// generates a random challenge
static void get_challenge(int *x, int *y, int *correct) {
    *x = rand() % 100;
    *y = rand() % 100;
    *correct = *x + *y;

    debug("challenge is %d + %d\n", *x, *y);
}


// sorts player by score, prints the ranking and return the highest score
static int print_ranking() {
    if(player_count == 0)
        return -1;

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


// plays a match, ends when there are not enough players or someone disconnects
static void play(int max_players, int win_score) {
    // players are kept sorted by score

    int i, x, y, correct, answer, ret;
    message_pack_t message;

    do {
        get_challenge(&x, &y, &correct);
        send_challenge(x, y);
        printf("Challenge sent, waiting for answers...\n");

        int round_end = 0;
        do {
            accept_connections(players, &player_count, max_players);
            accept_disconnections(players, &player_count);

            i = accept_answer(players, player_count);
            if(i >= 0) {
                answer = players[i].answer;
                if(answer == correct) {
                    players[i].score += 1;
                    message.type = MESSAGE_ANSWER_CORRECT;
                    message.player_id = players[i].player_id;
                    ret = write(players[i].fifo, &message, sizeof(message));
                    debug("sent answer correct message to player %d, ret %d errno %d\n", i, ret, errno);

                    round_end = 1;
                    printf("Got correct answer from player %lu!\n", players[i].player_id);
                }
                else {
                    players[i].score -= 1;
                    message.type = MESSAGE_ANSWER_WRONG;
                    message.player_id = players[i].player_id;
                    ret = write(players[i].fifo, &message, sizeof(message));
                    debug("sent answer incorrect message to player %d, ret %d errno %d\n", i, ret, errno);
                    printf("Got wrong answer from player %lu!\n", players[i].player_id);
                }
            }
            else sleep(1);
        } while(!round_end && player_count > 0);

        message.type = MESSAGE_ROUND_END;
        message.player_id = players[i].player_id;
        for(i = 0; i < player_count; i++) {
            ret = write(players[i].fifo, &message, sizeof(message));
            debug("sent round end message to player %d, ret %d errno %d\n", i, ret, errno);
        }
    } while(player_count > 0);
}


int server_main(int max_players, int win_score) {

    printf("Accepting at most %d players, victory score is %d\n",
           max_players, win_score);

    if(signal(SIGINT, signal_handler) == SIG_ERR) {
        perror("signal");
        exit(-1);
    }

    srand(time(NULL));

    if(fifo_init() < 0) 
        return -1;

    wait_for_players(max_players);
    play(max_players, win_score);

    shutdown();
    print_ranking();

    return 0;
}
