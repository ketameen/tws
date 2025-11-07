
CC     = gcc
CFLAGS = -std=c99 -Iinclude -Wall -Wextra -O2
SRC    = main.c $(wildcard include/*.c)
OBJ    = $(patsubst %.c, build/%.o, $(SRC))
TARGET = build/main

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p build
	$(CC) $(OBJ) -o $(TARGET)

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	@echo "Starting TWS..."
	@./$(TARGET)

clean:
	rm -rf build

.PHONY: all clean run
