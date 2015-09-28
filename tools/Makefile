CC=gcc
CFLAGS=-Wall -std=c99 -O2 -g -D_FILE_OFFSET_BITS=64 -Wp,-D_FORTIFY_SOURCE=2 -fstack-protector --param=ssp-buffer-size=4 -fPIC -Wl,-z,relro -Wl,-z,now -pie

all: semi_brif bv2csv

semi_brif: semi_brif.c
	$(CC) $(CFLAGS) -o semi_brif semi_brif.c

bv2csv: bv2csv.c
	$(CC) $(CFLAGS)  -o bv2csv bv2csv.c -lm

clean:
	rm -f bv2csv semi_brif
