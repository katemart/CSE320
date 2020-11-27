#include <sys/time.h>

#include "debug.h"
#include "csapp.h"
#include "semaphore.h"
#include "client_registry.h"

typedef struct client_registry {
	sem_t sem;
	fd_set c_fds;
	int num_clients;
	pthread_mutex_t mutex;
} CLIENT_REGISTRY;

CLIENT_REGISTRY *creg_init() {
	CLIENT_REGISTRY *c_reg;
	/* allocate space for the client registry */
	c_reg = malloc(sizeof(CLIENT_REGISTRY));
	if(c_reg == NULL) {
		debug("error allocating space for creg");
		return NULL;
	}
	/* initialize client registry */
	FD_ZERO(&c_reg->c_fds);
	c_reg->num_clients = 0;
	if(sem_init(&c_reg->sem, 0, 1) < 0) {
		free(c_reg);
		debug("error initializing semaphore");
		return NULL;
	}
	if(pthread_mutex_init(&c_reg->mutex, NULL) != 0) {
		free(c_reg);
		debug("error initializing mutex");
		return NULL;
	}
	return c_reg;
}

void creg_fini(CLIENT_REGISTRY *cr) {
	free(cr);
}

CLIENT *creg_register(CLIENT_REGISTRY *cr, int fd) {
	if(pthread_mutex_lock(&cr->mutex) != 0) {
		debug("pthread_mutex_lock error");
		return NULL;
	}
	if(cr->num_clients >= MAX_CLIENTS) {
		if(pthread_mutex_unlock(&cr->mutex) != 0) {
			debug("pthread_mutex_unlock error");
		}
		return NULL;
	}
	FD_SET(fd, &cr->c_fds);
	cr->num_clients++;
	CLIENT *client = client_create(cr, fd);
	if(client == NULL) {
		if(pthread_mutex_unlock(&cr->mutex) != 0) {
			debug("pthread_mutex_unlock error");
		}
		return NULL;
	}
	CLIENT *c_ref = client_ref(client, "increase count due to client registration");
	if(pthread_mutex_unlock(&cr->mutex) != 0) {
		debug("pthread_mutex_unlock error");
		return NULL;
	}
	return c_ref;
}

int creg_unregister(CLIENT_REGISTRY *cr, CLIENT *client) {
	if(pthread_mutex_lock(&cr->mutex) != 0) {
		debug("pthread_mutex_lock error");
		return -1;
	}
	if(cr->num_clients <= 0) {
		if(pthread_mutex_unlock(&cr->mutex) != 0) {
			debug("pthread_mutex_unlock error");
		}
		return -1;
	}
	int c_fd = client_get_fd(client);
	if(FD_ISSET(c_fd, &cr->c_fds) == 0) {
		if(pthread_mutex_unlock(&cr->mutex) != 0) {
			debug("pthread_mutex_unlock error");
		}
		return -1;
	}
	cr->num_clients--;
	client_unref(client, "decrease count due to client unregistration");
	if(pthread_mutex_unlock(&cr->mutex) != 0) {
		debug("pthread_mutex_unlock error");
	}
	return 0;
}

CLIENT *creg_lookup(CLIENT_REGISTRY *cr, char *user) {
	return NULL;
}

PLAYER **creg_all_players(CLIENT_REGISTRY *cr) {
	return NULL;
}

void creg_wait_for_empty(CLIENT_REGISTRY *cr) {

}

void creg_shutdown_all(CLIENT_REGISTRY *cr) {

}
