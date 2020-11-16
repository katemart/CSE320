/*
 * Legion: Command-line interface
 */

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "daemon.h"
#include "debug.h"

#define prompt "legion>"
#define help_message "Available commands:\n" \
"help (0 args) Print this help message\n" \
"quit (0 args) Quit the program\n" \
"register (2 or more args) Register a daemon\n" \
"unregister (1 args) Unregister a daemon\n" \
"status (1 args) Show the status of a daemon\n" \
"status-all (0 args) Show the status of all daemons\n" \
"start (1 args) Start a daemon\n" \
"stop (1 args) Stop a daemon\n" \
"logrotate (1 args) Rotate log files for a daemon\n"

/* env var from libc */
extern char **environ;

/* vars for signal handling */
typedef void (*sig_handler)(int);
static volatile sig_atomic_t alarm_flag = 0;
static volatile sig_atomic_t sigint_flag = 0;
static volatile sig_atomic_t sigchld_flag = 0;

/* declare prototypes */
void reap_children();
void term_all_daemons();
void alarm_handler(int signum);
void sigint_handler(int signum);
void sigchld_handler(int signum);
void send_term_signal(D_STRUCT *d, int term_sig);
int create_signal(int signum, sig_handler handler);

/* ------------------------ OTHER HELPER FUNCS ------------------------ */
/* function to parse input line args */
int parse_args(char ***args_arr, int *arr_len, FILE *out) {
	errno = 0;
  	/* getline */
  	size_t len = 0;
	char *linebuf = NULL;
	if(sigint_flag) return -1;
	int linelen;
	while(1) {
	  	linelen = getline(&linebuf, &len, stdin);
	  	/* check for EOF and sigint flag*/
	  	if(linelen < 0 && errno == EINTR && sigint_flag) {
	  		free(linebuf);
	  		term_all_daemons();
	  		return -1;
	  	}
	  	/* else check for EOF and alarm flag */
	  	else if(linelen < 0 && errno == EINTR) {
	  		clearerr(stdin);
	  	}
	  	/* else check for EOF */
	  	else if(linelen < 0) {
	  		free(linebuf);
	  		term_all_daemons();
	  		fprintf(out, "Error reading command -- giving up.\n");
	  		return -1;
	  	} else break;
	}
  	/* (dynamically allocated) arr to hold args */
	/* initial arr size is set to 5 */
	int n = 5;
	*args_arr = malloc(n *sizeof(char *));
	/* check that memory was successfully allocated */
	if(*args_arr == NULL) {
		fprintf(out, "Error allocating memory for array\n");
		return -1;
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
			fprintf(out, "Error duplicating string\n");
			return -1;
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
				fprintf(out, "Error expanding memory for array\n");
				return -1;
			}
		}
	}
	(*args_arr)[args] = NULL;
	/* free getline buffer */
	free(linebuf);
	/* get actual arr len */
	*arr_len = args;
  	return 0;
}

/* function to free mem in array */
void free_arr_mem(char **arr, int arr_len) {
	for(int i = 0; i < arr_len; i++) {
		free(arr[i]);
	}
	free(arr);
}

