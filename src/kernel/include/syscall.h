#pragma once

#include <idt.h>
#include <syscallid.h>

void setup_syscalls();

void syscall_handler(registers_t *regs);

/* vim: set ts=4 sw=4 tw=0 noet :*/
