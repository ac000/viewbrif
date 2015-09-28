CC=gcc
CFLAGS=-Wall -std=c99 -O2 -g -D_FILE_OFFSET_BITS=64 -Wp,-D_FORTIFY_SOURCE=2 -fstack-protector -fPIC -Wl,-z,relro -Wl,-z,now -pie
LIBS=`pkg-config --libs gtk+-2.0` -pthread
INCS=`pkg-config --cflags gtk+-2.0`

viewbrif: viewbrif.c
	$(CC) $(CFLAGS) viewbrif.c -o viewbrif ${INCS} ${LIBS}
	gzip -c viewbrif.1 > viewbrif.1.gz

clean:
	rm -f viewbrif viewbrif.1.gz
