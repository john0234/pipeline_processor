CC=gcc


all: assembler.c
	$(CC) $(CFLAGS) assembler.c -o assembler

assembler: assembler.c
	$(CC) $(CFLAGS) -c assembler

clean:
	rm *.o *.out assembler
