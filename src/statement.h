#ifndef STATEMENT_H
#define STATEMENT_H

#include "row.h"
#include "table.h"

typedef enum {
    STATEMENT_INSERT,
    STATEMENT_SELECT,
} StatementType;

typedef enum {
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT,
    PREPARE_SYNTAX_ERROR,
    PREPARE_STRING_TOO_LONG,
    PREPARE_NEGATIVE_ID,
} PrepareResult;

typedef struct {
    StatementType type;
    Row row_to_insert;
} Statement;

PrepareResult prepare_insert(char* buffer, Statement *statement);
PrepareResult prepare_statement(char *buffer, Statement *statement);
ExecuteResult execute_insert(Statement *statement, Table *table);
ExecuteResult execute_select(Statement *statement, Table *table);
ExecuteResult execute_statement(Statement *statement, Table *table);

#endif
