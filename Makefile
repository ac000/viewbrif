CC=gcc
CFLAGS=-Wall -std=c99 -O2 -g
LIBS=`pkg-config --libs gtk+-2.0 gthread-2.0`
INCS=`pkg-config --cflags gtk+-2.0`

viewbrif: viewbrif.c
	$(CC) $(CFLAGS) viewbrif.c -o viewbrif ${INCS} ${LIBS}
	gzip -c viewbrif.1 > viewbrif.1.gz

clean:
	rm viewbrif viewbrif.1.gz
