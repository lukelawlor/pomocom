/*
 * ansi.cc contains functions for using the ANSI interface.
 */

#include <chrono>	// For sleeping
#include <cstdlib>	// For std::time_t and std::time()
#include <iostream>	// For std::cout
#include <thread>	// For sleeping

#include "../state.hh"
#include "../pomocom.hh"
#include "base.hh"

// Macros for using ANSI terminal escape codes

// Clear the screen
#define	AT_CLEAR	"\033[2J\033[0;0H"

// Clear the entire line that the cursor is on
#define	AT_CLEAR_LINE	"\r\033[K"

// Clear from the cursor to the end of the line the cursor is on
#define	AT_CLEAR_REST	"\033[K"

// Move the cursor up/down/left/right x times
#define	AT_CUR_UP(x)	"\033[" #x "A"
#define	AT_CUR_DOWN(x)	"\033[" #x "B"
#define	AT_CUR_LEFT(x)	"\033[" #x "D"
#define	AT_CUR_RIGHT(x)	"\033[" #x "C"

namespace pomocom
{
	// Runs the interface loop
	// This is only exited by a forced shutdown of the program (ex. when SIGINT is sent on posix)
	void interface_ansi_loop()
	{
		// Time points of current timing section
		std::time_t time_start;
		std::time_t time_current;
		std::time_t time_end;

		for (;;)
		{
			// Reference to info on the current section
			SectionInfo &si = state.section_info[state.current_section];

			// Print the pomocom text
			std::cout << AT_CLEAR;
			std::cout << "pomocom: " << state.file_name << '\n';
			
			// Print the section name
			std::cout << si.name << '\n';

			// Start the timing section
			time_start = std::time(nullptr);
			time_end = time_start + si.secs;

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

			base_next_section();
		}
	}
}
