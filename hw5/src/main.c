#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "debug.h"
#include "protocol.h"
#include "server.h"
#include "client_registry.h"
#include "player_registry.h"
#include "jeux_globals.h"
#include "csapp.h"

#ifdef DEBUG
int _debug_packets_ = 1;
#endif

static void terminate(int status);
static volatile sig_atomic_t sighup_flag = 0;

static void sighup_handler(int sig) {
    sighup_flag = 1;
}

/*
 * "Jeux" game server.
 *
 * Usage: jeux <port>
 */
int main(int argc, char* argv[]){
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.

    /* vars for starting up server */
    char *port_num;
    int listen_fd, *conn_fd;
    socklen_t client_len;
    struct sockaddr_storage client_addr;
    pthread_t tid;
    //sighup_flag = 0;

    /* check that the given args is the right number */
    if(argc != 3) {
        fprintf(stderr, "Usage: demo/jeux -p <port>\n");
        exit(EXIT_FAILURE);
    }
    /* if so, check for port number */
    if(strcmp(argv[1], "-p") == 0) {
        port_num = argv[2];
    } else {
        fprintf(stderr, "Usage: demo/jeux -p <port>\n");
        exit(EXIT_FAILURE);
    }

    // Perform required initializations of the client_registry and
    // player_registry.
    client_registry = creg_init();
    player_registry = preg_init();

    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function jeux_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.

    /* install sigaction for SIGHUP */
    struct sigaction s_action;
    s_action.sa_handler = sighup_handler;
    sigemptyset(&s_action.sa_mask);
    s_action.sa_flags = 0;
    if (sigaction(SIGHUP, &s_action, NULL) == -1) {
        fprintf(stderr, "SIGHUP error\n");
        exit(EXIT_FAILURE);
    }
    /* attempt to start server */
    if((listen_fd = open_listenfd(port_num)) < 0) {
        fprintf(stderr, "open_listenfd error\n");
        exit(EXIT_FAILURE);
    }
    debug("Jeux server listening on port %s", port_num);
    while(1) {
        client_len = sizeof(struct sockaddr_storage);
        conn_fd = malloc(sizeof(int));
        if(conn_fd == NULL) {
            fprintf(stderr, "conn_fd malloc error\n");
            exit(EXIT_FAILURE);
        }
        if((*conn_fd = accept(listen_fd, (SA *)&client_addr, &client_len)) < 0) {
            /* check for SIGHUP */
            if(sighup_flag) terminate(EXIT_SUCCESS);
            /* if not sIGHUP, something else went wrong */
            fprintf(stderr, "conn_fd accept error\n");
            exit(EXIT_FAILURE);
        }
        Pthread_create(&tid, NULL, jeux_client_service, conn_fd);
    }
    exit(EXIT_SUCCESS);
}

/*
 * Function called to cleanly shut down the server.
 */
void terminate(int status) {
    // Shutdown all client connections.
    // This will trigger the eventual termination of service threads.
    creg_shutdown_all(client_registry);

    debug("%ld: Waiting for service threads to terminate...", pthread_self());
    creg_wait_for_empty(client_registry);
    debug("%ld: All service threads terminated.", pthread_self());

    // Finalize modules.
    creg_fini(client_registry);
    preg_fini(player_registry);

    debug("%ld: Jeux server terminating", pthread_self());
    exit(status);
}
