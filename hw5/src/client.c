#include "debug.h"
#include "csapp.h"
#include "player_registry.h"
#include "client_registry.h"
#include "jeux_globals.h"
#include "client.h"

typedef struct client {
	int fd;
	int log_state;
	int ref_count;
	PLAYER *player;
	struct inv_list {
		int inv_id;
		INVITATION *inv;
		struct inv_list *next, *prev;
	} *invs;
	pthread_mutex_t mutex;
	pthread_mutexattr_t mutex_attr;
} CLIENT;

CLIENT *client_create(CLIENT_REGISTRY *creg, int fd) {
	CLIENT *client;
	/* allocate space for the client */
	client = malloc(sizeof(CLIENT));
	if(client == NULL) {
		debug("error allocating space for client");
		return NULL;
	}
	/* init mutex attr */
	if(pthread_mutexattr_init(&client->mutex_attr) != 0) {
		free(client);
		debug("error initializing mutex attr");
		return NULL;
	}
	/* set mutex attr type */
	if(pthread_mutexattr_settype(&client->mutex_attr, PTHREAD_MUTEX_RECURSIVE) != 0) {

	}free(client);
		debug("error setting mutex attr");
		return NULL;
	/* init mutex*/
	if(pthread_mutex_init(&client->mutex, &client->mutex_attr) != 0) {
		free(client);
		debug("error initializing mutex");
		return NULL;
	}
	/* set client fields */
	client->fd = fd;
	client->log_state = 0;
	client->ref_count = 0;
	client->player = NULL;
	/* init invitation list */
	client->invs = NULL;
	/* increment client ref count */
	client_ref(client, "for newly created client");
	return client;
}

CLIENT *client_ref(CLIENT *client, char *why) {
	/* lock mutex */
	if(pthread_mutex_lock(&client->mutex) != 0) {
		debug("pthread_mutex_lock error");
		return NULL;
	}
	int prev_ref_count = client->ref_count;
	if(prev_ref_count <= 0) {
		prev_ref_count = 0;
	}
	client->ref_count++;
	debug("%lu: Increase reference count on client %p (%d -> %d) %s",
		pthread_self(), client, prev_ref_count, client->ref_count, why);
	/* unlock mutex */
	if(pthread_mutex_unlock(&client->mutex) != 0) {
		debug("pthread_mutex_unlock error");
		return NULL;
	}
	return client;
}

void client_unref(CLIENT *client, char *why) {
	/* lock mutex */
	if(pthread_mutex_lock(&client->mutex) != 0) {
		debug("pthread_mutex_lock error");
	}
	int prev_ref_count = client->ref_count;
	if(prev_ref_count <= 0) {
		prev_ref_count = 0;
	}
	client->ref_count--;
	debug("%lu: Decrease reference count on client %p (%d -> %d) %s",
		pthread_self(), client, prev_ref_count, client->ref_count, why);
	if(client->ref_count <= 0) {
		/* decrement player ref count */
		player_unref(client->player, "because client is being freed");
	}
	/* unlock mutex */
	if(pthread_mutex_unlock(&client->mutex) != 0) {
		debug("pthread_mutex_unlock error");
	}
	/* free client if ref count has reached zero */
	if(client->ref_count <= 0) {
		free(client);
	}
}

int client_login(CLIENT *client, PLAYER *player) {
	/* lock mutex */
	if(pthread_mutex_lock(&client->mutex) != 0) {
		debug("pthread_mutex_lock error");
		return -1;
	}
	/* if client is already logged in, then error */
	if(client->log_state != 0) {
		if(pthread_mutex_unlock(&client->mutex) != 0) {
			debug("pthread_mutex_unlock error");
			return -1;
		}
		return -1;
	}
	/* if client is logged in as specified player, then error */
	char *p_name = player_get_name(player);
	if(client->player != NULL && strcmp(p_name, player_get_name(client->player)) == 0) {
		if(pthread_mutex_unlock(&client->mutex) != 0) {
			debug("pthread_mutex_unlock error");
			return -1;
		}
		return -1;
	}
	/* else try to register */
	PLAYER *p_reg = preg_register(player_registry, p_name);
	if(p_reg == NULL) {
		if(pthread_mutex_unlock(&client->mutex) != 0) {
			debug("pthread_mutex_unlock error");
			return -1;
		}
		return -1;
	}
	client->log_state = 1;
	client->player = p_reg;
	debug("%lu: Log in client %p as player %p [%s]", pthread_self(), client, player, p_name);
	/* unlock mutex */
	if(pthread_mutex_unlock(&client->mutex) != 0) {
		debug("pthread_mutex_unlock error");
		return -1;
	}
	return 0;
}

