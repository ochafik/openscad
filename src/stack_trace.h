#pragma once

#include <execinfo.h>
#include <iostream>
#include <unistd.h>

inline void printStackTrace(const std::string &title)
{
	void *buffer[5];
	int size = backtrace(buffer, 5);
	std::cerr << "[" << title << "] stack:\n";
	backtrace_symbols_fd(buffer, size, STDERR_FILENO);
}
