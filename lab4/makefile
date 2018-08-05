OBJECTS = start.o main.o lab4_util.o
CFLAGS = -m64 -Wall -ansi -c -nostdlib -fno-stack-protector

all: search

search: $(OBJECTS)
	ld -m elf_x86_64 $(OBJECTS) -o search

start.o: start.s
	nasm -f elf64 start.s -o start.o

lab4_util.o: lab4_util.c
	gcc $(CFLAGS) lab4_util.c -o lab4_util.o

main.o: task2c.c
	gcc $(CFLAGS) task2c.c -o main.o

.PHONY: clean
clean:
	rm $(OBJECTS) search