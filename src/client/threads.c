#include <pthread.h>
#include "client.h"


typedef struct {
    int x;
    int y;
    pthread_t supervisor_tid;
    int answer;
    int has_answer;
} answer_thread_args_t;


typedef struct {
    pthread_t answer_tid;
    long winner_id;
} supervisor_thread_args_t;


// gets an answer from the player
static void *answer_thread(void *arg) {
    char buffer[32], *ptr;

    answer_thread_args_t *a = arg;
    int old_cancel_type;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &old_cancel_type);

    a->has_answer = 0;

    do {
        printf("Challenge is %d + %d\nEnter your answer: ", a->x, a->y);
        fgets(buffer, sizeof(buffer), stdin);
        a->answer = strtol(buffer, &ptr, 10);
    } while(ptr[0] != '\n');

    a->has_answer = 1;
    debug("killing supervisor thread (tid %d)\n", (int)a->supervisor_tid);
    pthread_cancel(a->supervisor_tid);

    return NULL;
}


// kill answer thread when round ends
// FIXME leaks memory allocated for message in wait_message and leaves open fd
// (can use pthread_cleanup_push?)
static void *supervisor_thread(void *arg) {
    supervisor_thread_args_t *a = arg;
    int old_cancel_type;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &old_cancel_type);

    message_pack_t *message = get_message(MESSAGE_ROUND_END);
    debug("killing answer thread (tid %d)\n", (int)a->answer_tid);
    pthread_cancel(a->answer_tid);

    a->winner_id = message->player_id;
    free(message);

    return NULL;
}

int get_answer(int *answer, int x, int y) {
    pthread_t answer_tid, supervisor_tid;

    answer_thread_args_t *answer_args = malloc(sizeof(answer_thread_args_t));
    supervisor_thread_args_t *supervisor_args = malloc(sizeof(answer_thread_args_t));

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    answer_args->x = x; answer_args->y = y;
    pthread_create(&answer_tid, &attr, &answer_thread, answer_args);
    pthread_create(&supervisor_tid, &attr, &supervisor_thread, supervisor_args);

    answer_args->supervisor_tid = supervisor_tid;
    supervisor_args->answer_tid = answer_tid;

    pthread_join(answer_tid, NULL);
    pthread_join(supervisor_tid, NULL);

    int has_answer = answer_args->has_answer,
        user_answer = answer_args->answer;

    pthread_attr_destroy(&attr);
    free(answer_args);
    free(supervisor_args);

    if(has_answer) {
        *answer = user_answer;
        return 1;
    }
    else return 0;
}
