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
    int answer;

    int x;
    int y;
} ui_shared_t;
static ui_shared_t *ui_shared;


static void answer_handler(int signo) {
    debug("received signal %d\n", signo);
    pthread_exit(NULL); // have to check that I am answer thread?
}

// gets an answer from the player
static void *answer_thread(void *arg) {
    debug("answer thread running%s", "\n");

    char buffer[128], *ptr;
    do {
        printf("Challenge is %d + %d\nEnter your answer: ", ui_shared->x, ui_shared->y);
        fgets(buffer, sizeof(buffer), stdin);
        ui_shared->answer = strtol(buffer, &ptr, 10);
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

    //pthread_exit(NULL);
}


// kill answer thread when round ends
static void *supervisor_thread(void *arg) {
    debug("supervisor thread running%s", "\n");
    message_pack_t *message = get_message(MESSAGE_ROUND_END);
    if(message) {  // if someone sends a SIGKILL to stop us this will be NULL
        debug("sending signal to answer process%s", "\n");
        pthread_kill(ui_shared->answer_tid, SIGUSR2);
    }
    else debug("killed by answer process%s", "\n");

    return NULL;
}


// returns 1 if answer was given by user (and puts answer in *answer)
static int get_answer(int *answer, int x, int y) {
    ui_shared = malloc(sizeof(ui_shared_t));
    ui_shared->has_answer = ui_shared->answer = 0;
    ui_shared->x = x;
    ui_shared->y = y;

    signal(SIGUSR1, supervisor_handler);
    signal(SIGUSR2, answer_handler);

    pthread_create(&ui_shared->answer_tid, NULL, &answer_thread, NULL);
    pthread_create(&ui_shared->supervisor_tid, NULL, &supervisor_thread, NULL);

    pthread_join(ui_shared->answer_tid, NULL);
    pthread_join(ui_shared->supervisor_tid, NULL);

    signal(SIGUSR1, SIG_DFL);
    signal(SIGUSR2, SIG_DFL);

    int ret = ui_shared->has_answer;
    *answer = ui_shared->answer;

    free(ui_shared);
    ui_shared = NULL;

    return ret;
}


// cleanup
static void stop_ui_processes() {
    if(ui_shared) {
        kill(ui_shared->supervisor_tid, SIGUSR1);
        kill(ui_shared->answer_tid, SIGUSR2);
        munmap(ui_shared, sizeof(ui_shared_t));
    }
}
