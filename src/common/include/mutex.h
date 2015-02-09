#pragma once

#include <stdint.h>

#define MUTEX_LOCKED 1
#define MUTEX_UNLOCKED 0


typedef uint32_t mutex_t;

void mutex_lock(mutex_t* mutex);	//wait for mutex to be free
int mutex_try_lock(mutex_t* mutex);	//lock mutex only if free, returns !0 if locked, 0 if was busy
void mutex_unlock(mutex_t* mutex);

// the mutex code assumes a yield() function is defined somewhere
void yield();

#define STATIC_MUTEX(name) static mutex_t name __attribute__((section("locks"))) = MUTEX_UNLOCKED;
	

/* vim: set ts=4 sw=4 tw=0 noet :*/
