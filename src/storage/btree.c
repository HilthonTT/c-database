#include "storage/btree.h"

#include <stdio.h>
#include <string.h>

#include "storage/node.h"

static void internal_node_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num);

static void* page(Table* table, uint32_t page_num) {
    return pager_get_page(table->pager, page_num);
}

static uint32_t node_max_key(Table* table, void* node) {
    while (node_type(node) == NODE_INTERNAL) {
        node = page(table, internal_node_right_child(node));
    }
    return leaf_node_key(node, leaf_node_num_cells(node) - 1);
}

static bool node_is_full(void* node) {
    return node_type(node) == NODE_LEAF ? leaf_node_num_cells(node) >= LEAF_NODE_MAX_CELLS
                                        : internal_node_num_keys(node) >= INTERNAL_NODE_MAX_KEYS;
}

static uint32_t pages_needed_for_insert(const Cursor* cursor) {
    uint32_t pages_needed = 0;
    void* node = page(cursor->table, cursor->page_num);

    while (node_is_full(node)) {
        pages_needed++;
        if (node_is_root(node)) {
            return pages_needed + 1;
        }
        node = page(cursor->table, node_parent(node));
    }

    return pages_needed;
}

static void set_parent(Table* table, uint32_t child_page_num, uint32_t parent_page_num) {
    node_set_parent(page(table, child_page_num), parent_page_num);
}

static void refresh_child_key(Table* table, uint32_t parent_page_num, uint32_t child_page_num) {
    void* parent = page(table, parent_page_num);
    uint32_t index = internal_node_child_index(parent, child_page_num);
    if (index < internal_node_num_keys(parent)) {
        internal_node_set_key(parent, index, node_max_key(table, page(table, child_page_num)));
    }
}

static void create_new_root(Table* table, uint32_t right_child_page_num) {
    void* root = page(table, table->root_page_num);
    uint32_t left_child_page_num = pager_allocate_page(table->pager);
    void* left_child = page(table, left_child_page_num);

    memcpy(left_child, root, PAGE_SIZE);
    node_set_root(left_child, false);

    if (node_type(left_child) == NODE_INTERNAL) {
        uint32_t num_keys = internal_node_num_keys(left_child);
        for (uint32_t i = 0; i <= num_keys; i++) {
            set_parent(table, internal_node_child(left_child, i), left_child_page_num);
        }
    }

    internal_node_initialize(root);
    node_set_root(root, true);
    internal_node_set_num_keys(root, 1);
    internal_node_set_child(root, 0, left_child_page_num);
    internal_node_set_key(root, 0, node_max_key(table, left_child));
    internal_node_set_right_child(root, right_child_page_num);

    set_parent(table, left_child_page_num, table->root_page_num);
    set_parent(table, right_child_page_num, table->root_page_num);
}

static void internal_node_split_and_insert(Table* table, uint32_t old_page_num,
                                           uint32_t child_page_num) {
    void* old_node = page(table, old_page_num);
    uint32_t child_max = node_max_key(table, page(table, child_page_num));
    bool splitting_root = node_is_root(old_node);

    uint32_t new_page_num = pager_allocate_page(table->pager);
    internal_node_initialize(page(table, new_page_num));

    uint32_t parent_page_num;
    if (splitting_root) {
        create_new_root(table, new_page_num);
        parent_page_num = table->root_page_num;
        old_page_num = internal_node_child(page(table, parent_page_num), 0);
        old_node = page(table, old_page_num);
    } else {
        parent_page_num = node_parent(old_node);
    }

    internal_node_insert(table, new_page_num, internal_node_right_child(old_node));
    internal_node_set_right_child(old_node, INVALID_PAGE_NUM);

    uint32_t num_keys = internal_node_num_keys(old_node);
    for (uint32_t i = INTERNAL_NODE_MAX_KEYS - 1; i > INTERNAL_NODE_MAX_KEYS / 2; i--) {
        internal_node_insert(table, new_page_num, internal_node_child(old_node, i));
        internal_node_set_num_keys(old_node, --num_keys);
    }

    internal_node_set_right_child(old_node, internal_node_child(old_node, num_keys - 1));
    internal_node_set_num_keys(old_node, num_keys - 1);

    uint32_t old_max = node_max_key(table, old_node);
    uint32_t destination_page_num = child_max < old_max ? old_page_num : new_page_num;
    internal_node_insert(table, destination_page_num, child_page_num);

    refresh_child_key(table, parent_page_num, old_page_num);

    if (!splitting_root) {
        internal_node_insert(table, parent_page_num, new_page_num);
    }
}

