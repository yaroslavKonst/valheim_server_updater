CC=gcc -Wall

.PHONY: all

all: server control

server: build/main.o build/cmd_parse.o
	$(CC) -o server build/main.o build/cmd_parse.o

control: build/control.o build/cmd_parse.o
	$(CC) -o control build/control.o build/cmd_parse.o

build/%.o: %.c
	$(CC) -c $< -o $@
