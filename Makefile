all: build

build:
	gcc -Wall -o swish swish.c -lm

clean:
	rm -f swish
