CFLAGS=-O2
all: viewer converter
debug: CFLAGS=-g
debug: all
viewer: viewer.o
	gcc ${CFLAGS} viewer.o -lglut -lGL -lGLU -lX11 -lXi -lm -o viewer
viewer.o: viewer.c format.h
	gcc -c ${CFLAGS} viewer.c
converter: converter.o
	gcc ${CFLAGS} converter.o -lpng -o converter
converter.o: converter.c format.h
	gcc -c ${CFLAGS} converter.c
clean:
	rm viewer.o converter.o
