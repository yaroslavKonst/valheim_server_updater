CC=gcc -Wall

.PHONY: all

all: main server steamcmd control

main: main.c
	$(CC) -o main main.c

server: server.c
	$(CC) -o server server.c

steamcmd: steamcmd.c
	$(CC) -o steamcmd steamcmd.c

control: control.c
	$(CC) -o control control.c
