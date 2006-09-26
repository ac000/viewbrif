viewbrif: viewbrif.c
	gcc -Wall viewbrif.c -o viewbrif `pkg-config --cflags gtk+-2.0` `pkg-config --libs gtk+-2.0`

clean:
	rm viewbrif

