#include "storage/row.h"

#include <stdio.h>
#include <string.h>

void row_serialize(const Row* source, void* destination) {
    uint8_t* bytes = destination;
    memcpy(bytes + ROW_ID_OFFSET, &source->id, ROW_ID_SIZE);
    strncpy((char*)bytes + ROW_USERNAME_OFFSET, source->username, ROW_USERNAME_SIZE);
    strncpy((char*)bytes + ROW_EMAIL_OFFSET, source->email, ROW_EMAIL_SIZE);
}

void row_deserialize(const void* source, Row* destination) {
    const uint8_t* bytes = source;
    memcpy(&destination->id, bytes + ROW_ID_OFFSET, ROW_ID_SIZE);
    memcpy(destination->username, bytes + ROW_USERNAME_OFFSET, ROW_USERNAME_SIZE);
    memcpy(destination->email, bytes + ROW_EMAIL_OFFSET, ROW_EMAIL_SIZE);
    destination->username[COLUMN_USERNAME_SIZE] = '\0';
    destination->email[COLUMN_EMAIL_SIZE] = '\0';
}

void row_print(const Row* row) {
    printf("(%u, %s, %s)\n", row->id, row->username, row->email);
}
