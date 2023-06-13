/*
 * fileio.cc contains functions for handling input & output with libc FILE pointers.
 */

#include <cstdio>	// For std::FILE

#include "exceptions.hh"
#include "fileio.hh"

namespace pomocom
{
	// Function from SoupDL 06 (spdl)
	// Writes chars from *stream (including \0) into *dest
	// Stops when the delim character is found, and doesn't include the delim in the string
	void spdl_readstr(char *dest, const int len_max, const int delim, std::FILE *stream)
	{
		// Index of *dest to access next
		int i = 0;

		// Current character being processed
		int c;

		// Reading loop
		for (;;)
		{
			c = std::fgetc(stream);
			if (c == delim)
			{
				dest[i] = '\0';
				break;
			}
			if (c == EOF)
			{
				// Reading error
				throw EXCEPT_IO;
			}
			dest[i] = c;
			if (++i == len_max - 1)
			{
				// The max amount of chars was read and the end of the string was not found
				dest[i] = '\0';
				throw EXCEPT_OVERRUN;
			}
		}
	}
}
