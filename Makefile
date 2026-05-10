build:
	gcc -std=c2x -o ddd.out -Wall -Wextra src/ch3.c `pkg-config --cflags dbus-1` `pkg-config --libs dbus-1`