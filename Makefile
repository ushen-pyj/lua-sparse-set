CC = gcc
CFLAGS = -Wall -O2 -fPIC
LDFLAGS = -shared

LUA_INC = $(shell pkg-config --cflags lua 2>/dev/null || echo "-I/usr/local/include/lua5.1 -I/usr/include/lua5.1 -I/usr/include/lua5.3 -I/usr/include/lua5.4")

BUILD_DIR = build
TARGET = $(BUILD_DIR)/sparseset.so

SRCS = sparse-set.c lua-sparse-set.c
OBJS = $(addprefix $(BUILD_DIR)/, $(SRCS:.c=.o))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LUA_INC) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
clean:
	rm -f $(OBJS) $(TARGET)

test: all
	lua test.lua

.PHONY: all clean test
