#include <mutex.h>
#include <process.h>

typedef struct process {
	pagedir_t *pd;

	thread_t *thread;

	int pid, ppid;
} process_t;


/* vim: set ts=4 sw=4 tw=0 noet :*/
