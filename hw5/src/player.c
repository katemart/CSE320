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
	debug("%lu: Derease reference count on player %p (%d -> %d) %s",
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

}
