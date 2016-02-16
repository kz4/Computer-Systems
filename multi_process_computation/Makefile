all: check

default: check
	
clean:
	rm -rf master worker

rst:
	gcc -o master master.c && gcc worker.c -lm -o worker

res:
	./master --worker_path ./worker --wait_mechanism $(mechanism) -x $(x) -n $(n)

worker:
	./worker -x 2 -n 3

check:
	make clean
	make rst
	make res

build: rst