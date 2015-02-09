#pragma once

// Things described in this file are essentially a public interface
// All implementation details are hidden in process.c

#include <thread.h>

#include <hashtbl.h>
#include <buffer.h>

#define PW_NOT_WAITING		0
#define PW_WAIT_ANY_MSG		1
#define PW_WAIT_MSG_ON_CHAN	2

#define PROCESS_MAILBOX_SIZE 42

typedef int chan_id_t;

typedef struct chan_pair {
	chan_id_t fst, snd;
} chan_pair_t;

typedef struct message {
	buffer_t *data;
	chan_id_t chan_id;
} message_t;

struct process;
typedef struct process process_t;

process_t *new_process(entry_t entry, void* data, chan_pair_t *give_chans);

chan_pair_t new_chan();		// not used very often, but still usefull
chan_id_t unbox_chan(chan_id_t chan, chan_id_t subchan);
void detach_chan(chan_id_t chan);		// chan ID is freed

int send_message(chan_id_t chan, buffer_t *msg);	// nonnull on error (recipient queue is full)

size_t await_message();		// returns the size of the first message to come
size_t await_message_on_chan(chan_id_t chan);		// returns the size of the first message to come
message_t get_message();	// gets the first message in the queue (or nothing when queue is empty)

/* vim: set ts=4 sw=4 tw=0 noet :*/
