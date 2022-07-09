TOP = ..
CC ?= gcc

OBJ = $(TOP)/ppos_core.o
EXES = pingpong-tasks1 pingpong-tasks2 pingpong-tasks3

CFLAGS = -I$(TOP) -g

all: $(EXES)

run: all
	./pingpong-tasks1 | diff pingpong-tasks1.txt -
	./pingpong-tasks2 | diff pingpong-tasks2.txt -
	./pingpong-tasks3 | diff pingpong-tasks3.txt -

$(EXES): $(OBJ)

$(OBJ):
	@ $(MAKE) -C $(TOP)

clean:
	@ rm -f $(EXES)

.PHONY: clean run
