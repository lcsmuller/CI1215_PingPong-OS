TOP = ..
CC ?= gcc

OBJS = $(TOP)/ppos_core.o $(TOP)/queue.o
EXE = pingpong-scheduler

CFLAGS = -I$(TOP) -g

all: $(EXE)

run: all
	./pingpong-scheduler | diff pingpong-scheduler.txt -

$(EXE): $(OBJS)

$(OBJS):
	@ $(MAKE) -C $(TOP)

clean:
	@ rm -f $(EXE)

.PHONY: clean run
