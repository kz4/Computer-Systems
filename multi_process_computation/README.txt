build: "make build"
check: "make check  mechanism=poll x=10 n=38" (poll can be substituted for sequential, select and epoll. 10 and 38 can be substiuted for some other numbers) It can also be shortened to "make mechanism=poll x=10 n=38"
clean: "make clean"

make check and make are for the master. For the worker, run: "./worker -x 10 -n 38" (10 and 38 can be substituted for some other numbers)

To run the master: 
./master --worker_path ./worker mechanism sequential x 2 n 12

To run worker:
./worker -x 2 -n 3