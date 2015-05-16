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
#include <pthread.h>
#include "const.h"


typedef struct {
    int x;
    int y;
    pthread_t supervisor_tid;
    int answer;
    int has_answer;
} answer_thread_args_t;


typedef struct {
    pthread_t answer_tid;
    long winner_id;
} supervisor_thread_args_t;


long player_id;
char fifo[FIFO_NAME_LENGTH];


static void disconnect();
static long connect();
static void signal_handler(int);


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


// default handler for messages
static void handle_message(message_pack_t *message) {
    // TODO just print some shit

    if(message->type == MESSAGE_SERVER_QUIT) {
        printf("Server has quit!\n");
        shutdown();
        exit(0);
    }
}


// wait until a message of a given type arrives (remember to free)
static message_pack_t *wait_message(int type) {
    int received;
    message_pack_t *message = malloc(sizeof(message_pack_t));
    
    do {
        int fd = open(fifo, O_RDONLY);
        read(fd, message, sizeof(message_pack_t));
        close(fd);

        debug("received message %d\n", message->type);
        if(message->type & type) {
            // if needed, check that we are the recipient
            if(message->type & (MESSAGE_ANSWER_CORRECT | MESSAGE_ANSWER_WRONG)) {
                if(message->player_id == player_id)
                    received = 1;
                else handle_message(message);
            }
            else received = 1;
        }
        else handle_message(message);

        close(fd);
    } while(!received);

    return message;
}


// connect to server and wait for ack, return > 0 -> ok, < 0 -> fatal
static long connect() {
    int fd;
    connection_pack_t pack;

    player_id = (long) getpid();
    printf("Our player id is %lu\n", player_id);

    sprintf(fifo, "/tmp/sisop-client-%lu", player_id);
    debug("creating fifo %s\n", fifo);
    if(mkfifo(fifo, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) != 0) {
        perror("mkfifo");
        return -1;
    }

    printf("Connecting to server...\n");
    if((fd = open(CONNECT_FIFO, O_WRONLY)) < 0) {
        perror("open");
        printf("No server running?\n");
        return -1;
    }

    debug("sending connection request%s", "\n");
    pack.player_id = player_id;
    strcpy(pack.fifo, fifo);
    write(fd, &pack, sizeof(pack));
    close(fd);

    debug("waiting for ack%s", "\n");  // FIXME hangs forever if server quits now
    message_pack_t *message = wait_message(MESSAGE_CONNECTION_ACCEPTED | MESSAGE_CONNECTION_REJECTED);

    if(message->type == MESSAGE_CONNECTION_ACCEPTED) {
        printf("Connected!\n");
        free(message);
        return 1;
    }
    else { // if(message->type == MESSAGE_CONNECTION_REJECTED) {
        printf("Our connection was rejected!\n");
        free(message);
        return -1;
    }
}


// waits for a challenge from the server
static void wait_challenge(int *x, int *y) {
    printf("Waiting for challenge...\n");
    message_pack_t *message = wait_message(MESSAGE_CHALLENGE);

    *x = message->x;
    *y = message->y;

    free(message);
}


// gets an answer from the playe 
static void *answer_thread(void *arg) {
    answer_thread_args_t *a = arg;
    int old_cancel_type;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &old_cancel_type);

    a->has_answer = 0;
    printf("Challenge is %d + %d\nEnter your answer: ", a->x, a->y);
    scanf("%d", &a->answer);
    a->has_answer = 1;

    debug("killing supervisor thread (tid %d)\n", (int)a->supervisor_tid);
    pthread_cancel(a->supervisor_tid);

    return NULL;
}


// kill answer thread when round ends
// FIXME leaks memory allocated for message in wait_message and leaves open fd (can use pthread_cleanup_push?)
static void *supervisor_thread(void *arg) {
    supervisor_thread_args_t *a = arg;
    int old_cancel_type;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &old_cancel_type);

    message_pack_t *message = wait_message(MESSAGE_ROUND_END);
    debug("killing answer thread (tid %d)\n", (int)a->answer_tid);
    pthread_cancel(a->answer_tid);

    a->winner_id = message->player_id;
    free(message);

    return NULL;
}


void play_round(int x, int y) {
    answer_thread_args_t *answer_args = malloc(sizeof(answer_thread_args_t));
    supervisor_thread_args_t *supervisor_args = malloc(sizeof(answer_thread_args_t));
    message_pack_t *message;
    int round_end;

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    pthread_t answer_tid, supervisor_tid;

    do {
        // create threads and wait until either we got an answer or the round ends
        answer_args->x = x; answer_args->y = y;
        pthread_create(&answer_tid, &attr, &answer_thread, answer_args);
        pthread_create(&supervisor_tid, &attr, &supervisor_thread, supervisor_args);

        answer_args->supervisor_tid = supervisor_tid;
        supervisor_args->answer_tid = answer_tid;

        pthread_join(answer_tid, NULL);
        pthread_join(supervisor_tid, NULL);

        if(answer_args->has_answer) {
            // send answer to server and wait for ack
            answer_pack_t pack = { .player_id = player_id, .answer = answer_args->answer };
            int fd = open(ANSWER_FIFO, O_WRONLY);
            write(fd, &pack, sizeof(pack));
            close(fd);

            message = wait_message(MESSAGE_ANSWER_CORRECT | MESSAGE_ANSWER_WRONG);

            // wait_message has already checked that the message was directed at us
            if(message->type == MESSAGE_ANSWER_CORRECT) {
                printf("Answer is correct!\n");
                message = wait_message(MESSAGE_ROUND_END);
            }
            else {
                printf("Wrong answer!\n");
            }
        }
        else {
            printf("Player %lu gave the correct answer before you!\n",
                supervisor_args->winner_id);
            round_end = 1;
        }
    } while(!round_end);

    pthread_attr_destroy(&attr);
    free(answer_args);
    free(supervisor_args);
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

    } while(1);

    disconnect();
}
