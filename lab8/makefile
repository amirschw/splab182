VALGRIND = valgrind --leak-check=full --show-reachable=yes

all: myELF

myELF: task2.o
	gcc -g -Wall -o myELF task2.o

task2.o: task2.c
	gcc -m64 -Wall -c -o task2.o task2.c

val: myELF
	$(VALGRIND) ./myELF

val-v: myELF
	$(VALGRIND) -v ./myELF

.PHONY: clean
clean:
	rm task2.o myELF