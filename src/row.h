#ifndef ROW_H
#define ROW_H

#include <stdint.h>

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)

typedef struct {
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE + 1];
    char email[COLUMN_EMAIL_SIZE + 1];
} Row;

void print_row(Row *row);
void serialize_row(Row *source, void *destination);
void deserialize_row(void *source, Row *destination);

#define ID_SIZE        size_of_attribute(Row, id)
#define USERNAME_SIZE  size_of_attribute(Row, username)
#define EMAIL_SIZE     size_of_attribute(Row, email)
#define ROW_SIZE       (ID_SIZE + USERNAME_SIZE + EMAIL_SIZE)

#define ID_OFFSET        0
#define USERNAME_OFFSET  (ID_OFFSET + ID_SIZE)
#define EMAIL_OFFSET     (USERNAME_OFFSET + USERNAME_SIZE)

#endif
