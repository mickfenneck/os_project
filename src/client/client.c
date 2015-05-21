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
#include "user_input.c"
#include "listener.c"


static void shutdown() {
    stop_ui_processes();
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


// waits for a challenge from the server, returns 1 if match has ended
static int wait_challenge(int *x, int *y) {
    int end = 0;

    printf("Waiting for challenge...\n");
    message_pack_t *message = get_message(MESSAGE_CHALLENGE | MESSAGE_MATCH_END, 1);

    pthread_mutex_lock(&shared.mutex);
    end |= shared.global_stop;
    pthread_mutex_unlock(&shared.mutex);

    if(message->type == MESSAGE_CHALLENGE) {
        *x = message->x;
        *y = message->y;
        free(message);
    }
    else {
        handle_victory(message->player_id, message->x);
        end = 1;
    }

    return end;
}


static void play_round(int x, int y) {
    message_pack_t *message;
    int round_end = 0, user_answered, answer;
    
    do {
        user_answered = get_answer(&answer, x, y);

        if(user_answered) {
            // send answer to server and wait for ack
            answer_pack_t pack = { .player_id = player_id, .answer = answer };
            int fd = open(ANSWER_FIFO, O_WRONLY);
            write(fd, &pack, sizeof(pack));
            close(fd);

            message = get_message(MESSAGE_ANSWER_CORRECT | MESSAGE_ANSWER_WRONG, 1);

            // wait_message has already checked that the message was directed at us
            if(message->type == MESSAGE_ANSWER_CORRECT) {
                printf("Answer is correct!\n");
                message = get_message(MESSAGE_ROUND_END, 1);
                round_end = 1;
            }
            else {
                printf("Wrong answer!\n");
                round_end = 0;
            }
        }
        else {
            printf("Another player gave the correct answer before you!\n");
            round_end = 1;
        }
    } while(!round_end);
}


static int init() {
    if(signal(SIGINT, signal_handler) == SIG_ERR) {
        perror("signal");
        return 0;
    }
    
    shared.waiting_type = shared.global_stop = 0;
    shared.messages = NULL;
    pthread_mutex_init(&shared.mutex, NULL);
    pthread_mutex_init(&shared.waiting, NULL);
    pthread_mutex_lock(&shared.waiting);
    
    return 1;
}


int client_main() {
    if(!init() || !connect()) {
        return -1;
    }

    // run listener thread
    pthread_t listener_tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&listener_tid, &attr, &listener_thread, NULL);

    int end = 0;
    do {

        int x, y;
        end = wait_challenge(&x, &y);
        if(!end)
            play_round(x, y);

    } while(!end);

    disconnect();
    shutdown();

    return 0;
}
