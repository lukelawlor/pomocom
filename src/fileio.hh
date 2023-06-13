/*
 * fileio.hh contains functions for handling input & output with libc FILE pointers.
 */

#pragma once

#include <cstdio>	// For std::FILE

#include "exceptions.hh"

namespace pomocom
{
	// Function from SoupDL 06 (spdl)
	// Writes chars from *stream (including \0) into *dest
	// Stops when the delim character is found, and doesn't include the delim in the string
	void spdl_readstr(char *dest, const int len_max, const int delim, std::FILE *stream);
}
