#include "daemon.h"
#include <string.h>

static D_STRUCT *head;
char *d_status[] = {"unknown", "inactive", "starting", "active",
					"stopping","exited", "crashed"};

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
	if(head != NULL) {
		D_STRUCT *curr = head;
		while(curr != NULL) {
			if(strcmp(curr->name, d_name) == 0) {
				return head;
			}
			curr = curr->next;
		}
	}
	return NULL;
}

/* remove daemon from list note: might need two vars */
void remove_daemon(char *d_name) {
	if(head != NULL) {
		D_STRUCT *temp = head;
		D_STRUCT *prev = NULL;
		/* if current head contains d_name */
		if(temp != NULL && strcmp(temp->name, d_name) == 0) {
			/* remove current head and set head's next as new head */
			head = temp->next;
			return;
		}
		/* while daemon is not found, keep traversing (while setting prev daemon) */
		while(temp != NULL && (strcmp(temp->name, d_name) != 0)) {
			/* set previous to current */
			prev = temp;
			/* and set current to next */
			temp = temp->next;
		}
		if(temp == NULL) return;
		/* unlink/remove daemon */
		prev->next = temp->next;
		free(temp);
	}
}

void print_daemon(FILE *out, char *d_name) {
	if(head != NULL) {
		D_STRUCT *curr = get_daemon(d_name);
		if(curr != NULL) {
			fprintf(out, "%s\t%d\t%s\n", curr->name, curr->pid, d_status[curr->status]);
		}
	}
}

/* iterate through daemons */
void print_daemons(FILE *out) {
	D_STRUCT *curr = head;
	if(head != NULL) {
		while(curr != NULL) {
			fprintf(out, "%s\t%d\t%s\n", curr->name, curr->pid, d_status[curr->status]);
			curr = curr->next;
		}
	}
}

