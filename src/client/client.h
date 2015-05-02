
typedef struct {
    char challenge[CHALLENGE_LENGTH];
    int global_stop;

    pthread_mutex_t mutex;
    pthread_mutex_t challenge_available;
    pthread_mutex_t answer_available;
} client_shared_t;
client_shared_t *shared = NULL; 


