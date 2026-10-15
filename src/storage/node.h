#ifndef STORAGE_NODE_H
#define STORAGE_NODE_H

#include <stdbool.h>
#include <stdint.h>

#include "storage/pager.h"
#include "storage/row.h"

typedef enum {
    NODE_INTERNAL,
    NODE_LEAF,
} NodeType;

enum {
    NODE_TYPE_SIZE = sizeof(uint8_t),
    NODE_TYPE_OFFSET = 0,
    IS_ROOT_SIZE = sizeof(uint8_t),
    IS_ROOT_OFFSET = NODE_TYPE_OFFSET + NODE_TYPE_SIZE,
    PARENT_POINTER_SIZE = sizeof(uint32_t),
    PARENT_POINTER_OFFSET = IS_ROOT_OFFSET + IS_ROOT_SIZE,
    COMMON_NODE_HEADER_SIZE = NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE,
};

enum {
    LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t),
    LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE,
    LEAF_NODE_NEXT_LEAF_SIZE = sizeof(uint32_t),
    LEAF_NODE_NEXT_LEAF_OFFSET = LEAF_NODE_NUM_CELLS_OFFSET + LEAF_NODE_NUM_CELLS_SIZE,
    LEAF_NODE_HEADER_SIZE =
        COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE + LEAF_NODE_NEXT_LEAF_SIZE,

    LEAF_NODE_KEY_SIZE = sizeof(uint32_t),
    LEAF_NODE_KEY_OFFSET = 0,
    LEAF_NODE_VALUE_SIZE = ROW_SIZE,
    LEAF_NODE_VALUE_OFFSET = LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE,
    LEAF_NODE_CELL_SIZE = LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE,
    LEAF_NODE_SPACE_FOR_CELLS = PAGE_SIZE - LEAF_NODE_HEADER_SIZE,
    LEAF_NODE_MAX_CELLS = LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE,
    LEAF_NODE_RIGHT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) / 2,
    LEAF_NODE_LEFT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) - LEAF_NODE_RIGHT_SPLIT_COUNT,
};

enum {
    INTERNAL_NODE_NUM_KEYS_SIZE = sizeof(uint32_t),
    INTERNAL_NODE_NUM_KEYS_OFFSET = COMMON_NODE_HEADER_SIZE,
    INTERNAL_NODE_RIGHT_CHILD_SIZE = sizeof(uint32_t),
    INTERNAL_NODE_RIGHT_CHILD_OFFSET = INTERNAL_NODE_NUM_KEYS_OFFSET + INTERNAL_NODE_NUM_KEYS_SIZE,
    INTERNAL_NODE_HEADER_SIZE =
        COMMON_NODE_HEADER_SIZE + INTERNAL_NODE_NUM_KEYS_SIZE + INTERNAL_NODE_RIGHT_CHILD_SIZE,

    INTERNAL_NODE_CHILD_SIZE = sizeof(uint32_t),
    INTERNAL_NODE_KEY_SIZE = sizeof(uint32_t),
    INTERNAL_NODE_CELL_SIZE = INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE,
    INTERNAL_NODE_MAX_KEYS = 3,
};

NodeType node_type(const void* node);
void node_set_type(void* node, NodeType type);
bool node_is_root(const void* node);
void node_set_root(void* node, bool is_root);
uint32_t node_parent(const void* node);
void node_set_parent(void* node, uint32_t parent_page_num);

void leaf_node_initialize(void* node);
uint32_t leaf_node_num_cells(const void* node);
void leaf_node_set_num_cells(void* node, uint32_t num_cells);
uint32_t leaf_node_next_leaf(const void* node);
void leaf_node_set_next_leaf(void* node, uint32_t page_num);
void* leaf_node_cell(void* node, uint32_t cell_num);
uint32_t leaf_node_key(const void* node, uint32_t cell_num);
void leaf_node_set_key(void* node, uint32_t cell_num, uint32_t key);
void* leaf_node_value(void* node, uint32_t cell_num);

void internal_node_initialize(void* node);
uint32_t internal_node_num_keys(const void* node);
void internal_node_set_num_keys(void* node, uint32_t num_keys);
uint32_t internal_node_right_child(const void* node);
void internal_node_set_right_child(void* node, uint32_t page_num);
void* internal_node_cell(void* node, uint32_t cell_num);
uint32_t internal_node_child(const void* node, uint32_t child_num);
void internal_node_set_child(void* node, uint32_t child_num, uint32_t page_num);
uint32_t internal_node_key(const void* node, uint32_t key_num);
void internal_node_set_key(void* node, uint32_t key_num, uint32_t key);
uint32_t internal_node_find_child(const void* node, uint32_t key);
uint32_t internal_node_child_index(const void* node, uint32_t child_page_num);

#endif
