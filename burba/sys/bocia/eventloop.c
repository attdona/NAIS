#include <xtimer.h>

#include "bocia.h"

#include "mutex.h"

#define ENABLE_DEBUG                (1)
#include "debug.h"

typedef enum { GROUND = 0, WARNING, NORMAL } app_state;

static app_state loop_status = GROUND;

static bocia_channel_t fd_map[SBAPI_SOCKETS];

static char _stack[1024 + THREAD_EXTRA_STACKSIZE_PRINTF];

static mutex_t mutex = MUTEX_INIT;

// an open socket has to be closed as soon as the sl_Select call return
//static bocia_channel_t* pending = 0;

void *bocia_loop_run(void *arg);


bocia_channel_t *channels(void) {
    return fd_map;
}


app_state sockloop_status(void) {
    return loop_status;
}

uint8_t bocia_channel_is_connected(bocia_channel_t *handler) {
    if (handler == 0) {
        return 0;
    }
    return (handler->fd > 0);
}

void bocia_loop_init(void) {

    if (loop_status == GROUND) {
        loop_status = NORMAL;

        // TODO review the priority
        thread_create(_stack, sizeof(_stack), SBAPI_PRIO + 4,
        THREAD_CREATE_STACKTEST, bocia_loop_run, 0, "subscribe");
    }
}

bocia_channel_t *bocia_register_handlers(
        int16_t sock, kernel_pid_t thread_id,
        void (*handler)(bocia_channel_t*),
        void (*tx_handler)(struct bocia_channel_t*, proto_msg_t, void *data)) {

    reader_id_t i;
    int8_t available = 0;

    for (i = 0; i < SBAPI_SOCKETS; i++) {
        if (fd_map[i].fd == 0) { // TODO
            DEBUG("registering [%d]: fd %d\n", i, sock);
            available = 1;
            fd_map[i].fd = sock;
            fd_map[i].target_pid = thread_id;
            fd_map[i].handler = handler;
            fd_map[i].send = tx_handler;

            mutex_unlock(&mutex);
            break;
        }
    }

    return available ? &fd_map[i] : NULL;
}

void bocia_close_soon(bocia_channel_t* channel) {

	channel->close_pending = 1;

}

void *bocia_loop_run(void* arg) {
    int16_t rc;
    int16_t nfds = 0;
    SlFdSet_t readfs;

    struct SlTimeval_t timval;

    while(1) {
        //mutex_lock(&mutex);

        SL_FD_ZERO(&readfs);
        rc = 0;
        nfds = 0;
        timval.tv_sec = 10;
        timval.tv_usec = 0;

        for (int i = 0; i < SBAPI_SOCKETS; i++) {
            if (fd_map[i].fd != 0) {
                if (nfds < fd_map[i].fd) {
                    nfds = fd_map[i].fd;
                }

                DEBUG("[%d] listen channel %d [%d]\n", i, fd_map[i].fd, fd_map[i].close_pending);
                rc = 1;
                SL_FD_SET(fd_map[i].fd, &readfs);
            }
        }
        if (rc == 0) {
            //DEBUG("no connected channels, waiting listener\n");
            //xtimer_usleep(10000);
            mutex_lock(&mutex);

            continue;
            //loop_status = GROUND;
            //return NULL;
        }

        //DEBUG("select ... %d\n", nfds);
        rc = sl_Select(nfds + 1, &readfs, NULL, NULL, &timval);
        //DEBUG("select fired: %d\n", rc);


        if (rc <= 0) {
            //DEBUG("select err/timeout: %d\n", rc);
        }
        else {

            for (int i = 0; i < SBAPI_SOCKETS; i++) {
                if (fd_map[i].close_pending) {
                    DEBUG("closing socket [%d]\n", fd_map[i].fd);
                    sl_Close(fd_map[i].fd);
                    fd_map[i].fd = 0;
                    fd_map[i].close_pending = 0;
                }
                if (fd_map[i].fd && SL_FD_ISSET(fd_map[i].fd, &readfs)) {
                    //DEBUG("READ event [%d] on %d\n", i, fd_map[i].fd);
                    fd_map[i].handler(&fd_map[i]);
                    if (fd_map[i].fd == 0) {
                        msg_t msg = { 0, BOCIA_SOCK_CLOSED, { 0 } };

                        //DEBUG("[%d] connection closed: %d\n", i, fd_map[i].fd);
                        msg_send(&msg, fd_map[i].target_pid);
                    }

                    //DEBUG("[%d] read done: %d\n", i, fd_map[i].fd);
                }
            }
        }

    }
    return NULL;
}

