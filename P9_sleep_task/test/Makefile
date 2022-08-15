TOP = ..
CC ?= gcc

OBJS = $(TOP)/ppos_core.o $(TOP)/queue.o
EXE = pingpong-sleep

CFLAGS = -I$(TOP) -g

all: $(EXE)

run: all
	@ echo 'Esse teste não deve apresentar uma saída idêntica'
	./pingpong-sleep | diff pingpong-sleep.txt -

$(EXE): $(OBJS)

$(OBJS):
	@ $(MAKE) -C $(TOP)

clean:
	@ rm -f $(EXE)

.PHONY: clean run
