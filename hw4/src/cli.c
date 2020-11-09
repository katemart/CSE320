/*
 * Legion: Command-line interface
 */

#include <stdio.h>
#include <string.h>
#include "daemon.h"

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
  	char *str = NULL;
	/* ptr to traverse args_arr */
	char arr[linelen];
	char *arrbuf = arr;
	strcpy(arrbuf, linebuf);
	/* account for spaces at beginning */
	while(*arrbuf && (*arrbuf == ' '))
	  arrbuf++;
	/* make args_arr from delim words */
	int args = 0;
	if(*arrbuf == '\'') {
		/* if a quot mark is encountered, read until next mark */
		arrbuf++;
		str = strchr(arrbuf, '\'');
		/* if there is not an ending quot mark, continue til end of line */
		if(!str) {
			str = strchr(arrbuf, '\n');
		}
	} else {
		/* use spaces as delimiter */
	    str = strchr(arrbuf, ' ');
	    if(!str) {
		    str = strchr(arrbuf, '\n');
		}
	}
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
			if(!str) {
				str = strchr(arrbuf, '\n');
			}
		} else {
		    str = strchr(arrbuf, ' ');
		    if(!str) {
			    str = strchr(arrbuf, '\n');
			}
		}
		/* expand mem if needed */
		if(args == n) {
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
	/* free getline buffer */
	free(linebuf);
	/* get actual arr len */
	*arr_len = args;

	/*for(int i = 0; i < args; i++) {
  		fprintf(out, "%s\n", (*args_arr)[i]);
  	}*/
}

/* function to free mem in array */
void free_arr_mem(char **arr, int arr_len) {
	for(int i = 0; i < arr_len; i++) {
		free(arr[i]);
	}
	free(arr);
}

void run_cli(FILE *in, FILE *out)
{
    // TO BE IMPLEMENTED
  	/* prompt loop */
  	while(1) {
  		/* print out prompt */
  		fprintf(out, "%s", prompt);
  		/* flush buffer
		 * note: we do this because "legion>" doesn't have \n at the end
		 * buffers are flushed when too large (or newline in the case of stdout), then
		 * when program returns from main, glibc flushes all buffers for FILE *
		 */
  		fflush(out);
  		/* parse given args */
  		/* create array to hold args */
    	int arr_len = 0;
  		char **args_arr = NULL;
  		parse_args(&args_arr, &arr_len, out);
	  	/* check that args array is not empty */
	  	if(arr_len <= 0) {
	  		sf_error("command execution");
	  		fprintf(out, "Command must be specified.\n");
	  		free_arr_mem(args_arr, arr_len);
	  		continue;
	  	}
	  	/* if not empty, continue to validate args */
	  	char *first_arg = args_arr[0];
	  	/* -- help -- */
	  	if(strcmp(first_arg, "help") == 0)
  			fprintf(out, "%s", help_message);
  		/* -- quit -- */
  		else if(strcmp(first_arg, "quit") == 0) {
  			if(arr_len != 1) {
				fprintf(out, "Wrong number of args (given: %d, required: 0) for command 'quit'\n", arr_len-1);
				sf_error("command execution");
				fprintf(out, "Error executing command: %s\n", first_arg);
				free_arr_mem(args_arr, arr_len);
				continue;
			}
			break;
		}
		/* -- register -- */
		else if(strcmp(first_arg, "register") == 0) {
			/* check that num of args is two or more (+1 for first_arg) */
			if(arr_len < 3) {
				fprintf(out, "Usage: register <daemon> <cmd-and-args>\n");
				sf_error("command execution");
				fprintf(out, "Error executing command: %s\n", first_arg);
				free_arr_mem(args_arr, arr_len);
				continue;
			}
			/* if not, create daemon */
			D_STRUCT *d = malloc(sizeof(D_STRUCT));
			if(d == NULL) {
				return;
			}
			d->name = args_arr[1];
			d->pid = 0;
			d->status = 1;
			d->command = args_arr + 2;
			/* check if daemon is already registered */
			if(get_daemon(d->name) != NULL) {
				fprintf(out, "Daemon %s is already registered.\n", d->name);
				sf_error("command execution");
				fprintf(out, "Error executing command: %s\n", first_arg);
				free_arr_mem(args_arr, arr_len);
				continue;
			}
			/* if not, add daemon to "list" */
			add_daemon(d);
			/* call register event function */
			sf_register(d->name, d->command[0]);
			//print_daemons(out);
		}
		/* -- status -- */
		else if(strcmp(first_arg, "status") == 0) {
			//fprintf(out, "%d\n", arr_len);
			if(arr_len != 2) {
				fprintf(out, "Wrong number of args (given: %d, required: 1) for command 'status'\n", arr_len-1);
				sf_error("command execution");
				fprintf(out, "Error executing command: %s\n", first_arg);
				free_arr_mem(args_arr, arr_len);
				continue;
			}
			/* check if daemon is NOT already registered */
			if(get_daemon(args_arr[1]) == NULL) {
				fprintf(out, "%s\t0\tunknown\n", args_arr[1]);
				free_arr_mem(args_arr, arr_len);
				continue;
			}
			print_daemon(out, args_arr[1]);
		}
		/* -- status-all -- */
		else if(strcmp(first_arg, "status-all") == 0) {
			print_daemons(out);
		}
		/* -- unregister -- */
		else if(strcmp(first_arg, "unregister") == 0) {
			D_STRUCT *d = get_daemon(args_arr[1]);
			/* check if daemon is NOT already registered */
			if(d == NULL) {
				fprintf(out, "Daemon %s is not registered.\n", args_arr[1]);
				sf_error("command execution");
				fprintf(out, "Error executing command: %s\n", first_arg);
				free_arr_mem(args_arr, arr_len);
				continue;
			} else {
				/* if daemon is registered, check that it is inactive */
				if(d->status == 2) {
					/* if it is inactive, remove daemon */
					remove_daemon(d->name);
					/* call unregister event function */
					sf_unregister(d->name);
				} else {
					/* if it is not inactive, throw error */
					fprintf(out, "Daemon %s is not inactive.\n", args_arr[1]);
					sf_error("command execution");
					fprintf(out, "Error executing command: %s\n", first_arg);
					free_arr_mem(args_arr, arr_len);
					continue;
				}
			}
		}
		/* -- invalid arg -- */
		else {
			sf_error("command execution");
			fprintf(out, "Error executing command: %s\n", first_arg);
			free_arr_mem(args_arr, arr_len);
			continue;
		}
		/* free memory from args_arr */
		//free_arr_mem(args_arr, arr_len);
  	}

  	/*for(int i = 0; i < arr_len; i++) {
  		fprintf(out, "%s\n", args_arr[i]);
  	}*/
}
