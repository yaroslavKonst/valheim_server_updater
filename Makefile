CC=gcc -Wall

.PHONY: all

all: server control

server: main.c
	$(CC) -o server main.c

control: control.c
	$(CC) -o control control.c
