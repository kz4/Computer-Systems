all: 
	check

clean: 
	rm -rf malloc.o t-test1 *~

build: 
	gcc -Wall -g -c malloc.c -o malloc.o -lpthread && gcc t-test1.c malloc.o -o t-test1 -lpthread

check:
	./t-test1
