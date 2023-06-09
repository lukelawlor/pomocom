/*
 * base.cc contains functions that handle base pomodoro functionality and are called in interface code.
 */

#include <cstdlib>		// For std::system()

#include "../error.hh"
#include "../pomocom.hh"	// For Section
#include "../state.hh"
#include "base.hh"

namespace pomocom
{
	// Used to switch sections in interface code
	static void base_switch_section(Section new_section);

	// Handles switching to the next timing section after one finishes
	void base_next_section()
	{
		if (state.current_section == SECTION_WORK)
		{
			if (state.breaks_until_long <= 0)
			{
				state.breaks_until_long = state.settings.breaks_until_long_reset;
				base_switch_section(SECTION_BREAK_LONG);
			}
			else
			{
				--state.breaks_until_long;
				base_switch_section(SECTION_BREAK);
			}
		}
		else
			base_switch_section(SECTION_WORK);
	}

	// Used to switch sections in interface code
	static void base_switch_section(Section new_section)
	{
		// Change section
		state.current_section = new_section;

		// Call section command
		char *cmd = state.section_info[new_section].cmd;
		int exit_code = std::system(cmd);
		if (exit_code)
			PERR("section command \"%s\" exited with nonzero exit code %d", cmd, exit_code);
	}
}
