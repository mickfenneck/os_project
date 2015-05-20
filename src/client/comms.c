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
        message = get_message(MESSAGE_RANKING);
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


// connect to server and wait for ack, return > 0 -> ok, < 0 -> fatal
static long connect() {
    int fd, ret;
    connection_pack_t pack;
    message_pack_t message;

    player_id = (long) getpid();
    printf("Our player id is %lu\n", player_id);

    sprintf(fifo, "/tmp/sisop-client-%lu", player_id);
    debug("creating fifo %s\n", fifo);
    if(mkfifo(fifo, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) != 0) {
        perror("mkfifo");
        return 0;
    }

    printf("Connecting to server...\n");
    if((fd = open(CONNECT_FIFO, O_WRONLY)) < 0) {
        perror("open");
        printf("No server running?\n");
        return 0;
    }

    pack.player_id = player_id;
    strcpy(pack.fifo, fifo);
    ret = write(fd, &pack, sizeof(pack));
    close(fd);
    debug("sent connection request, ret %d errno %d\n", ret, errno);

    debug("waiting for ack%s", "\n");  // FIXME hangs forever if server quits now
    fd = open(fifo, O_RDONLY);
    ret = read(fd, &message, sizeof(message));
    close(fd);
    debug("received message of type %d, ret %d errno %d\n", message.type,
        ret, errno);
    
    if(message.type == MESSAGE_CONNECTION_ACCEPTED) {
        printf("Connected!\n");
        return 1;
    }
    else { // if(message->type == MESSAGE_CONNECTION_REJECTED) {
        printf("Our connection was rejected!\n");
        return 0;
    }
}

