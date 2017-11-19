CC=gcc
CFLAGS= -std=c99 -pipe 

all: pipeline
	$(CC) $(CFLAGS) pipelinesim.o -o pipeline

pipeline: pipelinesim.c
	$(CC) $(CFLAGS) -c pipelinesim.c

clean:
	rm *.o *.out pipeline
