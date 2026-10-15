#ifndef STORAGE_ROW_H
#define STORAGE_ROW_H

#include <stdint.h>

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

typedef struct {
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE + 1];
    char email[COLUMN_EMAIL_SIZE + 1];
} Row;

#define ROW_FIELD_SIZE(field) ((uint32_t)sizeof(((Row*)0)->field))

enum {
    ROW_ID_SIZE = ROW_FIELD_SIZE(id),
    ROW_USERNAME_SIZE = ROW_FIELD_SIZE(username),
    ROW_EMAIL_SIZE = ROW_FIELD_SIZE(email),
    ROW_ID_OFFSET = 0,
    ROW_USERNAME_OFFSET = ROW_ID_OFFSET + ROW_ID_SIZE,
    ROW_EMAIL_OFFSET = ROW_USERNAME_OFFSET + ROW_USERNAME_SIZE,
    ROW_SIZE = ROW_ID_SIZE + ROW_USERNAME_SIZE + ROW_EMAIL_SIZE,
};

void row_serialize(const Row* source, void* destination);
void row_deserialize(const void* source, Row* destination);
void row_print(const Row* row);

#endif
