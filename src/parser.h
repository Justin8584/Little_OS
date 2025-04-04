#ifndef PARSER_H
#define PARSER_H

#include "string.h"

#define MAX_ARGUMENTS 16
#define MAX_ARG_LENGTH 32
#define MAX_COMMANDS 4

typedef struct Command
{
    char args[MAX_ARGUMENTS][MAX_ARG_LENGTH];
    int arg_count;

    // For redirection
    char input_file[MAX_ARG_LENGTH];
    char output_file[MAX_ARG_LENGTH];
    int append_output; // 1 if >> (append), 0 if > (overwrite)

    // For piping
    struct Command *next_command;
} Command;

// Parse a command line
int parse_command_line(const char *input, Command *commands, int *command_count);

// Execute a single command
int execute_command(Command *cmd);

// Execute a command pipeline
int execute_pipeline(Command *commands, int command_count);

#endif /* PARSER_H */