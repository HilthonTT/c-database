SRC_DIR   := src
BUILD_DIR := build
TARGET    := db

CC       ?= gcc
CSTD     := -std=c11
WARNINGS := -Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes -Wmissing-prototypes
CPPFLAGS := -I$(SRC_DIR) -D_POSIX_C_SOURCE=200809L -MMD -MP
OPTFLAGS ?= -g -O0
CFLAGS   := $(CSTD) $(WARNINGS) $(OPTFLAGS)

SOURCES := $(shell find $(SRC_DIR) -name '*.c')
HEADERS := $(shell find $(SRC_DIR) -name '*.h')
OBJECTS := $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
DEPS    := $(OBJECTS:.o=.d)

.PHONY: all release sanitize test format clean

all: $(TARGET)

release: clean
	$(MAKE) OPTFLAGS="-O2 -DNDEBUG"

sanitize: clean
	$(MAKE) OPTFLAGS="-g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer" \
		LDFLAGS="-fsanitize=address,undefined"

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

test: $(TARGET)
	bundle exec rspec

format:
	clang-format -i $(SOURCES) $(HEADERS)

clean:
	$(RM) -r $(BUILD_DIR) $(TARGET) *.db

-include $(DEPS)
