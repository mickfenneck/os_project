#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "client.h"


typedef struct {
    pid_t answer_pid;
    pid_t supervisor_pid;

    int has_answer;
    int answer;
} uinput_shared_t;


// gets an answer from the player
static void answer_process(uinput_shared_t *shared, int x, int y) {
    signal(SIGINT, SIG_DFL);  // remove (the copy we have of) the main client handler

    shared->answer_pid = getpid();
    debug("waiting for user input (pid %d)\n", getpid());

    char buffer[128], *ptr;
    do {
        printf("Challenge is %d + %d\nEnter your answer: ", x, y);
        fgets(buffer, sizeof(buffer), stdin);
        shared->answer = strtol(buffer, &ptr, 10);
    } while(ptr[0] != '\n' && shared->answer >= 0);

    debug("killing supervisor process (pid %d)\n", shared->supervisor_pid);
    shared->has_answer = 1;
    kill(shared->supervisor_pid, SIGUSR1);
}


// get_message will be stuck in a pthread_mutex_lock, we have to unlock it!
static void supervisor_handler(int signo) {
    pthread_mutex_lock(&shared.mutex);  // global client shared
    shared.waiting_type = 0;
    pthread_mutex_unlock(&shared.mutex);
    pthread_mutex_unlock(&shared.waiting);
}


// kill answer thread when round ends
static void supervisor_process(uinput_shared_t *shared) {
    signal(SIGINT, SIG_DFL);  // remove (the copy we have of) the main client handler
    signal(SIGUSR1, supervisor_handler);
    shared->supervisor_pid = getpid();

    debug("waiting for MESSAGE_ROUND_END (%d)\n", MESSAGE_ROUND_END);
    message_pack_t *message = get_message(MESSAGE_ROUND_END);
    if(message) {// if answer_process sends a SIGUSR1 this will be NULL
        debug("sending signal to answer process (pid %d)\n", shared->answer_pid);
        kill(shared->answer_pid, SIGUSR1);
    }
    else debug("killed by answer process (pid %d)\n", shared->answer_pid);
}


// returns 1 if answer was given by user (and puts answer in *answer)
int get_answer(int *answer, int x, int y) {
    uinput_shared_t *shared = mmap(NULL, sizeof(uinput_shared_t), PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    shared->has_answer = shared->answer = 0;

    if(fork() == 0) {
        if(fork() == 0)
            supervisor_process(shared);
        else
            answer_process(shared, x, y);
        exit(0);
    }
    else {
        wait(NULL);  // wait either child, they will both die

        int ret = shared->has_answer;
        *answer = shared->answer;
        munmap(shared, sizeof(uinput_shared_t));

        return ret;
    }
}
