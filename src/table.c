#include "table.h"

const uint32_t NODE_TYPE_SIZE = sizeof(uint8_t);
const uint32_t NODE_TYPE_OFFSET = 0;
const uint32_t IS_ROOT_SIZE = sizeof(uint8_t);
const uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;
const uint32_t PARENT_POINTER_SIZE = sizeof(uint32_t);
const uint32_t PARENT_POINTER_OFFSET = IS_ROOT_OFFSET + IS_ROOT_SIZE;
const uint8_t COMMON_NODE_HEADER_SIZE =
    NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE;

const uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_NEXT_LEAF_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NEXT_LEAF_OFFSET =
    LEAF_NODE_NUM_CELLS_OFFSET + LEAF_NODE_NUM_CELLS_SIZE;
const uint32_t LEAF_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE +
                                       LEAF_NODE_NUM_CELLS_SIZE +
                                       LEAF_NODE_NEXT_LEAF_SIZE;

const uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_KEY_OFFSET = 0;
const uint32_t LEAF_NODE_VALUE_SIZE = ROW_SIZE;
const uint32_t LEAF_NODE_VALUE_OFFSET =
    LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE;
const uint32_t LEAF_NODE_CELL_SIZE = LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE;
const uint32_t LEAF_NODE_SPACE_FOR_CELLS = PAGE_SIZE - LEAF_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_MAX_CELLS =
    LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE;

const uint32_t LEAF_NODE_RIGHT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) / 2;
const uint32_t LEAF_NODE_LEFT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) - LEAF_NODE_RIGHT_SPLIT_COUNT;

const uint32_t INTERNAL_NODE_NUM_KEYS_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_NUM_KEYS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t INTERNAL_NODE_RIGHT_CHILD_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_RIGHT_CHILD_OFFSET =
    INTERNAL_NODE_NUM_KEYS_OFFSET + INTERNAL_NODE_NUM_KEYS_SIZE;
const uint32_t INTERNAL_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE +
                                           INTERNAL_NODE_NUM_KEYS_SIZE +
                                           INTERNAL_NODE_RIGHT_CHILD_SIZE;

const uint32_t INTERNAL_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_CHILD_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_CELL_SIZE = 
    INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE;

const uint32_t INTERNAL_NODE_MAX_CELLS = 3;

Table* db_open(const char *filename) {
    Pager* pager = pager_open(filename);

    Table* table = malloc(sizeof(Table));
    table->pager = pager;
    table->root_page_num = 0;

    if (pager->num_pages == 0) {
        // New database file. Initialize page 0 as leaf node.
        void* root_node = get_page(pager, 0);
        initialize_leaf_node(root_node);
        set_node_root(root_node, true);
    }

    return table;
}

Pager* pager_open(const char* filename) {
    int fd = open(
        filename,
        O_RDWR | O_CREAT,          // read/write, create if doesn't exist
        S_IWUSR | S_IRUSR          // user read + write permission (0600)
    );

    if (fd == -1) {
        printf("Unable to open file\n");
        exit(EXIT_FAILURE);
    }

    off_t file_length = lseek(fd, 0, SEEK_END);

    Pager* pager = malloc(sizeof(Pager));
    pager->file_descriptor = fd;
    pager->file_length = file_length;
    pager->num_pages = (file_length / PAGE_SIZE);

    if (file_length % PAGE_SIZE != 0) {
        printf("DB file is nto a whole number of pages. Corrupt file.\n");
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        pager->pages[i] = NULL;
    }

    return pager;
}

void* get_page(Pager* pager, uint32_t page_num) {
    if (page_num > TABLE_MAX_PAGES) {
        printf("Tried to fetch page number out of bounds. %d > %d\n", page_num, TABLE_MAX_PAGES);
        exit(EXIT_FAILURE);
    }

    if (pager->pages[page_num] == NULL) {
        // Cache miss. Allocate memory and load from file.
        void* page = malloc(PAGE_SIZE);
        uint32_t num_pages = pager->file_length / PAGE_SIZE;

        // We might save a partial page at the end of the file.
        if (pager->file_length % PAGE_SIZE) {
            num_pages += 1;
        }

        if (page_num <= num_pages) {
            lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
            ssize_t bytes_read = read(pager->file_descriptor, page, PAGE_SIZE);
            if (bytes_read == -1) {
                printf("Error reading file: %d\n", errno);
                exit(EXIT_FAILURE);
            }
        }

        pager->pages[page_num] = page;

        if (page_num >= pager->num_pages) {
            pager->num_pages = page_num + 1;
        }
    }

    return pager->pages[page_num];
}

