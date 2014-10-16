#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>

#include "list.h"

void delComment(char *s){
	// Get rid of the comment
	for (int i = 0; i < strlen(s); i++){
		if (s[i] == '#'){
			s[i] = '\0';
			break;
		}
	}
}

char** parseCmd(const char *s) {
    // Parse a single command
    char *str1 = strdup(s);
    char *str2 = strdup(s);
    char *tok;
    int count = 0;
    tok = strtok(str1, " \n\t");
    while (tok != NULL) {
		count++;
        tok = strtok(NULL, " \n\t");        
    }
	if (count == 0){
		return NULL;
	}
    char **ptr = malloc((count+1)*sizeof(char*));
    tok = strtok(str2, " \n\t");
    for (int i = 0; i < count; i++){
        ptr[i] = strdup(tok);
        tok = strtok(NULL, " \n\t");
    }
    ptr[count] = NULL;
	free(str1);
	free(str2);
    return ptr;
}


char*** separateCmds(const char *s) {
    // separate the distinct commands in a single line
    char *str1 = strdup(s);
    char *str2 = strdup(s);
    char *tok;
    int count = 0;
    tok = strtok(str1, ";");
    while (tok != NULL) {
		count++;
        tok = strtok(NULL, ";");        
    }
    char ***ptr = malloc((count+1)*sizeof(char**));
	char **cmds = malloc((count)*sizeof(char*));
    tok = strtok(str2, ";");
    for (int i = 0; i < count; i++){      
        cmds[i] = strdup(tok);
        tok = strtok(NULL, ";");
    }
	for (int i = 0; i < count; i++){
		ptr[i] = parseCmd(cmds[i]);
	}
    ptr[count] = NULL;
	free(str1);
	free(str2);
	for (int i = 0; i < count; i++){
		free(cmds[i]);
	}
	free(cmds);
    return ptr;
}

void changeMode(char **cmd, bool *newMode, bool parallel){
	char *para1 = "parallel";
	char *para2 = "p";
	char *seq1 = "sequential";
	char *seq2 = "s";
	if (cmd[1] == NULL){
		if (parallel){
			printf("The shell is running in parallel mode.\n");
		}else{
			printf("The shell is running in sequential mode.\n");
		}
	}else if (cmd[2] != NULL){
		printf("Invalid command!\n");
	}else if (strcasecmp(cmd[1], para1) == 0 || strcasecmp(cmd[1], para2) == 0){
		*newMode = true;
	}else if (strcasecmp(cmd[1], seq1) == 0 || strcasecmp(cmd[1], seq2) == 0){
		*newMode = false;
	}else{
		printf("Invalid command!\n");
	}
}

struct path *process_data(FILE *input_file) {
	char line[255];	
	struct path *head = NULL;
	while (fgets(line, 255, input_file) != NULL){		
		char *s = line;
		s[strlen(s)-1] = '/';
		s[strlen(s)] = '\0';
		path_append(s, &head);
	}
	return head;
}

bool fileExistence(char *address){
	struct stat statresult;
	int rv = stat(address, &statresult);
	if (rv < 0) {
    	return false;
	}else{
		return true;
	}
}

void fileSearch(char **file, struct path *config){
	if (fileExistence(*file)){
		return;
	}else{
		while (config != NULL){
			char *directory = malloc(sizeof(char)*50);
			strcpy(directory, config->name);
			strcat(directory, *file);
			if (fileExistence(directory)){
				char *tmp = *file;
				*file = directory;
				free(tmp);
				return;
			}else{
				config = config->next;
			}
		}
	}
}

