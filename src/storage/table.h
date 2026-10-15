#ifndef STORAGE_TABLE_H
#define STORAGE_TABLE_H

#include <stdint.h>

#include "storage/pager.h"

typedef struct {
    Pager* pager;
    uint32_t root_page_num;
} Table;

Table* table_open(const char* filename);
void table_close(Table* table);

#endif
