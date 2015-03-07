# kogata operating system project

kogata operating system : small and <del>beautiful</del>small.

## Project characteristics

### Written in C

I have considered using higher-level languages but for lack of understanding of
the associated compiler, I never have had the same impression of understanding
exactly what was happening that I had with C. Also, the necessity of a runtime
is bothering. I do regret not being able to exploit the magic of strongly typed
functional languages, but what can I do...

### Monolithic design

I would have liked to have more of a client/server architecture
(microkernel-like), but the necessity for asynchronous communication makes it
much more difficult to code, and contradicts the goal of having simple and
straightforward code.

### Emphasis on code quality and reliability

The code for the project must make sense, be simple and straightforward, and be
easily understandable in complete detail so that we can track bugs and extend
the system more easily.

### Capability-like security system

A normal ring-3 application managed by a ring-0 kernel is a bit like a virtual
machine in which the process runs : it has a full memory space and doesn't see
it when it is interrupted by other things happening on the system. We take this
a bit further by saying that a process that creates a child process creates a
"box" in which some of the resources of the parent can be made accessible,
possibly with some restrictions. In particular this is true of filesystems :
each process has its own filesystem namespace. Basically it means that the
login manager has full access to all disk devices and system hardware, the
session manager for a user session only has access to the user's data and
read-only access to system files, and an untrusted user application can be
sandboxed in an environment where it will only see its own data and necessary
libraries, with "bridges" enabling access to user-approved data (for instance a
file chooser, or taking a picture with a webcam or such). Also when a process
has a child, it is seen as sharing part of it's computing resources with the
child, therefore when a process is terminated, all its children are terminated
as well.

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

To build, clone the git repository somewhere and simply run:

    $ make

Launching qemu is also included in the makefile:

	$ make run_qemu

Warning: dependencies between portions of code are not necessarily well handled
by the makefile system. If you made changes and the OS fails miserably, try
doing a `make rebuild` before blaming your code.

### Running the tests

The directory `src/tests/` contains a few tests for some of the core
components. The running of these tests is automated. To run the test suite,
simply invoke the corresponding make target (everything is rebuilt before
running the tests):

	$ make run_tests

## Structure of the project

### Modules

* The kernel
* Libraries
  * `libkogata` : basic system functionality (memory allocator, mutexes, debugging)
  * `libc` : implementation of a (very restricted) subset of the standard C library, basically just
    the functions that were needed somewhere
  * `libalgo` : useful data structures (hashtables, AVL trees, maybe more in the future)
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

* Complete process management (implement process exiting, process deletion, waiting, ...)
* Implement missing syscalls
* Write device drivers : VGA, keyboard, FAT (or EXT2 ?), VESA
* Work on userland
* Define a format for "packages", i.e. read-only filesystem images used for adding system
  components or apps to the OS ; make sure to implement it in a way that does not waste
  memory
* IPC : sockets and pipes (both anonymous)
* GUI server with help from kernel for framebuffer management

### Things to design

* Cache architecture :
  - A RAM file is basically a bunch of pages that contain the data (rather than a segment of
    malloc()'ed memory)
  - A framebuffer device is basically a bunch of pages at a fixed hardware location, that cannot
    grow
  - In the two previous cases, the pages are never freed : they are not a cache, they are the
    data itself.
  - In the case of an on-disk file, the pages are a copy of the file's data that is originally
    on-disk, and we might want to free these pages even while a process is using them, because
    we can always reload them from the disk.
  - An on-disk file with a page cache must be aware of all the places where the page is mapped,
    so that it can unmap it when reclaiming pages.
* Reclaiming physical memory :
  - Freeing some cached stuff, ie swapping pages from mmap regions
  - Swapping pages from processes non-mmap regions (ie real data regions)
* Better handling of errors (rather than panicing) ; userspace apps should not
  have the possibility of crashing the system
* How does a process transmit information (such as environment, arguments, file
  descriptors) to its children ?
* How do we do stream-like IO on files ? (ie how do we implement the append access mode
  and non-position-dependant read/write calls & seek call)
* Better error reporting for system calls that fail

### Things not sure

* VFS thread safety : is the design correct ? (probably)

### Plans for the later future

* Module system for extending the kernel
* In userspace, simple Scheme-like scripting language
* POSIX compatibility layer, self-hosting OS
* The obvious stuff ;-)

## Licence

None of the source files have a licence header because it's cumbersome. All the
code and associated documentation found in this repository is released under
the ISC licence as detailed in the `COPYING` file. Some parts of the code are very
directly inspired from examples found on the OSDev wiki, thank you all!

