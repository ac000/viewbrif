viewbrif: viewbrif.c
	gcc -Wall -g viewbrif.c -o viewbrif `pkg-config --cflags gtk+-2.0` `pkg-config --libs gtk+-2.0`
