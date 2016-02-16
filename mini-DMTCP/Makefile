all:	check

default: check
	
clean:
	rm -rf myckpt myrestart hello ckpt.o libckpt.a ckpt_reg

libckpt:
	gcc -c -o ckpt.o ckpt.c && ar rcs libckpt.a ckpt.o

prog:	libckpt
	gcc -g -static -L`pwd` -lckpt -Wl,-u,init_signal -o hello hello.c

rst:	prog
	gcc -g -static -Wl,-Ttext-segment=5000000 -Wl,-Tdata=5100000 -Wl,-Tbss=5200000 -o myrestart myrestart.c

res:
	./myrestart myckpt

gdb:
	gdb --args ./myrestart myckpt

check:	clean rst
	(sleep 3 && kill -12 `pgrep -n hello` && sleep 2 && pkill -9 hello && make res) &
	./hello	

dist:
	dir=`basename $$PWD`; cd ..; tar cvf $$dir.tar ./$$dir; gzip $$dir.tar

build:	rst
