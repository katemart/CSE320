/*
 * Legion: Command-line interface
 */

#include "legion.h"
#include <stdio.h>
#include <string.h>

#define prompt "legion>"
#define help_message "Available commands:\n" \
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
	char *command;
} sf_daemon;

/* create a (singly) linked list of daemons */


/* function to parse input line args */
void parse_args(char ***args_arr, int *arr_len, FILE *out) {
	/* (dynamically allocated) arr to hold args */
	/* initial arr size is set to 5 */
	int n = 5;
	*args_arr = malloc(n *sizeof(char *));
	/* check that memory was successfully allocated */
	if(*args_arr == NULL) {
		fprintf(out, "Error allocating memory for array");
		return;
	}
  	/* getline */
  	size_t len = 0;
	char *linebuf = NULL;
  	int linelen = getline(&linebuf, &len, stdin);
  	/* check for EOF */
  	if(linelen < 0) {
  		fprintf(out, "Error reading command -- giving up.\n");
  		sf_fini();
  		exit(0);
  	}
  	/* var to point to delim str */
  	char *str;
	/* ptr to traverse args_arr */
	char arr[linelen];
	char *arrbuf = arr;
	strcpy(arrbuf, linebuf);
	/* turn newline to space */
	arrbuf[linelen-1] = ' ';
	/* account for spaces at beginning */
	while(*arrbuf && (*arrbuf == ' '))
	  arrbuf++;
	/* make args_arr from delim words */
	int args = 0;
	if(*arrbuf == '\'') {
		arrbuf++;
		str = strchr(arrbuf, '\'');
	} else str = strchr(arrbuf, ' ');
	while(str) {
		/* null-terminate str */
		*str = '\0';
		/* make sure that string was correctly dup */
		if(((*args_arr)[args++] = strdup(arrbuf)) == NULL) {
			fprintf(out, "Error duplicating string");
			return;
		}
		arrbuf = str + 1;
		/* account for spaces */
		while(*arrbuf && (*arrbuf == ' '))
			arrbuf++;
		if(*arrbuf == '\'') {
			arrbuf++;
			str = strchr(arrbuf, '\'');
		} else str = strchr(arrbuf, ' ');
		/* expand mem if needed */
		if(args == n) {
			//printf("getting more mem!\n");
			n += 5;
			*args_arr = realloc(*args_arr, (n)*sizeof(char *));
			/* check that memory was successfully allocated */
			if(*args_arr == NULL) {
				fprintf(out, "Error expanding memory for array");
				return;
			}
		}
	}
	(*args_arr)[args] = NULL;
	/* get actual arr len */
	*arr_len = args;
}


void run_cli(FILE *in, FILE *out)
{
    // TO BE IMPLEMENTED
    /* create array tp hold args */
    int arr_len = 0;
  	char **args_arr = NULL;
  	/* prompt loop */
  	while(1) {
  		/* print out prompt */
  		fprintf(out, "%s", prompt);
  		/* flush buffer
		 * note: we do this because "legion>" doesn't have \n at the end
		 * buffers are flushed when too large (or newline in the case of stdout), then
		 * when proggraam returns from main, glibc flushes all buffers for FILE *
		 */
  		fflush(out);
  		/* parse given args */
  		parse_args(&args_arr, &arr_len, out);
	  	/* check that args array is not empty */
	  	if(arr_len == 0) {
	  		sf_error("command execution");
	  		fprintf(out, "Command must be specified.\n");
	  		continue;
	  	}
	  	/* if not empty, continue to validate args */
	  	char *first_arg = args_arr[0];
	  	/* -- help -- */
	  	if(strcmp(first_arg, "help") == 0)
  			fprintf(out, "%s", help_message);
  		/* -- quit -- */
  		else if(strcmp(first_arg, "quit") == 0) {
			sf_fini();
			exit(0);
		}
		else {
			sf_error("Command execution.\n");
			fprintf(out, "Error executing command: %s", first_arg);
			continue;
		}
  	}

  	/*for(int i = 0; i < arr_len; i++) {
  		fprintf(out, "%s\n", args_arr[i]);
  	}*/
}
