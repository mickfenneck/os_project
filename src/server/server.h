#include "const.h"


const long stop_msg = 0xfedcba9876543210;  // better if negative

struct p_info_t {
    int score;
    long int player_id;

    struct p_info_t *next;
    struct p_info_t *prev;
};
typedef struct p_info_t player_info_t;


typedef struct {
    player_info_t *players;
    answer_pack_t *answers;

    int player_count;
    int global_stop;

    pthread_mutex_t mutex;
} server_shared_t;
server_shared_t *shared;
