#include "sql/executor.h"

#include "storage/btree.h"
#include "storage/cursor.h"

static ExecuteResult execute_insert(const Statement* statement, Table* table) {
    const Row* row = &statement->row_to_insert;
    Cursor cursor = cursor_find(table, row->id);

    if (cursor_points_at_key(&cursor, row->id)) {
        return EXECUTE_DUPLICATE_KEY;
    }
    if (!btree_has_room_for_insert(&cursor)) {
        return EXECUTE_TABLE_FULL;
    }

    btree_insert(&cursor, row->id, row);
    return EXECUTE_SUCCESS;
}

static ExecuteResult execute_select(Table* table) {
    Row row;
    for (Cursor cursor = cursor_start(table); !cursor.end_of_table; cursor_advance(&cursor)) {
        row_deserialize(cursor_value(&cursor), &row);
        row_print(&row);
    }
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(const Statement* statement, Table* table) {
    switch (statement->type) {
        case STATEMENT_INSERT:
            return execute_insert(statement, table);
        case STATEMENT_SELECT:
            return execute_select(table);
    }
    return EXECUTE_SUCCESS;
}
