#pragma once

#include <idt.h>
#include <proto/syscall.h>

void setup_syscall_table();

void syscall_handler(registers_t *regs);

/* vim: set ts=4 sw=4 tw=0 noet :*/
