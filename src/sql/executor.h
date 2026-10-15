#ifndef SQL_EXECUTOR_H
#define SQL_EXECUTOR_H

#include "sql/statement.h"
#include "storage/table.h"

typedef enum {
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL,
    EXECUTE_DUPLICATE_KEY,
} ExecuteResult;

ExecuteResult execute_statement(const Statement* statement, Table* table);

#endif
