viewbrif: viewbrif-0.02.c
	gcc -Wall -g viewbrif-0.02.c -o viewbrif `pkg-config --cflags gtk+-2.0` `pkg-config --libs gtk+-2.0`
