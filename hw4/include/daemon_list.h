#ifndef DAEMON_LIST_H
#define DAEMON_LIST_H

#include "legion.h"

/* create a structure for each daemon */
typedef struct d_struct {
	char *name;
	int pid;
	enum daemon_status status;
	char *command;
} D_STRUCT;

/* create a (singly) linked list of daemons */
typedef struct d_list {
	D_STRUCT *d;
	struct d_list *next;
} D_LIST;

/* add daemon to list */
void add_daemon(D_STRUCT *daemon);

/* get daemon from list */
void get_daemon(D_STRUCT *daemon);

/* remove daemon from list */
void remove_daemon(D_STRUCT *daemon);

/* iterate through daemons */
void print();

#endif