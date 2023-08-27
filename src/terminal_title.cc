/*
 * terminal_title.hh contains a function for setting the terminal window title
 */

#include <iostream>

#include "terminal_title.hh"

namespace pomocom
{
	// Sets the terminal title to *str
	void set_terminal_title(std::string_view str)
	{
		std::cout << "\033]0;" << str << "\007" << std::flush;
	}
}
