/*
 * fileio.hh contains functions for handling input & output with libc FILE pointers.
 */

#pragma once

#include <cstdio>	// For std::FILE, std::fopen(), and std::fclose()

#include "error.hh"
#include "exceptions.hh"

namespace pomocom
{
	// Smart pointer for std::FILE pointers
	struct SmartFilePtr{
		std::FILE *m_fp;

#ifdef	POMOCOM_PRINT_ERRORS
		// Used to print the path of the file in error messages
		const char *m_path;
#endif

		// Takes the same arguments as std::fopen()
		SmartFilePtr(const char *path, const char *mode);

		// Calls std::fclose(m_fp)
		~SmartFilePtr();
	};

	// Function from SoupDL 06 (spdl)
	// Writes chars from *stream (including \0) into *dest
	// Stops when the delim character is found, and doesn't include the delim in the string
	void spdl_readstr(char *dest, const int len_max, const int delim, std::FILE *stream);
}
