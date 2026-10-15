#include "cli/input.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "common/fatal.h"
#include "common/memory.h"

InputBuffer* input_buffer_new(void) {
    return xcalloc(1, sizeof(InputBuffer));
}

bool input_buffer_read_line(InputBuffer* input, FILE* stream) {
    errno = 0;
    ssize_t bytes_read = getline(&input->buffer, &input->capacity, stream);
    if (bytes_read == -1) {
        if (errno != 0) {
            fatal("error reading input: %s", strerror(errno));
        }
        return false;
    }

    size_t length = (size_t)bytes_read;
    while (length > 0 && (input->buffer[length - 1] == '\n' || input->buffer[length - 1] == '\r')) {
        length--;
    }
    input->buffer[length] = '\0';
    input->length = length;
    return true;
}

void input_buffer_free(InputBuffer* input) {
    free(input->buffer);
    free(input);
}