void db_close(Table *table) {
    Pager* pager = table->pager;

    for (uint32_t i = 0; i < pager->num_pages; i++) {
        if (pager->pages[i] == NULL) {
            continue;
        }

        pager_flush(pager, i);
        free(pager->pages[i]);
        pager->pages[i] = NULL;
    }

    int result = close(pager->file_descriptor);
    if (result == -1) {
        printf("Error closing db file.\n");
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        void* page = pager->pages[i];
        if (page) {
            free(page);
            pager->pages[i] = NULL;
        }
    }

    free(pager);
    free(table);
}

void pager_flush(Pager* pager, uint32_t page_num) {
    if (pager->pages[page_num] == NULL) {
        printf("Tried to flush null page\n");
        exit(EXIT_FAILURE);
    }

    off_t offset = lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);

    if (offset == -1) {
        printf("Error seeking: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_written = 
        write(pager->file_descriptor, pager->pages[page_num], PAGE_SIZE);

    if (bytes_written == -1) {
        printf("Error writing: %d\n", errno);
        exit(EXIT_FAILURE);
    }
}

void* cursor_value(Cursor* cursor) {
    uint32_t page_num = cursor->page_num;
    void* page = get_page(cursor->table->pager, page_num);
    return leaf_node_value(page, cursor->cell_num);
}

Cursor* table_start(Table* table) {
    Cursor* cursor = table_find(table, 0);

    void* node = get_page(table->pager, cursor->page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);
    cursor->end_of_table = (num_cells == 0);

    return cursor;
}

Cursor* table_find(Table* table, uint32_t key) {
    uint32_t root_page_num = table->root_page_num;
    void* root_node = get_page(table->pager, root_page_num);

    if (get_node_type(root_node) == NODE_LEAF) {
        return leaf_node_find(table, root_page_num, key);
    } else {
        return internal_node_find(table, root_page_num, key);
    }
}

Cursor* leaf_node_find(Table* table, uint32_t page_num, uint32_t key) {
    void* node = get_page(table->pager, page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);

    Cursor* cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->page_num = page_num;

    // Binary search
    uint32_t min_index = 0;
    uint32_t one_past_max_index = num_cells;
    while (one_past_max_index != min_index) {
        uint32_t index = (min_index + one_past_max_index) / 2;
        uint32_t key_at_index = *leaf_node_key(node, index);

        if (key == key_at_index) {
            cursor->cell_num = index;
            return cursor;
        }

        if (key < key_at_index) {
            one_past_max_index = index;
        } else {
            min_index = index + 1;
        }
    }

    cursor->cell_num = min_index;
    return cursor;
}

void cursor_advance(Cursor* cursor) {
    int32_t page_num = cursor->page_num;
    void* node = get_page(cursor->table->pager, page_num);

    cursor->cell_num += 1;
    if (cursor->cell_num >= (*leaf_node_num_cells(node))) {
        // Advance to next leaf node
        uint32_t next_page_num = *leaf_node_next_leaf(node);
        if (next_page_num == 0) {
            // This was the rightmost leaf
            cursor->end_of_table = true;
        } else {
            cursor->page_num = next_page_num;
            cursor->cell_num = 0;
        }
    }
}

uint32_t* leaf_node_num_cells(void* node) {
    return node + LEAF_NODE_NUM_CELLS_OFFSET;
}

void* leaf_node_cell(void* node, uint32_t cell_num) {
    return node + LEAF_NODE_HEADER_SIZE + cell_num * LEAF_NODE_CELL_SIZE;
}

uint32_t* leaf_node_key(void* node, uint32_t cell_num) {
    return leaf_node_cell(node, cell_num);
}

void* leaf_node_value(void* node, uint32_t cell_num) {
    return leaf_node_cell(node, cell_num) + LEAF_NODE_KEY_SIZE;
}

void initialize_leaf_node(void* node) {
    set_node_type(node, NODE_LEAF);
    set_node_root(node, false);
    *leaf_node_num_cells(node) = 0;
    *leaf_node_next_leaf(node) = 0;
}

void leaf_node_insert(Cursor* cursor, uint32_t key, Row* value) {
    void* node = get_page(cursor->table->pager, cursor->page_num);

    uint32_t num_cells = *leaf_node_num_cells(node);
    if (num_cells >= LEAF_NODE_MAX_CELLS) {
        // Node full
        leaf_node_split_and_insert(cursor, key, value);
        return;
    }

    if (cursor->cell_num < num_cells) {
        // Make room for new call
        for (uint32_t i = num_cells; i > cursor->cell_num; i--) {
            memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i - 1), LEAF_NODE_CELL_SIZE);
        }
    }

    *(leaf_node_num_cells(node)) += 1;
    *(leaf_node_key(node, cursor->cell_num)) = key;
    serialize_row(value, leaf_node_value(node, cursor->cell_num));
}

