all: client server

client: client.o common.o
	gcc -g -Wall -o client client.o common.o

client.o: client.c
	gcc -g -Wall -c -o client.o client.c

server: server.o common.o
	gcc -g -Wall -o server server.o common.o

server.o: server.c
	gcc -g -Wall -c -o server.o server.c

common.o: common.c common.h
	gcc -g -Wall -c -o common.o common.c

.PHONY: clean
clean:
	rm client.o server.o common.o client server