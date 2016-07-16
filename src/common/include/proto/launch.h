#pragma once

// 	Standard file descriptor numbers
//	If these FDs are mapped when a process is launched, a specific role for them is assumed

// {{ Automatically causes libc stdio initialization

// tty_stdio : assume we have a terminal window for keyboard I/O
#define STD_FD_TTY_STDIO 1

// if stdin is mapped, we assume no terminal window
#define STD_FD_STDIN	2
#define STD_FD_STDOUT	3
#define STD_FD_STDERR	4

// }}


#define STD_FD_GIP 	5		// graphics initiation protocol, i.e. GUI window

// For system services
#define STD_FD_GIOSRV 10

#define STD_FD_TTYSRV 11

/* vim: set sts=0 ts=4 sw=4 tw=0 noet :*/