NodeType get_node_type(void* node) {
    uint8_t value = *((uint8_t*)(node + NODE_TYPE_OFFSET));
    return (NodeType)value;
}

void set_node_type(void *node, NodeType type) {
    uint8_t value = type;
    *((uint8_t*)(node + NODE_TYPE_OFFSET)) = value;
}

uint32_t* node_parent(void* node) {
    return node + PARENT_POINTER_OFFSET;
}

void leaf_node_split_and_insert(Cursor* cursor, uint32_t key, Row* value) {
    /*
    Create a new node and move half the cells over.
    Insert the new value and in one of the two nodes.
    Update parent or create a new parent.
    */
   void* old_node = get_page(cursor->table->pager, cursor->page_num);
   uint32_t old_max = get_node_max_key(cursor->table->pager, old_node);
   uint32_t new_page_num = get_unused_page_num(cursor->table->pager);
   void* new_node = get_page(cursor->table->pager, new_page_num);
   initialize_leaf_node(new_node);
   *node_parent(new_node) = *node_parent(old_node);
   *leaf_node_next_leaf(new_node) = *leaf_node_next_leaf(old_node);
   *leaf_node_next_leaf(old_node) = new_page_num;

  /*
   All existing keys plus new key should be divided 
   evenly between old (left) and new (right) nodes.
   Starting from the right, move each key to correct position.
  */
  for (int32_t i = LEAF_NODE_MAX_CELLS; i >= 0; i--) {
    void* destination_node;
    if (i >= LEAF_NODE_LEFT_SPLIT_COUNT) {
        destination_node = new_node;
    } else {
        destination_node = old_node;
    }

    uint32_t index_within_node = i % LEAF_NODE_LEFT_SPLIT_COUNT;
    void* destination = leaf_node_cell(destination_node, index_within_node);

    if (i == cursor->cell_num) {
        serialize_row(value, leaf_node_value(destination_node, index_within_node));
        *leaf_node_key(destination_node, index_within_node) = key;
    } else if (i > cursor->cell_num) {
        memcpy(destination, leaf_node_cell(old_node, i -1), LEAF_NODE_CELL_SIZE);
    } else {
        memcpy(destination, leaf_node_cell(old_node, i), LEAF_NODE_CELL_SIZE);
    }
  }

  // Update cell count on both leaf nodes
  *(leaf_node_num_cells(old_node)) = LEAF_NODE_LEFT_SPLIT_COUNT;
  *(leaf_node_num_cells(new_node)) = LEAF_NODE_RIGHT_SPLIT_COUNT;

  if (is_root_node(old_node)) {
    return create_new_root(cursor->table, new_page_num);
  } else {
    uint32_t parent_page_num = *node_parent(old_node);
    uint32_t new_max = get_node_max_key(cursor->table->pager,old_node);
    void* parent = get_page(cursor->table->pager, parent_page_num);

    update_internal_node_key(parent, old_max, new_max);
    internal_node_insert(cursor->table, parent_page_num, new_page_num);
    return;
  }
}

