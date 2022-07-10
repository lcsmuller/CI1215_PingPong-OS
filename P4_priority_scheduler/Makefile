CC ?= gcc

TEST_DIR = test

OBJS = ppos_core.o queue.o

CFLAGS += -std=c99 -Wall -Wextra -I. -g

all: $(OBJS)

test: all
	@ $(MAKE) -C $(TEST_DIR) run

debug:
	@ CFLAGS=-DDEBUG $(MAKE) all

clean:
	@ rm -f $(OBJS)
	@ $(MAKE) -C $(TEST_DIR) clean

.PHONY: test debug clean