void checkPoll(struct process **jobs){
	int status;
	// declare an array of a single struct pollfd
   	struct pollfd pfd[1];
   	pfd[0].fd = 0; // stdin is file descriptor 0
   	pfd[0].events = POLLIN;
   	pfd[0].revents = 0;
	while (true){ 
    	// wait for input on stdin, up to 1000 milliseconds
    	int rv = poll(&pfd[0], 1, 1000);
     		// the return value tells us whether there was a 
    		// timeout (0), something happened (>0) or an
    		// error (<0).

   		if (rv == 0) {
       		int pid;
			while ((pid = waitpid (-1, &status, WNOHANG)) > 0){
				struct process *curr = proc_find(pid, *jobs);
   				fprintf (stderr, "Process %d (%s) has terminated \n", pid, curr->cmd);
				proc_delete(pid, jobs);
			}    
   		} else if (rv > 0) {
       		return;
   		} else {
       		printf("there was some kind of error: %s\n", strerror(errno));
   		}
	}
}

bool builtIn(int i, char ***cmdArr, struct process *jobs, bool *newMode, bool parallel, bool *exitShell){
	if (strcasecmp(cmdArr[i][0], "mode") == 0){
		changeMode(cmdArr[i], newMode, parallel);	
		return true;	
	}else if (strcasecmp(cmdArr[i][0], "exit") == 0){
		if (cmdArr[i][1] != NULL){
			printf("Command does not exist\n");
		}else if (jobs != NULL){
			printf("Processes are still running\n");
		}else{
			*exitShell = true;
		}
		return true;
	}else if (strcasecmp(cmdArr[i][0], "jobs") == 0){
		proc_print(jobs);
		return true;
	}else if (strcasecmp(cmdArr[i][0], "pause") == 0){
		if (cmdArr[i][1] == NULL || cmdArr[i][2] != NULL){
			printf("Command does not exist\n");
		}else{
			int pid = atoi(cmdArr[i][1]);
			kill(pid, SIGSTOP);
			struct process *curr = proc_find(pid, jobs);
			curr->running = false;
		}
		return true;
	}else if (strcasecmp(cmdArr[i][0], "resume") == 0){
		if (cmdArr[i][1] == NULL || cmdArr[i][2] != NULL){
			printf("Command does not exist\n");
		}else{
			int pid = atoi(cmdArr[i][1]);
			kill(pid, SIGCONT);
			struct process *curr = proc_find(pid, jobs);
			curr->running = true;
		}
		return true;
	}else{
		return false;
	}
}

int main(int argc, char **argv) {
	FILE *datafile = NULL;
	bool parallel = false;
	char *prompt = "Dong's Shell>";
	char buffer[1024];
	datafile = fopen("shell-config", "r");  		 
	struct process *jobs = NULL; 
	struct path *config = process_data(datafile);
	fclose(datafile);
	printf("%s", prompt);
	fflush(stdout);		
	while (fgets(buffer, 1024, stdin) != NULL){
		delComment(buffer);
		char ***cmdArr = separateCmds(buffer);
		int i = 0;
		bool builtInCmd;
		bool newMode = parallel;
		bool exitShell = false;
		while (cmdArr[i] != NULL){
			builtInCmd = builtIn(i, cmdArr, jobs, &newMode, parallel, &exitShell);		
			if (!builtInCmd){		
				fileSearch(&cmdArr[i][0], config);		
				int pid = fork();
				if (pid == 0){					
					if (execv(cmdArr[i][0], cmdArr[i]) < 0){
						fprintf(stderr, "execv failed: %s\n", strerror(errno));
					}
					return 0;
				}else{					
					if (!parallel){
						wait(&pid);
					}else{
						proc_append(pid, cmdArr[i][0], &jobs);
					}
				}
			}
			i++;
		}		
		int j = 0;
		int k = 0;
		while (cmdArr[j] != NULL){
			while (cmdArr[j][k] != NULL){
				free(cmdArr[j][k]);
				k++;
			}
			free(cmdArr[j]);
			j++;
			k = 0;
		}
		free(cmdArr);	
		if (newMode){
			parallel = true;
		}else{
			parallel = false;
		}
		if (exitShell){			
			path_clear(config);
			exit(0);
		}else{
			printf("%s", prompt);
			fflush(stdout);
		}
		if (parallel && jobs != NULL){
			checkPoll(&jobs);
		}
	}	
	path_clear(config);
    return 0;
}