bool is_root_node(void* node) {
    uint8_t value = *((uint8_t*)(node + IS_ROOT_OFFSET));
    return (bool)value;
}

void set_node_root(void* node, bool is_root) {
    uint8_t value = is_root;
    *((uint8_t*)(node + IS_ROOT_OFFSET)) = value;
}

/*
Until we start recycling free pages, new pages will always go onto the end of the database file.
*/
uint32_t get_unused_page_num(Pager* pager) {
    if (pager->num_pages >= TABLE_MAX_PAGES) {
        printf("Need to implement splitting internal node\n");
        exit(EXIT_FAILURE);
    }
    return pager->num_pages;
}

void create_new_root(Table* table, uint32_t right_child_page_num) {
    /*
    Handle splitting the root.
    Old root copied to new page, becomes left child.
    Address of right child passed in.
    Re-initialize root page to contain the new root node.
    New root node points to two children.
    */

    void* root = get_page(table->pager, table->root_page_num);
    void* right_child = get_page(table->pager, right_child_page_num);
    uint32_t left_child_page_num = get_unused_page_num(table->pager);
    void* left_child = get_page(table->pager, left_child_page_num);

    if (get_node_type(root) == NODE_INTERNAL) {
        initialize_internal_node(right_child);
        initialize_internal_node(left_child);
    }

    // Left child has data copied from old root.
    memcpy(left_child, root, PAGE_SIZE);
    set_node_root(left_child, false);

    if (get_node_type(left_child) == NODE_INTERNAL) {
        void* child;
        for (int i = 0; i < *internal_node_num_keys(left_child); i++) {
            child = get_page(table->pager, *internal_node_child(left_child, i));
            *node_parent(child) = left_child_page_num;
        }
        child = get_page(table->pager, *internal_node_right_child(left_child));
        *node_parent(child) = left_child_page_num;
    }

    // Root node is a new internal node with one key and two children
    initialize_internal_node(root);
    set_node_root(root, true);

    *internal_node_num_keys(root) = 1;
    *internal_node_child(root, 0) = left_child_page_num;

    uint32_t left_child_max_key = get_node_max_key(table->pager, left_child);
    *internal_node_key(root, 0) = left_child_max_key;
    *internal_node_right_child(root) = right_child_page_num;

    *node_parent(left_child) = table->root_page_num;
    *node_parent(right_child) = table->root_page_num;
}

uint32_t* internal_node_num_keys(void* node) {
    return node + INTERNAL_NODE_NUM_KEYS_OFFSET;
}

uint32_t* internal_node_right_child(void* node) {
    return node + INTERNAL_NODE_RIGHT_CHILD_OFFSET;
}

uint32_t* internal_node_cell(void* node, uint32_t cell_num) {
    return node + INTERNAL_NODE_HEADER_SIZE + cell_num * INTERNAL_NODE_CELL_SIZE;
}

uint32_t* internal_node_child(void* node, uint32_t child_num) {
    uint32_t num_keys = *internal_node_num_keys(node);
    if (child_num > num_keys) {
        printf("Tried to access child_num %d > num_keys %d\n", child_num, num_keys);
        exit(EXIT_FAILURE);
    } else if (child_num == num_keys) {
        uint32_t* right_child = internal_node_right_child(node);
        if (*right_child == INVALID_PAGE_NUM) {
            printf("Tried to access right child of node, but was invalid page\n");
            exit(EXIT_FAILURE);
        }

        return right_child;
    } else {
        uint32_t* child = internal_node_cell(node, child_num);
        if (*child == INVALID_PAGE_NUM) {
            printf("Tried to access child %d of node, but was invalid page\n", child_num);
            exit(EXIT_FAILURE);
        }

        return child;
    }
}

uint32_t* internal_node_key(void* node, uint32_t key_num) {
    return (void*)internal_node_cell(node, key_num) + INTERNAL_NODE_CHILD_SIZE;
}

uint32_t get_node_max_key(Pager* pager, void* node) {
    if (get_node_type(node) == NODE_LEAF) {
        return *leaf_node_key(node, *leaf_node_num_cells(node) - 1);
    }

    void * right_child = get_page(pager, *internal_node_right_child(node));
    return get_node_max_key(pager, right_child);
}

