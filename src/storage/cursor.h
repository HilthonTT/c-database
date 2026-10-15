#ifndef STORAGE_CURSOR_H
#define STORAGE_CURSOR_H

#include <stdbool.h>
#include <stdint.h>

#include "storage/table.h"

typedef struct {
    Table* table;
    uint32_t page_num;
    uint32_t cell_num;
    bool end_of_table;
} Cursor;

Cursor cursor_start(Table* table);
Cursor cursor_find(Table* table, uint32_t key);
bool cursor_points_at_key(const Cursor* cursor, uint32_t key);
void* cursor_value(const Cursor* cursor);
void cursor_advance(Cursor* cursor);

#endif
