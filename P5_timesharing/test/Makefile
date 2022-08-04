TOP = ..
CC ?= gcc

OBJS = $(TOP)/ppos_core.o $(TOP)/queue.o
EXES = pingpong-preempcao pingpong-preempcao-stress

CFLAGS = -I$(TOP) -g

all: $(EXES)

run: all
	@ echo 'Esse teste não deve apresentar uma saída idêntica'
	./pingpong-preempcao | diff pingpong-preempcao.txt -

$(EXES): $(OBJS)

$(OBJS):
	@ $(MAKE) -C $(TOP)

clean:
	@ rm -f $(EXES)

.PHONY: clean run
