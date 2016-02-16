<img src="https://github.com/kz4/Computer-Systems/blob/master/multi_process_computation/ProblemScreenShots/hw2-1.png"/>
<img src="https://github.com/kz4/Computer-Systems/blob/master/multi_process_computation/ProblemScreenShots/hw2-2.png"/>

For this assignment, we will implement a multi-process solution to compute exponential function using a master-worker architecture:


Master

The master is launched using the following command:
 $ ./master --worker_path ./worker --wait_mechanism MECHANISM -x 2 -n 12 
The flag --worker_path is used to point to the worker binary.
Where MECHANISM is one of sequential, select, poll, or epoll. See below for more details.
The rest of the flags indicate the values of x and n to compute ex (for n in [0..11]).
The master spawns n child worker processes, each of which computes xn/n!.
Worker

The worker should be individually testable, i.e., one should be able to run it as follows:
 $ ./worker -x 2 -n 3 x^n / n! : 1.3333 
It should print additional help text (e.g., x^n / n! : ) on the console if its standard-out is a terminal.
If the standard-out is a pipe, worker should simply write the result into the pipe before exiting.
Master-worker communication

Master will communicate with each worker via a separate pipe.
The write-end of the pipe is made standard-out of the worker.
The master reads the result from worker on the read-end of the pipe and prints it on the screen. Here is an example for worker 3:
 worker 3: 2^3 / 3! : 1.3333 
Waiting for workers

You have to implement the following mechanisms in the Master:
Sequential: first worker, followed by second, third, and so on.
Using select system call to read the workers' output in the order it becomes available.
The above using poll system call.
The above using epoll system call.