CC = gcc
CCFLAGS = -Wall -g -O3 -fomit-frame-pointer
INCLUDES = $(shell sdl-config --cflags)
LDFLAGS =
LIBS = $(shell sdl-config --libs) -lgmp

SRCS = mandel.c

OBJS = $(SRCS:.c=.o)

MAIN = mandel

.SUFFIXES: .c .o

.c.o:
	$(CC) $(CCFLAGS) $(INCLUDES) -c $< -o $@

.PHONY: .depend clean

$(MAIN): $(OBJS)
	$(CC) $(CCFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LIBS) $(LDFLAGS)

clean:
	rm -f *.o *~ $(MAIN)

.depend: $(SRCS)
	$(CC) -M $(CCFLAGS) $(INCLUDES) $^ > $@

include .depend