static void internal_node_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num) {
    void* parent = page(table, parent_page_num);
    uint32_t num_keys = internal_node_num_keys(parent);

    if (num_keys >= INTERNAL_NODE_MAX_KEYS) {
        internal_node_split_and_insert(table, parent_page_num, child_page_num);
        return;
    }

    set_parent(table, child_page_num, parent_page_num);

    uint32_t right_child_page_num = internal_node_right_child(parent);
    if (right_child_page_num == INVALID_PAGE_NUM) {
        internal_node_set_right_child(parent, child_page_num);
        return;
    }

    uint32_t child_max = node_max_key(table, page(table, child_page_num));
    uint32_t right_max = node_max_key(table, page(table, right_child_page_num));
    uint32_t index = internal_node_find_child(parent, child_max);
    internal_node_set_num_keys(parent, num_keys + 1);

    if (child_max > right_max) {
        internal_node_set_child(parent, num_keys, right_child_page_num);
        internal_node_set_key(parent, num_keys, right_max);
        internal_node_set_right_child(parent, child_page_num);
        return;
    }

    memmove(internal_node_cell(parent, index + 1), internal_node_cell(parent, index),
            (num_keys - index) * INTERNAL_NODE_CELL_SIZE);
    internal_node_set_child(parent, index, child_page_num);
    internal_node_set_key(parent, index, child_max);
}

static void leaf_node_split_and_insert(Cursor* cursor, uint32_t key, const Row* value) {
    Table* table = cursor->table;
    uint32_t old_page_num = cursor->page_num;
    void* old_node = page(table, old_page_num);
    uint32_t new_page_num = pager_allocate_page(table->pager);
    void* new_node = page(table, new_page_num);

    leaf_node_initialize(new_node);
    node_set_parent(new_node, node_parent(old_node));
    leaf_node_set_next_leaf(new_node, leaf_node_next_leaf(old_node));
    leaf_node_set_next_leaf(old_node, new_page_num);

    for (uint32_t n = LEAF_NODE_MAX_CELLS + 1; n-- > 0;) {
        void* destination_node = n >= LEAF_NODE_LEFT_SPLIT_COUNT ? new_node : old_node;
        uint32_t index_within_node = n % LEAF_NODE_LEFT_SPLIT_COUNT;

        if (n == cursor->cell_num) {
            leaf_node_set_key(destination_node, index_within_node, key);
            row_serialize(value, leaf_node_value(destination_node, index_within_node));
        } else {
            uint32_t source_cell = n > cursor->cell_num ? n - 1 : n;
            memcpy(leaf_node_cell(destination_node, index_within_node),
                   leaf_node_cell(old_node, source_cell), LEAF_NODE_CELL_SIZE);
        }
    }

    leaf_node_set_num_cells(old_node, LEAF_NODE_LEFT_SPLIT_COUNT);
    leaf_node_set_num_cells(new_node, LEAF_NODE_RIGHT_SPLIT_COUNT);

    if (node_is_root(old_node)) {
        create_new_root(table, new_page_num);
        return;
    }

    uint32_t parent_page_num = node_parent(old_node);
    refresh_child_key(table, parent_page_num, old_page_num);
    internal_node_insert(table, parent_page_num, new_page_num);
}

bool btree_has_room_for_insert(const Cursor* cursor) {
    return pager_has_free_pages(cursor->table->pager, pages_needed_for_insert(cursor));
}

void btree_insert(Cursor* cursor, uint32_t key, const Row* value) {
    void* node = page(cursor->table, cursor->page_num);
    uint32_t num_cells = leaf_node_num_cells(node);

    if (num_cells >= LEAF_NODE_MAX_CELLS) {
        leaf_node_split_and_insert(cursor, key, value);
        return;
    }

    memmove(leaf_node_cell(node, cursor->cell_num + 1), leaf_node_cell(node, cursor->cell_num),
            (num_cells - cursor->cell_num) * LEAF_NODE_CELL_SIZE);
    leaf_node_set_num_cells(node, num_cells + 1);
    leaf_node_set_key(node, cursor->cell_num, key);
    row_serialize(value, leaf_node_value(node, cursor->cell_num));
}

static void indent(uint32_t level) {
    for (uint32_t i = 0; i < level; i++) {
        printf("  ");
    }
}

static void print_node(Table* table, uint32_t page_num, uint32_t level) {
    void* node = page(table, page_num);

    if (node_type(node) == NODE_LEAF) {
        uint32_t num_cells = leaf_node_num_cells(node);
        indent(level);
        printf("- leaf (size %u)\n", num_cells);
        for (uint32_t i = 0; i < num_cells; i++) {
            indent(level + 1);
            printf("- %u\n", leaf_node_key(node, i));
        }
        return;
    }

    uint32_t num_keys = internal_node_num_keys(node);
    indent(level);
    printf("- internal (size %u)\n", num_keys);
    if (num_keys == 0) {
        return;
    }
    for (uint32_t i = 0; i < num_keys; i++) {
        print_node(table, internal_node_child(node, i), level + 1);
        indent(level + 1);
        printf("- key %u\n", internal_node_key(node, i));
    }
    print_node(table, internal_node_right_child(node), level + 1);
}

void btree_print(Table* table) {
    print_node(table, table->root_page_num, 0);
}
