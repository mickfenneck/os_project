#ifndef __CONST_H__
#define __CONST_H__

#define CHALLENGE_FIFO "/tmp/sisop-challenge"
#define ANSWER_FIFO "/tmp/sisop-answer"

#define CONNECT_FIFO "/tmp/sisop-connect"
#define DISCONNECT_FIFO "/tmp/sisop-disconnect"

#define CHALLENGE_LENGTH 25
#define ANSWER_LENGTH 25

struct a_pack_t {
    // filled by client
    long player_id;
    int answer;

    struct a_pack_t *next;  // filled by server
};

typedef struct a_pack_t answer_pack_t;

#endif
