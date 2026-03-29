#ifndef INPUT_H
#define INPUT_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "table.h"

typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND,
} MetaCommandResult;

typedef struct {
    char *buffer;
    size_t buffer_length;
    ssize_t input_length;
} InputBuffer;

InputBuffer* new_input_buffer(void);
void print_prompt(void);
void read_input(InputBuffer *input_buffer);
void close_input_buffer(InputBuffer *input_buffer);
MetaCommandResult do_meta_command(InputBuffer* input_buffer, Table* table);
void print_constants();
void print_leaf_node(void *node);

#endif
