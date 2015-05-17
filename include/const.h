#ifndef __CONST_H__
#define __CONST_H__

#define DISCONNECT_FIFO "/tmp/sisop-disconnect"
#define CONNECT_FIFO "/tmp/sisop-connect"
#define ANSWER_FIFO "/tmp/sisop-answer"

#define FIFO_NAME_LENGTH 25

#define MESSAGE_ANSWER_CORRECT      1<<1
#define MESSAGE_ANSWER_WRONG        1<<2
#define MESSAGE_CHALLENGE           1<<3
#define MESSAGE_MATCH_END           1<<4
#define MESSAGE_SERVER_QUIT         1<<5
#define MESSAGE_CONNECTION_ACCEPTED 1<<6
#define MESSAGE_CONNECTION_REJECTED 1<<7
#define MESSAGE_ROUND_END           1<<8
#define MESSAGE_RANKING             1<<9

// useful macro for debugging
#ifdef DEBUG
    #define debug(fmt, ...) \
        fprintf(stderr, "[debug - %s:%d] ", __func__, __LINE__); \
        fprintf(stderr, fmt, __VA_ARGS__);
#else
    #define debug(fmt, ...)
#endif

typedef struct {
    long player_id;
    char fifo[FIFO_NAME_LENGTH];
} connection_pack_t;

typedef struct {
    long player_id;
    int answer;
} answer_pack_t;

typedef struct {
    int type;
    int x;
    int y;
    long player_id;
} message_pack_t;  // server -> client

#endif
