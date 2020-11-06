#include "daemon.h"

static D_STRUCT *head;

/* add daemon to list */
void add_daemon(D_STRUCT *daemon) {
	if(head == NULL) {
		head = daemon;
		head->next = NULL;
	} else {
		/* add daemon to start instead of end (for efficiency purposes) */
		daemon->next = head;
		head = daemon;
	}
}

/* get daemon from list */
void get_daemon(char *daemon);

/* remove daemon from list note: might need two vars */
void remove_daemon(D_STRUCT *daemon);

/* iterate through daemons */
void print_daemon();

