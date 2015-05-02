#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include "server.h"


pthread_t connection_tid;
pthread_t disconnection_tid;
pthread_t answer_tid;
 

void *connection_thread(void *arg) {
    int stop, connect_fifo = open(CONNECT_FIFO, O_RDONLY);

    printf("Waiting for players...\n");
    do {
        long player_id = -1;
        int ret = read(connect_fifo, &player_id, sizeof(player_id));
        if(ret < 0) {
            perror("[connection] read");
            break;
        }
        else if(ret == 0)
            continue;

        player_info_t *info = malloc(sizeof(player_info_t));
        if(!info) {
            printf("[connection] malloc\n");
            break;
        }
        else if(player_id == stop_msg) {
            printf("[connection] stopping\n");
            break;
        }

        pthread_mutex_lock(&shared->mutex);
        info->score = 0;
        info->player_id = player_id;
        info->next = shared->players;
        info->prev = NULL;
        if(shared->players)
            shared->players->prev = info;
        shared->players = info;
        shared->player_count += 1;
        stop = shared->global_stop;
        pthread_mutex_unlock(&shared->mutex);

        printf("Player %lu connected\n", info->player_id);
    } while(!stop);

    printf("[connection] stopped\n");
    close(connect_fifo);
    return NULL;
}


void *disconnection_thread(void *arg) {
    int stop;
    int fifo = open(DISCONNECT_FIFO, O_RDONLY);

    do {
        long player_id = -1;
        int ret = read(fifo, &player_id, sizeof(player_id));
        if(ret < 0) {
            perror("[disconnection] read");
            break;
        }
        else if(player_id == stop_msg) {
            printf("[disconnection] stopping\n");
            stop = 1;
            break;
        }
        else if(ret == 0)
            continue;

        pthread_mutex_lock(&shared->mutex);
        player_info_t *cursor = shared->players;
        while(cursor && cursor->player_id != player_id)
            cursor = cursor->next;

        if(cursor) {
            if(cursor->prev)
                cursor->prev->next = cursor->next;

            if(cursor->next)
                cursor->next->prev = cursor->prev;

            if(cursor == shared->players)
                shared->players = cursor->next;

            free(cursor);
            shared->player_count -= 1;
        }
        stop = shared->global_stop;
        pthread_mutex_unlock(&shared->mutex);

        printf("Player %lu disconnected.\n", player_id);
    } while(!stop);

    printf("[disconnection] stopped\n");
    close(fifo);
    return NULL;
}


void *answer_thread(void *arg) {
    int stop, fifo = open(ANSWER_FIFO, O_RDONLY);

    do {
        answer_pack_t *answer = malloc(sizeof(answer_pack_t));
        if(!answer) {
            perror("[answer] malloc");
            break;
        }

        int ret = read(fifo, &answer, sizeof(answer_pack_t));
        if(ret < 0) {
            perror("[answer] read");
            break;
        }
        else if( *((long*)answer) == stop_msg)  // FIXME makes me vomit
            break;
        
        player_info_t *check = shared->players;
        while(check && check->player_id != answer->player_id)
            check = check->next;
        if(!check) {
            printf("[answer] got pack from unknown player: %lu\n", answer->player_id);
            continue;
        }
        else {
            printf("[answer] got pack from player %lu, answer is %d\n",
                    answer->player_id, answer->answer);
        }

        pthread_mutex_lock(&shared->mutex);
        stop = shared->global_stop;
        answer->next = NULL;
        if(!shared->answers)
            shared->answers = answer;
        else {
            answer_pack_t *cursor = shared->answers;
            while(cursor->next)
                cursor = cursor->next;
            cursor->next = answer;
        }
        pthread_mutex_unlock(&shared->mutex);
    } while(!stop);

    close(fifo);
    return NULL;
}


void make_threads() {
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    if(pthread_create(&connection_tid, &attr, &connection_thread, NULL) ||
            pthread_create(&disconnection_tid, &attr, &disconnection_thread, NULL) ||
            pthread_create(&answer_tid, &attr, &answer_thread, NULL)) {

        perror("pthread_create");
        exit(-1);
    }
}


void send_stop(const char *fifo) {
    printf("stopping %s\n", fifo);
    int fd = open(fifo, O_WRONLY | O_NONBLOCK);
    write(fd, &stop_msg, sizeof(stop_msg));
    close(fd);
}


void destroy_threads() {
    printf("Sending stop command...\n");
    send_stop(CONNECT_FIFO);
    send_stop(DISCONNECT_FIFO);
    send_stop(ANSWER_FIFO);

    printf("Joining threads...\n");
    pthread_join(disconnection_tid, NULL);
    pthread_join(connection_tid, NULL);
    pthread_join(answer_tid, NULL);
}
