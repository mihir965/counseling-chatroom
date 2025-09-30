# Compiler and flags
CC = clang
CFLAGS = -Wall -Wextra -g -std=c11 -Isrc -Iinclude

# Directories
SRCDIR = src
BUILDDIR = build
INCLUDEDIR = include

# Sources and Objects
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SOURCES))

# Binary output
BINARY = chat_server

# Default target
all: $(BINARY)

$(BINARY): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ -lcurl

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILDDIR) $(BINARY)

.PHONY: all clean
