CC=gcc
CFLAGS=-std=c2x -Wall -Wextra $(shell pkg-config --cflags dbus-1) 
LIBS=$(shell pkg-config --libs x11 dbus-1) 

build:
	$(CC) $(CFLAGS) -o a.out main.c $(LIBS)