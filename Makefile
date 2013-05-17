
CC=gcc
CFLAGS=-O3 -D_BIN_TRANSLATION_ -D__RCDAEMON__ -Wall -Wextra
LDFLAGS=-static -pthread

all: vpc_lnk

bin-translator:
	$(CC) $(CFLAGS) $(LDFLAGS) -c bin_translator.c -o vpc_objects/bin_translator.o

vpc_lnk: zlog_mod bin-translator
	rm /lhome/bgan/vpc /lhome/bgan/l0/vpc;cd vpc_objects; make vpc;cp vpc /lhome/bgan/l0

zlog_mod:
	gcc -Wall -Wextra -O3 -c zlog_mod.c -o vpc_objects/zlog_mod.o

NativeCodeBlockTest:
	gcc -Wall -Wextra -O3 -g3 -c NativeCodeBlock.c

