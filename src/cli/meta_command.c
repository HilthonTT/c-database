#include "cli/meta_command.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "storage/btree.h"
#include "storage/node.h"

typedef MetaCommandResult (*MetaCommandHandler)(Table* table);

typedef struct {
    const char* name;
    MetaCommandHandler handler;
} MetaCommand;

static MetaCommandResult exit_command(Table* table) {
    (void)table;
    return META_COMMAND_EXIT;
}

static MetaCommandResult constants_command(Table* table) {
    (void)table;
    printf("Constants:\n");
    printf("ROW_SIZE: %d\n", ROW_SIZE);
    printf("COMMON_NODE_HEADER_SIZE: %d\n", COMMON_NODE_HEADER_SIZE);
    printf("LEAF_NODE_HEADER_SIZE: %d\n", LEAF_NODE_HEADER_SIZE);
    printf("LEAF_NODE_CELL_SIZE: %d\n", LEAF_NODE_CELL_SIZE);
    printf("LEAF_NODE_SPACE_FOR_CELLS: %d\n", LEAF_NODE_SPACE_FOR_CELLS);
    printf("LEAF_NODE_MAX_CELLS: %d\n", LEAF_NODE_MAX_CELLS);
    return META_COMMAND_SUCCESS;
}

static MetaCommandResult btree_command(Table* table) {
    printf("Tree:\n");
    btree_print(table);
    return META_COMMAND_SUCCESS;
}

static const MetaCommand META_COMMANDS[] = {
    {".exit", exit_command},
    {".constants", constants_command},
    {".btree", btree_command},
};

bool is_meta_command(const char* input) {
    return input[0] == '.';
}

MetaCommandResult execute_meta_command(const char* input, Table* table) {
    for (size_t i = 0; i < sizeof(META_COMMANDS) / sizeof(META_COMMANDS[0]); i++) {
        if (strcmp(input, META_COMMANDS[i].name) == 0) {
            return META_COMMANDS[i].handler(table);
        }
    }
    return META_COMMAND_UNRECOGNIZED_COMMAND;
}
