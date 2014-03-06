
CC=gcc
CXX=g++
CFLAGS=-O3 -D_BIN_TRANSLATION_ -D__RCDAEMON__
CXXFLAGS=-g3 -Wall -Wextra
# CFLAGS+=-D_DEBUG_PRINT_NATIVE_INSTR_
# CFLAGS+=-D_DEBUG_PRINT_INSTR_
# CFLAGS+=-D_DEBUG_GREP_
# CFLAGS+=-D_DEBUG_MOVQ_
LDFLAGS=-static -pthread

all: bin-translator

bin-translator:
	$(CC) $(CFLAGS) $(LDFLAGS) -c bin_translator.c -o bin_translator.o

scan_func: scan_func.cpp
	$(CXX) $(CXXFLAGS) -o scan_func scan_func.cpp
