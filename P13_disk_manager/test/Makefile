TOP = ..
CC ?= gcc

OBJS = $(TOP)/ppos_core.o $(TOP)/ppos_disk.o $(TOP)/queue.o $(TOP)/disk.o
EXES = pingpong-disco1 pingpong-disco2

CFLAGS += -I$(TOP) -g
LDLIBS += -lm -lrt

all: $(EXES)

run: all
	@ echo 'Esse teste não deve apresentar uma saída idêntica'
	./pingpong-disco1 | diff pingpong-disco1.txt -
	./pingpong-disco2 | diff pingpong-disco2.txt -

$(EXES): $(OBJS)

$(OBJS):
	@ $(MAKE) -C $(TOP)

clean:
	@ rm -f $(EXES)

.PHONY: clean run