/* ------------------------ PROCESSES ------------------------ */
int set_processes(D_STRUCT *d, FILE *out) {
	int fd[2];
	pid_t fork_val;
	pid_t child_pid;
	if(pipe(fd) != 0) {
		fprintf(out, "Error creating pipe\n");
		return -1;
	}
	/* mask */
	sigset_t new_mask, old_mask;
	sigemptyset(&new_mask);
	sigaddset(&new_mask, SIGCHLD);
	if(sigprocmask(SIG_BLOCK, &new_mask, &old_mask) < 0) {
		return -1;
	}
	/* fork returns child PID to parent and zero to the child */
	fork_val = fork();
	if(fork_val < 0) {
		/* if fork fails, set daemon status to inactive */
		fprintf(out, "Error in fork()\n");
		d->status = 1;
		return -1;
	} else if (fork_val == 0) {
		/* this is child process */
		sigprocmask(SIG_SETMASK, &old_mask, NULL);
		/* set pgid */
		if(setpgid(0, 0) < 0) {
			exit(1);
		}
		/* close child read side since we are writing to parent */
		close(fd[0]);
		/* redirect stdout of child process */
		dup2(fd[1], SYNC_FD);
		/* check if logs dir exists, if not make it */
		if(mkdir(LOGFILE_DIR, 0777) < 0 && errno != EEXIST) {
			exit(1);
		}
		/* redirect daemon output to file log */
		/* create large enough string (+1 for / and +1 for 'num' and +1 for \0) */
		int file_buf_len = strlen(LOGFILE_DIR) + 1 + strlen(d->name) + strlen(".log.") + 1 + 1;
		char *file_buf = malloc(file_buf_len);
		snprintf(file_buf, file_buf_len, "%s/%s.log.%c", LOGFILE_DIR, d->name, '0');
		FILE *log_fp = fopen(file_buf, "ab+");
		if(log_fp == NULL) {
			fprintf(out, "Error creating log file\n");
			exit(1);
		}
		int log_fd = fileno(log_fp);
		if(log_fd == -1) {
			fprintf(out, "Error creating log file\n");
			exit(1);
		}
		if(dup2(log_fd, 1) < 0)
			exit(1);
		/* close child write side once we are done writing */
		if(close(fd[1]) < 0)
			exit(1);
		free(file_buf);
		/* get PATH env (and check if NULL) */
		char *old_env = getenv(PATH_ENV_VAR);
		size_t old_env_len = old_env == NULL ? 0 : strlen(old_env);
		/* create large enough string (+1 for : and +1 for \0) */
		char *new_path = malloc(strlen(DAEMONS_DIR) + 1 + old_env_len + 1);
		if(new_path == NULL)
			exit(1);
		/* prepend strings */
		strcpy(new_path, DAEMONS_DIR);
		size_t d_dir_len = strlen(DAEMONS_DIR);
		new_path[d_dir_len] = ':';
		new_path[d_dir_len + 1] = '\0';
		strcat(new_path, old_env);
		/* set new PATH env */
		setenv(PATH_ENV_VAR, new_path, 1);
		free(new_path);
		/* use execvpe to execute command registered for the daemon */
		if(execvpe(d->command[0], d->command, environ) < 0) {
			abort();
		}
	} else {
		/* this is parent process */
		int result = 0;
		/* set child pid */
		child_pid = fork_val;
		/* set struct pid value */
		d->pid = child_pid;
		/* close parent write side since we are reading from child */
		close(fd[1]);
		/* reset alarm flag */
		alarm_flag = 0;
		/* set alarm */
		alarm(CHILD_TIMEOUT);
		/* read one-byte */
		char read_buf[1];
		if(read(fd[0], read_buf, 1) > 0) {
			/* cancel alarm */
			alarm(0);
			/* set daemon status to active */
			d->status = 3;
			/* call active event function */
			sf_active(d->name, child_pid);
			/* close pipe file descriptor */
			close(fd[0]);
			/* go back to prompt */
			result = 0;
		} else {
			kill(child_pid, SIGKILL);
			sf_kill(d->name, d->pid);
			sigsuspend(&old_mask);
			close(fd[0]);
			result = -1;
		}
		if(sigprocmask(SIG_SETMASK, &old_mask, NULL) < 0) {
			result = -1;
		}
		return result;
	}
	return 0;
}

/* function to update status for reaped children*/
void update_term(int pid, int w_status) {
	D_STRUCT *d = get_daemon_pid(pid);
	if(d != NULL) {
		d->status = 5;
		sf_term(d->name, d->pid, WEXITSTATUS(w_status));
		d->pid = 0;
	}
}

/* function to update status for reaped children*/
void update_crash(int pid, int w_status) {
	D_STRUCT *d = get_daemon_pid(pid);
	if(d != NULL) {
		d->pid = 0;
		d->status = 6;
		sf_crash(d->name, d->pid, WTERMSIG(w_status));
	}
}

/* function to update status for reaped children*/
void update_children(int c_pid, int w_status) {
	if(WIFEXITED(w_status)) {
		update_term(c_pid, w_status);
	} else if(WIFSIGNALED(w_status)) {
		update_crash(c_pid, w_status);
	}
}

/* function to update status for reaped children*/
void reap_children() {
	pid_t c_pid;
	int w_status;
	while((c_pid = waitpid(-1, &w_status, WNOHANG | WUNTRACED)) > 0) {
		update_children(c_pid, w_status);
	}
	sigchld_flag = 0;
}

