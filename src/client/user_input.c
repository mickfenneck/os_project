#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "client.h"


typedef struct {
    pthread_t answer_tid;
    pthread_t supervisor_tid;

    int has_answer;
    int other_answered;
    int answer;

    int x;
    int y;
} ui_shared_t;
static ui_shared_t *ui_shared;


// gets an answer from the player
static void *answer_thread(void *arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    debug("answer thread running%s", "\n");

    char buffer[128], *ptr;
    do {
        printf("Challenge is %d + %d\nEnter your answer: ", ui_shared->x, ui_shared->y);
        fflush(stdout);  // force write last line to stdout
        read(0, buffer, sizeof(buffer));

        ui_shared->answer = strtol(buffer, &ptr, 10);
        debug("answer %d\n", ui_shared->answer);
    } while(ptr[0] != '\n' && ui_shared->answer >= 0);

    debug("killing supervisor process%s", "\n");
    ui_shared->has_answer = 1;
    pthread_kill(ui_shared->supervisor_tid, SIGUSR1);

    return NULL;
}


// get_message will be stuck in a pthread_mutex_lock, we have to unlock it!
static void supervisor_handler(int signo) {
    debug("received signal %d\n", signo);
    pthread_mutex_lock(&shared.mutex);  // global client shared
    shared.waiting_type = 0;
    pthread_mutex_unlock(&shared.waiting);
    pthread_mutex_unlock(&shared.mutex);
}


// kill answer thread when round ends
static void *supervisor_thread(void *arg) {
    debug("supervisor thread running%s", "\n");
    message_pack_t *message = get_message(MESSAGE_ROUND_END | MESSAGE_SERVER_QUIT, 1);

    // if we receive a SIGUSR1 and wake up message will be NULL
    if(message) {
        if(message->type == MESSAGE_ROUND_END) {
            debug("round end, killing answer thread%s", "\n");
            ui_shared->other_answered = 1;
        }
        else debug("server quit, killing answer thread%s", "\n");

        pthread_cancel(ui_shared->answer_tid);
        free(message);
    }
    else debug("killed by another process%s", "\n");

    return NULL;
}


// returns 1 if answer was given by user (and puts answer in *answer)
// returns 0 if answer was given by user, 1 if by other player, 2 if cancelled
static int get_answer(int *answer, int x, int y) {
    ui_shared = malloc(sizeof(ui_shared_t));
    ui_shared->has_answer = ui_shared->answer = ui_shared->other_answered = 0;
    ui_shared->x = x;
    ui_shared->y = y;

    signal(SIGUSR1, supervisor_handler);

    pthread_create(&ui_shared->answer_tid, NULL, &answer_thread, NULL);
    pthread_create(&ui_shared->supervisor_tid, NULL, &supervisor_thread, NULL);

    pthread_join(ui_shared->answer_tid, NULL);
    pthread_join(ui_shared->supervisor_tid, NULL);

    signal(SIGUSR1, SIG_DFL);

    int ret;
    if(ui_shared->has_answer)
        ret = 0;
    else if(ui_shared->other_answered)
        ret = 1;
    else ret = 2;
    *answer = ui_shared->answer;

    free(ui_shared);
    ui_shared = NULL;

    return ret;
}


// cleanup
static void stop_ui_processes() {
    if(ui_shared) {
        debug("killing user input threads%s", "\n");
        pthread_kill(ui_shared->supervisor_tid, SIGUSR1);
        pthread_cancel(ui_shared->answer_tid);
        munmap(ui_shared, sizeof(ui_shared_t));
    }
}
