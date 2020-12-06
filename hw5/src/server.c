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
	JEUX_PACKET_HEADER *r_packet = malloc(sizeof(JEUX_PACKET_HEADER));
	while(1) {
		debug("AT START OF WHILE");
		int received = proto_recv_packet(fd, r_packet, &payloadp);
		if(received < 0) {
			break;
		}
		/* LOGIN PACKET */
		if(r_packet->type == JEUX_LOGIN_PKT) {
			debug("%lu: [%d] LOGIN packet received", pthread_self(), fd);
			 /* check if login has been processed already */
			if(player == NULL) {
				/* if login hasn't been called, login */
				player = preg_register(player_registry, payloadp);
				if(player == NULL) {
					debug("error creating player");
					break;
				}
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
				/* else if login has been already called, error */
				debug("%lu: [%d] Already logged in (player %p [%p])", pthread_self(),
					fd, player, payloadp);
				if(client_send_nack(client) < 0) {
					debug("error sending nack");
				}
			}
		}
		/* USERS PACKET */
		else if(r_packet->type == JEUX_USERS_PKT) {
			/* if login hasn't been called, send NACK */
			if(player == NULL) {
				if(client_send_nack(client) < 0) {
					debug("error sending nack");
				}
			} else {
				/* if login has been processed, show users */

			}
		}
		/* INVITE PACKET */

		/* REVOKE PACKET */

		/* DECLINE PACKET */

		/* ACCEPT PACKET */

		/* MOVE PACKET */

		/* RESIGN PACKET */
	}
	/* decrease ref count on player (if any) */
	if(player != NULL) {
		player_unref(player, "because server thread is discarding reference to logged in player");
	}
	/* log out client */
	debug("%lu: [%d] Logging out client", pthread_self(), fd);
	if(client_logout(client) < 0) {
		debug("error logging out");
	}
	/* unregister client */
	if(creg_unregister(client_registry, client) < 0) {
		debug("error unregistering client");
	}
	close(fd);
	debug("%lu: [%d] Ending client service", pthread_self(), fd);
	return NULL;
}