int client_logout(CLIENT *client) {
	/* lock mutex */
	if(pthread_mutex_lock(&client->mutex) != 0) {
		debug("pthread_mutex_lock error");
		return -1;
	}
	/* if client is not logged in, then error */
	if(client->log_state != 1) {
		if(pthread_mutex_unlock(&client->mutex) != 0) {
			debug("pthread_mutex_unlock error");
			return -1;
		}
		return -1;
	}
	/* else, attempt to log out */
	/* decr player ref */
	player_unref(client->player, "because client is being freed");
	/* revoke/decline invs (if any) */
	struct inv_list **root = &client->invs;
	while(*root != NULL) {
		CLIENT *source = inv_get_source((*root)->inv);
		CLIENT *target = inv_get_target((*root)->inv);
		client_revoke_invitation(source, (*root)->inv_id);
		client_decline_invitation(target, (*root)->inv_id);
		root = &((*root)->next);
	}
	/* unregister client?? */
	if(creg_unregister(client_registry, client) != 0) {
		debug("error unregistering client during logout");
		if(pthread_mutex_unlock(&client->mutex) != 0) {
			debug("pthread_mutex_unlock error");
			return -1;
		}
	}
	/* unlock mutex */
	if(pthread_mutex_unlock(&client->mutex) != 0) {
		debug("pthread_mutex_unlock error");
		return -1;
	}
	return 0;
}

PLAYER *client_get_player(CLIENT *client) {
	/* lock mutex */
	if(pthread_mutex_lock(&client->mutex) != 0) {
		debug("pthread_mutex_lock error");
		return NULL;
	}
	PLAYER *temp = client->player;
	/* unlock mutex */
	if(pthread_mutex_unlock(&client->mutex) != 0) {
		debug("pthread_mutex_unlock error");
		return NULL;
	}
	return temp;
}

int client_get_fd(CLIENT *client) {
	return client->fd;
}

int client_send_packet(CLIENT *player, JEUX_PACKET_HEADER *pkt, void *data) {
	/* lock mutex */
	if(pthread_mutex_lock(&player->mutex) != 0) {
		debug("pthread_mutex_lock error");
		return -1;
	}
	/* attempt to send packet */
	if(proto_send_packet(player->fd, pkt, data) != 0) {
		if(pthread_mutex_unlock(&player->mutex) != 0) {
			debug("pthread_mutex_unlock error");
			return -1;
		}
		return -1;
	}
	/* unlock mutex */
	if(pthread_mutex_unlock(&player->mutex) != 0) {
		debug("pthread_mutex_unlock error");
		return -1;
	}
	return 0;
}

int client_send_ack(CLIENT *client, void *data, size_t datalen) {
	return 0;
}

int client_send_nack(CLIENT *client) {
	return 0;
}

int client_add_invitation(CLIENT *client, INVITATION *inv) {
	return 0;
}

int client_remove_invitation(CLIENT *client, INVITATION *inv) {
	return 0;
}

int client_make_invitation(CLIENT *source, CLIENT *target,
			   GAME_ROLE source_role, GAME_ROLE target_role) {
	return 0;
}

int client_revoke_invitation(CLIENT *client, int id) {
	return 0;
}

int client_decline_invitation(CLIENT *client, int id) {
	return 0;
}

int client_accept_invitation(CLIENT *client, int id, char **strp) {
	return 0;
}

int client_resign_game(CLIENT *client, int id) {
	return 0;
}

int client_make_move(CLIENT *client, int id, char *move) {
	return 0;
}
