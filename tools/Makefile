CC=gcc
CFLAGS=-Wall -std=c99 -O2 -g

all: semi_brif bv2csv

semi_brif: semi_brif.c
	$(CC) $(CFLAGS) -o semi_brif semi_brif.c

bv2csv: bv2csv.c
	$(CC) $(CFLAGS)  -o bv2csv bv2csv.c -lm

clean:
	rm -f bv2csv semi_brif
