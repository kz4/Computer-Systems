<img src="https://github.com/kz4/Computer-Systems/blob/master/mini-DMTCP/ProblemScreenShots/hw1-1.png"/>
<img src="https://github.com/kz4/Computer-Systems/blob/master/mini-DMTCP/ProblemScreenShots/hw1-2.png"/>
<img src="https://github.com/kz4/Computer-Systems/blob/master/mini-DMTCP/ProblemScreenShots/hw1-3.png"/>
<img src="https://github.com/kz4/Computer-Systems/blob/master/mini-DMTCP/ProblemScreenShots/hw1-4.png"/>
<img src="https://github.com/kz4/Computer-Systems/blob/master/mini-DMTCP/ProblemScreenShots/hw1-5.png"/>
<img src="https://github.com/kz4/Computer-Systems/blob/master/mini-DMTCP/ProblemScreenShots/hw1-6.png"/>
<img src="https://github.com/kz4/Computer-Systems/blob/master/mini-DMTCP/ProblemScreenShots/hw1-7.png"/>
<img src="https://github.com/kz4/Computer-Systems/blob/master/mini-DMTCP/ProblemScreenShots/hw1-8.png"/>
<img src="https://github.com/kz4/Computer-Systems/blob/master/mini-DMTCP/ProblemScreenShots/hw1-9.png"/>

In this assignment, you will build a mini-DMTCP.  DMTCP is a widely
used software package for checkpoint-restart:  http://dmtcp.sourceforge.net
However, you do not need any prior knowledge of DMTCP for this assignment.
The software that you produce will be self-contained.

You will write a static library, libckpt.a.  Given an arbitrary application
in C or C++, you will then re-compile it (or simply re-link it)
with the following command-line:
  gcc -static -LPATH_TO_LIBCKPT -lckpt -Wl,-u,myconstructor FILES
where PATH_TO_LIBCKPT must be an absolute pathname for the directory where
you stored your libckpt.a file.  (The flag "-Wl,-u,myconstructor"
is a special requirement, which is explained in part 6 of
"SAVING A CHECKPOINT IMAGE FILE", below.)
Some examples of the usage are:
 gcc -static -L/home/user/hw2 -lckpt -Wl,-u,myconstructor \
     -o hello-world hello-world.c
OR:
 gcc -c hello-part1.c
 gcc -c hello-part2.c
 gcc -static -L/home/user/hw2 -lckpt -Wl,-u,myconstructor \
     -o hello-world hello-part1.o hello-part2.o

You must provide a Makefile for this project. 

Here is some advice for writing the code for libckpt.a.  However, if you
prefer a different design, you are welcome to follow that.
Your code will only be tested on single-threaded targets.  You are
not required to support multi-threaded applications.  Similarly,
you are not required to support shared memory.

(Note that the memory layout of a process is stored in /proc/PID/maps.
 The fields such as rwxp can be read as "read", "write", "execute"
 and "private".  Naturally, rwx refer to the permissions.
 Private refers to private memory.  If you see "s" instead
 of "p", then it is shared memory.)

