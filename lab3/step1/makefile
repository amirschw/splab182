OBJECTS = main.o toy_printf.o

all: main

main: $(OBJECTS)
	gcc -g -Wall -o main $(OBJECTS)

main.o: main.c toy_stdio.h
	gcc -g -Wall -c -o main.o main.c

toy_printf.o: toy_printf.c
	gcc -g -Wall -c -o toy_printf.o toy_printf.c

.PHONY: clean
clean:
	rm $(OBJECTS) main