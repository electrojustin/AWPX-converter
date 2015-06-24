all: viewer converter
viewer: viewer.o
	gcc -O2 viewer.o -lglut -lGL -lGLU -lX11 -lXi -lm -o viewer
viewer.o: viewer.c format.h
	gcc -c -O2 viewer.c
converter: converter.o
	gcc -O2 converter.o -lpng -o converter
converter.o: converter.c format.h
	gcc -c -O2 converter.c
clean:
	rm viewer.o converter.o
