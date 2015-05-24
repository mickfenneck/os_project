/****
 * Multiplayer Game
 * Progetto Sistemi Operativi 2015
 *
 * Dorigatti Emilio - 165907
 * Sordo Michele - 164492
 ****/

#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <pthread.h>
#include "const.h"

typedef struct s_message_queue_t {
    message_pack_t *message;
    struct s_message_queue_t *next;
} message_queue_t;

typedef struct {
    message_queue_t *messages;
    int waiting_type;
    int global_stop;
    pthread_mutex_t waiting;
    
    pthread_mutex_t mutex;
} client_shared_t;


long player_id;
client_shared_t shared;


static int on_message_received(message_pack_t *message);
static void *listener_thread(void *arg);
static message_pack_t *get_from_old(int type);
static message_pack_t *get_message(int type, int wait);
static void handle_message(message_pack_t *message);
static void *answer_thread(void *arg);
static void supervisor_handler(int signo);
static void *supervisor_thread(void *arg);
static int get_answer(int *answer, int x, int y);
static void stop_ui_processes();
static void shutdown();
static void disconnect();
static void signal_handler(int signo);
static int wait_challenge(int *x, int *y);
static void play_round(int x, int y);
static int init();
static pthread_t start_listener();
static void stop_listener(pthread_t tid);
static void handle_victory(long winner, int player_count);
static long connect();


#endif
