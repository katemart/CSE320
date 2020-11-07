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
D_STRUCT *get_daemon(char *d_name) {
	while(head != NULL) {
		if(head->name == d_name)
			return head;
		head = head->next;
	}
	return NULL;
}

/* remove daemon from list note: might need two vars */
void remove_daemon(char *d_name) {
	D_STRUCT *temp = head;
	D_STRUCT *prev = NULL;
	/* if current head contains d_name */
	if(temp != NULL && temp->name == d_name) {
		/* remove current head and set head's next as new head */
		head = temp->next;
		return;
	}
	/* while daemon is not found, keep traversing (while setting prev daemon) */
	while(temp != NULL && temp->name != d_name) {
		/* set previous to current */
		prev = temp;
		/* and set current to next */
		temp = temp->next;
	}
	if(temp == NULL) return;
	/* unlink/remove daemon */
	prev->next = temp->next;
}

/* iterate through daemons */
void print_daemons(FILE *out) {
	while(head != NULL) {
		fprintf(out, "Daemon [Name %s, PID %d, Command %s, Status %d]\n",
			head->name, head->pid, head->command, head->status);
		head = head->next;
	}
}

