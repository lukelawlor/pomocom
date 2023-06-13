/*
 * state.cc contains the initializer for the global state object, pomocom::state.
 */

#include <cstdlib>	// For std::getenv()
#include <cstring>	// For strdup()
#include <string>

#include "error.hh"
#include "exceptions.hh"
#include "settings.hh"
#include "state.hh"

namespace pomocom
{
	// Initializer for global state
	ProgramState state = {
		.settings = {
			.interface = INTERFACE_ANSI,
			.update_interval = 1,
			.show_controls = false,
			.breaks_until_long_reset = 3,
			// paths left uninitialized because they're initialized by set_paths()
		},
		.current_section = SECTION_WORK,
		.breaks_until_long = 3,
		// section_info left uninitialized because it will be set when files are read
	};

	// Initializes program settings paths by modifying state.settings.paths
	void set_paths()
	{
		// Alias for state.settings.paths
		auto &paths = state.settings.paths;

		// Path to home
		char *path_home;

		// Get path to home based on OS
#ifdef	__linux__
		path_home = std::getenv("HOME");
#else
#error "compilation target platform unknown"
#endif

		// Buffer used for manipulating strings
		std::string buf("");

		// Set path_config
		buf += path_home;
		buf += "/.config/pomocom/";
		paths.config = strdup(buf.c_str());
		if (paths.config == nullptr)
		{
			PERR("failed to allocate mem for config file path");
			throw EXCEPT_BAD_ALLOC;
		}
		
		// Set other paths which are the same as the config path for now
		paths.bin = paths.section = paths.config;
	}
}
