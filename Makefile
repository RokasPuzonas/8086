CC=gcc
CFLAGS=-g -Wall
INCLUDES=./include

.DEFAULT_GOAL := main

%: src/%.c
	$(CC) -o $@ $< $(CFLAGS) -I$(INCLUDES)

clean:
	rm main.exe