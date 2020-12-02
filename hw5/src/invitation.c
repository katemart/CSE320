#include <stdlib.h>
#include <pthread.h>

#include "debug.h"
#include "client_registry.h"
#include "invitation.h"

extern CLIENT_REGISTRY *client_registry;

typedef struct invitation {
	GAME *game;
	int ref_count;
	CLIENT *source;
	CLIENT *target;
	GAME_ROLE s_role;
	GAME_ROLE t_role;
	pthread_mutex_t mutex;
	enum invitation_state state;
} INVITATION;

INVITATION *inv_create(CLIENT *source, CLIENT *target, GAME_ROLE source_role, GAME_ROLE target_role) {
	INVITATION *invitation;
	/* allocate space for the invitation */
	invitation = malloc(sizeof(INVITATION));
	if(invitation == NULL) {
		debug("error allocating space for invitation");
		return NULL;
	}
	/* if source is same as target then error */
	if(source == target) {
		free(invitation);
		return NULL;
	}
	/* else set invitation fields */
	invitation->game = NULL;
	invitation->ref_count = 0;
	invitation->source = source;
	invitation->target = target;
	invitation->s_role = source_role;
	invitation->t_role = target_role;
	invitation->state = INV_OPEN_STATE;
	/* increment client ref counts */
	client_ref(invitation->source, "as source of new invitation");
	client_ref(invitation->target, "as target of new invitation");
	/* init mutex*/
	if(pthread_mutex_init(&invitation->mutex, NULL) != 0) {
		free(invitation);
		debug("error initializing mutex");
		return NULL;
	}
	/* increment inv ref count */
	inv_ref(invitation, "for newly created invitation");
	return invitation;
}

INVITATION *inv_ref(INVITATION *inv, char *why) {
	/* lock mutex */
	if(pthread_mutex_lock(&inv->mutex) != 0) {
		debug("pthread_mutex_lock error");
		return NULL;
	}
	int prev_ref_count = inv->ref_count;
	if(prev_ref_count <= 0) {
		prev_ref_count = 0;
	}
	inv->ref_count++;
	debug("%lu: Increase reference count on invitation %p (%d -> %d) %s",
		pthread_self(), inv, prev_ref_count, inv->ref_count, why);
	/* unlock mutex */
	if(pthread_mutex_unlock(&inv->mutex) != 0) {
		debug("pthread_mutex_unlock error");
		return NULL;
	}
	return inv;
}

void inv_unref(INVITATION *inv, char *why) {
	/* lock mutex */
	if(pthread_mutex_lock(&inv->mutex) != 0) {
		debug("pthread_mutex_lock error");
	}
	int prev_ref_count = inv->ref_count;
	if(prev_ref_count <= 0) {
		prev_ref_count = 0;
	}
	inv->ref_count--;
	debug("%lu: Decrease reference count on invitation %p (%d -> %d) %s",
		pthread_self(), inv, prev_ref_count, inv->ref_count, why);
	if(inv->ref_count <= 0) {
		/* decrement client ref counts */
		client_unref(inv->source, "because invitation is being freed");
		client_unref(inv->target, "because invitation is being freed");
	}
	/* unlock mutex */
	if(pthread_mutex_unlock(&inv->mutex) != 0) {
		debug("pthread_mutex_unlock error");
	}
	/* free invitation if ref count has reached zero */
	if(inv->ref_count <= 0) {
		free(inv);
	}
}

CLIENT *inv_get_source(INVITATION *inv) {
	return inv->source;
}

CLIENT *inv_get_target(INVITATION *inv) {
	return inv->target;
}

GAME_ROLE inv_get_source_role(INVITATION *inv) {
	return inv->s_role;
}

GAME_ROLE inv_get_target_role(INVITATION *inv) {
	return inv->t_role;
}

GAME *inv_get_game(INVITATION *inv) {
	/* lock mutex */
	if(pthread_mutex_lock(&inv->mutex) != 0) {
		debug("pthread_mutex_lock error");
	}
	GAME *temp = inv->game;
	/* unlock mutex */
	if(pthread_mutex_unlock(&inv->mutex) != 0) {
		debug("pthread_mutex_unlock error");
	}
	return temp;
}

int inv_accept(INVITATION *inv) {
	/* lock mutex */
	if(pthread_mutex_lock(&inv->mutex) != 0) {
		debug("pthread_mutex_lock error");
		return -1;
	}
	/* if invitation state is not OPEN then error */
	if(inv->state != INV_OPEN_STATE) {
		debug("invitation state is not OPEN");
		if(pthread_mutex_unlock(&inv->mutex) != 0) {
			debug("pthread_mutex_unlock error");
		}
		return -1;
	}
	/* else, change state to ACCEPTED */
	inv->state = INV_ACCEPTED_STATE;
	/* and create new game */
	inv->game = game_create();
	/* if game was nott successfully created then error */
	if(inv->game == NULL) {
		debug("error creating game");
		if(pthread_mutex_unlock(&inv->mutex) != 0) {
			debug("pthread_mutex_unlock error");
		}
		return -1;
	}
	/* unlock mutex */
	if(pthread_mutex_unlock(&inv->mutex) != 0) {
		debug("pthread_mutex_unlock error");
		return -1;
	}
	return 0;
}

int inv_close(INVITATION *inv, GAME_ROLE role) {
	/* lock mutex */
	if(pthread_mutex_lock(&inv->mutex) != 0) {
		debug("pthread_mutex_lock error");
		return -1;
	}
	/* if invitation state is not OPEN or ACCEPTED then error */
	if(inv->state == INV_CLOSED_STATE) {
		debug("invitation state is not OPEN or ACCEPTED");
		if(pthread_mutex_unlock(&inv->mutex) != 0) {
			debug("pthread_mutex_unlock error");
		}
		return -1;
	}
	/* check that a game exists */
	if(inv->game == NULL) {
		inv->state = INV_CLOSED_STATE;
		if(pthread_mutex_unlock(&inv->mutex) != 0) {
			debug("pthread_mutex_unlock error");
		}
		return 0;
	}
	/* check if game is in progress */
	int game_over = game_is_over(inv->game);
	/* if game is in progress and role is not NULL_ROLL, resign by roll */
	if(game_over == 0 && role != NULL_ROLE) {
		int resign = game_resign(inv->game, role);
		if(resign != 0) {
			debug("error resigning game");
			if(pthread_mutex_unlock(&inv->mutex) != 0) {
				debug("pthread_mutex_unlock error");
			}
			return -1;
		}
		inv->state = INV_CLOSED_STATE;
	} else if(game_over == 1) {
		/* else if the game is not in progress, role doesn't matter so just close (and return 0) */
		inv->state = INV_CLOSED_STATE;
	} else {
		/* else, return error */
		if(pthread_mutex_unlock(&inv->mutex) != 0) {
			debug("pthread_mutex_unlock error");
		}
		return -1;
	}
	/* unlock mutex */
	if(pthread_mutex_unlock(&inv->mutex) != 0) {
		debug("pthread_mutex_unlock error");
		return -1;
	}
	return 0;
}
