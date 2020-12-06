#include <stdlib.h>

#include "debug.h"
#include "csapp.h"
#include "game.h"
#include "server.h"
#include "client.h"
#include "player.h"
#include "protocol.h"
#include "invitation.h"
#include "client_registry.h"
#include "player_registry.h"
#include "jeux_globals.h"

void *jeux_client_service(void *arg) {
	/* pointer to fd to be used for communication */
	int fd = *((int *)arg);
	/* free fd storage */
	free(arg);
	/* detach thread */
	if(pthread_detach(pthread_self()) != 0) {
		debug("error detaching thread");
	}
	/* register client descriptor w client reg*/
	debug("%lu: [%d] Starting client service", pthread_self(), fd);
	CLIENT *client = creg_register(client_registry, fd);
	if(client == NULL) {
		return NULL;
	}
	/* set up */
	PLAYER *player = NULL;
	void *payloadp = NULL;
	JEUX_PACKET_HEADER r_packet;
	//JEUX_PACKET_HEADER *s_packet = malloc(sizeof(JEUX_PACKET_HEADER));
	while(1) {
		debug("AT START OF WHILE");
		int received = proto_recv_packet(fd, &r_packet, &payloadp);
		if(received < 0) {
			break;
		}
		/* FIRST TIME PROCESSING LOGIN PACKET */
		if(player == NULL) {
			/* LOGIN PACKET */
			if(r_packet.type == JEUX_LOGIN_PKT) {
				/* copy payload for player name */
				char *user = calloc((ntohs(r_packet.size) + 1), sizeof(char*));
				if(user == NULL) {
					debug("error allocating space for username");
				}
				memcpy(user, payloadp, ntohs(r_packet.size));
				/* register player */
				player = preg_register(player_registry, user);
				if(player == NULL) {
					debug("error creating player");
					break;
				}
				free(user);
				/* attempt to login */
				if(client_login(client, player) < 0) {
					/* if login fails, send NACK */
					if(client_send_nack(client) < 0) {
						debug("error sending nack");
					}
				} else {
					/* else if login is successful, send ACK */
					if(client_send_ack(client, NULL, 0) < 0) {
						debug("error sending ack");
					}
				}
			} else {
				debug("%lu: [%d] Login required", pthread_self(), fd);
				if(client_send_nack(client) < 0) {
					debug("error sending nack");
				}
			}
		}
		/* LOGIN PACKET HAS ALREADY BEEN PROCESSED */
		else {
			/* LOGIN PACKET */
			if(r_packet.type == JEUX_LOGIN_PKT) {
				char *user = calloc((ntohs(r_packet.size) + 1), sizeof(char*));
				if(user == NULL) {
					debug("error allocating space for username");
				}
				memcpy(user, payloadp, ntohs(r_packet.size));
				debug("%lu: [%d] Already logged in (player %p [%s])", pthread_self(),
					fd, player, user);
				free(user);
				if(client_send_nack(client) < 0) {
					debug("error sending nack");
				}
			}
			/* USERS PACKET */
			else if(r_packet.type == JEUX_USERS_PKT) {

			}
			/* INVITE PACKET */
			else if(r_packet.type == JEUX_INVITE_PKT) {

			}
			/* REVOKE PACKET */
			else if(r_packet.type == JEUX_REVOKE_PKT) {

			}
			/* DECLINE PACKET */
			else if(r_packet.type == JEUX_DECLINE_PKT) {

			}
			/* ACCEPT PACKET */
			else if(r_packet.type == JEUX_ACCEPT_PKT) {

			}
			/* MOVE PACKET */
			else if(r_packet.type == JEUX_MOVE_PKT) {

			}
			/* RESIGN PACKET */
			else if(r_packet.type == JEUX_RESIGN_PKT) {

			}
		}
		free(payloadp);
	}
	/* decrease ref count on player (if any) */
	if(player != NULL) {
		player_unref(player, "because server thread is discarding reference to logged in player");
		/* log out client */
		debug("%lu: [%d] Logging out client", pthread_self(), fd);
		if(client_logout(client) < 0) {
			debug("error logging out");
		}
	}
	/* unregister client */
	if(creg_unregister(client_registry, client) < 0) {
		debug("error unregistering client");
	}
	close(fd);
	debug("%lu: [%d] Ending client service", pthread_self(), fd);
	return NULL;
}
