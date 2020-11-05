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

/* create a structure for each daemon */
typedef struct sf_daemon {
	char *name;
	int pid;
	char *status;
} sf_daemon;

/* create a (singly) linked list of daemons */


char *get_quot_field(char *orig_str, char **new_ptr) {
	char *start_mark, *end_mark;
	/* deal with quotation marks */
	if((start_mark = strchr(orig_str, '\''))) {
		/* get first char after quotation mark */
		start_mark++;
		/* get next quotation mark */
		end_mark = strchr(start_mark, '\'');
		/* check if there really is a next quotation mark, if so continue
		 * else field will be treated as one until new line
		 */
		if(!end_mark) {
			end_mark = strchr(start_mark, '\n');
		}
		/* set pointer (for str after last quot mark) */
		*new_ptr = end_mark + 1;
		/* get string within marks */
		int str_size = end_mark - start_mark;
		char *quot_string = malloc(str_size + 1);
		strncpy(quot_string, start_mark, str_size);
		quot_string[str_size] = 0;
		return quot_string;
	}
	return NULL;
}

void run_cli(FILE *in, FILE *out)
{
    // TO BE IMPLEMENTED
    /* print out prompt */
	fprintf(out, "legion>");
	/* flush buffer
	 * note: we do this because "legion>" doesn't have \n at the end
	 * buffers are flushed when too large (or newline in the case of stdout), then
	 * when proggraam returns from main, glibc flushes all buffers for FILE *
	 */
	fflush(out);
    /* set fields for getline */
	size_t len = 0;
	ssize_t linelen;
	char *linebuf = NULL;
	/* iterate through args while NOT EOF */
	while((linelen = getline(&linebuf, &len, in)) != EOF) {
		int count = 0;
		/* use strtok to get "first" arg */
		char *copy = strdup(linebuf);
  		char *first_arg = strtok(copy, " \n");
  		/* get leftover args (after first) */
  		char *leftover_args = strpbrk(linebuf, " \'");
		/* -- help -- */
		if(strcmp(first_arg, "help") == 0 ) {
			fprintf(out, help_menu);
		}
		/* -- quit -- */
		else if(strcmp(first_arg, "quit") == 0) {
			sf_fini();
			exit(0);
		}
		/* -- status -- */
		else if(strcmp(first_arg, "status") == 0) {
			char *name, *new_str, *new_ptr = NULL;
			if((name = get_quot_field(leftover_args, &new_ptr))) {
				count++;
				strcpy(leftover_args, new_ptr);
			} else {
				new_str = strtok(leftover_args, " ");
				if(new_str != NULL) {
					while(new_str) {
						name = new_str;
						new_str = strtok(NULL, " ");
						count++;
					}
				}
			}
			if(count == 1) {
				fprintf(out, "%s", name);
			} else {
				fprintf(out, "Wrong number of args (given: 2, required: 1) for command 'status'\n");
			}
			//goto PROMPT;
		}
		//PROMPT:
		/* print out prompt again every time after a field has been given */
		fprintf(out, "legion>");
		/* flush buffer again */
		fflush(out);
	}
	/* if EOF, then print error msg */
	if(linelen < 0) {
		fprintf(out, "Error reading command -- giving up.\n");
		sf_fini();
		exit(0);
	}
	//fflush(out);
    //abort();
}
