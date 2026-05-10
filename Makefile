build:
	gcc -o ddd.out -Wall src/ch2.c `pkg-config --cflags dbus-1` `pkg-config --libs dbus-1`