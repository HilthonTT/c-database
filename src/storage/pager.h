#ifndef STORAGE_PAGER_H
#define STORAGE_PAGER_H

#include <stdbool.h>
#include <stdint.h>

#define PAGE_SIZE 4096
#define TABLE_MAX_PAGES 100
#define INVALID_PAGE_NUM UINT32_MAX

typedef struct {
    int file_descriptor;
    uint32_t file_length;
    uint32_t num_pages;
    void* pages[TABLE_MAX_PAGES];
} Pager;

Pager* pager_open(const char* filename);
void pager_close(Pager* pager);
void* pager_get_page(Pager* pager, uint32_t page_num);
uint32_t pager_allocate_page(Pager* pager);
bool pager_has_free_pages(const Pager* pager, uint32_t count);

#endif
