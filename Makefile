CC=gcc


all: sim.c
	$(CC) $(CFLAGS) sim.c -o sim

sim: sim.c
	$(CC) $(CFLAGS) -c assembler

clean:
	rm *.o *.out sim
