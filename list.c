#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "list.h"
#include <string.h>
#include <stdbool.h>





void path_clear(struct path *list) {
    while (list != NULL) {
        struct path *tmp = list;
        list = list->next;
        free(tmp);
    }
}

void path_append(const char *name, struct path **head) {
	struct path *newNode = malloc(sizeof(struct path));
	struct path *temp = *head;
	strcpy(newNode->name, name);
	newNode->next = NULL;
	if (*head == NULL){
		*head = newNode;
	}else{
		while(temp->next != NULL){
			temp = temp->next;
		}
		temp->next = newNode;
	}
}

void proc_clear(struct process *list) {
    while (list != NULL) {
        struct process *tmp = list;
        list = list->next;
		free(tmp->cmd);
        free(tmp);
    }
}

struct process *proc_find(int pid, struct process *head) {
	while (head != NULL){
		if (head->pid == pid){
			return head;
		}
		head = head->next;
	}
	return head;    
}

void proc_print(const struct process *list) {
    printf("All background process:\n");
    while (list != NULL) {
        printf("Process ID: %d\t Command: %s\t State: ", list->pid, list->cmd);
		if (list->running){
			printf("Running\n");
		}else{
			printf("Paused\n");
		}
        list = list->next;
    }
}

int proc_delete(int pid, struct process **head) {
	struct process *temp = *head;
	if (temp->pid == pid){
		*head = temp->next;
		free(temp);
		return 1;
	}
	temp = temp->next;
	struct process *prev = *head;
	while (temp != NULL){
		if (temp->pid == pid){
			prev->next = temp->next;
			free(temp);
			return 1;
		}
		prev = temp;
		temp = temp->next;
	}
	return 0;
}

void proc_append(int pid, const char *cmd, struct process **head) {
	struct process *newNode = malloc(sizeof(struct process));
	struct process *temp = *head;
	newNode->pid = pid;
	strcpy(newNode->cmd, cmd);
	newNode->running = true;
	newNode->next = NULL;
	if (*head == NULL){
		*head = newNode;
	}else{
		while(temp->next != NULL){
			temp = temp->next;
		}
		temp->next = newNode;
	}
}

