all: viewer
viewer: viewer.o
	gcc -O2 viewer.o -lglut -lGL -lGLU -lX11 -lXi -lm -o viewer
viewer.o: viewer.c format.h
	gcc -c -O2 viewer.c
clean:
	rm viewer.o
