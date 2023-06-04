/*
 * error.cc contains functions for printing error messages.
 */

#include <cstdio>

#include "error.hh"

namespace pomocom
{
	// Prints the start of an error message
	void print_error_start()
	{
		std::fputs(POMOCOM_OUTPUT_PREFIX "error: ", stderr);
	}
}
