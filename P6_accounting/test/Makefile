TOP = ..
CC ?= gcc

OBJS = $(TOP)/ppos_core.o $(TOP)/queue.o
EXES = pingpong-contab pingpong-contab-prio

CFLAGS = -I$(TOP) -g

all: $(EXES)

run: all
	@ echo 'Esse teste não deve apresentar uma saída idêntica'
	./pingpong-contab | diff pingpong-contab.txt -
	./pingpong-contab-prio | diff pingpong-contab-prio.txt -

$(EXES): $(OBJS)

$(OBJS):
	@ $(MAKE) -C $(TOP)

clean:
	@ rm -f $(EXES)

.PHONY: clean run
