SOURCE_DIR := src
BUILD_DIR := build
INCLUDE_DIR := include

# Create build directories
$(shell mkdir -p $(BUILD_DIR)/server $(BUILD_DIR)/client)

# Server and client source files
SERVER_SOURCES := $(shell find $(SOURCE_DIR)/server -type f -name "*.c")
UTIL_SOURCES := $(shell find $(SOURCE_DIR)/utils -type f -name "*.c")
CLIENT_SOURCES := $(shell find $(SOURCE_DIR)/client -type f -name "*.c")

# Object files
SERVER_OBJS := $(SERVER_SOURCES:$(SOURCE_DIR)/%.c=$(BUILD_DIR)/%.o)
UTIL_OBJS := $(UTIL_SOURCES:$(SOURCE_DIR)/%.c=$(BUILD_DIR)/%.o)
CLIENT_OBJS := $(CLIENT_SOURCES:$(SOURCE_DIR)/%.c=$(BUILD_DIR)/%.o)

# Compiler flags
CC := gcc
CFLAGS := -Wall -Wextra -Werror -std=gnu11 \
          -Wno-unused-parameter -Wno-implicit-fallthrough \
          -D_POSIX_C_SOURCE=200112L
CFLAGS += -I$(SOURCE_DIR) -I$(INCLUDE_DIR)
LDFLAGS := -lpthread

# Main targets
all: server client

# Server build
server: $(BUILD_DIR)/server/werewolf_server

$(BUILD_DIR)/server/werewolf_server: $(SERVER_OBJS) $(UTIL_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Client build
client: $(BUILD_DIR)/client/werewolf_client

$(BUILD_DIR)/client/werewolf_client: $(CLIENT_OBJS) $(UTIL_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Object file rules
$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean server client
