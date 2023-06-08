/*
 * error.hh contains functions for printing error messages.
 */

// For fprintf()
#include <cstdio>

// If defined, print errors
#define	POMOCOM_PRINT_ERRORS

// Macro that expands to nothing
#define	POMOCOM_NOTHING(...)	do{}while(0)

#define	POMOCOM_OUTPUT_PREFIX	"pomocom: "

#ifdef	POMOCOM_PRINT_ERRORS
	#define	PERR(...)	::pomocom::print_error_start();\
				::std::fprintf(stderr, __FILE__ ":%d: ", __LINE__);\
				::std::fprintf(stderr, __VA_ARGS__);\
				::std::fputc('\n', stderr)
#else
	#define	PERR(...)	POMOCOM_NOTHING()
#endif

namespace pomocom
{
	// Prints the start of an error message
	void print_error_start();
}