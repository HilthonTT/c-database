#ifndef TABLE_H
#define TABLE_H

#include "row.h"
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define PAGE_SIZE 4096
#define TABLE_MAX_PAGES 100
#define ROWS_PER_PAGE (PAGE_SIZE / ROW_SIZE)
#define TABLE_MAX_ROWS (ROWS_PER_PAGE * TABLE_MAX_PAGES)

typedef enum {
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL,
} ExecuteResult;

typedef struct {
    int file_descriptor;
    uint32_t file_length;
    void *pages[TABLE_MAX_PAGES];
} Pager;

typedef struct {
    Pager* pager;
    uint32_t num_rows;
} Table;

Table* db_open(const char *filename);
void* row_slot(Table* table, uint32_t row_num);
void* get_page(Pager* pager, uint32_t page_num);
Pager* pager_open(const char* filename);
void db_close(Table *table);
void pager_flush(Pager* pager, uint32_t page_num, uint32_t size);

#endif
