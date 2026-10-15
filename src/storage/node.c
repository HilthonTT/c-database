#include "storage/node.h"

#include <string.h>

#include "common/fatal.h"

static uint32_t read_u32(const void* node, uint32_t offset) {
    uint32_t value;
    memcpy(&value, (const uint8_t*)node + offset, sizeof(value));
    return value;
}

static void write_u32(void* node, uint32_t offset, uint32_t value) {
    memcpy((uint8_t*)node + offset, &value, sizeof(value));
}

static uint32_t leaf_cell_offset(uint32_t cell_num) {
    return LEAF_NODE_HEADER_SIZE + cell_num * LEAF_NODE_CELL_SIZE;
}

static uint32_t internal_cell_offset(uint32_t cell_num) {
    return INTERNAL_NODE_HEADER_SIZE + cell_num * INTERNAL_NODE_CELL_SIZE;
}

NodeType node_type(const void* node) {
    return (NodeType)((const uint8_t*)node)[NODE_TYPE_OFFSET];
}

void node_set_type(void* node, NodeType type) {
    ((uint8_t*)node)[NODE_TYPE_OFFSET] = (uint8_t)type;
}

bool node_is_root(const void* node) {
    return ((const uint8_t*)node)[IS_ROOT_OFFSET] != 0;
}

void node_set_root(void* node, bool is_root) {
    ((uint8_t*)node)[IS_ROOT_OFFSET] = (uint8_t)is_root;
}

uint32_t node_parent(const void* node) {
    return read_u32(node, PARENT_POINTER_OFFSET);
}

void node_set_parent(void* node, uint32_t parent_page_num) {
    write_u32(node, PARENT_POINTER_OFFSET, parent_page_num);
}

void leaf_node_initialize(void* node) {
    node_set_type(node, NODE_LEAF);
    node_set_root(node, false);
    node_set_parent(node, INVALID_PAGE_NUM);
    leaf_node_set_num_cells(node, 0);
    leaf_node_set_next_leaf(node, 0);
}

uint32_t leaf_node_num_cells(const void* node) {
    return read_u32(node, LEAF_NODE_NUM_CELLS_OFFSET);
}

void leaf_node_set_num_cells(void* node, uint32_t num_cells) {
    write_u32(node, LEAF_NODE_NUM_CELLS_OFFSET, num_cells);
}

uint32_t leaf_node_next_leaf(const void* node) {
    return read_u32(node, LEAF_NODE_NEXT_LEAF_OFFSET);
}

void leaf_node_set_next_leaf(void* node, uint32_t page_num) {
    write_u32(node, LEAF_NODE_NEXT_LEAF_OFFSET, page_num);
}

void* leaf_node_cell(void* node, uint32_t cell_num) {
    return (uint8_t*)node + leaf_cell_offset(cell_num);
}

uint32_t leaf_node_key(const void* node, uint32_t cell_num) {
    return read_u32(node, leaf_cell_offset(cell_num) + LEAF_NODE_KEY_OFFSET);
}

void leaf_node_set_key(void* node, uint32_t cell_num, uint32_t key) {
    write_u32(node, leaf_cell_offset(cell_num) + LEAF_NODE_KEY_OFFSET, key);
}

void* leaf_node_value(void* node, uint32_t cell_num) {
    return (uint8_t*)node + leaf_cell_offset(cell_num) + LEAF_NODE_VALUE_OFFSET;
}

void internal_node_initialize(void* node) {
    node_set_type(node, NODE_INTERNAL);
    node_set_root(node, false);
    node_set_parent(node, INVALID_PAGE_NUM);
    internal_node_set_num_keys(node, 0);
    internal_node_set_right_child(node, INVALID_PAGE_NUM);
}

uint32_t internal_node_num_keys(const void* node) {
    return read_u32(node, INTERNAL_NODE_NUM_KEYS_OFFSET);
}

void internal_node_set_num_keys(void* node, uint32_t num_keys) {
    write_u32(node, INTERNAL_NODE_NUM_KEYS_OFFSET, num_keys);
}

uint32_t internal_node_right_child(const void* node) {
    return read_u32(node, INTERNAL_NODE_RIGHT_CHILD_OFFSET);
}

void internal_node_set_right_child(void* node, uint32_t page_num) {
    write_u32(node, INTERNAL_NODE_RIGHT_CHILD_OFFSET, page_num);
}

void* internal_node_cell(void* node, uint32_t cell_num) {
    return (uint8_t*)node + internal_cell_offset(cell_num);
}

uint32_t internal_node_child(const void* node, uint32_t child_num) {
    uint32_t num_keys = internal_node_num_keys(node);
    if (child_num > num_keys) {
        fatal("child %u out of range for internal node with %u keys", child_num, num_keys);
    }

    uint32_t child = child_num == num_keys ? internal_node_right_child(node)
                                           : read_u32(node, internal_cell_offset(child_num));
    if (child == INVALID_PAGE_NUM) {
        fatal("child %u of internal node is an invalid page", child_num);
    }
    return child;
}

void internal_node_set_child(void* node, uint32_t child_num, uint32_t page_num) {
    write_u32(node, internal_cell_offset(child_num), page_num);
}

uint32_t internal_node_key(const void* node, uint32_t key_num) {
    return read_u32(node, internal_cell_offset(key_num) + INTERNAL_NODE_CHILD_SIZE);
}

void internal_node_set_key(void* node, uint32_t key_num, uint32_t key) {
    write_u32(node, internal_cell_offset(key_num) + INTERNAL_NODE_CHILD_SIZE, key);
}

uint32_t internal_node_find_child(const void* node, uint32_t key) {
    uint32_t min_index = 0;
    uint32_t max_index = internal_node_num_keys(node);

    while (min_index != max_index) {
        uint32_t index = min_index + (max_index - min_index) / 2;
        if (internal_node_key(node, index) >= key) {
            max_index = index;
        } else {
            min_index = index + 1;
        }
    }

    return min_index;
}

uint32_t internal_node_child_index(const void* node, uint32_t child_page_num) {
    uint32_t num_keys = internal_node_num_keys(node);
    for (uint32_t i = 0; i < num_keys; i++) {
        if (read_u32(node, internal_cell_offset(i)) == child_page_num) {
            return i;
        }
    }
    if (internal_node_right_child(node) == child_page_num) {
        return num_keys;
    }
    fatal("page %u is not a child of this internal node", child_page_num);
}
