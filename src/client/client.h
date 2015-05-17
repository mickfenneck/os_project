#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "const.h"

long player_id;


static void disconnect();
static long connect();
static void signal_handler(int);
static message_pack_t *wait_message(int);
static void shutdown();
static void wait_challenge(int *, int *);
static void *answer_thread(void *);
static void *supervisor_thread(void *);
static void play_round(int, int);
static void handle_message(message_pack_t *);
static message_pack_t *wait_message(int);
static int get_answer(int *, int, int);
static void handle_victory(long, int);

#endif
