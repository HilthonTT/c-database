# File format

A database file is a flat array of 4096-byte pages. Page `N` starts at byte offset `N × 4096`. The file
length must be a whole number of pages; otherwise opening it fails with a corrupt-file error.

| Constant           | Value        |
| ------------------ | ------------ |
| `PAGE_SIZE`        | 4096 bytes   |
| `TABLE_MAX_PAGES`  | 100          |
| `INVALID_PAGE_NUM` | `0xFFFFFFFF` |

Page 0 is always the root node. Every page is a B+ tree node: either a **leaf** or an **internal**
node. All integers are unsigned and stored in the host's byte order (little-endian on x86-64 and
ARM64). Fields are packed with no alignment padding.

You can print the layout constants from a running database with `.constants`.

## Common node header (6 bytes)

| Offset | Size | Field        | Notes                                      |
| -----: | ---: | ------------ | ------------------------------------------ |
| 0      | 1    | `node_type`  | `0` = internal, `1` = leaf                 |
| 1      | 1    | `is_root`    | `1` for page 0, `0` otherwise              |
| 2      | 4    | `parent`     | Page number of the parent; unused for the root |

## Leaf node

### Header (14 bytes)

| Offset | Size | Field        | Notes                                                   |
| -----: | ---: | ------------ | ------------------------------------------------------- |
| 0      | 6    | common header |                                                        |
| 6      | 4    | `num_cells`  | Number of key/value cells                               |
| 10     | 4    | `next_leaf`  | Page number of the right sibling, `0` for the last leaf |

### Body

`num_cells` cells start at offset 14, sorted by key. Each cell is 297 bytes:

| Offset in cell | Size | Field   |
| -------------: | ---: | ------- |
| 0              | 4    | `key` (the row's `id`) |
| 4              | 293  | `value` (encoded row, see below) |

```text
LEAF_NODE_SPACE_FOR_CELLS   = 4096 − 14   = 4082
LEAF_NODE_MAX_CELLS         = 4082 / 297  = 13
LEAF_NODE_LEFT_SPLIT_COUNT  = 7
LEAF_NODE_RIGHT_SPLIT_COUNT = 7
```

The 11 bytes left over at the end of each leaf page are unused.

## Internal node

### Header (14 bytes)

| Offset | Size | Field         | Notes                                                    |
| -----: | ---: | ------------- | -------------------------------------------------------- |
| 0      | 6    | common header |                                                          |
| 6      | 4    | `num_keys`    | Number of (child, key) cells                             |
| 10     | 4    | `right_child` | Page of the right-most child, `INVALID_PAGE_NUM` if empty |

### Body

`num_keys` cells start at offset 14. Each cell is 8 bytes:

| Offset in cell | Size | Field   | Notes |
| -------------: | ---: | ------- | ----- |
| 0              | 4    | `child` | Page number of the child |
| 4              | 4    | `key`   | Largest key in that child's subtree |

Keys are in ascending order. To look up key `k`, take the first cell whose `key ≥ k`. If there is no
such cell, follow `right_child`.

`INTERNAL_NODE_MAX_KEYS` is set to **3**. That is far below what a page could hold (510 cells), and
it is deliberate: trees grow several levels deep with only a few dozen rows, which keeps splits easy
to test and to look at with `.btree`.

## Row encoding (293 bytes)

| Offset | Size | Field      | Notes                                       |
| -----: | ---: | ---------- | ------------------------------------------- |
| 0      | 4    | `id`       | `uint32`                                    |
| 4      | 33   | `username` | Up to 32 bytes, NUL-padded                  |
| 37     | 256  | `email`    | Up to 255 bytes, NUL-padded                 |

## Example

Inserting ids 1 through 14 into an empty database fills the root leaf and then splits it:

```text
db > .btree
Tree:
- internal (size 1)
  - leaf (size 7)
    - 1
    ...
    - 7
  - key 7
  - leaf (size 7)
    - 8
    ...
    - 14
```

The file now has three pages:

| Page | Contents |
| ---: | -------- |
| 0    | Internal root: `num_keys = 1`, cell `(child = 2, key = 7)`, `right_child = 1` |
| 1    | Leaf holding keys 8–14, `next_leaf = 0` |
| 2    | Leaf holding keys 1–7, `next_leaf = 1` (copy of the old root) |
