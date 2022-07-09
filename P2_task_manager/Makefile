CC ?= gcc

TEST_DIR = test

OBJ = ppos_core.o

CFLAGS += -std=c99 -Wall -Wextra -I. -g

all: $(OBJ)

test: all
	@ $(MAKE) -C $(TEST_DIR) run

debug:
	@ CFLAGS=-DDEBUG $(MAKE) all

clean:
	@ rm -f $(OBJ)
	@ $(MAKE) -C $(TEST_DIR) clean

.PHONY: test debug clean
