Extracted from the [NARP protocol specification v2](http://adnab.me/cgi-bin/cgit.cgi/NARP.git/plain/doc/spec-2.html) :

### Architecture of the OS

The basic primitive of the system being message-passing, the system looks a lot like a micro-kernel. Only the message format has a complex semantic and the communication layer is not really “simple”. Furthermore, the system has device drivers, file system and networking running as kernel-mode processes, making the kernel more monolithic (but still having a micro-kernel spirit). It should be easy to make any user mode process run as a kernel mode process instead, for the sake of performance (eg : graphical server & compositor).

The kernel land is divided in three major parts, with strict dependency order:

- Level 0 : System ressource managment : physical memory, virtual memory, hardware interaction (IRQ, v86), debug output

- Level 1 : Scheduler, IPC & NARP core server : builds on top of level 0, adds support for processes and communication between them restricted to NARP protocol data.

- Level 2 : System processes : hardware, file systems, network, … (may access level 0 and level 1 features)

User processes are restricted to syscalls that call level 1 primitives.

Here are a few basic principles for the design of these three levels :

- Level 2 processes may not communicate directly nor share memory : they must go through level 0 and level 1 primitives to achieve such a goal. Each level 2 process has a separate heap, which is completely freed when the process dies. Level 2 processes do not use separate virtual memory spaces : since the kernel memory space is mapped in all page directories, a level 2 process may run with any page directory.

- Benefits : critical system parts are restricted to level 0 and level 1. Level 2 components may leak or crash with less consequences.

  All synchronization & locking is handle by level 1, except for level 0 that must implement its own locking devices (since it cannot rely on level 1).

- Benefits : no complex synchronization in most of the code (which is either level 2 or userland), only simple message passing and waiting for stuff to happen

- No concept of “threads” : system processes are actually kernel threads, but we call them processes since they use separate parts of memory. Userlands processes cannot spawn multiple threads of execution either : they must fork and communicate through NARP if they want to do so (eg: launching an expensive communication in the background).

    (since fork is a complicated system call, and features such as copy-on-write depend on processes using different paging directories, the fork system call is accessible only to userland processes : level 2 processes may not fork, but only create new processes)

- Level 1 also has a memory heap ; it is used with `core_malloc/core_free`. Level 2 proceses use standard malloc/free, which are modified to act on the heap of the current process.

- Each process (system or user) has a mailbox, ie a queue of incoming NARP messages waiting to be transferred. The mailbox has a maximum size (buffer size), and a send call may fail with a no space left in queue error. This is the only possible failure for a send call.

  System processes (level 2) spend most of their time in waiting mode ; they may be waked up by either recieving a NARP messsage or by a hardware event. Therefore the `wait_for_event` function that composes the main loop may return either : a message was recieved or a system event happenned. If the reason is a message was recieved, the process is free not to read the message immediately.

  On the other hand, user processes can wait for only one thing : recieving a NARP message. Each user process has a message zone in its memory space, and the wait for message function just copies the first message of the mailbox into this zone (overwriting whatever was there before) and returns control to the process (returning the length of the message).

- Handling of IRQs : some hardware stuff requires action as soon as the interrupt is fired, therefore a specifi IRQ handler may be used. Such a handler must do as little as possible, and when it is done signal level 1 that an IRQ has happenned (it may add specific data to the “IRQ happenned” message). Level 1 adds a message to the queue of the recipient process (if there is one) and returns immediately : the IRQ handler must leave as soon as possible. An IRQ is handled on whatever stack is currently used, and the IF flag is constantly off while the IRQ handler is running. The timer IRQ is the only one that behaves differently, since it has to trigger a task switch.

### Steps of the developpment of the OS

1. Develop level 0 completely and with cleanest possible design

2. Develop level 1 with only basic funcionnality

3. Develop some basic applications in level 2 : display, keyboard, mini kernel shell, mini file system, …

4. Improve level 1 with more complex stuff ; try to quickly attain a complete level 1

5. Work on the rest of the stuff

