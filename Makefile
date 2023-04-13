CC=gcc
CFLAGS=-g -Wall

.DEFAULT_GOAL := main

%: src/%.c
	$(CC) -o $@ $< $(CFLAGS)

clean:
	rm main.exe