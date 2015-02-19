# kogata operating system project

kogata operating system : small and <del>beautiful</del>small.

## Project characteristics

### Written in C

I have considered using higher-level languages but for lack of understanding of
the associated compiler, I never have had the same impression of understanding
exactly what was happening that I had with C. Also, the necessity of a runtime
is bothering. I do regret not being able to exploit the magic of strongly typed
functionnal languages, but what can I do...

### Monolithic design

I would have liked to do a microkernel, but the necessity for asynchronous
communication makes it much more difficult to code, and contradicts the goal of
having simple and straightforward code.

### Emphasis on code quality and reliability

The code for the project must make sense, be simple and straightforward, and be
easily understandable in complete detail so that we can track bugs and extend
the system more easily.

### Goal : small and cool

I would love to have kogata fit on a 1.44MB floppy and run with a full GUI and
some cool apps (remember the QNX demo floppy!). Also, I want to be able to use
it on older computers and prove that such machines can still be put to use.

## How to build

### Requirements

* git for accessing the repository
* `i586-elf` cross-compiler built by [these scripts](http://adnab.me/cgit/cross-scripts.git/about/).
* nasm
* for testing, either qemu or bochs

### Building and running

To build, clone the repo somewhere and simply run:

    $ make

Launching qemu is also included in the makefile:

	$ make run_qemu

Warning: dependencies between portions of code are not necessarily well handled
by the makefile system. If you made changes and the OS fails miserably, try
doing a `make rebuild` before blaming your code.

### Runing the tests

The directory `src/tests/` contains a few tests for some of the core
components. The running of these tests is automated. To run the test suite,
simply invoke the corresponding make target (everything is rebuilt before
running the tests):

	$ make run_tests

## Structure of the project

### Modules

* The kernel
* Libraries
  * `libkogata` : basic system functionnality (memory allocator, mutexes, debugging)
  * `libc` : implementation of a (very restricted) subset of the standard C library, basically just
    the functions that were needed somewhere
  * `libalgo` : usefull data structures (hashtables, AVL trees, maybee more in the future)
* Userspace : not much work done yet

### Files in the repository

    doc/                  documentation (none yet, sorry!)
	src/
	src/kernel/           code for the kogata kernel
	src/common/           code shared between kernel and userspace libs
	src/lib/              code for userspace libraries
	src/apps/             userspace binaries
	src/tests/            test suite

## Roadmap

### Plans for soon

* Implement syscalls
* Write device drivers : VGA, keyboard, ATA, FAT, VESA

### Things to design

* How does a process exit, what does it do, how do processes synchronize ?
* Timers, workers, sleeping
* Have several threads in a single process
* Better handling of errors (rather than panicing) ; userspace apps should not
  have the possibility of crashing the system
* How does a process transmit information (such as environment or arguments) to its children ?

### Things not sure

* VFS thread safety : is the design correct ? (probably)

### Plans for the later future

* Module system for extending the kernel
* In userspace, simple Scheme-like scripting language
* The obvious stuff ;-)

## Licence

None of the source files have a licence header because it's cumbersome. All the
code and associated documentation found in this repository is released under
the ISC licence as detailed in the `COPYING` file.

