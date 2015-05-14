#ifndef __CONST_H__
#define __CONST_H__

#define STATUS_FIFO "/tmp/sisop-status"
#define CONNECT_FIFO "/tmp/sisop-connect"
#define ANSWER_FIFO "/tmp/sisop-answer"

#define MIN_PLAYERS 1  // minimum number of players needed to start the game
#define MAX_PLAYERS 10  // maximum allowed number of connected players

#define FIFO_NAME_LENGTH 25

#define MESSAGE_DISCONNECTION 1

// useful macro for debugging
#ifdef DEBUG
    #define debug(fmt, ...) \
        fprintf(stderr, "[debug - %s] ", __func__); \
        fprintf(stderr, fmt, __VA_ARGS__);
#else
    #define debug(fmt, ...)
#endif

typedef struct {
    int x;
    int y;
} challenge_pack_t;

typedef struct {
    long player_id;
    char fifo_rd[FIFO_NAME_LENGTH];
    char fifo_rw[FIFO_NAME_LENGTH];
} connection_pack_t;

typedef struct {
    int message_type;
    int argument;
} message_pack_t;

#endif
