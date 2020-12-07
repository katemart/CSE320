#include "debug.h"
#include "csapp.h"
#include "game.h"

typedef struct game {
	int ref_count;
	int game_over;
	GAME_ROLE winner;
	char game_state[9];
	char player_to_move[1];
	pthread_mutex_t mutex;
} GAME;

typedef struct game_move {
	GAME *game;
	int position;
	enum game_role role;
} GAME_MOVE;

GAME *game_create(void) {
	GAME *game;
	/* allocate space for game */
	game = malloc(sizeof(GAME));
	if(game == NULL) {
		debug("error allocating space for game");
		return NULL;
	}
	/* set game fields */
	game->winner = NULL_ROLE;
	game->ref_count = 0;
	game->game_over = 0;
	for(int i = 0; i < 9; i++) {
		game->game_state[i] = ' ';
	}
	char init_player[1] = {0};
	strcpy(game->player_to_move, init_player);
	/* init mutex*/
	if(pthread_mutex_init(&game->mutex, NULL) != 0) {
		free(game);
		debug("error initializing mutex");
		return NULL;
	}
	/* increment game ref count */
	game_ref(game, "for newly created game");
	return game;
}

GAME *game_ref(GAME *game, char *why) {
	/* lock mutex */
	if(pthread_mutex_lock(&game->mutex) != 0) {
		debug("pthread_mutex_lock error");
		return NULL;
	}
	int prev_ref_count = game->ref_count;
	if(prev_ref_count <= 0) {
		prev_ref_count = 0;
	}
	game->ref_count++;
	debug("%lu: Increase reference count on game %p (%d -> %d) %s",
		pthread_self(), game, prev_ref_count, game->ref_count, why);
	/* unlock mutex */
	if(pthread_mutex_unlock(&game->mutex) != 0) {
		debug("pthread_mutex_unlock error");
		return NULL;
	}
	return game;
}

void game_unref(GAME *game, char *why) {
	/* lock mutex */
	if(pthread_mutex_lock(&game->mutex) != 0) {
		debug("pthread_mutex_lock error");
	}
	int prev_ref_count = game->ref_count;
	if(prev_ref_count <= 0) {
		prev_ref_count = 0;
	}
	game->ref_count--;
	debug("%lu: Decrease reference count on game %p (%d -> %d) %s",
		pthread_self(), game, prev_ref_count, game->ref_count, why);
	/* unlock mutex */
	if(pthread_mutex_unlock(&game->mutex) != 0) {
		debug("pthread_mutex_unlock error");
	}
	/* free invitation if ref count has reached zero */
	if(game->ref_count <= 0) {
		free(game);
	}
}

int game_apply_move(GAME *game, GAME_MOVE *move) {
	/* lock mutex */
	if(pthread_mutex_lock(&game->mutex) != 0) {
		debug("pthread_mutex_lock error");
		return -1;
	}
	/* check if move is invalid */
	if(move->position < 1 || move->position > 9) {
		debug("invalid game move");
		if(pthread_mutex_unlock(&game->mutex) != 0) {
			debug("pthread_mutex_unlock error");
		}
		return -1;
	}
	/* if valid, check if already taken */
	if(game->game_state[move->position - 1] == 'X' ||
		game->game_state[move->position - 1] == 'O') {
		debug("invalid game move");
		if(pthread_mutex_unlock(&game->mutex) != 0) {
			debug("pthread_mutex_unlock error");
		}
		return -1;
	}
	char game_piece;
	/* check if Xs or Os goes */
	if(move->role == FIRST_PLAYER_ROLE) {
		game_piece = 'X';
		game->game_state[move->position - 1] = game_piece;
		/* update who goes next */
		game->player_to_move[0] = 'O';
		move->role = SECOND_PLAYER_ROLE;
	} else if(move->role == SECOND_PLAYER_ROLE) {
		game_piece = 'O';
		game->game_state[move->position -1 ] = game_piece;
		/* update who goes next */
		game->player_to_move[0] = 'X';
		move->role = FIRST_PLAYER_ROLE;
	}
	/* unlock mutex */
	if(pthread_mutex_unlock(&game->mutex) != 0) {
		debug("pthread_mutex_unlock error");
		return -1;
	}
	return 0;
}

