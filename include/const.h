#ifndef __CONST_H__
#define __CONST_H__

#define CHALLENGE_FIFO "/tmp/sisop-challenge"
#define ANSWER_FIFO "/tmp/sisop-answer"

#define CONNECT_FIFO "/tmp/sisop-connect"
#define DISCONNECT_FIFO "/tmp/sisop-disconnect"

#define MIN_PLAYERS 1  // minimum number of players needed to start the game
#define MAX_PLAYERS 10  // maximum allowed number of connected players

// useful macro for debugging
#ifdef DEBUG
    #define debug(fmt, ...) \
        fprintf(stderr, "[debug - %s] ", __func__); \
        fprintf(stderr, fmt, __VA_ARGS__);
#else
    #define debug(fmt, ...)
#endif

typedef struct {
    long int player_id;
    int score;
    int answer;
    int has_answered;
} player_info_t;

typedef struct {
    int x;
    int y;

    player_info_t ranking[MAX_PLAYERS];
    int player_num;
} challenge_pack_t;

extern challenge_pack_t SERVER_QUIT_PACK;  // TBD
extern challenge_pack_t SERVER_VICTORY_PACK;  // TBD

typedef struct {
    long player_id;
    int answer;
} answer_pack_t;

#endif
