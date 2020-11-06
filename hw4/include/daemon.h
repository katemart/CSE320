#ifndef DAEMON_H
#define DAEMON_H

#include "legion.h"

/* create a structure for each daemon */
typedef struct d_struct {
	char *name;
	int pid;
	enum daemon_status status;
	char *command;
	struct d_struct *next;
} D_STRUCT;

/* add daemon to list */
void add_daemon(D_STRUCT *daemon);

/* get daemon from list */
void get_daemon(char *daemon);

/* remove daemon from list */
void remove_daemon(D_STRUCT *daemon);

/* iterate through daemons */
void print_daemon();

#endif