int game_resign(GAME *game, GAME_ROLE role) {
	/* lock mutex */
	if(pthread_mutex_lock(&game->mutex) != 0) {
		debug("pthread_mutex_lock error");
		return -1;
	}
	if(game == NULL || role == NULL_ROLE || game->game_over == 1) {
		return -1;
	}
	if(role == FIRST_PLAYER_ROLE) {
		game->winner = SECOND_PLAYER_ROLE;
		game->game_over = 1;
	} else if(role == SECOND_PLAYER_ROLE) {
		game->winner = FIRST_PLAYER_ROLE;
		game->game_over = 1;
	}
	debug("%lu: Resignation of game %p by %d", pthread_self(), game, role);
	/* unlock mutex */
	if(pthread_mutex_unlock(&game->mutex) != 0) {
		debug("pthread_mutex_unlock error");
		return -1;
	}
	return 0;
}

char *game_unparse_state(GAME *game) {
	char *game_str;
	/* lock mutex */
	if(pthread_mutex_lock(&game->mutex) != 0) {
		debug("pthread_mutex_lock error");
		return NULL;
	}
	game_str = malloc(100 * sizeof(char));
	if(game_str == NULL) {
		debug("error allocating space for game state string");
		return NULL;
	}
	if(game->player_to_move[0] == 0) {
		game->player_to_move[0] = 'X';
	}
	snprintf(game_str, 50, "%c|%c|%c\n-----\n%c|%c|%c\n-----\n%c|%c|%c\n%c to move\n",
		game->game_state[0], game->game_state[1], game->game_state[2], game->game_state[3],
		game->game_state[4], game->game_state[5], game->game_state[6], game->game_state[7],
		game->game_state[8], game->player_to_move[0]);
	/* unlock mutex */
	if(pthread_mutex_unlock(&game->mutex) != 0) {
		debug("pthread_mutex_unlock error");
		return NULL;
	}
	return game_str;
}

int game_is_over(GAME *game) {
	/* lock mutex */
	if(pthread_mutex_lock(&game->mutex) != 0) {
		debug("pthread_mutex_lock error");
		return game->game_over;
	}
	/* check if game is already over */
	if(game->game_over == 1) {
		/* unlock mutex */
		if(pthread_mutex_unlock(&game->mutex) != 0) {
			debug("pthread_mutex_unlock error");
		}
		return game->game_over;
	}
	/* check rows and cols */
	for(int i = 0; i < 3; i++) {
		/* rows */
		if(game->game_state[3 * i] == game->game_state[(3 * i) + 1] &&
			game->game_state[(3 * i) + 1] == game->game_state[(3 * i) + 2]) {
			/* get who won */
			if(game->game_state[3 * i] == 'X') {
				game->winner = FIRST_PLAYER_ROLE;
				/* record game as over */
				game->game_over = 1;
			} else if(game->game_state[3 * i] == 'O'){
				game->winner = SECOND_PLAYER_ROLE;
				/* record game as over */
				game->game_over = 1;
			}
		}
		/* cols */
		if(game->game_state[i] == game->game_state[i + 3] &&
			game->game_state[i + 3] == game->game_state[i + 6]) {
			/* get who won */
			if(game->game_state[i] == 'X') {
				game->winner = FIRST_PLAYER_ROLE;
				/* record game as over */
				game->game_over = 1;
			} else if(game->game_state[i] == 'O') {
				game->winner = SECOND_PLAYER_ROLE;
				/* record game as over */
				game->game_over = 1;
			}
		}
	}
	/* check diagonals */
	if(game->game_state[0] == game->game_state[4] &&
		game->game_state[4] == game->game_state[8]) {
		/* get who won */
			if(game->game_state[0] == 'X') {
				game->winner = FIRST_PLAYER_ROLE;
				/* record game as over */
				game->game_over = 1;
			} else if(game->game_state[0] == 'O') {
				game->winner = SECOND_PLAYER_ROLE;
				/* record game as over */
				game->game_over = 1;
			}
	}
	if(game->game_state[2] == game->game_state[4] &&
		game->game_state[4] == game->game_state[6]) {
		/* get who won */
			if(game->game_state[2] == 'X') {
				game->winner = FIRST_PLAYER_ROLE;
				/* record game as over */
				game->game_over = 1;
			} else if(game->game_state[2] == 'O') {
				game->winner = SECOND_PLAYER_ROLE;
				/* record game as over */
				game->game_over = 1;
			}
	}
	/* check for a draw */
	int taken = 0;
	for(int i = 0; i < 9; i++) {
		if(game->game_state[i] != ' ') {
			taken++;
		}
	}
	if(taken == 9 && game->game_over == 0) {
		game->winner = NULL_ROLE;
		/* record game as over */
		game->game_over = 1;
	}
	/* unlock mutex */
	if(pthread_mutex_unlock(&game->mutex) != 0) {
		debug("pthread_mutex_unlock error");
		return game->game_over;
	}
	return game->game_over;
}

