
CC=gcc
CFLAGS=-O3 -D_BIN_TRANSLATION_ -D__RCDAEMON__
LDFLAGS=-static -pthread

all: bin-translator

bin-translator:
	$(CC) $(CFLAGS) $(LDFLAGS) -c bin_translator.c -o bin_translator.o
