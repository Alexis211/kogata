#pragma once
#include <stddef.h>

int chdir(const char* path);

char* getcwd(char* buf, size_t buf_len);

char* pathncat(char* buf, const char* add, size_t buf_len);		// path simplification

/* vim: set ts=4 sw=4 tw=0 noet :*/
