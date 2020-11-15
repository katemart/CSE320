#ifndef DAEMON_H
#define DAEMON_H

#include "legion.h"

/* create a structure for each daemon */
typedef struct d_struct {
	char *name;
	int pid;
	enum daemon_status status;
	char **command, **words;
	struct d_struct *next;
} D_STRUCT;

/* add daemon to list */
void add_daemon(D_STRUCT *daemon);

/* get daemon from list using name */
D_STRUCT *get_daemon_name(char *d_name);

/* get daemon from list using PID */
D_STRUCT *get_daemon_pid(int pid);

/* custom "free" function for daemon */
void free_daemon(D_STRUCT *d);

/* remove daemon from list */
void remove_daemon_name(char *d_name);

/* remove all daemons in list */
void remove_daemons();

/* print status for registered daemon */
void print_daemon(FILE *out, char *d_name);

/* iterate through daemons */
void print_daemons(FILE *out);

/* return head of list */
D_STRUCT *get_head();

#endif