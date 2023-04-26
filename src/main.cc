/*
 * main.cc does everything.
 */

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "ansi_term.hh"

namespace pomocom
{
	// Timing sections
	enum Section{
		SECTION_WORK,
		SECTION_BREAK,
		SECTION_BREAK_LONG,
		SECTION_MAX,
	};

	// Info on each timing section
	struct SectionInfo{
		const char *name;
		const char *cmd;
		int secs;
	};

	// Program interface types
	enum ProgramInterface{
		// Interface that uses ANSI terminal escape codes
		INTERFACE_ANSI,

		// Interface using the ncurses library
		INTERFACE_NCURSES,
	};

	// Program settings
	struct ProgramSettings{
		ProgramInterface interface;

		// # of seconds to wait between updating the screen
		int update_interval;

		bool show_controls : 1;

		// Number of breaks until a long break
		int breaks_until_long_reset;
	};

	// Global state
	struct ProgramState{
		ProgramSettings settings;
		int breaks_until_long;
		SectionInfo section_info[SECTION_MAX];
	} state = {
		.settings = {
			.interface = INTERFACE_ANSI,
			.update_interval = 1,
			.show_controls = false,
			.breaks_until_long_reset = 3,
		},
		.breaks_until_long = 3,
		.section_info = {
			{.name="work time", .cmd="echo 0", .secs=5},
			{.name="break time", .cmd="echo 1", .secs=5},
			{.name="long break", .cmd="echo 2", .secs=5},
		},
	};
}

int main()
{
	// Times of current timing section
	std::time_t time_start;
	std::time_t time_current;
	std::time_t time_end;

	using namespace pomocom;

	// Current time section 
	Section section = SECTION_WORK;

	// Breaks left until a long break
	switch (state.settings.interface)
	{
	case INTERFACE_ANSI:
		for (;;)
		{
			// Print the pomocom text
			std::cout << AT_CLEAR;
			std::cout << "pomocom:\n";
			
			// Print the section name
			std::cout << state.section_info[section].name << '\n';

			// Start the timing section
			time_start = std::time(nullptr);
			time_end = time_start + state.section_info[section].secs;
			while ((time_current = std::time(nullptr)) < time_end)
			{
				// Print the time remaining
				std::time_t time_left = time_end - time_current;
				int mins = time_left / 60;
				int secs = time_left % 60;
				std::cout << AT_CLEAR_LINE;
				std::cout << mins << "m " << secs << "s" << std::flush;
				std::this_thread::sleep_for(std::chrono::seconds(state.settings.update_interval));
			}

			// Switch to the next timing section
			#define	SWITCH_SECTION(new_section)	do{section = new_section;\
								std::system(state.section_info[new_section].cmd);}while(0)
			if (section == SECTION_WORK)
			{
				if (--state.breaks_until_long == 0)
				{
					state.breaks_until_long = state.settings.breaks_until_long_reset;
					SWITCH_SECTION(SECTION_BREAK_LONG);
				}
				else
					SWITCH_SECTION(SECTION_BREAK);
			}
			else
				SWITCH_SECTION(SECTION_WORK);
		}
		break;
	case INTERFACE_NCURSES:
		std::cerr << "ncurses interface not implemented yet\n";
		abort();
	default:
		std::cerr << "unknown interface\n";
		abort();
	}

	return 0;
}
