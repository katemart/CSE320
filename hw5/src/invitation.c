#include <stdlib.h>
#include <pthread.h>

#include "debug.h"
#include "client_registry.h"
#include "invitation.h"

extern CLIENT_REGISTRY *client_registry;

typedef struct invitation {
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
		debug("error allocating space for creg");
		return NULL;
	}
	/* if source is same as target then error */
	if(source == target) {
		free(invitation);
		return NULL;
	}
	/* else create invitation */
	invitation->state = 0;
	invitation->ref_count = 1;
	invitation->source = source;
	invitation->target = target;
	invitation->s_role = source_role;
	invitation->t_role = target_role;
	/* increment client ref counts */
	client_ref(source, "increase ref count due to source invitation");
	client_ref(target, "increase ref count due to target invitation");
	/* init mutex*/
	if(pthread_mutex_init(&invitation->mutex, NULL) != 0) {
		free(invitation);
		debug("error initializing mutex");
		return NULL;
	}
	return invitation;
}

INVITATION *inv_ref(INVITATION *inv, char *why) {
	return NULL;
}

void inv_unref(INVITATION *inv, char *why) {

}

CLIENT *inv_get_source(INVITATION *inv) {
	return NULL;
}

CLIENT *inv_get_target(INVITATION *inv) {
	return NULL;
}

GAME_ROLE inv_get_source_role(INVITATION *inv) {
	return 0;
}

GAME_ROLE inv_get_target_role(INVITATION *inv) {
	return 0;
}

GAME *inv_get_game(INVITATION *inv) {
	return NULL;
}

int inv_accept(INVITATION *inv) {
	return 0;
}

int inv_close(INVITATION *inv, GAME_ROLE role) {
	return 0;
}
