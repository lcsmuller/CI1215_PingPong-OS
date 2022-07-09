CC ?= gcc

OBJS = queue.o
EXES = testafila

CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -I. -g

all: $(EXES)

$(EXES): $(OBJS)

clean:
	@ rm -f $(OBJS) $(EXES)
