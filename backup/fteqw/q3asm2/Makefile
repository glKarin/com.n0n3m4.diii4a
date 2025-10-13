# yeah, couldn't do more simple really

CC=gcc
CFLAGS=-O3 -Wall

default:	q3asm2

q3asm2:	q3asm2.c ../engine/qclib/hash.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f q3asm2 q3asm *~ *.o

