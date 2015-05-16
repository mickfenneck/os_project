#ifndef __SERVER_H__
#define __SERVER_H__

typedef struct {
    long player_id;
    int score;
    int answer;
    int fifo;
} player_info_t; 


#define MIN_PLAYERS 1  // minimum number of players needed to start the game
#define MAX_PLAYERS 10  // maximum allowed number of connected players

#endif
