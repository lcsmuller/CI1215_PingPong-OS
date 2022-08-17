TOP = ..
CC ?= gcc

OBJS = $(TOP)/ppos_core.o $(TOP)/queue.o
EXES = pingpong-semaphore pingpong-racecond

CFLAGS = -I$(TOP) -g

all: $(EXES)

run: all
	@ echo 'Esse teste não deve apresentar uma saída idêntica'
	./pingpong-semaphore | diff pingpong-semaphore.txt -
	./pingpong-racecond | diff pingpong-racecond.txt -

$(EXES): $(OBJS)

$(OBJS):
	@ $(MAKE) -C $(TOP)

clean:
	@ rm -f $(EXES)

.PHONY: clean run
