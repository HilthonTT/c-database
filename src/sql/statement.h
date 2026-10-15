#ifndef SQL_STATEMENT_H
#define SQL_STATEMENT_H

#include "storage/row.h"

typedef enum {
    STATEMENT_INSERT,
    STATEMENT_SELECT,
} StatementType;

typedef struct {
    StatementType type;
    Row row_to_insert;
} Statement;

#endif
