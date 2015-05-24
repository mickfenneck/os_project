/****
 * Multiplayer Game
 * Progetto Sistemi Operativi 2015
 *
 * Dorigatti Emilio - 165907
 * Sordo Michele - 164492
 ****/

#ifndef __SERVER_H__
#define __SERVER_H__

typedef struct {
    long player_id;
    int score;
    int answer;
    int fifo;
} player_info_t; 

static void send_message(int player, int type, int x, int y, long player_id);

#define MIN_PLAYERS 1  // minimum number of players needed to start the game
#define MAX_PLAYERS 10  // maximum allowed number of connected players


static void shutdown();
static void signal_handler(int signo);
static void pipe_handler(int signo);
static void send_message(int player, int type, int x, int y, long player_id);
static void wait_for_players(int max_players);
static void send_challenge(int x, int y);
static void get_challenge(int *x, int *y, int *correct);
static int print_ranking();
static void play_round(int max_players, int correct);
static void match_end(int winner);
static void play(int max_players, int win_score);
static int connect_fifo, disconnect_fifo, answer_fifo;
static int fifo_init();
static void fifo_destroy();
static void accept_connections(player_info_t *players, int *player_count, int max_players);
static void accept_disconnections(player_info_t *players, int *player_count);
static int accept_answer(player_info_t *players, int player_count);


#endif
