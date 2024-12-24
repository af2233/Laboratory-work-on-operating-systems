CC=gcc
FLAGS=-fsanitize=thread -pthread -g

all: main

main:
	${CC} main.c ${FLAGS} -o main

clean:
	rm -rf main
