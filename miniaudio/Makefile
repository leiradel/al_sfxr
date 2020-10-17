CC = gcc
CFLAGS = -std=c99 -O0 -g -Wall -Wextra -Wpedantic `sdl2-config --cflags`
INCLUDES = -I.. -Iminiaudio
LIBS = -lm -ldl -lpthread

all: play

play: main.o
	$(CC) -o $@ $+ $(LIBS)

main.o: main.c ../al_sfxr.h miniaudio/miniaudio.h
	$(CC) $(INCLUDES) $(CFLAGS) -Werror -c $< -o $@

clean: FORCE
	rm -f play main.o

.PHONY: FORCE
