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
        debug("sent quit message (%d) to player %d, ret %d errno %d\n",
            MESSAGE_SERVER_QUIT, i, ret, errno);
    }
}


static void signal_handler(int signo) {
    printf("Shutting down\n");
    shutdown();
    fifo_destroy();
    exit(0);
}


static void pipe_handler(int signo) {
    debug("BROKEN PIPE!!! %s\n", "D:<");
}


// sends a message to the given player
//  if player_id < 0 it will be filled with id of recipient
static void send_message(int player, int type, int x, int y, long player_id) {
    message_pack_t message = {
        .type = type,
        .x = x,
        .y = y,
        .player_id = player_id > 0 ? player_id : players[player].player_id
    };

    int ret = 0;
    ret = write(players[player].fifo, &message, sizeof(message));
    debug("sent message %d to player %d, ret %d errno %d\n",
        message.type, player, ret, errno);
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
    int i;

    debug("sending challenge (%d, %d)\n", x, y);
    for(i = 0; i < player_count; i++)
        send_message(i, MESSAGE_CHALLENGE, x, y, -1);
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


// playes a single round, until someone gives the right answer or everyone disconnects
static void play_round(int max_players, int correct) {
    int round_end = 0, i, answer;

    do {
        accept_connections(players, &player_count, max_players);
        accept_disconnections(players, &player_count);

        i = accept_answer(players, player_count);
        if(i >= 0) {
            answer = players[i].answer;
            if(answer == correct) {
                printf("Got correct answer from player %lu!\n", players[i].player_id);

                players[i].score += 1;
                send_message(i, MESSAGE_ANSWER_CORRECT, 0, 0, -1);
                round_end = 1;
            }
            else {
                printf("Got wrong answer from player %lu!\n", players[i].player_id);

                players[i].score -= 1;
                send_message(i, MESSAGE_ANSWER_WRONG, 0, 0, -1);
            }
        }
        else sleep(1);
    } while(!round_end && player_count > 0);

    for(i = 0; i < player_count; i++)
        send_message(i, MESSAGE_ROUND_END, 0, 0, -1);
}


static void match_end(int winner) {
    int i, j;

    printf("Match has ended! Sending final ranking to players...\n");
    for(i = 0; i < player_count; i++) {
        send_message(i, MESSAGE_MATCH_END, player_count, 0, winner);

        // send ranking
        for(j = 0; j < player_count; j++)
            send_message(i, MESSAGE_RANKING, j, players[j].score, players[j].player_id);
    }
}


// plays a match, ends when there are not enough players or someone disconnects
static void play(int max_players, int win_score) {
    int x, y, correct, high_score;

    do {
        get_challenge(&x, &y, &correct);
        send_challenge(x, y);

        printf("Challenge sent, waiting for answers...\n");
        play_round(max_players, correct);

        high_score = print_ranking();
        if(high_score >= win_score)
            match_end(players[0].player_id);

    } while(player_count > 0 && high_score < win_score);
}


int server_main(int max_players, int win_score) {
    printf("Accepting at most %d players, victory score is %d\n",
           max_players, win_score);

    // clean up resources and alert server in case of abnormal termination
    if(signal(SIGINT, signal_handler) == SIG_ERR ||
        signal(SIGQUIT, signal_handler) == SIG_ERR ||
        signal(SIGHUP, signal_handler) == SIG_ERR ||
        signal(SIGTERM, signal_handler) == SIG_ERR ||
        signal(SIGPIPE, pipe_handler) == SIG_ERR) {
        
        perror("signal");
        exit(-1);
    }

    srand(time(NULL));

    if(fifo_init() < 0) 
        return -1;

    wait_for_players(max_players);
    play(max_players, win_score);
    print_ranking();
    fifo_destroy();

    return 0;
}
