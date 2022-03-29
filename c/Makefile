# Compiles an executable for each .c file in the same directory.
# libserialport is linked in, which must be installed (visible from pkg-config)

CC = gcc
CFLAGS = -g -Wall $(shell pkg-config --cflags libserialport)
LIBS = $(shell pkg-config --libs libserialport)

SOURCES = $(wildcard *.c)
BINARIES = $(SOURCES:.c=)

%: %.c
	$(CC) $(CFLAGS) $< $(LIBS) -o $@

all: $(BINARIES)

clean:
	rm $(BINARIES)
