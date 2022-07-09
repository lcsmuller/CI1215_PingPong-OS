TOP = ..
CC ?= gcc

OBJS = $(TOP)/ppos_core.o $(TOP)/queue.o
EXE = pingpong-dispatcher

CFLAGS = -I$(TOP) -g

all: $(EXE)

run: all
	./pingpong-dispatcher | diff pingpong-dispatcher.txt -

$(EXE): $(OBJS)

$(OBJS):
	@ $(MAKE) -C $(TOP)

clean:
	@ rm -f $(EXE)

.PHONY: clean run
