#include "storage/cursor.h"

#include "storage/node.h"

static Cursor leaf_node_find(Table* table, uint32_t page_num, uint32_t key) {
    void* node = pager_get_page(table->pager, page_num);
    uint32_t min_index = 0;
    uint32_t one_past_max_index = leaf_node_num_cells(node);

    while (min_index != one_past_max_index) {
        uint32_t index = min_index + (one_past_max_index - min_index) / 2;
        uint32_t key_at_index = leaf_node_key(node, index);
        if (key == key_at_index) {
            min_index = index;
            break;
        }
        if (key < key_at_index) {
            one_past_max_index = index;
        } else {
            min_index = index + 1;
        }
    }

    return (Cursor){
        .table = table,
        .page_num = page_num,
        .cell_num = min_index,
        .end_of_table = false,
    };
}

Cursor cursor_find(Table* table, uint32_t key) {
    uint32_t page_num = table->root_page_num;
    void* node = pager_get_page(table->pager, page_num);

    while (node_type(node) == NODE_INTERNAL) {
        uint32_t child_index = internal_node_find_child(node, key);
        page_num = internal_node_child(node, child_index);
        node = pager_get_page(table->pager, page_num);
    }

    return leaf_node_find(table, page_num, key);
}

Cursor cursor_start(Table* table) {
    Cursor cursor = cursor_find(table, 0);
    void* node = pager_get_page(table->pager, cursor.page_num);
    cursor.end_of_table = leaf_node_num_cells(node) == 0;
    return cursor;
}

bool cursor_points_at_key(const Cursor* cursor, uint32_t key) {
    void* node = pager_get_page(cursor->table->pager, cursor->page_num);
    return cursor->cell_num < leaf_node_num_cells(node) &&
           leaf_node_key(node, cursor->cell_num) == key;
}

void* cursor_value(const Cursor* cursor) {
    void* node = pager_get_page(cursor->table->pager, cursor->page_num);
    return leaf_node_value(node, cursor->cell_num);
}

void cursor_advance(Cursor* cursor) {
    void* node = pager_get_page(cursor->table->pager, cursor->page_num);

    cursor->cell_num++;
    if (cursor->cell_num < leaf_node_num_cells(node)) {
        return;
    }

    uint32_t next_page_num = leaf_node_next_leaf(node);
    if (next_page_num == 0) {
        cursor->end_of_table = true;
    } else {
        cursor->page_num = next_page_num;
        cursor->cell_num = 0;
    }
}
