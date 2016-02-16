build: "make build"
check: "make check  mechanism=poll x=10 n=38" (poll can be substituted for sequential, select and epoll. 10 and 38 can be subsituted for some other numbers) It can also be shortened to "make mechanism=poll x=10 n=38"
clean: "make clean"

make check and make are for the master. For the worker, run: "./worker -x 10 -n 38" (10 and 38 can be substituted for some other numbers)