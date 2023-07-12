/*
 * fileio.cc contains functions for handling input & output with libc FILE pointers.
 */

#include <cstdio>	// For std::FILE and std::fgetc()

#include "error.hh"
#include "fileio.hh"

namespace pomocom
{
	// Takes the same arguments as std::fopen()
	SmartFilePtr::SmartFilePtr(const char *path, const char *mode)
	{
		m_fp = std::fopen(path, mode);
		if (m_fp == nullptr)
		{
			PERR("failed to open file \"%s\"", path);
			throw EXCEPT_IO;
		}
#ifdef	POMOCOM_PRINT_ERRORS
		m_path = path;
#endif
	}

	// Calls std::fclose(m_fp)
	SmartFilePtr::~SmartFilePtr()
	{
#ifdef	POMOCOM_PRINT_ERRORS
		if (std::fclose(m_fp))
			PERR("failed to close file \"%s\"", m_path);
#else
		std::fclose(m_fp);
#endif
	}

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
