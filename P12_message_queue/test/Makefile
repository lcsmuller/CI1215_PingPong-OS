TOP = ..
CC ?= gcc

OBJS = $(TOP)/ppos_core.o $(TOP)/queue.o
EXES = pingpong-mqueue

CFLAGS += -I$(TOP) -g
LDLIBS += -lm

all: $(EXES)

run: all
	@ echo 'Esse teste não deve apresentar uma saída idêntica'
	./pingpong-mqueue | diff pingpong-mqueue.txt -

$(EXES): $(OBJS)

$(OBJS):
	@ $(MAKE) -C $(TOP)

clean:
	@ rm -f $(EXES)

.PHONY: clean run
