#include "client.h"

char fifo[FIFO_NAME_LENGTH];


static void handle_victory(long winner, int player_count) {
    message_pack_t *message;

    printf("Match has ended!\n\n");
    debug("winner %lu player id %lu\n", winner, player_id);
    if(winner == player_id) {
        printf("-------------------------------\n");
        printf("---        YOU WON          ---\n");
        printf("-------------------------------\n");
    }
    else printf("Player %lu won\n", winner);
    printf("\n*** Final Ranking ***\n");

    // receive and print ranking
    int received = 0;
    while(received < player_count) {
        message = wait_message(MESSAGE_RANKING);
        int position = message->x,
            score = message->y;

        if(message->player_id == player_id) {
            printf("\t%d)  ---> Player %lu - Score %d <---\n", 
                position, message->player_id, score);
        }
        else {
            printf("\t%d)       Player %lu - Score %d\n",
                position, message->player_id, score);
        }

        received = message->x;
        free(message);
    }
}


// default handler for messages
static void handle_message(message_pack_t *message) {
    if(message->type == MESSAGE_SERVER_QUIT) {
        printf("Server has quit!\n");
        free(message);

        shutdown();
        exit(0);
    }
    else if(message->type == MESSAGE_MATCH_END) {
        handle_victory(message->player_id, message->x);
        free(message);
    }
}


// wait until a message of a given type arrives (remember to free)
static message_pack_t *wait_message(int type) {
    int received;
    message_pack_t *message = malloc(sizeof(message_pack_t));
    
    do {
        int fd = open(fifo, O_RDONLY);
        read(fd, message, sizeof(message_pack_t));
        close(fd);

        debug("received message %d\n", message->type);
        if(message->type & type) {
            // if needed, check that we are the recipient
            if(message->type & (MESSAGE_ANSWER_CORRECT | MESSAGE_ANSWER_WRONG)) {
                if(message->player_id == player_id)
                    received = 1;
                else handle_message(message);
            }
            else received = 1;
        }
        else handle_message(message);

        close(fd);
    } while(!received);

    return message;
}


// connect to server and wait for ack, return > 0 -> ok, < 0 -> fatal
static long connect() {
    int fd;
    connection_pack_t pack;

    player_id = (long) getpid();
    printf("Our player id is %lu\n", player_id);

    sprintf(fifo, "/tmp/sisop-client-%lu", player_id);
    debug("creating fifo %s\n", fifo);
    if(mkfifo(fifo, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) != 0) {
        perror("mkfifo");
        return -1;
    }

    printf("Connecting to server...\n");
    if((fd = open(CONNECT_FIFO, O_WRONLY)) < 0) {
        perror("open");
        printf("No server running?\n");
        return -1;
    }

    debug("sending connection request%s", "\n");
    pack.player_id = player_id;
    strcpy(pack.fifo, fifo);
    write(fd, &pack, sizeof(pack));
    close(fd);

    debug("waiting for ack%s", "\n");  // FIXME hangs forever if server quits now
    message_pack_t *message = wait_message(MESSAGE_CONNECTION_ACCEPTED |
                                           MESSAGE_CONNECTION_REJECTED);
    if(message->type == MESSAGE_CONNECTION_ACCEPTED) {
        printf("Connected!\n");
        free(message);
        return 1;
    }
    else { // if(message->type == MESSAGE_CONNECTION_REJECTED) {
        printf("Our connection was rejected!\n");
        free(message);
        return -1;
    }
}

