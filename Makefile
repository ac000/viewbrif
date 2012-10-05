CC=gcc
CFLAGS=-Wall -std=c99 -O2 -g -D_FILE_OFFSET_BITS=64
LIBS=`pkg-config --libs gtk+-2.0 gthread-2.0`
INCS=`pkg-config --cflags gtk+-2.0`

viewbrif: viewbrif.c
	$(CC) $(CFLAGS) viewbrif.c -o viewbrif ${INCS} ${LIBS}
	gzip -c viewbrif.1 > viewbrif.1.gz

clean:
	rm -f viewbrif viewbrif.1.gz
