all: task2 

task2: task2.o start.o
	ld -o task2 task2.o startup.o start.o -L/usr/lib32 -lc -T linking_script -dynamic-linker /lib32/ld-linux.so.2

task2.o: task2.c
	gcc -c -m32 -g -Wall task2.c -o task2.o

start.o: start.s 
	nasm -g -f elf -w+all -o start.o start.s


clean:
	rm -f *.o task2
	
