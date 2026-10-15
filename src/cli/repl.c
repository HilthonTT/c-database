#include "cli/repl.h"

#include <stdbool.h>
#include <stdio.h>

#include "cli/input.h"
#include "cli/meta_command.h"
#include "sql/executor.h"
#include "sql/parser.h"

#define PROMPT "db > "

static bool handle_meta_command(const char* input, Table* table) {
    switch (execute_meta_command(input, table)) {
        case META_COMMAND_SUCCESS:
            return true;
        case META_COMMAND_EXIT:
            return false;
        case META_COMMAND_UNRECOGNIZED_COMMAND:
            printf("Unrecognized command '%s'\n", input);
            return true;
    }
    return true;
}

static bool prepare(const char* input, Statement* statement) {
    switch (prepare_statement(input, statement)) {
        case PREPARE_SUCCESS:
            return true;
        case PREPARE_SYNTAX_ERROR:
            printf("Syntax error. Could not parse statement.\n");
            return false;
        case PREPARE_STRING_TOO_LONG:
            printf("String is too long.\n");
            return false;
        case PREPARE_NEGATIVE_ID:
            printf("ID must be positive.\n");
            return false;
        case PREPARE_UNRECOGNIZED_STATEMENT:
            printf("Unrecognized keyword at start of '%s'.\n", input);
            return false;
    }
    return false;
}

static void execute(const Statement* statement, Table* table) {
    switch (execute_statement(statement, table)) {
        case EXECUTE_SUCCESS:
            printf("Executed.\n");
            break;
        case EXECUTE_DUPLICATE_KEY:
            printf("Error: Duplicate key.\n");
            break;
        case EXECUTE_TABLE_FULL:
            printf("Error: Table full.\n");
            break;
    }
}

void repl_run(Table* table) {
    InputBuffer* input = input_buffer_new();

    for (;;) {
        printf(PROMPT);
        if (!input_buffer_read_line(input, stdin)) {
            break;
        }

        if (is_meta_command(input->buffer)) {
            if (!handle_meta_command(input->buffer, table)) {
                break;
            }
            continue;
        }

        Statement statement;
        if (prepare(input->buffer, &statement)) {
            execute(&statement, table);
        }
    }

    input_buffer_free(input);
}
