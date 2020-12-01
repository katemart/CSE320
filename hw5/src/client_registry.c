#include <string.h>
#include <sys/time.h>

#include "debug.h"
#include "csapp.h"
#include "player.h"
#include "semaphore.h"
#include "client_registry.h"

typedef struct client_registry {
	sem_t sem;
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
	/* set client count to zero */
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
	/* destroy mutex and sem */
	if(sem_destroy(&cr->sem) < 0) {
		debug("error destroying semaphore");
	}
	if(pthread_mutex_destroy(&cr->mutex) != 0) {
		debug("error destroying mutex");
	}
	/* free client registry */
	free(cr);
}

CLIENT *creg_register(CLIENT_REGISTRY *cr, int fd) {
	/* lock mutex */
	if(pthread_mutex_lock(&cr->mutex) != 0) {
		debug("pthread_mutex_lock error");
		return NULL;
	}
	/* create client */
	CLIENT *client = client_create(cr, fd);
	if(client == NULL) {
		if(pthread_mutex_unlock(&cr->mutex) != 0) {
			debug("pthread_mutex_unlock error");
		}
		return NULL;
	}
	/* increment client count by one */
	cr->num_clients++;
	/* add client to list */
	for(int i = 0; i < MAX_CLIENTS; i++) {
		if(cr->c_list[i] == NULL) {
			cr->c_list[i] = client;
			break;
		}
	}
	debug("%lu: Register client fd %d (total connected: %d)", pthread_self(), fd, cr->num_clients);
	/* if the num of clients is zero, put a "marble" */
	if(cr->num_clients == 0) {
		V(&cr->sem);
	}
	/* unlock mutex */
	if(pthread_mutex_unlock(&cr->mutex) != 0) {
		debug("pthread_mutex_unlock error");
		return NULL;
	}
	return client;
}

int creg_unregister(CLIENT_REGISTRY *cr, CLIENT *client) {
	/* lock mutex */
	if(pthread_mutex_lock(&cr->mutex) != 0) {
		debug("pthread_mutex_lock error");
		return -1;
	}
	/* if the num of clients is <= zero then error unregistering */
	if(cr->num_clients <= 0) {
		if(pthread_mutex_unlock(&cr->mutex) != 0) {
			debug("pthread_mutex_unlock error");
		}
		return -1;
	}
	/* search for client to unregister */
	for(int i = 0; i < MAX_CLIENTS; i++) {
		 /* if client is found, remove from list */
		if(cr->c_list[i] == client) {
			cr->c_list[i] = NULL;
			debug("%lu: Unregister client fd %d (total connected: %d)", pthread_self(), client_get_fd(client), cr->num_clients);
			/* decrement client count by one */
			cr->num_clients--;
			client_unref(client, "decrease ref count due to client unregistration");
			break;
		} else {
			/* else unlock mutex and return -1 */
			if(pthread_mutex_unlock(&cr->mutex) != 0) {
				debug("pthread_mutex_unlock error");
			}
			return -1;
		}
	}
	/* if the num of clients is > zero, take the "marble" */
	if(cr->num_clients > 0) {
		P(&cr->sem);
	}
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
		if(cr->c_list[i] != NULL) {
			PLAYER *p = client_get_player(cr->c_list[i]);
			/* if there is no player, return NULL */
			if(p == NULL) {
				break;
			}
			/* if there is a player, get the player's username */
			char *name = player_get_name(p);
			/* if a client with that username is found, return it */
			if(strcmp(name, user) == 0) {
				client = cr->c_list[i];
				/* increment client ref count by one */
				client_ref(client, "increase ref count due to client lookup");
				break;
			}
		}
	}
	/* unlock mutex */
	if(pthread_mutex_unlock(&cr->mutex) != 0) {
		debug("pthread_mutex_unlock error");
		return NULL;
	}
	return client;
}

PLAYER **creg_all_players(CLIENT_REGISTRY *cr) {
	PLAYER **p_list;
	/* lock mutex */
	if(pthread_mutex_lock(&cr->mutex) != 0) {
		debug("pthread_mutex_lock error");
		return NULL;
	}
	/* allocate space for the player list (including NULL mark at end) */
	p_list = malloc((cr->num_clients + 1) * sizeof(PLAYER *));
	if(p_list == NULL) {
		debug("error allocating space for player list");
		return NULL;
	}
	/* search through clients to find players */
	for(int i = 0; i < MAX_CLIENTS; i++) {
		if(cr->c_list[i] != NULL) {
			/* get player for the specified logged-in client */
			PLAYER *p = client_get_player(cr->c_list[i]);
			/* if player is logged-in (aka not NULL), add it to the array*/
			if(p != NULL) {
				p_list[i] = p;
				/* increase player ref count */
				player_ref(p, "increase ref count due to player being added to list of players");
			}
		}
	}
	/* set NULL pointer marking end of array */
	p_list[cr->num_clients] = NULL;
	/* unlock mutex */
	if(pthread_mutex_unlock(&cr->mutex) != 0) {
		debug("pthread_mutex_unlock error");
		return NULL;
	}
	return p_list;
}

void creg_wait_for_empty(CLIENT_REGISTRY *cr) {
	P(&cr->sem);
}

void creg_shutdown_all(CLIENT_REGISTRY *cr) {
	/* lock mutex */
	if(pthread_mutex_lock(&cr->mutex) != 0) {
		debug("pthread_mutex_lock error");
	}
	/* shut down client fds */
	for(int i = 0; i < MAX_CLIENTS; i++) {
		if(cr->c_list[i] != NULL) {
			shutdown(client_get_fd(cr->c_list[i]), SHUT_RD);
		}
	}
	/* unlock mutex */
	if(pthread_mutex_unlock(&cr->mutex) != 0) {
		debug("pthread_mutex_unlock error");
	}
}
