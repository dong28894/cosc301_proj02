#ifndef __LIST_H__
#define __LIST_H__
#include <stdbool.h>

/* list data structure declarations */
struct path {
    char name[128];
    struct path *next; 
};

struct process{
	int pid;
	char cmd[128];
	bool running;
	struct process *next;
};

/* function declarations associated with the list */
void path_clear(struct path *);
void path_append(const char *, struct path **);

void proc_clear(struct process *);
struct process *proc_find(int, struct process *);
void proc_print(const struct process *);
int proc_delete(int, struct process **);
void proc_append(int, const char *, struct process **);

#endif // __LIST_H__
