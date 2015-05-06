#ifndef __CONST_H__
#define __CONST_H__

#define CHALLENGE_FIFO "/tmp/sisop-challenge"
#define ANSWER_FIFO "/tmp/sisop-answer"

#define CONNECT_FIFO "/tmp/sisop-connect"
#define DISCONNECT_FIFO "/tmp/sisop-disconnect"

#define CHALLENGE_LENGTH 25
#define ANSWER_LENGTH 25

// useful macro for debugging
#ifdef DEBUG
    #define debug(fmt, ...) \
        fprintf(stderr, "[debug - %s] ", __func__); \
        fprintf(stderr, fmt, ##__VA_ARGS__);
#else
    #define debug(fmt, ...)
#endif

typedef struct {
    int x;
    int y;
} challenge_pack_t;
extern challenge_pack_t SERVER_QUIT_PACK;  // TBD


typedef struct {
    long player_id;
    int answer;
} answer_pack_t;
#endif
