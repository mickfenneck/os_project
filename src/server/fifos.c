#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "const.h"
#include "server.h"

static int connect_fifo, disconnect_fifo, answer_fifo;


// init fifos, returns 0 -> ok, -1 -> fatal
static int fifo_init() {
    int MODE = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    if(mkfifo(DISCONNECT_FIFO, MODE) != 0 ||
        mkfifo(CONNECT_FIFO, MODE) != 0 ||
        mkfifo(ANSWER_FIFO, MODE) != 0) {

        perror("mkfifo");
        printf("Another server running?\n");
        return -1;
    }

    if((connect_fifo = open(CONNECT_FIFO, O_RDONLY | O_NONBLOCK)) < 0 ||
        (answer_fifo = open(ANSWER_FIFO, O_RDONLY | O_NONBLOCK)) < 0 || 
        (disconnect_fifo = open(DISCONNECT_FIFO, O_RDONLY | O_NONBLOCK)) < 0) {

        perror("open");
        return -1;
    }

    return 0;
}


// destroys all the fifos
static void fifo_destroy() {
    debug("closing fifos%s", "\n");
    close(connect_fifo); close(disconnect_fifo); close(answer_fifo);

    debug("destroying fifos%s", "\n");
    unlink(ANSWER_FIFO); unlink(CONNECT_FIFO); unlink(DISCONNECT_FIFO);
}


// accept all pending connections
static void accept_connections(player_info_t *players, int *player_count, int max_players) {
    int ret, fd, i;
    connection_pack_t pack;
    message_pack_t message;

    while((ret = read(connect_fifo, &pack, sizeof(pack))) > 0) {
        debug("ret %d errno %d\n", ret, errno);

        // opening in read/write avoids SIGPIPE later
        fd = open(pack.fifo, O_RDWR | O_NONBLOCK);
        if(fd < 0) {
            debug("cannot open write fifo %s for player %lu\n",
                pack.fifo, pack.player_id);
            printf("Error while a player was connecting.\n");
            close(fd);
        }
        else {
            debug("player fifo is %s\n", pack.fifo);
            if(*player_count < max_players) {
                players[*player_count].player_id = pack.player_id;
                players[*player_count].score = max_players - *player_count - 1;
                players[*player_count].fifo = fd;

                send_message(*player_count, MESSAGE_CONNECTION_ACCEPTED, 0, 0, -1);
                for(i = 0; i < *player_count; i++)
                    send_message(i, MESSAGE_PLAYER_CONNECTED, 0, 0, pack.player_id);

                printf("Player %lu connected (%d players connected)\n",
                    players[*player_count].player_id, *player_count + 1);

                *player_count += 1;
            }
            else {
                message.type = MESSAGE_CONNECTION_REJECTED;
                message.player_id = pack.player_id;
                message.x = message.y = 0;
                write(fd, &message, sizeof(message));
                close(fd);

                printf("Rejecting connection from player %lu (too many connected players)\n",
                       pack.player_id);
            }
        }
    }
}


// handle all pending disconnections
static void accept_disconnections(player_info_t *players, int *player_count) {
    int ret, i;
    long player_id;

    while((ret = read(disconnect_fifo, &player_id, sizeof(player_id))) > 0) {
        debug("ret %d errno %d\n", ret, errno);

        for(i = 0; i < *player_count; i++)
            if(players[i].player_id == player_id)
                break;

        if(i < *player_count) {
            for(; i < *player_count; i++)
                players[i] = players[i + 1];
            *player_count -= 1;
            printf("Player %lu disconnected.\n", players[i].player_id);
            for(i = 0; i < *player_count; i++)
                send_message(i, MESSAGE_PLAYER_DISCONNECTED, 0, 0, player_id);
        }
        else {
            debug("Unknown disconnection message from player %lu\n", player_id);
        }
    }
}


// accept the first available answer, returns player or -1
static int accept_answer(player_info_t *players, int player_count) {
    int ret, player = -1;
    answer_pack_t pack;

    if((ret = read(answer_fifo, &pack, sizeof(pack))) > 0) {
        debug("ret %d errno %d\n", ret, errno);

        int i = 0;
        while(i < player_count && players[i].player_id != pack.player_id)
            i += 1;

        if(i < player_count) {
            players[i].answer = pack.answer;
            printf("Received answer %d from player %lu\n", pack.answer,
                   pack.player_id);
            player = i;
        }
        else {
            printf("Received answer from unknown player %lu\n", pack.player_id);
        }
    }

    return player;
}

