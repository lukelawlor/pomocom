/*
 * state.hh contains the global program state struct & extern declaration
 */

#pragma once

#include "pomocom.hh"
#include "settings.hh"

namespace pomocom
{
	// Global state
	struct ProgramState{
		ProgramSettings settings;
		int breaks_until_long;
		SectionInfo section_info[SECTION_MAX];
	};

	extern ProgramState state;

	// Initializes program settings paths by modifying state.settings.paths
	void set_paths();
}