GAME_ROLE game_get_winner(GAME *game) {
	/* lock mutex */
	if(pthread_mutex_lock(&game->mutex) != 0) {
		debug("pthread_mutex_lock error");
		return NULL_ROLE;
	}
	int temp = game->winner;
	/* unlock mutex */
	if(pthread_mutex_unlock(&game->mutex) != 0) {
		debug("pthread_mutex_unlock error");
		return NULL_ROLE;
	}
	return temp;
}

GAME_MOVE *game_parse_move(GAME *game, GAME_ROLE role, char *str) {
	/* allocate space for game move to be returned */
	GAME_MOVE *move;
	move = malloc(sizeof(GAME_MOVE));
	if(move == NULL) {
		return NULL;
	}
	int position = atoi(&str[0]);
	/* check if given move is valid */
	if(position < 1 || position > 9) {
		if(pthread_mutex_unlock(&game->mutex) != 0) {
			debug("pthread_mutex_unlock error");
		}
		return NULL;
	}
	if(str[1] != '\0') {
		/* check when given move str is #<-X/O */
		if(strcmp(&str[1], "<-X") == 0) {
			if(role != FIRST_PLAYER_ROLE) {
				if(pthread_mutex_unlock(&game->mutex) != 0) {
					debug("pthread_mutex_unlock error");
				}
				return NULL;
			}
		} else if(strcmp(&str[1], "<-O") == 0) {
			if(role != SECOND_PLAYER_ROLE) {
				if(pthread_mutex_unlock(&game->mutex) != 0) {
					debug("pthread_mutex_unlock error");
				}
				return NULL;
			}
		} else {
			if(pthread_mutex_unlock(&game->mutex) != 0) {
				debug("pthread_mutex_unlock error");
			}
			return NULL;
		}
	}
	/* if move is valid, return move */
	move->role = role;
	move->game = game;
	move->position = position;
	return move;
}

char *game_unparse_move(GAME_MOVE *move) {
	char *str;
	/* allocate space for str to be returned */
	str = malloc(10 * sizeof(char));
	if(str == NULL) {
		debug("error allocating space for string");
		return NULL;
	}
	/* unparse move */
	if(move->role == FIRST_PLAYER_ROLE) {
		snprintf(str, 10, "%d<-X", move->position);
	} else if(move->role == SECOND_PLAYER_ROLE) {
		snprintf(str, 10, "%d<-O", move->position);
	}
	return str;
}
