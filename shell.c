#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SHELL_TOK_BUFSIZE 64
#define SHELL_TOK_DELIM " \t\r\n\a"

int in_background;
int tokens_len;

int SHELL_cd(char **args);
int SHELL_help(char **args);
int SHELL_exit(char **args);
int SHELL_history(char **args);

char *builtin_str[] = { "cd", "help", "exit", "history" };
char **history_buffer;
int history_size, history_pos;

int (*builtin_func[]) (char **) = { &SHELL_cd, &SHELL_help, &SHELL_exit, &SHELL_history };

void waitChildren(int signo) {
	int status;
	while (waitpid(-1, &status, WNOHANG) > 0);
}

int SHELL_num_builtins() {
	return sizeof(builtin_str) / sizeof(char *);
}

int SHELL_cd(char **args) {
	if (args[1] == NULL) {
	    fprintf(stderr, "SHELL: expected argument to \"cd\"\n");
	} else {
    	if (chdir(args[1]) != 0) {
    		perror("SHELL");
    	}
	}
	return 1;
}

int SHELL_help(char **args) {
  	int i;
  	printf("Liang Yongqing's SHELL root@lyq.me\n");
  	printf("Type program names and arguments, and hit enter.\n");
  	printf("The following functions are built in:\n");

  	for (i = 0; i < SHELL_num_builtins(); i++) {
    	printf("  %s\n", builtin_str[i]);
  	}

  	printf("Use the man command for information on other programs.\n");
  	return 1;
}

int SHELL_exit(char **args) {
  	return 0;
}

int SHELL_history(char **args) {
	
	int i;
	for (i = 0; i < history_pos; i++) {
		printf("%s", history_buffer[i]);
	}
	return 1;
}

char *SHELL_read_line(void) {
  	
	char *line = NULL;
  	ssize_t bufsize = 0;
  	getline(&line, &bufsize, stdin);
  	return line;
}

char **SHELL_split_line(char *line) {
  	
	int bufsize = SHELL_TOK_BUFSIZE, position = 0;
  	char **tokens = malloc(bufsize * sizeof(char*));
  	char *token;

  	if (!tokens) {
    	fprintf(stderr, "SHELL: allocation error\n");
    	exit(EXIT_FAILURE);
  	}

  	token = strtok(line, SHELL_TOK_DELIM);
  	while (token != NULL) {
    	tokens[position] = token;
    	position++;

    	if (position >= bufsize) {
      		bufsize += SHELL_TOK_BUFSIZE;
      		tokens = realloc(tokens, bufsize * sizeof(char*));
      		if (!tokens) {
        		fprintf(stderr, "SHELL: allocation error\n");
        		exit(EXIT_FAILURE);
      		}
    	}

	    token = strtok(NULL, SHELL_TOK_DELIM);
  	}
  	if (strcmp(tokens[position-1],"&") == 0 ) {
		position--;
		in_background = 1;
	} else {
		in_background = 0;
	}
	tokens[position] = NULL;
	
	tokens_len = position;
  	
	return tokens;
}

int SHELL_launch(char **args) {
  	
	pid_t pid, wpid;
  	int status;
	
	int i;
	for (i = 0; i < tokens_len; i++) {
		if (strcmp(args[i],"|") == 0) {

			char **args1, **args2;
			args1 = malloc(i * sizeof(char*));
			args2 = malloc((tokens_len-i) * sizeof(char*));
			int j;
			for (j = 0; j < i; j++) args1[j] = args[j];
			for (j = i+1; j < tokens_len; j++) args2[j-i-1] = args[j];

			pid_t pid1, pid2;
			int fd[2];
			pipe(fd);

			pid1 = fork();
			if (pid1 == 0) {
				close(fd[0]);
				dup2(fd[1],STDOUT_FILENO);
				execvp(args1[0], args1);
				exit(EXIT_FAILURE);
			} else {
				pid2 = fork();
				if (pid2 == 0) {
					close(fd[1]);
					dup2(fd[0],STDIN_FILENO);
					execvp(args2[0], args2);
					exit(EXIT_FAILURE);
				} else {
					close(fd[0]);
					close(fd[1]);
					int status;
					while (wait(NULL) > 0);
				}	
			}
			return 1;
		}
	}
  	pid = fork();
  	if (pid == 0) {
	    // Child process
    	if (execvp(args[0], args) == -1) {
      		perror("SHELL");
    	}
    	exit(EXIT_FAILURE);
  	} else if (pid < 0) {
    	// Error forking
    	perror("SHELL");
  	} else {
    	// Parent process
		if (!in_background) {
    		do {
      			wpid = waitpid(pid, &status, WUNTRACED);
    		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
		} else {
			printf("Child PID is %d\n", pid);
		}
		
  	}
	
  	return 1;
}

int SHELL_execute(char **args) {
  	
	int i;

  	if (args[0] == NULL) {
    	return 1;
  	}

  	for (i = 0; i < SHELL_num_builtins(); i++) {
    	if (strcmp(args[0], builtin_str[i]) == 0) {
    	  	return (*builtin_func[i])(args);
    	}
  	}

  	return SHELL_launch(args);
}

void SHELL_loop(void) {
  	char *line;
  	char **args;
  	int status;
	history_size = SHELL_TOK_BUFSIZE;
	history_pos = 0;
	history_buffer = malloc(history_size * sizeof(char*));	

  	do {
    	printf("> ");
    	line = SHELL_read_line();
		history_buffer[history_pos] = malloc(sizeof(line));
		strcpy(history_buffer[history_pos], line);
		history_pos++;
		if (history_pos >= history_size) {
			history_size += SHELL_TOK_BUFSIZE;
			history_buffer = realloc(history_buffer, history_size * sizeof(char*));
			if (!history_buffer) {
				fprintf(stderr, "SHELL: fail to allocation.");
				exit(EXIT_FAILURE);
			}
		}

    	args = SHELL_split_line(line);
    	status = SHELL_execute(args);

    	free(line);
    	free(args);
  	} while (status);
	
	int i;
	for (i = 0; i < history_pos; i++) {
		free(history_buffer[i]);
	}
	free(history_buffer);
}

int main(int argc, char **argv) {
	
	signal(SIGCHLD, waitChildren);

	SHELL_loop();

	// Perform any shutdown/cleanup.

	return EXIT_SUCCESS;
}
