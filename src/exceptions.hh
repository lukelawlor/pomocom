/*
 * exceptions.hh contains generic exception codes.
 */

#pragma once

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
	};
}
