#include <mutex.h>
#include <process.h>

typedef struct process {
	thread_t *thread;
	int pid;

	mutex_t com_mutex;

	hashtbl_t *chans;
	chan_id_t next_chan_id;

	message_t mbox[PROCESS_MAILBOX_SIZE];
	int mbox_size;	// number of messages in queue
	int mbox_first;	// first message in queue (circular buffer)

	// a process can be in several waiting states :
	// - wait for any message
	// - wait for a message on a particular channel
	//   in this case, if a message is pushed on this particular channel,
	//	 then it is put in front of the queue, so that it is the next message read
	//	 (it is guaranteed that doing await_message_on_chan() immediately followed by get_message()
	//   gets the message whose size was returned by await_...)
	int wait_state;
	chan_id_t wait_chan_id;		// when waiting for a message on a particular channel
} process_t;

int push_message(process_t *proc, message_t msg);		// nonnull on error (process queue is full)

/* vim: set ts=4 sw=4 tw=0 noet :*/
