#pragma once

#include <thread.h>
#include <mutex.h>

void start_workers(int num_worker_threads);		// default : one is enough

bool worker_push(entry_t fun, void* data);
bool worker_push_in(int usecs, entry_t fun, void* data);

void worker_notify_time(int usecs);		// time source : PIT IRQ0

uint64_t worker_get_time();				// usecs since we started some worker threads, ie since kernel startup

/* vim: set ts=4 sw=4 tw=0 noet :*/
