#include "sql/parser.h"

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TOKENS 5

typedef struct {
    const char* start;
    size_t length;
} Token;

static size_t tokenize(const char* input, Token tokens[], size_t max_tokens) {
    size_t count = 0;
    const char* cursor = input;

    while (*cursor != '\0') {
        while (isspace((unsigned char)*cursor)) {
            cursor++;
        }
        if (*cursor == '\0') {
            break;
        }

        const char* start = cursor;
        while (*cursor != '\0' && !isspace((unsigned char)*cursor)) {
            cursor++;
        }
        if (count < max_tokens) {
            tokens[count] = (Token){.start = start, .length = (size_t)(cursor - start)};
        }
        count++;
    }

    return count;
}

static bool token_equals(Token token, const char* text) {
    return token.length == strlen(text) && strncmp(token.start, text, token.length) == 0;
}

static PrepareResult parse_id(Token token, uint32_t* id) {
    char buffer[32];
    if (token.length >= sizeof(buffer)) {
        return PREPARE_SYNTAX_ERROR;
    }
    memcpy(buffer, token.start, token.length);
    buffer[token.length] = '\0';

    char* end = NULL;
    errno = 0;
    long long value = strtoll(buffer, &end, 10);
    if (*end != '\0' || errno == ERANGE) {
        return PREPARE_SYNTAX_ERROR;
    }
    if (value < 0) {
        return PREPARE_NEGATIVE_ID;
    }
    if (value > UINT32_MAX) {
        return PREPARE_SYNTAX_ERROR;
    }

    *id = (uint32_t)value;
    return PREPARE_SUCCESS;
}

static void copy_token(Token token, char* destination) {
    memcpy(destination, token.start, token.length);
    destination[token.length] = '\0';
}

static PrepareResult prepare_insert(const Token tokens[], size_t count, Statement* statement) {
    if (count != 4) {
        return PREPARE_SYNTAX_ERROR;
    }

    Row* row = &statement->row_to_insert;
    PrepareResult result = parse_id(tokens[1], &row->id);
    if (result != PREPARE_SUCCESS) {
        return result;
    }

    if (tokens[2].length > COLUMN_USERNAME_SIZE || tokens[3].length > COLUMN_EMAIL_SIZE) {
        return PREPARE_STRING_TOO_LONG;
    }

    copy_token(tokens[2], row->username);
    copy_token(tokens[3], row->email);
    statement->type = STATEMENT_INSERT;
    return PREPARE_SUCCESS;
}

static PrepareResult prepare_select(size_t count, Statement* statement) {
    if (count != 1) {
        return PREPARE_SYNTAX_ERROR;
    }
    statement->type = STATEMENT_SELECT;
    return PREPARE_SUCCESS;
}

PrepareResult prepare_statement(const char* input, Statement* statement) {
    Token tokens[MAX_TOKENS];
    size_t count = tokenize(input, tokens, MAX_TOKENS);

    if (count == 0) {
        return PREPARE_UNRECOGNIZED_STATEMENT;
    }
    if (token_equals(tokens[0], "insert")) {
        return prepare_insert(tokens, count, statement);
    }
    if (token_equals(tokens[0], "select")) {
        return prepare_select(count, statement);
    }
    return PREPARE_UNRECOGNIZED_STATEMENT;
}
