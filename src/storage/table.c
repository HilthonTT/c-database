#include "storage/table.h"

#include <stdlib.h>

#include "common/memory.h"
#include "storage/node.h"

Table* table_open(const char* filename) {
    Pager* pager = pager_open(filename);

    Table* table = xmalloc(sizeof(Table));
    table->pager = pager;
    table->root_page_num = 0;

    if (pager->num_pages == 0) {
        void* root = pager_get_page(pager, 0);
        leaf_node_initialize(root);
        node_set_root(root, true);
    }

    return table;
}

void table_close(Table* table) {
    pager_close(table->pager);
    free(table);
}
