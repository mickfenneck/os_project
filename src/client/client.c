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
#include "client.h"
#include "const.h"
#include "comms.c"
#include "threads.c"


static void shutdown() {
    unlink(fifo);
}


static void disconnect() {
    printf("\nDisconnecting...\n");
    int fifo = open(DISCONNECT_FIFO, O_WRONLY | O_NONBLOCK);
    write(fifo, &player_id, sizeof(player_id));
    close(fifo);
}


static void signal_handler(int signo) {
    disconnect();
    shutdown();
    exit(-1);
}


// waits for a challenge from the server
static void wait_challenge(int *x, int *y) {
    printf("Waiting for challenge...\n");
    message_pack_t *message = wait_message(MESSAGE_CHALLENGE);

    *x = message->x;
    *y = message->y;

    free(message);
}


static void play_round(int x, int y) {
    message_pack_t *message;
    int round_end, user_answered, answer;

    
    do {
        user_answered = get_answer(&answer, x, y);

        if(user_answered) {
            // send answer to server and wait for ack
            answer_pack_t pack = { .player_id = player_id, .answer = answer };
            int fd = open(ANSWER_FIFO, O_WRONLY);
            write(fd, &pack, sizeof(pack));
            close(fd);

            message = wait_message(MESSAGE_ANSWER_CORRECT | MESSAGE_ANSWER_WRONG);

            // wait_message has already checked that the message was directed at us
            if(message->type == MESSAGE_ANSWER_CORRECT) {
                printf("Answer is correct!\n");
                message = wait_message(MESSAGE_ROUND_END);
                round_end = 1;
            }
            else {
                printf("Wrong answer!\n");
            }
        }
        else {
            printf("Another player gave the correct answer before you!\n");
            round_end = 1;
        }
    } while(!round_end);
}


int client_main() {
    if(signal(SIGINT, signal_handler) == SIG_ERR) {
        perror("signal");
        exit(-1);
    }

    if(connect() <= 0) {
        return -1;
    }

    do {
        int x, y;
        wait_challenge(&x, &y);
        play_round(x, y);
    } while(1);  // exit will be called upon receiving MESSAGE_SERVER_QUIT

    disconnect();
}