void initialize_internal_node(void* node) {
    set_node_type(node, NODE_INTERNAL);
    set_node_root(node, false);
    *leaf_node_num_cells(node) = 0;
    *leaf_node_next_leaf(node) = 0;
    /*
    Necessary because the root page number is 0; by not initializing an internal node's right
    child to an invalid page number when initializing the node, we may
    end up with 0 as the node's right child, which makes the node a parent of the root.
    */
   *internal_node_right_child(node) = INVALID_PAGE_NUM;
}

uint32_t* leaf_node_next_leaf(void* node) {
    return node + LEAF_NODE_NEXT_LEAF_OFFSET;
}

Cursor* internal_node_find(Table* table, uint32_t page_num, uint32_t key) {
    void* node = get_page(table->pager, page_num);
    uint32_t num_keys = *internal_node_num_keys(node);

    // BInary search to find index of child to search
    uint32_t min_index = 0;
    uint32_t max_index = num_keys;

    while (min_index != max_index) {
        uint32_t index = (min_index + max_index) / 2;
        uint32_t key_to_right = *internal_node_key(node, index);
        if (key_to_right >= key) {
            max_index = index;
        } else {
            min_index = index + 1;
        }
    }

    uint32_t child_num = *internal_node_child(node, min_index);
    void* child = get_page(table->pager, child_num);
    switch (get_node_type(child)) {
        case NODE_LEAF:
            return leaf_node_find(table, child_num, key);
        case NODE_INTERNAL:
            return internal_node_find(table, child_num, key);
        default:
            printf("Unexpected node type\n");
            exit(EXIT_FAILURE);
    }
}

void update_internal_node_key(void* node, uint32_t old_key, uint32_t new_key) {
    uint32_t old_child_index = internal_node_find_child(node, old_key);
    *internal_node_key(node, old_child_index) = new_key;
}

uint32_t internal_node_find_child(void* node, uint32_t key) {
    /*
    Return the index of the child which should contain
    the given key.
    */

    uint32_t num_keys = *internal_node_num_keys(node);
    /*Binary search*/
    uint32_t min_index = 0;
    uint32_t max_index = num_keys; /*there is one more child than key*/

    while (min_index != max_index) {
        uint32_t index = (min_index + max_index) / 2;
        uint32_t key_to_right = *internal_node_key(node, index);
        if (key_to_right >= key) {
            max_index = index;
        } else {
            min_index = index + 1;
        }
    }

    return min_index;
}

void internal_node_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num) {
    /*
    Add a new child/key pair to parent that corresponds to child
    */
    
    void* parent = get_page(table->pager, parent_page_num);
    void* child = get_page(table->pager, child_page_num);
    uint32_t child_max_key = get_node_max_key(table->pager, child);
    uint32_t index = internal_node_find_child(parent, child_max_key);

    uint32_t original_num_keys = *internal_node_num_keys(parent);

    if (original_num_keys >= INTERNAL_NODE_MAX_CELLS) {
        internal_node_split_and_insert(table, parent_page_num, child_page_num);
        return;
    }

    uint32_t right_child_page_num = *internal_node_right_child(parent);

    /*
    An internal node with a right child of INVALID_PAGE_NUM is empty
    */
    if (right_child_page_num == INVALID_PAGE_NUM) {
        *internal_node_right_child(parent) = child_page_num;
        return;
    }

    void* right_child = get_page(table->pager, right_child_page_num);

    /*
    If we are already at the max number of cells for a node, we cannot increment
    before splitting. Incrementing without inserting a new key/child pair
    and immediately calling internal_node_split_and_insert has the effect
    of creating a new key at (max_cells + 1) with an uninitialized value
    */
   *internal_node_num_keys(parent) = original_num_keys + 1;

    if (child_max_key > get_node_max_key(table->pager, right_child)) {
        /*Replace right child*/
        *internal_node_child(parent, original_num_keys) = right_child_page_num;
        *internal_node_key(parent, original_num_keys) = get_node_max_key(table->pager, right_child);
        *internal_node_right_child(parent) = child_page_num;
    } else {
        // Make room for the new cell
        for (uint32_t i = original_num_keys; i > index; i--) {
            void* destination = internal_node_cell(parent, i);
            void* source = internal_node_cell(parent, i - 1);
            memcpy(destination, source, INTERNAL_NODE_CELL_SIZE);
        }
        *internal_node_child(parent, index) = child_page_num;
        *internal_node_key(parent, index) = child_max_key;
    }
}

