#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include "const.h"


long player_id;

static void disconnect();
long connect();
static void signal_handler(int);
static void wait_challenge(challenge_pack_t *);
static void send_answer(int);
static int pack_eq(challenge_pack_t *, challenge_pack_t *);


static int pack_eq(challenge_pack_t *p1, challenge_pack_t *p2) {
    return p1->x == p2->x && p1->y == p2->y;
}


static void disconnect() {
    printf("\nDisconnecting...\n");
    int fifo = open(DISCONNECT_FIFO, O_WRONLY | O_NONBLOCK);
    write(fifo, &player_id, sizeof(player_id));
    close(fifo);
}


long connect() {
    player_id = (long) getpid();
    printf("Our player id is %lu\n", player_id);

    int fifo;
    printf("Connecting to server...\n");
    if((fifo = open(CONNECT_FIFO, O_WRONLY)) < 0) {
        perror("open");
        printf("No server running?\n");
        exit(-1);
    }
    write(fifo, &player_id, sizeof(player_id));
    close(fifo);
    printf("Connected!\n");

    return player_id;
}


static void signal_handler(int signo) {
    disconnect();
    exit(-1);
}


static void wait_challenge(challenge_pack_t *challenge) {
    int fifo = open(CHALLENGE_FIFO, O_RDONLY);
    if(fifo < 0) {
        perror("open");
        exit(-1);
    }

    int ret = read(fifo, challenge, sizeof(challenge_pack_t));
    close(fifo);

    debug("ret %d errno %d\n", ret, errno);

    if(ret < 0) {
        perror("read");
        exit(-1);
    }

    return;
}


static void send_answer(int answer) {
    int fifo = open(ANSWER_FIFO, O_WRONLY);
    if(fifo < 0) {
        perror("open");
        printf("Server has quit?\n");
        exit(-1);
    }

    answer_pack_t pack;
    pack.answer = answer;
    pack.player_id = player_id;
    write(fifo, &pack, sizeof(pack));
    close(fifo);

    debug("challenge sent - errno %d\n", errno);
}


static void print_ranking(player_info_t *players, int player_num, const char *caption) {
    int i;

    printf("*** %s ***\n", caption);
    for(i = 0; i < player_num; i++) {
        printf(players[i].player_id == player_id ? "   --> " : "       ");
        printf("Player %lu - Score %d", players[i].player_id, players[i].score);
        printf(players[i].player_id == player_id ? " <--\n" : "\n");
    }
}


int client_main() {
    if(signal(SIGINT, signal_handler) == SIG_ERR) {
        perror("signal");
        exit(-1);
    }

    connect();
    while(player_id >= 0) {
        int answer;
        challenge_pack_t challenge;

        printf("Waiting for challenge (CTRL+C to quit)...\n");
        wait_challenge(&challenge);

        if(pack_eq(&challenge, &SERVER_QUIT_PACK)) {
            printf("Server has ended the game.\n");
            player_id = -1;
        }
        else if(pack_eq(&challenge, &SERVER_VICTORY_PACK)) {
            printf("\n\nMatch ended! Player %lu won!\n", challenge.ranking[0].player_id);
            print_ranking(challenge.ranking, challenge.player_num, "Final Ranking");
            disconnect();
            break;
        }
        else {
            printf("Challenge received!\n");
            print_ranking(challenge.ranking, challenge.player_num, "Current Ranking");

            printf("%d + %d = ", challenge.x, challenge.y);
            scanf("%d", &answer);
            send_answer(answer);
        }
    }

    return 0;
}
