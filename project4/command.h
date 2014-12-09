#ifndef COMMAND_H
#define COMMAND_H

typedef struct {
    char *name;
    char **argv;
    int argc;
    char *input_file;
    char *output_file;
} command;

command *new_command(char *);
void delete_command(command *);

#endif
