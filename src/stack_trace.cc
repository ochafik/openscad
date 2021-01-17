#pragma once

#include <cxxabi.h>
#include <execinfo.h>
#include <iostream>
#include <unistd.h>
#include "stack_trace.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>

#define MAX_STACK_ENTRIES 10
#define MAX_SYMBOL_LENGTH 160

// See http://charette.no-ip.com:81/programming/2010-01-25_Backtrace/
void printStackTrace(const std::string &title)
{
	void *buffer[MAX_STACK_ENTRIES];
	int size = backtrace(buffer, MAX_STACK_ENTRIES);
	std::cerr << "[" << title << "] stack:\n";

  char **ptr = backtrace_symbols(buffer, size);
  // Start at 1 to skip printStackTrace
  for (int i = 1; i < size; i++) {
    auto stack_info = ptr[i];
    // std::cerr << "\t" << stack_info << std::endl;//": ";

    std::vector<std::string> tokens;
    boost::split_regex(tokens, stack_info, boost::regex("\\s+"));
    // for (auto token : tokens) {
    //   std::cerr << "\t - '" << token << "'\n";
    // }
    const char* symbol = tokens[3].c_str();

    int status = -1;
    char *demangled_name = abi::__cxa_demangle(symbol, NULL, NULL, &status );
    std::string name = status ? symbol : demangled_name;
    std::cerr << "\t" << (name.length() > MAX_SYMBOL_LENGTH ? name.substr(0, MAX_SYMBOL_LENGTH) + "..." : name).c_str() << std::endl;
    free(demangled_name);
  }
  free(ptr);
	// backtrace_symbols_fd(buffer, size, STDERR_FILENO);
}
