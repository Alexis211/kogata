#pragma once

// TODO

typedef int sig_atomic_t;

#define SIG_DFL 0	// stupid

#define SIGINT 42	// stupid

void (*signal(int sig, void (*func)(int)))(int);



/* vim: set sts=0 ts=4 sw=4 tw=0 noet :*/
