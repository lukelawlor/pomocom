/*
 * settings.hh contains the settings struct type.
 */

#pragma once

#include "pomocom.hh"

namespace pomocom
{
	// Program settings
	struct ProgramSettings{
		ProgramInterface interface;

		// # of seconds to wait between updating the screen
		int update_interval;

		bool show_controls : 1;
		bool pause_before_section_start : 1;

		// Number of breaks until a long break
		int breaks_until_long_reset;

		// Keyboard controls
		struct SettingsKeys{
			char pause;
			char section_begin;
			char section_skip;
		} keys;

		// Paths
		// These should all be C strings that end in with '/'
		struct SettingsPaths{
			// Path to directory where config files are stored
			char *config;

			// Path to directory that section files are stored in
			char *section;

			// Path to directory that script files are stored in
			char *bin;

		} paths;
	};
}
