CC = gcc
CFLAGS = -Wall -g -s


all: oss worker

oss: oss.o
	$(CC) $(CFLAGS) -o oss oss.o

worker: worker.o
	$(CC) $(CFLAGS) -o worker worker.o

oss.o: oss.c oss.h
	$(CC) $(CFLAGS) -c oss.c

worker.o: worker.c oss.h
	$(CC) $(CFLAGS) -c worker.c

clean:
	rm -f *.o
