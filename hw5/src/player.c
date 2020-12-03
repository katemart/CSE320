#include <stdlib.h>

#include "debug.h"
#include "csapp.h"
#include "player.h"

typedef struct player {
	int rating;
	int ref_count;
	char *username;
	pthread_mutex_t mutex;
} PLAYER;

PLAYER *player_create(char *name) {
	PLAYER *player;
	/* allocate space for the player */
	player = malloc(sizeof(PLAYER));
	if(player == NULL) {
		debug("error allocating space for player");
	}
	/* make private copy of name */
	char *user = strdup(name);
	if(user == NULL) {
		debug("error copying username");
	}
	/* set player fields */
	player->ref_count = 0;
	player->username = user;
	player->rating = PLAYER_INITIAL_RATING;
	/* init mutex*/
	if(pthread_mutex_init(&player->mutex, NULL) != 0) {
		free(player);
		debug("error initializing mutex");
		return NULL;
	}
	/* increment player ref count */
	player_ref(player, "for newly created player");
	return player;
}

PLAYER *player_ref(PLAYER *player, char *why) {
	/* lock mutex */
	if(pthread_mutex_lock(&player->mutex) != 0) {
		debug("pthread_mutex_lock error");
		return NULL;
	}
	int prev_ref_count = player->ref_count;
	if(prev_ref_count <= 0) {
		prev_ref_count = 0;
	}
	player->ref_count++;
	debug("%lu: Increase reference count on player %p (%d -> %d) %s",
		pthread_self(), player, prev_ref_count, player->ref_count, why);
	/* unlock mutex */
	if(pthread_mutex_unlock(&player->mutex) != 0) {
		debug("pthread_mutex_lock error");
		return NULL;
	}
	return player;
}

void player_unref(PLAYER *player, char *why) {
	/* lock mutex */
	if(pthread_mutex_lock(&player->mutex) != 0) {
		debug("pthread_mutex_lock error");
	}
	int prev_ref_count = player->ref_count;
	if(prev_ref_count <= 0) {
		prev_ref_count = 0;
	}
	player->ref_count--;
	debug("%lu: Decrease reference count on player %p (%d -> %d) %s",
		pthread_self(), player, prev_ref_count, player->ref_count, why);
	if(player->ref_count <= 0) {
		/* free associated resources */
		free(player->username);
	}
	/* unlock mutex */
	if(pthread_mutex_unlock(&player->mutex) != 0) {
		debug("pthread_mutex_lock error");
	}
	/* free player if ref count has reached zero */
	if(player->ref_count <= 0) {
		free(player);
	}
}

char *player_get_name(PLAYER *player) {
	return player->username;
}

int player_get_rating(PLAYER *player) {
	return player->rating;
}

void player_post_result(PLAYER *player1, PLAYER *player2, int result) {
	/* lock mutex */
	if(pthread_mutex_lock(&player1->mutex) != 0) {
		debug("pthread_mutex_lock error");
	}
	/* lock mutex */
	if(pthread_mutex_lock(&player2->mutex) != 0) {
		debug("pthread_mutex_lock error");
	}
	/* get actual result */
	double S1, S2;
	if(result == 0) {
		/* draw */
		S1 = 0.5;
		S2 = 0.5;
	} else if(result == 1) {
		/* player 1 wins */
		S1 = 1.0;
		S2 = 0.0;
	} else if(result == 2) {
		/* player 2 wins */
		S1 = 0.0;
		S2 = 1.0;
	}
	/* calculate score */
	double E1 = (1.0 / (1.0 + pow(10, ((player2->rating - player1->rating) / 400))));
	double E2 = (1.0 / (1.0 + pow(10, ((player1->rating - player2->rating) / 400))));
	/* update ratings */
	double R1 = (player1->rating + 32 * (S1 - E1));
	double R2 = (player2->rating + 32 * (S2 - E2));
	player1->rating = (int) R1;
	player2->rating = (int) R2;
	/* unlock mutex */
	if(pthread_mutex_unlock(&player1->mutex) != 0) {
		debug("pthread_mutex_lock error");
	}
	/* unlock mutex */
	if(pthread_mutex_unlock(&player2->mutex) != 0) {
		debug("pthread_mutex_lock error");
	}
}
