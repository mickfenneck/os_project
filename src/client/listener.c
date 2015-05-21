#include <pthread.h>
#include "client.h"


// handle a message, appending it to the received queue and waking up waiting thread if needed
static int on_message_received(message_pack_t *message) {
    int stop;

    if(message->type == MESSAGE_SERVER_QUIT) {
        printf("Server has quit!\n");
        shutdown();
        exit(0);
    }

    pthread_mutex_lock(&shared.mutex);
    stop = shared.global_stop;

    // put message in queue
    if(!shared.messages) {
        shared.messages = malloc(sizeof(message_queue_t));
        shared.messages->message = message;
        shared.messages->next = NULL;
    }
    else {
        message_queue_t *cursor = shared.messages;
        while(cursor->next)
            cursor = cursor->next;
        cursor->next = malloc(sizeof(message_queue_t));
        cursor->next->message = message;
        cursor->next->next = NULL;
    }

    // if needed wake up waiting thread
    if(message->type & shared.waiting_type) {
        debug("waking up waiting thread%s", "\n");
        shared.waiting_type = 0;
        pthread_mutex_unlock(&shared.waiting);
    }
    pthread_mutex_unlock(&shared.mutex);

    return stop;
}


// continuously listens for messages from server
static void *listener_thread(void *arg) {
    message_pack_t *message;
    int stop, ret, fd;
    
    debug("listener thread running with id %lu\n", (long) pthread_self());
    
    do {
        stop = 0;
        fd = open(fifo, O_RDONLY);

        do {
            message = malloc(sizeof(message_pack_t));
            ret = read(fd, message, sizeof(message_pack_t));

            debug("received message %d from server, ret %d errno %d\n", message->type,
                ret, errno);

            stop |= on_message_received(message);
        } while(ret > 0);

        close(fd);
    } while(!stop);

    debug("listener thread has quit%s", "\n");
    return NULL;
}


// get the first message of the given type in the received queue
// also consumes all previous messages with the default handler
// lock the mutex before calling this!!!
static message_pack_t *get_from_old(int type) {
    while(shared.messages) {
        message_pack_t *message = shared.messages->message;
        
        message_queue_t *next = shared.messages->next;
        free(shared.messages);
        shared.messages = next;

        if(message->type & type) {
            debug("found suitable message of type %d\n", message->type);
            return message;
        }
        else handle_message(message);
    }
    
    debug("no suitable messages of type %d in received queue\n", type);
    return NULL;
}


// get the first available message of the given type
static message_pack_t *get_message(int type, int wait) {
    pthread_mutex_lock(&shared.mutex);
    
    message_pack_t *message = get_from_old(type);
    if(message) {
        pthread_mutex_unlock(&shared.mutex);
        return message;
    }
    else if(wait) {
        debug("waiting for message of type %d...\n", type);
        shared.waiting_type = type;
        pthread_mutex_unlock(&shared.mutex);
        pthread_mutex_lock(&shared.waiting);
        
        debug("woke up, checking messages%s", "\n");
        pthread_mutex_lock(&shared.mutex);
        message_pack_t *message = get_from_old(type);
        pthread_mutex_unlock(&shared.mutex);

        return message;
    }
    else return NULL;
}


// default handler for messages
static void handle_message(message_pack_t *message) {
    debug("consuming message of type %d with default handler\n", message->type);
    free(message);
}

