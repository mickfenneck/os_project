#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <pthread.h>
#include "const.h"

typedef struct s_message_queue_t {
    message_pack_t *message;
    struct s_message_queue_t *next;
    struct s_message_queue_t *prev;
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


static void disconnect();
static long connect();
static void signal_handler(int);
static void shutdown();
static int init();
static void wait_challenge(int *, int *);
static void *answer_thread(void *);
static void *supervisor_thread(void *);
static void play_round(int, int);
static int get_answer(int *, int, int);
static void handle_victory(long, int);

static void handle_message(message_pack_t *);
static message_pack_t *get_message(int);
static message_pack_t *get_from_old(int);
static void *listener_thread(void *arg);

#endif