int stop_daemon(D_STRUCT *d, FILE *out) {
	/* reset alarm flag */
	alarm_flag = 0;
	/* mask */
	sigset_t new_mask, old_mask;
	sigemptyset(&new_mask);
	sigaddset(&new_mask, SIGCHLD);
	sigaddset(&new_mask, SIGALRM);
	if(sigprocmask(SIG_BLOCK, &new_mask, &old_mask) < 0) {
		return -1;
	}
	/* set daemon status to stopping */
	d->status = 4;
	/* call stop event function */
	sf_stop(d->name, d->pid);
	kill(d->pid, SIGTERM);
	/*set alarm */
	alarm(CHILD_TIMEOUT);
	/* pause Legion with sigsuspend */
	sigsuspend(&old_mask);
	/* send SIGTERM signal */
	/* if time expires and so sigchld notification, send sigkill */
	if(alarm_flag == 1 && sigchld_flag == 0) {
		kill(d->pid, SIGKILL);
		sf_kill(d->name, d->pid);
		return -1;
	} else reap_children();
	if(sigprocmask(SIG_SETMASK, &old_mask, NULL) < 0) {
		return -1;
	}
	/* set status to inactive */
	d->status = 1;
	return 0;
}

/* function to rotate log files */
int rotate_files(D_STRUCT *d, FILE *out) {
	/* check if logs dir exists, if not make it */
	if(mkdir(LOGFILE_DIR, 0777) < 0 && errno != EEXIST) {
		return -1;
	}
	/* create large enough string (+1 for / and +1 for 'num' and +1 for \0) */
	int file_buf_len = strlen(LOGFILE_DIR) + 1 + strlen(d->name) + strlen(".log.") + 1 + 1;
	char *file_buf = malloc(file_buf_len);
	if(file_buf == NULL) return -1;
	/* unlink max version number file */
	int f_num = LOG_VERSIONS-1;
	snprintf(file_buf, file_buf_len, "%s/%s.log.%c", LOGFILE_DIR, d->name, (f_num + '0'));
	/* check if file to be unlinked exists, if so unlink */
	if(unlink(file_buf) < 0 && errno != ENOENT) {
		free(file_buf);
		return -1;
	}
	char *temp_buf = strdup(file_buf);
	if(temp_buf == NULL) return -1;
	for(int i = f_num-1; i >= 0; i--) {
		if(i + 1 < LOG_VERSIONS) {
			snprintf(file_buf, file_buf_len, "%s/%s.log.%c", LOGFILE_DIR, d->name, (i + '0'));
			snprintf(temp_buf, file_buf_len, "%s/%s.log.%c", LOGFILE_DIR, d->name, ((i + 1) + '0'));
			if(rename(file_buf, temp_buf) < 0 && errno != ENOENT) {
				free(file_buf);
				free(temp_buf);
				return -1;
			}
		}
	}
	/* check if daemon is active */
	if(d->status == 3) {
		/* call logrotate event function */
		sf_logrotate(d->name);
		/* stop daemon */
		stop_daemon(d, out);
		/* restart daemon */
		if(set_processes(d, out) == -1) {
			fprintf(out, "Failed to restart daemon in logrotate\n");
			free(file_buf);
			free(temp_buf);
			return -1;
		}
	}
	/* free buffers */
	free(file_buf);
	free(temp_buf);
	return 0;
}

/* ------------------------ SIGNAL HANDLERS ------------------------ */
void sigchld_handler(int signum) {
	sigchld_flag = 1;
}

void sigint_handler(int signum) {
	sigint_flag = 1;
}

void alarm_handler(int signum) {
	alarm_flag = 1;
}

void send_term_signal(D_STRUCT *d, int term_sig) {
	kill(d->pid, term_sig);
	int status;
	pid_t pid = waitpid(d->pid, &status, 0);
	if(d->pid > 0)
		update_children(pid, status);
}

/* general func to "install" signal handler using sigaction */
int create_signal(int signum, sig_handler handler) {
	struct sigaction s_action;
	s_action.sa_handler = handler;
	sigemptyset(&s_action.sa_mask);
	s_action.sa_flags = 0;
	return sigaction(signum, &s_action, NULL);
}

/* function to terminate all processes */
void term_all_daemons() {
	D_STRUCT *d = get_head();
	while(d != NULL) {
		if(d->pid != 0) {
			sf_stop(d->name, d->pid);
			send_term_signal(d, SIGTERM);
		}
		d = d->next;
	}
}

