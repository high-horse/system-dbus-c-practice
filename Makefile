build:
	gcc -std=c2x -o ddd.out -Wall -Wextra src/ch4.c `pkg-config --cflags dbus-1` `pkg-config --libs dbus-1`