#ifndef __SERVER_H__
#define __SERVER_H__

typedef struct {
    long player_id;
    int score;
    int answer;
    int fifo;
} player_info_t; 

static void send_message(int player, int type, int x, int y, long player_id);

#define MIN_PLAYERS 2  // minimum number of players needed to start the game
#define MAX_PLAYERS 10  // maximum allowed number of connected players


static void shutdown();
static void signal_handler(int);
static void pipe_handler(int);
static void send_message(int, int, int, int, long);
static void wait_for_players(int);
static void send_challenge(int, int);
static void get_challenge(int *, int *, int *);
static int print_ranking();
static void play_round(int, int);
static void play(int, int);
static int fifo_init();
static void fifo_destroy();
static int accept_answer();
static void accept_connections(player_info_t *, int *, int);
static void accept_disconnections(player_info_t *, int *);
static int accept_answer(player_info_t *, int);

#endif
