# Development

## Prerequisites

| Tool              | Used for                          |
| ----------------- | --------------------------------- |
| `gcc` or `clang`  | Building (C11, POSIX.1-2008)      |
| `make`            | Build orchestration               |
| Ruby + Bundler    | Running the RSpec test suite      |
| `clang-format`    | Optional, for `make format`       |

## Build

```sh
make            # debug build: -g -O0
make release    # optimised build: -O2 -DNDEBUG
make sanitize   # ASan + UBSan build, recommended while changing storage code
make clean
```

Objects go into `build/`, laid out like `src/`. Header dependencies are tracked automatically
(`-MMD -MP`), so editing a header rebuilds every file that includes it.

The build uses `-std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes
-Wmissing-prototypes`, and the tree compiles with no warnings. Keep it that way.

## Tests

The suite in `spec/main_spec.rb` runs end to end: it starts `./db test.db`, sends commands to its
standard input, and checks standard output line by line.

```sh
bundle install   # once
make test
```

To run a single example:

```sh
bundle exec rspec spec/main_spec.rb -e "duplicate"
```

Run the suite against a `make sanitize` build as well. Any sanitizer report goes to `stderr` and
fails the process.

## Coding conventions

- **Style**: `clang-format` with the repository's `.clang-format` (Google style, 4-space indent,
  100 columns, `Type* name`). Run `make format` before committing.
- **Layering**: `cli → sql → storage → common`. Never include a header from a higher layer.
- **Includes**: paths are relative to `src/`, e.g. `#include "storage/pager.h"`. Each `.c` file
  includes its own header first.
- **Naming**: functions are `module_verb_object` (`pager_get_page`, `leaf_node_set_key`). Types are
  `PascalCase`. Enum values and constants are `UPPER_SNAKE_CASE`, prefixed with their module.
- **Node access**: always go through the getters and setters in `storage/node.h`. Never cast a
  page offset to `uint32_t*`, because most fields are unaligned.
- **Errors**: return a result enum for anything the user caused. Use `fatal()` only for I/O failures
  and broken invariants.
- **Visibility**: helpers used by only one file are `static`. `-Wmissing-prototypes` flags any
  non-static function that has no declaration in a header.
- **Comments**: the code doesn't use comments. Names should say what code does, and the
  explanations belong in these docs.

## Adding a meta-command

1. Write a `static MetaCommandResult <name>_command(Table* table)` handler in
   `src/cli/meta_command.c`.
2. Add it to the `META_COMMANDS` table.
3. Add a spec.

## Adding a statement

1. Add a `STATEMENT_*` value to `src/sql/statement.h`, plus any fields the statement needs.
2. Parse it in `src/sql/parser.c` (`prepare_statement`).
3. Run it in `src/sql/executor.c` (`execute_statement`).
4. If it adds new result codes, map them to messages in `src/cli/repl.c`.
5. Add specs.
