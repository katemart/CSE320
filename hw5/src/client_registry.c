#include <string.h>
#include <sys/time.h>

#include "debug.h"
#include "csapp.h"
#include "player.h"
#include "semaphore.h"
#include "client_registry.h"

typedef struct client_registry {
	sem_t sem;
	fd_set c_fds;
	int num_clients;
	pthread_mutex_t mutex;
	CLIENT *c_list[MAX_CLIENTS];
} CLIENT_REGISTRY;

CLIENT_REGISTRY *creg_init() {
	CLIENT_REGISTRY *cr;
	/* allocate space for the client registry */
	cr = malloc(sizeof(CLIENT_REGISTRY));
	if(cr == NULL) {
		debug("error allocating space for creg");
		return NULL;
	}
	/* init fd set */
	FD_ZERO(&cr->c_fds);
	/* set client ref count to zero */
	cr->num_clients = 0;
	/* init sem */
	if(sem_init(&cr->sem, 0, 1) < 0) {
		free(cr);
		debug("error initializing semaphore");
		return NULL;
	}
	/* init client list */
	for(int i = 0; i < MAX_CLIENTS; i++) {
		cr->c_list[i] = NULL;
	}
	/* init mutex*/
	if(pthread_mutex_init(&cr->mutex, NULL) != 0) {
		free(cr);
		debug("error initializing mutex");
		return NULL;
	}
	debug("%lu: Initialize client registry", pthread_self());
	return cr;
}

void creg_fini(CLIENT_REGISTRY *cr) {
	debug("%lu: Finalize client registry", pthread_self());
	free(cr);
}

CLIENT *creg_register(CLIENT_REGISTRY *cr, int fd) {
	/* lock mutex */
	if(pthread_mutex_lock(&cr->mutex) != 0) {
		debug("pthread_mutex_lock error");
		return NULL;
	}
	/* add fd to set */
	FD_SET(fd, &cr->c_fds);
	/* create client */
	CLIENT *client = client_create(cr, fd);
	if(client == NULL) {
		if(pthread_mutex_unlock(&cr->mutex) != 0) {
			debug("pthread_mutex_unlock error");
		}
		return NULL;
	}
	/* increment client ref count by one */
	cr->num_clients++;
	CLIENT *c_ref = client_ref(client, "increase ref count due to client registration");
	/* add client to list */
	for(int i = 0; i < MAX_CLIENTS; i++) {
		if(cr->c_list[i] == NULL) {
			cr->c_list[i] = c_ref;
			break;
		}
	}
	debug("%lu: Register client fd %d (total connected: %d)", pthread_self(), fd, cr->num_clients);
	/* unlock mutex */
	if(pthread_mutex_unlock(&cr->mutex) != 0) {
		debug("pthread_mutex_unlock error");
		return NULL;
	}
	return c_ref;
}

int creg_unregister(CLIENT_REGISTRY *cr, CLIENT *client) {
	/* lock mutex */
	if(pthread_mutex_lock(&cr->mutex) != 0) {
		debug("pthread_mutex_lock error");
		return -1;
	}
	/* if the num of client refs is <= zero then error unregistering */
	if(cr->num_clients <= 0) {
		if(pthread_mutex_unlock(&cr->mutex) != 0) {
			debug("pthread_mutex_unlock error");
		}
		return -1;
	}
	/* get fd from passed in client */
	int c_fd = client_get_fd(client);
	/* if client is not in set then error unregistering */
	if(FD_ISSET(c_fd, &cr->c_fds) == 0) {
		if(pthread_mutex_unlock(&cr->mutex) != 0) {
			debug("pthread_mutex_unlock error");
		}
		return -1;
	}
	/* decrement client ref count by one */
	cr->num_clients--;
	client_unref(client, "decrease count due to client unregistration");
	/* remove client from list */
	for(int i = 0; i < MAX_CLIENTS; i++) {
		if(cr->c_list[i] == client) {
			cr->c_list[i] = NULL;
			break;
		}
	}
	debug("%lu: Unregister client fd %d (total connected: %d)", pthread_self(), fd, cr->num_clients);
	/* unlock mutex */
	if(pthread_mutex_unlock(&cr->mutex) != 0) {
		debug("pthread_mutex_unlock error");
	}
	return 0;
}

CLIENT *creg_lookup(CLIENT_REGISTRY *cr, char *user) {
	CLIENT *client = NULL;
	/* lock mutex */
	if(pthread_mutex_lock(&cr->mutex) != 0) {
		debug("pthread_mutex_lock error");
		return NULL;
	}
	/* search through each client registered */
	for(int i = 0; i < MAX_CLIENTS; i++) {
		/* if a client with that username is found, return it */
		if(strcmp(player_get_name(client_get_player(cr->c_list[i])), user) == 0) {
			client = cr->c_list[i];
			break;
		}
	}
	/* increment client ref count by one */
	cr->num_clients++;
	client_ref(client, "increase ref count due to client lookup");
	/* unlock mutex */
	if(pthread_mutex_unlock(&cr->mutex) != 0) {
		debug("pthread_mutex_unlock error");
		return NULL;
	}
	return client;
}

PLAYER **creg_all_players(CLIENT_REGISTRY *cr) {
	return NULL;
}

void creg_wait_for_empty(CLIENT_REGISTRY *cr) {

}

void creg_shutdown_all(CLIENT_REGISTRY *cr) {

}
