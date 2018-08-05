OBJECTS = task1c.o LineParser.o

all: myshell

myshell: $(OBJECTS)
	gcc -g -Wall -o myshell $(OBJECTS)

task1c.o: task1c.c
	gcc -g -Wall -c -o task1c.o task1c.c

LineParser.o: LineParser.c LineParser.h
	gcc -g -Wall -c -o LineParser.o LineParser.c

.PHONY: clean
clean:
	rm $(OBJECTS) myshell