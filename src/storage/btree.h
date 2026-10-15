#ifndef STORAGE_BTREE_H
#define STORAGE_BTREE_H

#include <stdbool.h>
#include <stdint.h>

#include "storage/cursor.h"
#include "storage/row.h"
#include "storage/table.h"

bool btree_has_room_for_insert(const Cursor* cursor);
void btree_insert(Cursor* cursor, uint32_t key, const Row* value);
void btree_print(Table* table);

#endif
