CC=gcc
CFLAGS=-O2
LDFLAGS=-lpulse

all: main.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o flowers main.c
