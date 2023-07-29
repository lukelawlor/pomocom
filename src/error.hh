/*
 * error.hh contains the PERR() macro for printing error messages, which takes the same arguments as printf(). This file also contains exception codes.
 */

#pragma once

#include <cstdio>	// For std::fprintf()

// If defined, print errors
#define	POMOCOM_PRINT_ERRORS

// Macro that expands to nothing
#define	POMOCOM_NOTHING(...)	do{}while(0)

#define	POMOCOM_OUTPUT_PREFIX	"pomocom: "

#ifdef	POMOCOM_PRINT_ERRORS
	#define	PERR(...)	do{\
				::pomocom::print_error_start();\
				::std::fprintf(stderr, __FILE__ ":%d: ", __LINE__);\
				::std::fprintf(stderr, __VA_ARGS__);\
				::std::fputc('\n', stderr);\
				}while(0)
#else
	#define	PERR(...)	POMOCOM_NOTHING()
#endif

namespace pomocom
{
	// Generic exception codes
	enum Exception{
		EXCEPT_GENERIC,

		// Input/output error
		EXCEPT_IO,

		// Bad memory allocation
		EXCEPT_BAD_ALLOC,

		// Buffer overrun was stopped
		EXCEPT_OVERRUN,

		// Invalid setting
		EXCEPT_BAD_SETTING,
	};

	// Prints the start of an error message
	void print_error_start();
}
