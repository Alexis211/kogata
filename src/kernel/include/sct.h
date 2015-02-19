#pragma once

#include <idt.h>
#include <syscallproto.h>

void setup_syscall_table();

void syscall_handler(registers_t *regs);

/* vim: set ts=4 sw=4 tw=0 noet :*/
