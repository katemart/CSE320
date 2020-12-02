#include <stdlib.h>

#include "debug.h"
#include "csapp.h"
#include "player_registry.h"

typedef struct player_registry {
	int num_players;
	pthread_mutex_t mutex;
	struct p_list {
		PLAYER *player;
		struct p_list *next, *prev;
	} *list;
} PLAYER_REGISTRY;

PLAYER_REGISTRY *preg_init(void) {
	PLAYER_REGISTRY *pr;
	/* allocate space for the player registry */
	pr = malloc(sizeof(PLAYER_REGISTRY));
	if(pr == NULL) {
		debug("error allocating space for preg");
		return NULL;
	}
	/* set player count to zero */
	pr->num_players = 0;
	/* init players list */
	pr->list = NULL;
	/* init mutex */
	if(pthread_mutex_init(&pr->mutex, NULL) != 0) {
		free(pr);
		debug("error initializing mutex");
		return NULL;
	}
	debug("%lu: Initialize player registry", pthread_self());
	return pr;
}

void preg_fini(PLAYER_REGISTRY *preg) {
	debug("%lu: Finalize player registry", pthread_self());
	/* lock mutex */
	if(pthread_mutex_lock(&preg->mutex) != 0) {
		debug("pthread_mutex_lock error");
	}
	/* free player registry */
	struct p_list *temp;
	struct p_list *root = preg->list;
	while(root != NULL) {
		temp = root;
		root = root->next;
		temp->prev = NULL;
		temp->next = NULL;
		player_unref(temp->player, "because player registry is being finalized");
		free(temp);
	}
	/* lock mutex */
	if(pthread_mutex_unlock(&preg->mutex) != 0) {
		debug("pthread_mutex_unlock error");
	}
	/* destroy mutex */
	if(pthread_mutex_destroy(&preg->mutex) != 0) {
		debug("error destroying mutex");
	}
	free(preg);
}

PLAYER *preg_register(PLAYER_REGISTRY *preg, char *name) {
	PLAYER *p = NULL;
	/* lock mutex */
	if(pthread_mutex_lock(&preg->mutex) != 0) {
		debug("pthread_mutex_lock error");
		return NULL;
	}
	/* copy name
	char *new_name = strdup(name);
	if(new_name == NULL) {
		debug("error copying player name");
		if(pthread_mutex_unlock(&preg->mutex) != 0) {
			debug("pthread_mutex_unlock error");
			return NULL;
		}
		return NULL;
	} */
	debug("Register player %s", name);
	struct p_list **root = &preg->list;
	struct p_list *temp_prev = NULL;
	/* search through each player registered first */
	while(*root != NULL) {
		char *p_name = player_get_name((*root)->player);
		/* check if a player with that username exists */
		if(strcmp(p_name, name) == 0) {
			p = (*root)->player;
			/* if so, increment player ref count by one */
			player_ref(p, "for reference being retained by player registry");
			/* and return existing player */
			if(pthread_mutex_unlock(&preg->mutex) != 0) {
				debug("pthread_mutex_unlock error");
				player_unref(p, "for reference being released by player registry");
				return NULL;
			}
			return p;
		}
		temp_prev = (*root);
		root = &((*root)->next);
	}
	debug("Player with that name does not yet exist");
	/* allocate space for players list */
	*root = malloc(sizeof(struct p_list));
	if(*root == NULL) {
		if(pthread_mutex_unlock(&preg->mutex) != 0) {
			debug("pthread_mutex_unlock error");
			return NULL;
		}
		return NULL;
	}

	(*root)->next = NULL;
	(*root)->prev = temp_prev;
	if(temp_prev != NULL) {
		temp_prev->next = *root;
	}
	(*root)->player = player_create(name);
	if((*root)->player != NULL) {
		p = (*root)->player;
		/* increment player ref count by one */
		player_ref(p, "for reference being retained by player registry");
	}
	/* increment player count by one */
	preg->num_players++;
	/* unlock mutex */
	if(pthread_mutex_unlock(&preg->mutex) != 0) {
		debug("pthread_mutex_unlock error");
		return NULL;
	}
	return p;
}
