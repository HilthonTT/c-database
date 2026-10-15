#include "storage/pager.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common/fatal.h"
#include "common/memory.h"

Pager* pager_open(const char* filename) {
    int fd = open(filename, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
    if (fd == -1) {
        fatal("unable to open '%s': %s", filename, strerror(errno));
    }

    off_t file_length = lseek(fd, 0, SEEK_END);
    if (file_length == -1) {
        fatal("unable to seek '%s': %s", filename, strerror(errno));
    }
    if (file_length % PAGE_SIZE != 0) {
        fatal("'%s' is not a whole number of pages; the file is corrupt", filename);
    }
    if (file_length / PAGE_SIZE > TABLE_MAX_PAGES) {
        fatal("'%s' has more than %d pages", filename, TABLE_MAX_PAGES);
    }

    Pager* pager = xcalloc(1, sizeof(Pager));
    pager->file_descriptor = fd;
    pager->file_length = (uint32_t)file_length;
    pager->num_pages = (uint32_t)(file_length / PAGE_SIZE);
    return pager;
}

static uint32_t pages_on_disk(const Pager* pager) {
    return pager->file_length / PAGE_SIZE;
}

static void load_page(const Pager* pager, uint32_t page_num, void* page) {
    off_t offset = (off_t)page_num * PAGE_SIZE;
    ssize_t bytes_read = pread(pager->file_descriptor, page, PAGE_SIZE, offset);
    if (bytes_read == -1) {
        fatal("error reading page %u: %s", page_num, strerror(errno));
    }
}

static void flush_page(const Pager* pager, uint32_t page_num) {
    off_t offset = (off_t)page_num * PAGE_SIZE;
    ssize_t bytes_written =
        pwrite(pager->file_descriptor, pager->pages[page_num], PAGE_SIZE, offset);
    if (bytes_written != PAGE_SIZE) {
        fatal("error writing page %u: %s", page_num, strerror(errno));
    }
}

void* pager_get_page(Pager* pager, uint32_t page_num) {
    if (page_num >= TABLE_MAX_PAGES) {
        fatal("page number %u out of bounds (max %d)", page_num, TABLE_MAX_PAGES);
    }

    if (pager->pages[page_num] == NULL) {
        void* page = xcalloc(1, PAGE_SIZE);
        if (page_num < pages_on_disk(pager)) {
            load_page(pager, page_num, page);
        }
        pager->pages[page_num] = page;
        if (page_num >= pager->num_pages) {
            pager->num_pages = page_num + 1;
        }
    }

    return pager->pages[page_num];
}

uint32_t pager_allocate_page(Pager* pager) {
    if (!pager_has_free_pages(pager, 1)) {
        fatal("table is full (%d pages)", TABLE_MAX_PAGES);
    }
    uint32_t page_num = pager->num_pages;
    pager_get_page(pager, page_num);
    return page_num;
}

bool pager_has_free_pages(const Pager* pager, uint32_t count) {
    return pager->num_pages + count <= TABLE_MAX_PAGES;
}

void pager_close(Pager* pager) {
    for (uint32_t i = 0; i < pager->num_pages; i++) {
        if (pager->pages[i] != NULL) {
            flush_page(pager, i);
            free(pager->pages[i]);
        }
    }

    if (close(pager->file_descriptor) == -1) {
        fatal("error closing database file: %s", strerror(errno));
    }

    free(pager);
}
