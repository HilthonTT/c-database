#ifndef CLI_META_COMMAND_H
#define CLI_META_COMMAND_H

#include <stdbool.h>

#include "storage/table.h"

typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_EXIT,
    META_COMMAND_UNRECOGNIZED_COMMAND,
} MetaCommandResult;

bool is_meta_command(const char* input);
MetaCommandResult execute_meta_command(const char* input, Table* table);

#endif
