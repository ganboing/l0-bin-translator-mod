
CC=gcc
CFLAGS=-O3 -D_BIN_TRANSLATION_ -D__RCDAEMON__ -Wall -Wextra
LDFLAGS=-static -pthread

all: bin-translator

bin-translator:
	$(CC) $(CFLAGS) $(LDFLAGS) -c bin_translator.c -o vpc_objects/bin_translator.o
