# Roadmap

## Plans for soon

* Write device drivers : keyboard, FAT (or EXT2 ?)
* Work on userland : init, terminal emulator, shell
* GUI server with help from kernel for framebuffer management

## Things to design

* Reclaiming physical memory :
  - Freeing some cached stuff, ie swapping pages from mmap regions
  - Swapping pages from processes non-mmap regions (ie real data regions)
* Better handling of errors (rather than panicing) ; userspace apps should not
  have the possibility of crashing the system
* How does a process transmit information (such as environment, arguments, file
  descriptors) to its children ?
* How do we do stream-like IO on files ? (ie how do we implement the append
* access mode
  and non-position-dependant read/write calls & seek call)
* Better error reporting for system calls that fail

## Things not sure

* VFS thread safety : is the design correct ? (probably)
* Cache architecture (called *pager*, after 4.4BSD terminology)
* Not enough tests!

## Plans for the later future

* Define a format for "packages", i.e. read-only filesystem images used for
* adding system
  components or apps to the OS ; make sure to implement it in a way that does
  not waste memory
* Module system for extending the kernel In userspace, simple Scheme-like
* scripting language POSIX compatibility layer, self-hosting OS The obvious
* stuff ;-)

