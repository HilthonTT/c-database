#ifndef SQL_PARSER_H
#define SQL_PARSER_H

#include "sql/statement.h"

typedef enum {
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT,
    PREPARE_SYNTAX_ERROR,
    PREPARE_STRING_TOO_LONG,
    PREPARE_NEGATIVE_ID,
} PrepareResult;

PrepareResult prepare_statement(const char* input, Statement* statement);

#endif
