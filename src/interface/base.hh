/*
 * base.hh contains functions that handle base pomodoro functionality and are called in interface code.
 */

#include <string>	// For std::string_view

#include "../pomocom.hh"	// For Section

namespace pomocom
{
	// Handles switching to the next timing section after one finishes
	void base_next_section();

	// Sets the terminal title to a countdown timer
	void base_set_terminal_title_countdown(int mins, int secs, std::string_view section_name);
}
