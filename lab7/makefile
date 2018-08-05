VALGRIND = valgrind --leak-check=full --show-reachable=yes

all: hexeditplus

hexeditplus: hexeditplus.o
	gcc -g -Wall -o hexeditplus hexeditplus.o

hexeditplus.o: hexeditplus.c
	gcc -g -Wall -c -o hexeditplus.o hexeditplus.c

val: hexeditplus
	$(VALGRIND) ./hexeditplus

val-v: hexeditplus
	$(VALGRIND) -v ./hexeditplus

.PHONY: clean
clean:
	rm hexeditplus.o hexeditplus