Consider the following pseudo-code:
SAVING A CHECKPOINT IMAGE FILE:
1.  Your code should read /proc/self/maps (which is the memory layout
    of the current process.
2.  Your code should then save that information in a checkpoint image.
3.  You should save a file header, to restore information about
     the entire process.  This should include saving the registers.
     (See below on how to save and restore register values.)
4.  For each memory section, you should save information that you will
     need to later restore the memory:
    a.  a section header (including address range of that memory,
	and r/w/x permissions)
    b.  the data in that address range.
        (See below for an easy way to read the hexadecimal
         numbers used to specify a memory address range.)
5.  We need a simple way to trigger the checkpoint.  Write a signal
     handler for SIGUSR2, that will initiate all of the previous steps.
     [ To trigger a checkpoint, you now just need to do from a command line:
         kill -12 PID
       where PID is the process id of the target process that you wish
       to checkpoint.  (Signal 12 is SIGUSR2, reserved for end users.]
6.  The signal handler will only help you if you can call 'signal()'
     to register the signal handler.  We must write the call to 'signal()'
     inside libckpt.a.  But it must be called before the 'main()'
     routine of the end user.  The solution is to define a constructor
     function in C inside libckpt.a.  For the details of how to do this,
     see, below:  "Declaring constructor functions"
     [ COMMENT:  The linker will remove any .o files in libckpt.a
         that don't contain any symbols (functions) that we call.
         This will save memory by not including those .o files
         in the executable.  Unfortunately, the user program does
         not call any functions in the .o file in our libckpt.a.
         So, our .o file will not be linked in (as an optimization
         by the linker).  To fix this, use the flag
         "-Wl,-u,myconstructor" along with -lckpt when you compile.
         To understand this flag, see "man gcc", which says that
         anything after "=Wl" is passed to the linker.  Then see
         "man ld", which says that "-u myconstructor" will force
         the executable to declare "myconstructor" as an undefined
         symbol that needs to be defined in some library. ]
7.  The name of your checkpoint image file must be 'myckpt'.

RESTORING FROM A CHECKPOINT IMAGE FILE:
1.  Create a file, myrestart.c, and compile it statically at
     an address that the target process is not likely to use.
    For example:
      gcc -static \
        -Wl,-Ttext-segment=5000000 -Wl,-Tdata=5100000 -Wl,-Tbss=5200000 \
        -o myrestart myrestart.c
    [ Note:  The gcc syntax '-Wl,Ttext=...' means pass on the parameter
             '-Ttext=...' to the linker that will be called by gcc.
             See 'man ld' for the Linux linker. ]
      Note:  In reading 'man ld', you'll see that numbers like 500000000000
             should be interpreted as hexadecimal. ]
2.  When myrestart begins executing, it will need to move its stack
     to some infrequently used address, like 0x530000100000.
     A good way to do that is to:
    a.  Map in some new memory for a new stack, for example in the range
          0x5300000 - 0x5301000
          To create this memory, use mmap().  [ See:  'man mmap' ]
    b.  The myrestart.c program should take one argument, the name
         of the checkpoint image file from which to restore the process.
         Now copy that filename from the stack (probably from 'argv[1]')
         into your data segment (probably into a global variable).
         (We will soon be unmapping the initial stack.  So, we better
         first save any important data.)  When you copy the filename
         into allocated storage in your data segment, an easy way
         is to declare a global variable (e.g., 'char ckpt_image[1000];'),
         and then copy to the global variable.
    c.  Use the inline assembly syntax of gcc to include code that will switch
         the stack pointer (syntax: $sp or %sp) from the initial stack to the
         new stack.  Then immediately make a function call to a new function.
         The new function will then use a call frame in the new stack.
       Specifically, you will want some code like:
         asm volatile (mov %0,%%rsp; : : "g" (stack_ptr) : "memory");
         restore_memory();
       [ Note:  you will see similar code near the beginning of the
          function restart_fast_path():
            http://www.ccs.neu.edu/course/cs5600f15/dmtcp/HTML/S/11.html#L364
       ]
    d.  Now remove the original stack of myrestart, in case the checkpoint
          image will also be mapping some memory into that location.
          You'll need to discover the address range used by your original
          stack for myrestart, in order to unmap it.  That should be easy.
          Just read /proc/self/maps, and the old stack will have
          a notation '[stack]' on the line showing the memory region
          of the stack.
    d.  Copy the registers from the file header of your checkpoint
          image file into some pre-allocated memory in your data segment.
    e.  The memory of your myrestart process should no longer conflict
          with the memory to be restored from the checkpoint image.
          Now read the addresses of each memory segment from your
          checkpoint image file.  It's in your section header.
          Call mmap to map into memory a corresponding memory section
          for your new process.  [ See 'man mmap' ]
    f.  Copy the data from the memory sections of your checkpoint image
          into the corresponding memory sections.
    g.  Now, you need to jump into the old program and restore the
          old registers.  In a previous step, you had copied the old
          registers into your data segment.  Now use setcontext()
          to restore those as the working registers.
          [ Note:  See 'man setcontext'.  Also, note that this will
              restore your program counter (pc) and your stack
              pointer (sp).  So, you will automatically start using
              the text segment and stack segment from your previously
              checkpointed process. ]

A. Saving and restoring registers:
  See 'man getcontext' and 'man setcontext'
  It will save register values to a struct of type 'ucontext_t'.
  You can copy the struct to your checkpoint header with code like
  the following:
    ucontext_t mycontext;
    write_context_to_ckpt_header(&mycontext, sizeof mycontext);

B. Reading hexadecimal numbers:
  In reading /proc/self/maps, you'll need to read hexadecimal numbers.
  An easy way to do this is to copy the code:  mtcp_readhex()
	(found in:  src/mtcp/mtcp_util.ic , or online a:
         http://www.ccs.neu.edu/course/cs5600f15/dmtcp/mtcp__util_8ic.html )
  For your own curiosity (not needed here), mtcp_printf() also exists.

C. Declaring constructor functions:
  We want to call 'signal()' on SIGUSR2 even before the end user's
        'main()' function is executed.
  If this were C++, we would initialize some global variable to a
        constructor, and we would call 'signal()' from inside the constructor.
        A global constructor will always execute before 'main()'.
  The equivalent trick in the C language is to use
        '__attribute__ ((constructor))'.  This is a GNU extension that
        is also supported by most other Linux compilers.  The code is:
            __attribute__ ((constructor))
            void myconstructor() {
              signal(SIGUSR2, ...);
            }
  The constructor attribute forces the 'myconstructor()' function to be
        executed even before 'main()'.

You will submit this work using the submit script:
  /course/cs5600sp16/homework/submit-cs5600sp16-ProblemScreenShots/hw1 ProblemScreenShots/hw1.tar.gz
You will need to include a Makefile.
As before, you should generate ProblemScreenShots/hw1.tar.gz using a target, 'make dist'.

Your Makefile _must_ include a target 'check', which depends on
  the files libckpt.a and hello.c.  When you run 'make check',
  your program should do:
     gcc [ADD YOUR FLAGS HERE] -o hello hello.c
     (sleep 3 && kill -12 `pgrep -n hello` && \
      sleep 2 && pkill -9 -n hello && make restart) &
     ./hello
  Next, you must create a 'restart' target in Makefile.  It should do:
    ./myrestart myckpt

Finally, you are responsible for writing the test program, hello.c.
Write code for hello.c that will go into an infinite loop that prints
to standard output the single character '.', and then sleeps for one second.
[ Note:  You will need to call 'fflush(stdout)' to force the output
         Normally, for efficiency, Linux will send the characters to the
         screen only after it has seen the `\n' character marking the end
         of a line of output. ]

==================
TIPS FOR DEBUGGING

It is especially difficult to debug 'myrestart' using GDB, since 'myrestart'
resets the stack pointer, and descends into assembly language in order
to call setcontext, and change registers.  Here are two advanced techniques
in GDB-based debugging that will help you.

1.  Debugging at assembly level with GDB:
    There are three additional GDB commands to learn in order to
    debug in GDB:
  a. Equivalent of 'list' command for assembly level:
     (gdb) x/10i $pc-20
     [This means: examine the next 10 instructions, starting with the
      value of the program counter ($pc) minus 20 bytes.  Try this on
      any GDB session, and it will quickly become intuitive.]
  b. Equivalent of 'step' and 'next commands for assembly level:
     They are 'si' ("step instruction") and 'ni' ("next instruction").
  c. (gdb) info registers
     [The name says it all.  Display current values of all registers.]

2.  Loading a new symbol table into GDB:
        After 'myrestart' has mapped in the memory of the user application
    (e.g., hello.c), we now want to see the symbols of 'hello.c' instead
    of the symbols of 'myrestart'.  So, we need to do
        Recall that a symbol table is a table in which each row specifies
    a symbol (variable name, function name, etc.), followed by
    the address where it is stored in RAM, and optionally followed
    by the type of the symbol (or at least the size of the memory
    buffer that the address points to, such as 4 bytes for an 'int').

    Try:
    (gdb) apropos symbol
      add-symbol-file -- Load symbols from FILE
      ...
    The 'add-symbol-file' command requires a filename and an address:
    (gdb) help add-symbol-file
      ...
      Usage: add-symbol-file FILE ADDR [-s   -s   ...]
      ADDR is the starting address of the file's text.

    Clearly, the FILE for us is 'hello' (the executable compiled from hello.c).
    ADDR should be the beginning of the text segment.  You can guess
    the address of the text segment:
      (For example, run "gdb ./hello", followed by
       (gdb) break main
       (gdb) run
       (gdb) info proc mappings
       If it's like my computer, you'll see the text begin at 0x400000.  So:
       (gdb) add-symbol-file ./hello 0x400000
       [Remember, your goal is to know about the text segment of ./hello,
        prior to checkpointing.]
      )
    Or else, you can use a general script.  For this, copy:
      util/gdb-add-symbol-file
    from DMTCP to your local directory, or you can get it from the
    developer's site:
  https://raw.githubusercontent.com/dmtcp/dmtcp/master/util/gdb-add-symbol-file
    Then run:
      (gdb) shell ./gdb-add-symbol-file
    and follow the instructions that it prints out.
