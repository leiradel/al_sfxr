CC = gcc
CFLAGS = -std=c99 -O0 -g -Wall -Wpedantic `sdl2-config --cflags`
INCLUDES = -I.. -Itinyfiledialogs -Idr_libs -Istb
LIBS = `sdl2-config --libs` -lm

all: sfxr

sfxr: main.o tinyfiledialogs/tinyfiledialogs.o
	$(CC) -o $@ $+ $(LIBS)

main.o: main.c ../al_sfxr.h tinyfiledialogs/tinyfiledialogs.h dr_libs/dr_wav.h
	$(CC) $(INCLUDES) $(CFLAGS) -Werror -c $< -o $@

main.c: icon.h ld48.h font.h
	touch $@

icon.h: sfxr.bmp imgconv
	./imgconv $< icon > $@

ld48.h: ld48.png imgconv
	./imgconv $< ld48 > $@

font.h: font.png imgconv
	./imgconv $< font > $@

tinyfiledialogs/tinyfiledialogs.o: tinyfiledialogs/tinyfiledialogs.c
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

imgconv: imgconv.o
	$(CC) -o $@ $+ $(LIBS)

imgconv.o: imgconv.c stb/stb_image.h
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

clean: FORCE
	rm -f sfxr main.o tinyfiledialogs/tinyfiledialogs.o icon.h ld48.h font.h imgconv imgconv.o

.PHONY: FORCE
