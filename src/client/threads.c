pthread_t challenge_tid;
pthread_t answer_tid;

void *challenge_thread(void *arg) {
    int stop, fifo = open(CHALLENGE_FIFO, O_RDONLY);

    char buffer[CHALLENGE_LENGTH];
    do {
        pthread_mutex_lock(&shared->answer_available);

        int ret = read(fifo, buffer, sizeof(buffer));
        if(ret < 0) {
            perro("[challenge] read");
            break;
        }

        pthread_mutex_lock(&shared->mutex);
        strcpy(shared->challenge, buffer);
        stop = shared->global_stop;
        pthread_mutex_unlock(&shared->mutex);

        pthread_mutex_unlock(&shared->challenge_available);
    } while(!stop);

    close(fifo);
    return NULL;
}


void *answer_thread(void *arg) {
    int stop, fifo = open(ANSWER_FIFO, O_WRONLY);

    char challenge[CHALLENGE_LENGTH];
    answer_pack_t answer;
    answer.player_id = player_id;

    do {
        pthread_mutex_lock(&shared->challenge_available);

        pthread_mutex_lock(&shared->mutex);
        strcpy(challenge, shared->challenge);
        stop = shared->global_stop;
        pthread_mutex_unlock(&shared->mutex);

        printf("Challenge received: %s\nAnswer is: ", challenge);
        scanf("%d", &answer.answer);
        write(fifo, &answer, sizeof(answer_pack_t));

        pthread_mutex_unlock(&shared->answer_available);
    } while(!stop);

    close(fifo);
    return NULL;
}
