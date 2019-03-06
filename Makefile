all: my_format

FLAGS = -Wall  -L./ -m32

my_format: my_format.c
	gcc ${FLAGS} my_format.c -o my_format -lm
