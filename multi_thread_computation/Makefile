all: res

default: check
	
clean:
	rm -rf main

res:
	gcc -g main.c threadpool.c -D THPOOL_DEBUG -lpthread -o main

check:
	make clean
	make res
	./main -threads $(threads) -x $(x) -n $(n)

build: res
