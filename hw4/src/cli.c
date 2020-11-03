/*
 * Legion: Command-line interface
 */

#include "legion.h"
#include <stdio.h>
#include <string.h>

#define help_menu "Available commands:\n" \
"help (0 args) Print this help message\n" \
"quit (0 args) Quit the program\n" \
"register (0 args) Register a daemon\n" \
"unregister (1 args) Unregister a daemon\n" \
"status (1 args) Show the status of a daemon\n" \
"status-all (0 args) Show the status of all daemons\n" \
"start (1 args) Start a daemon\n" \
"stop (1 args) Stop a daemon\n" \
"logrotate (1 args) Rotate log files for a daemon\n"

void run_cli(FILE *in, FILE *out)
{
    // TO BE IMPLEMENTED
    /* print out prompt */
	fprintf(out, "legion>");
    /* loop through to get arg fields */
	size_t len = 0;
	ssize_t linelen;
	char *linebuf = NULL;
	char delimit[]=" \n";
	while((linelen = getline(&linebuf, &len, in)) != EOF) {
		if(strcmp(strtok(linebuf, delimit), "help") == 0 ) {
			fprintf(out, help_menu);
		}
	}

    //abort();
}
