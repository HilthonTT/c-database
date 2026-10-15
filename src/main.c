#include <stdio.h>
#include <stdlib.h>

#include "cli/repl.h"
#include "storage/table.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <database-file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    Table* table = table_open(argv[1]);
    repl_run(table);
    table_close(table);
    return EXIT_SUCCESS;
}
