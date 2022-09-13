CC ?= gcc

TEST_DIR = test

OBJS = ppos_core.o ppos_disk.o queue.o disk.o

CFLAGS += -Wall -Wextra -I. -g
LDLIBS  = lrt

all: $(OBJS)

test: all
	@ $(MAKE) -C $(TEST_DIR) run

debug:
	@ CFLAGS=-DDEBUG $(MAKE) all

clean:
	@ rm -f $(OBJS)
	@ $(MAKE) -C $(TEST_DIR) clean

.PHONY: test debug clean
