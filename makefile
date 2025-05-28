all: proyecto_4

proyecto_4: proyecto_4.c
	gcc -o proyecto_4 proyecto_4.c `pkg-config --cflags --libs gtk+-3.0 cairo` -lm

clean:
	rm -f ./proyecto_4 || true

