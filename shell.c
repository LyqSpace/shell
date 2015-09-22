#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SHELL_TOK_BUFSIZE 64
#define SHELL_TOK_DELIM " \t\r\n\a"

int SHELL_cd(char **args);
int SHELL_help(char **args);
int SHELL_exit(char **args);

char *builtin_str[] = { "cd", "help", "exit" };

int (*builtin_func[]) (char **) = { &SHELL_cd, &SHELL_help, &SHELL_exit };

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
  	
	tokens[position] = NULL;
  	return tokens;
}

int SHELL_launch(char **args) {
  	
	pid_t pid, wpid;
  	int status;

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
    	do {
      		wpid = waitpid(pid, &status, WUNTRACED);
    	} while (!WIFEXITED(status) && !WIFSIGNALED(status));
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

  	do {
    	printf("> ");
    	line = SHELL_read_line();
    	args = SHELL_split_line(line);
    	status = SHELL_execute(args);

    	free(line);
    	free(args);
  	} while (status);
}

int main(int argc, char **argv) {

	SHELL_loop();

	// Perform any shutdown/cleanup.

	return EXIT_SUCCESS;
}
