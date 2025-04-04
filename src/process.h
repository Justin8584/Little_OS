// src/process.h
#ifndef PROCESS_H
#define PROCESS_H

#define MAX_PROCESSES 16
#define PROCESS_NAME_LENGTH 32

typedef enum
{
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} ProcessState;

typedef struct
{
    char name[PROCESS_NAME_LENGTH];
    ProcessState state;
    unsigned int pid;
} Process;

void process_init(void);
int process_create(const char *name);
int process_list(void);
int process_kill(unsigned int pid);
int process_set_state(unsigned int pid, ProcessState new_state);

#endif