viewbrif: viewbrif.c
	gcc -Wall viewbrif.c -o viewbrif `pkg-config --cflags gtk+-2.0` `pkg-config --libs gtk+-2.0 gthread-2.0`
	gzip -c viewbrif.1 > viewbrif.1.gz

debug: viewbrif.c
	gcc -Wall -g viewbrif.c -o viewbrif `pkg-config --cflags gtk+-2.0` `pkg-config --libs gtk+-2.0 gthread-2.0`

clean:
	rm viewbrif viewbrif.1.gz

