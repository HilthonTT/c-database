#ifndef CLI_INPUT_H
#define CLI_INPUT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef struct {
    char* buffer;
    size_t capacity;
    size_t length;
} InputBuffer;

InputBuffer* input_buffer_new(void);
bool input_buffer_read_line(InputBuffer* input, FILE* stream);
void input_buffer_free(InputBuffer* input);

#endif
