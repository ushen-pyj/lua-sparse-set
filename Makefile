CC = gcc
CFLAGS = -Wall -O2 -fPIC
LDFLAGS = -shared

LUA_INC = $(shell pkg-config --cflags lua 2>/dev/null || echo "-I/usr/local/include/lua5.1 -I/usr/include/lua5.1 -I/usr/include/lua5.3 -I/usr/include/lua5.4")

BUILD_DIR = build
TARGET = $(BUILD_DIR)/sparseset.so

SRCS = register.c sparse-set.c lua-sparse-set.c

all: $(TARGET)

$(TARGET): $(SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LUA_INC) $(LDFLAGS) -o $@ $^

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
clean:
	rm -rf $(BUILD_DIR)

test: all
	lua test/test.lua

.PHONY: all clean test
