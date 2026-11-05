CC = gcc
CFLAGS = -Wall -Wextra -O2 $(shell llvm-config --cflags)
LDFLAGS = $(shell llvm-config --ldflags --libs core executionengine native --system-libs)

SRC = src/main.c src/lexer.c src/parser.c src/codegen.c
OBJ = $(SRC:.c=.o)
TARGET = consumer

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
