/*
 * state.hh contains the global program state struct & extern declaration
 */

#pragma once

#include "pomocom.hh"	// For Section and SectionInfo
#include "settings.hh"	// For ProgramSettings

namespace pomocom
{
	// Global state
	struct ProgramState{
		ProgramSettings settings;

		// Current section being timed
		Section current_section;

		// # of breaks left until long break
		int breaks_until_long;

		// C string containing the name of the pomo file opened
		const char *file_name;

		SectionInfo section_info[SECTION_MAX];
	};

	extern ProgramState state;

	// Initializes program settings paths by modifying state.settings.paths
	void set_paths();
}