/* ------------------------ MAIN FUNCTION ------------------------ */
void run_cli(FILE *in, FILE *out)
{
	/* install signal handlers at beginning */
	if(create_signal(SIGALRM, alarm_handler) < 0) {
		return;
	}
	if(create_signal(SIGINT, sigint_handler) < 0) {
		return;
	}
	if(create_signal(SIGCHLD, sigchld_handler) < 0) {
		return;
	}
  	/* prompt loop */
  	while(1) {
  		/* call func to update reaped children */
		reap_children();
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
  		/* check that args were succesfully parsed */
  		if(parse_args(&args_arr, &arr_len, out) < 0) {
  			remove_daemons();
  			break;
  		}
	  	/* check that args array is not empty */
	  	if(arr_len <= 0) {
	  		free_arr_mem(args_arr, arr_len);
	  		continue;
	  	}
	  	/* if not empty, continue to validate args */
	  	char *first_arg = args_arr[0];
	  	/* -- help -- */
	  	if(strcmp(first_arg, "help") == 0) {
  			fprintf(out, "%s", help_message);
  			free_arr_mem(args_arr, arr_len);
	  	}
  		/* -- quit -- */
  		else if(strcmp(first_arg, "quit") == 0) {
  			if(arr_len != 1) {
				fprintf(out, "Wrong number of args (given: %d, required: 0) for command 'quit'\n", arr_len-1);
				sf_error("command execution");
				fprintf(out, "Error executing command: %s\n", first_arg);
				free_arr_mem(args_arr, arr_len);
				continue;
			}
			term_all_daemons();
			free_arr_mem(args_arr, arr_len);
			remove_daemons();
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
			d->words = args_arr;
			/* check if daemon is already registered */
			if(get_daemon_name(d->name) != NULL) {
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
		}
		/* -- start -- */
		else if(strcmp(first_arg, "start") == 0) {
			if(arr_len  != 2) {
				fprintf(out, "Wrong number of args (given: %d, required: 1) for command 'start'\n", arr_len-1);
				sf_error("command execution");
				fprintf(out, "Error executing command: %s\n", first_arg);
				free_arr_mem(args_arr, arr_len);
				continue;
			}
			D_STRUCT *d = get_daemon_name(args_arr[1]);
			/* check if daemon is NOT already registered */
			if(d == NULL) {
				fprintf(out, "Daemon %s is not registered.\n", args_arr[1]);
				sf_error("command execution");
				fprintf(out, "Error executing command: %s\n", first_arg);
				free_arr_mem(args_arr, arr_len);
				continue;
			}
			/* if daemon is registered, check that it isn't started already
			 * i.e., if current status is anything other than inactive then error
			 */
			if(d->status != 1) {
				fprintf(out, "Daemon %s is not inactive.\n", d->name);
				sf_error("command execution");
				fprintf(out, "Error executing command: %s\n", first_arg);
				free_arr_mem(args_arr, arr_len);
				continue;
			}
			/* call start event function */
			sf_start(d->name);
			/* set daemon status to starting */
			d->status = 2;
			/*start parent and children processes */
			if(set_processes(d, out) == -1) {
				sf_error("command execution");
				fprintf(out, "Error executing command: %s\n", first_arg);
				free_arr_mem(args_arr, arr_len);
				continue;
			}
			free_arr_mem(args_arr, arr_len);
		}
		/* -- stop -- */
		else if(strcmp(first_arg, "stop") == 0) {
			if(arr_len  != 2) {
				fprintf(out, "Wrong number of args (given: %d, required: 1) for command 'stop'\n", arr_len-1);
				sf_error("command execution");
				fprintf(out, "Error executing command: %s\n", first_arg);
				free_arr_mem(args_arr, arr_len);
				continue;
			}
			D_STRUCT *d = get_daemon_name(args_arr[1]);
			/* check if daemon is NOT already registered */
			if(d == NULL) {
				fprintf(out, "Daemon %s is not registered.\n", args_arr[1]);
				sf_error("command execution");
				fprintf(out, "Error executing command: %s\n", first_arg);
				free_arr_mem(args_arr, arr_len);
				continue;
			}
			/* if daemon status is exited or crashed, set to inactive  */
			if(d->status == 5 || d->status == 6)  {
				d->status = 1;
				/* call reset event function */
				sf_reset(d->name);
				free_arr_mem(args_arr, arr_len);
				continue;
			}
			/* if status is active, attempt to stop daemon */
			else if (d->status == 3) {
				if(stop_daemon(d, out) < 0) {
					sf_error("command execution");
					fprintf(out, "Error executing command: %s\n", first_arg);
					free_arr_mem(args_arr, arr_len);
					continue;
				} else {
					free_arr_mem(args_arr, arr_len);
					continue;
				}
			}
			/* if status is anything other than active, error */
			else {
				fprintf(out, "Daemon %s is not active.\n", d->name);
				sf_error("command execution");
				fprintf(out, "Error executing command: %s\n", first_arg);
				free_arr_mem(args_arr, arr_len);
				continue;
			}
		}
		/* -- logrotate -- */
		else if(strcmp(first_arg, "logrotate") == 0) {
			if(arr_len != 2) {
				fprintf(out, "Wrong number of args (given: %d, required: 1) for command 'logrotate'\n", arr_len-1);
				sf_error("command execution");
				fprintf(out, "Error executing command: %s\n", first_arg);
				free_arr_mem(args_arr, arr_len);
				continue;
			}
			D_STRUCT *d = get_daemon_name(args_arr[1]);
			/* check if daemon is NOT already registered */
			if(d == NULL) {
				fprintf(out, "Daemon %s is not registered.\n", args_arr[1]);
				sf_error("command execution");
				fprintf(out, "Error executing command: %s\n", first_arg);
				free_arr_mem(args_arr, arr_len);
				continue;
			}
			if(rotate_files(d, out) < 0) {
				sf_error("command execution");
				fprintf(out, "Error executing command: %s\n", first_arg);
				free_arr_mem(args_arr, arr_len);
				continue;
			}
			free_arr_mem(args_arr, arr_len);
		}
		/* -- status -- */
		else if(strcmp(first_arg, "status") == 0) {
			if(arr_len != 2) {
				fprintf(out, "Wrong number of args (given: %d, required: 1) for command 'status'\n", arr_len-1);
				sf_error("command execution");
				fprintf(out, "Error executing command: %s\n", first_arg);
				free_arr_mem(args_arr, arr_len);
				continue;
			}
			/* check if daemon is NOT already registered */
			if(get_daemon_name(args_arr[1]) == NULL) {
				fprintf(out, "%s\t0\tunknown\n", args_arr[1]);
				free_arr_mem(args_arr, arr_len);
				continue;
			}
			print_daemon(out, args_arr[1]);
			free_arr_mem(args_arr, arr_len);
		}
		/* -- status-all -- */
		else if(strcmp(first_arg, "status-all") == 0) {
			print_daemons(out);
			free_arr_mem(args_arr, arr_len);
		}
		/* -- unregister -- */
		else if(strcmp(first_arg, "unregister") == 0) {
			if(arr_len  != 2) {
				fprintf(out, "Wrong number of args (given: %d, required: 1) for command 'unregister'\n", arr_len-1);
				sf_error("command execution");
				fprintf(out, "Error executing command: %s\n", first_arg);
				free_arr_mem(args_arr, arr_len);
				continue;
			}
			D_STRUCT *d = get_daemon_name(args_arr[1]);
			/* check if daemon is NOT already registered */
			if(d == NULL) {
				fprintf(out, "Daemon %s is not registered.\n", args_arr[1]);
				sf_error("command execution");
				fprintf(out, "Error executing command: %s\n", first_arg);
				free_arr_mem(args_arr, arr_len);
				continue;
			}
			/* if daemon is registered, check that it is inactive */
			if(d->status == 1) {
				/* call unregister event function */
				sf_unregister(d->name);
				/* if it is inactive, remove daemon */
				remove_daemon_name(d->name);
			} else {
				/* if it is not inactive, throw error */
				fprintf(out, "Daemon %s is not inactive.\n", d->name);
				sf_error("command execution");
				fprintf(out, "Error executing command: %s\n", first_arg);
				free_arr_mem(args_arr, arr_len);
				continue;
			}
			free_arr_mem(args_arr, arr_len);
		}
		/* -- invalid arg -- */
		else {
			sf_error("command execution");
			fprintf(out, "Error executing command: %s\n", first_arg);
			free_arr_mem(args_arr, arr_len);
			continue;
		}
  	}
}