void internal_node_split_and_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num) {
    uint32_t old_page_num = parent_page_num;
    void* old_node = get_page(table->pager, parent_page_num);
    uint32_t old_max = get_node_max_key(table->pager, old_node);

    void* child = get_page(table->pager, child_page_num);
    uint32_t child_max = get_node_max_key(table->pager, child);

    uint32_t new_page_num = get_unused_page_num(table->pager);

    /*
    Declaring a flag before updating pointers which
    records whether this operation involves splitting the root -
    if it does, we will insert our newly created node during
    the step where the table's new root is created. If it does 
    not, we have to insert the newly created node into its parent
    after the old nodeś keys have been transferred over. We are not
    able to do this if the newly created node's parent is not a newly
    initialized root node, because in that case its parent may have existing
    keys aside from our old node which we are splitting. If that is true, we need
    to find a place for our newly created_node in its parent, and we cannot insert it at the correct index
    if it does not yet have any keys.
    */
   uint32_t splitting_root = is_node_root(old_node);

   void* parent;
   void* new_node;

   if (splitting_root) {
    create_new_root(table, new_page_num);
    /*
    After create_new_root, the new root is at table->root_page_num.
    old_node becomes the new root's left child (which holds the old keys).
    new_page_num is already installed as the right child of the new root.
    */
    parent = get_page(table->pager, table->root_page_num);
    old_page_num = *internal_node_child(parent, 0);
    old_node = get_page(table->pager, old_page_num);
   } else {
    parent = get_page(table->pager, *node_parent(old_node));
    new_node = get_page(table->pager, new_page_num);
    initialize_internal_node(new_node);
   }

   uint32_t* old_num_keys = internal_node_num_keys(old_node);
   uint32_t cur_page_num = *internal_node_right_child(old_node);

   void* cur = get_page(table->pager, cur_page_num);

   /*
   First put right child into new node and set right child of old node to invalid page number
   */
   internal_node_insert(table, new_page_num, cur_page_num);
   *node_parent(cur) = new_page_num;
   *internal_node_right_child(old_node) = INVALID_PAGE_NUM;

   /*
   For each key until you get to the middle key, mvoe the key and the child to the new node
   */
   for (int i = INTERNAL_NODE_MAX_CELLS - 1; i > INTERNAL_NODE_MAX_CELLS / 2; i--) {
    cur_page_num = *internal_node_child(old_node, i);
    cur = get_page(table->pager, cur_page_num);

    internal_node_insert(table, new_page_num, cur_page_num);
    *node_parent(cur) = new_page_num;

   (*old_num_keys)--;
   }

   /*
   After the loop, old_node's right_child is INVALID_PAGE_NUM but it still has
   num_keys cells. The last cell's child must be promoted to be old_node's right
   child, and num_keys decremented — otherwise get_node_max_key follows INVALID_PAGE_NUM.
   */
   *internal_node_right_child(old_node) = *internal_node_cell(old_node, *old_num_keys - 1);
   (*old_num_keys)--;

   /*
   Determine which of the two nodes after the split should contain the child to be inserted,
   and insert the child
   */
   uint32_t max_after_split = get_node_max_key(table->pager, old_node);

   uint32_t destination_page_num = child_max < max_after_split ? old_page_num : new_page_num;

   internal_node_insert(table, destination_page_num, child_page_num);
   *node_parent(child) = destination_page_num;

   update_internal_node_key(parent, old_max, get_node_max_key(table->pager, old_node));

   if (!splitting_root) {
    internal_node_insert(table, *node_parent(old_node), new_page_num);
    *node_parent(new_node) = *node_parent(old_node);
   }
}

bool is_node_root(void* node) {
    uint8_t value = *((uint8_t*)(node + IS_ROOT_OFFSET));
    return (bool)value;
}
