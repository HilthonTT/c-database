<p align="center">
  <img src="docs/assets/logo.svg" alt="c-database logo" width="128" height="128">
</p>

<h1 align="center">c-database</h1>

<p align="center">
  A small SQLite-style database written from scratch in C11:<br>
  a REPL, a tiny SQL front end, and a disk-backed B+ tree storage engine.
</p>

<p align="center">
  <a href="#quick-start">Quick start</a> ·
  <a href="#usage">Usage</a> ·
  <a href="docs/architecture.md">Architecture</a> ·
  <a href="docs/file-format.md">File format</a> ·
  <a href="docs/development.md">Development</a>
</p>

---

## Overview

`c-database` stores rows of a single fixed table in one file on disk. Under the hood it has the same
basic parts as SQLite, kept small enough to read in an afternoon:

- **REPL**: an interactive `db > ` prompt that accepts statements and dot-commands.
- **SQL front end**: a tokenizer and parser that compile input into a `Statement`, plus an executor
  that runs it.
- **B+ tree**: rows are kept sorted by `id` in leaf nodes linked left to right. Internal nodes route
  lookups, and both kinds of node split when they fill up.
- **Pager**: reads and writes fixed 4 KiB pages, caches them in memory, and flushes them to disk on exit.

The table schema is fixed:

| Column     | Type          | Constraint            |
| ---------- | ------------- | --------------------- |
| `id`       | `uint32`      | primary key, unique   |
| `username` | `varchar(32)` |                       |
| `email`    | `varchar(255)`|                       |

## Quick start

Requirements: a C11 compiler (`gcc` or `clang`) and `make` on a POSIX system (Linux, macOS, WSL).

```sh
git clone https://github.com/HilthonTT/c-database.git
cd c-database
make
./db my.db
```

## Usage

```text
$ ./db my.db
db > insert 1 alice alice@example.com
Executed.
db > insert 2 bob bob@example.com
Executed.
db > select
(1, alice, alice@example.com)
(2, bob, bob@example.com)
Executed.
db > .exit
```

The data is saved to `my.db` and is there again the next time the file is opened.

### Statements

| Statement                          | Description                                  |
| ---------------------------------- | -------------------------------------------- |
| `insert <id> <username> <email>`   | Insert a row. `id` must be a non-negative 32-bit integer. |
| `select`                           | Print every row in ascending `id` order.     |

### Meta-commands

| Command      | Description                                              |
| ------------ | -------------------------------------------------------- |
| `.exit`      | Flush all pages to disk and quit. End of input does the same. |
| `.btree`     | Print the shape of the B+ tree.                          |
| `.constants` | Print the on-disk layout constants.                      |

### Messages

| Output                                        | Meaning                                         |
| --------------------------------------------- | ----------------------------------------------- |
| `Executed.`                                   | The statement succeeded.                        |
| `Error: Duplicate key.`                       | A row with that `id` already exists.            |
| `Error: Table full.`                          | No free page left for the split this insert needs. |
| `String is too long.`                         | `username` > 32 or `email` > 255 bytes.         |
| `ID must be positive.`                        | `id` is negative.                               |
| `Syntax error. Could not parse statement.`    | Wrong number of fields or an `id` that is not a number. |
| `Unrecognized keyword at start of '...'.`     | Unknown statement.                              |
| `Unrecognized command '...'`                  | Unknown meta-command.                           |

## Project layout

```text
c-database/
├── src/
│   ├── main.c              Entry point: opens the table and runs the REPL
│   ├── cli/                Front end: input, meta-commands, REPL loop
│   ├── sql/                Statement model, parser and executor
│   ├── storage/            Row codec, pager, node layout, B+ tree, cursor, table
│   └── common/             Fatal-error and allocation helpers
├── spec/                   End-to-end RSpec tests that drive the ./db binary
├── docs/                   Architecture, file format and development guides
├── Makefile
└── .clang-format
```

Each layer depends only on the layers below it: `cli → sql → storage → common`. See
[docs/architecture.md](docs/architecture.md) for a module-by-module walkthrough.

## Building and testing

| Command          | What it does                                               |
| ---------------- | ---------------------------------------------------------- |
| `make`           | Debug build (`-g -O0`) of `./db`.                          |
| `make release`   | Optimised build (`-O2`).                                   |
| `make sanitize`  | Build with AddressSanitizer and UndefinedBehaviorSanitizer. |
| `make test`      | Build, then run the RSpec suite (`bundle exec rspec`).     |
| `make format`    | Run `clang-format` on every source file.                   |
| `make clean`     | Remove build output and `*.db` files.                      |

The tests need Ruby and Bundler: run `bundle install` once, then `make test`. See
[docs/development.md](docs/development.md) for details.

## Limitations

This is a learning project, not a production database:

- One hard-coded table. There is no `CREATE TABLE`, `WHERE`, `UPDATE` or `DELETE`.
- At most 100 pages (about 400 KiB) per database file.
- Every page in use stays in memory, and all of them are written back on exit. There are no
  transactions, journal or crash recovery.
- Internal nodes are capped at 3 keys so that multi-level trees show up with only a few rows.
- Values cannot contain spaces.
- Integers are stored in host byte order, so database files can't be moved between machines with
  different byte orders.

## Acknowledgements

Inspired by [cstack's "Let's Build a Simple Database"](https://cstack.github.io/db_tutorial/) tutorial.

## License

Released under the [MIT License](LICENSE).
