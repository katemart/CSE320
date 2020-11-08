#ifndef DAEMON_H
#define DAEMON_H

#include "legion.h"

/* create a structure for each daemon */
typedef struct d_struct {
	char *name;
	int pid;
	enum daemon_status status;
	char **command;
	struct d_struct *next;
} D_STRUCT;

/* add daemon to list */
void add_daemon(D_STRUCT *daemon);

/* get daemon from list */
D_STRUCT *get_daemon(char *d_name);

/* remove daemon from list */
void remove_daemon(char *d_name);

/* print status for registered daemon */
void print_daemon(FILE *out, char *d_name);

/* iterate through daemons */
void print_daemons(FILE *out);

#endif