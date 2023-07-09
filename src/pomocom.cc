/*
 * pomocom.cc contains the main function and section and timing code.
 */

#include <chrono>	// For sleeping
#include <cstdlib>	// For std::time()
#include <cstring>	// For std::strcpy() and std::strlen()
#include <cstdio>	// For std::FILE and std::fopen
#include <iostream>
#include <thread>	// For sleeping

#include <ncurses.h>

#include "ansi_term.hh"
#include "error.hh"
#include "exceptions.hh"
#include "fileio.hh"
#include "pomocom.hh"
#include "state.hh"

namespace pomocom
{
	// Reads sections from the file at *path where *path is altered
	static void read_sections(const char *path);

	// Reads sections from the file at *path where *path is unaltered
	static void read_sections_raw(const char *path);

	// Used to switch sections in interface code
	static void base_switch_section(Section new_section);

	// Handles switching to the next timing section after one finishes
	static void base_next_section();
}

int main(int argc, char **argv)
{
	namespace chrono = std::chrono;
	using namespace pomocom;

	// Init pomocom
	try
	{
		// Read pomocom.conf
		settings_read(state.settings);

		// Set state values
		state.current_section = SECTION_WORK;
		state.breaks_until_long = state.settings.breaks_until_long_reset;
		state.file_name = nullptr;

		// Read command line args
		if (argc == 1)
			read_sections("standard");
		else if (argc == 2)
			read_sections(argv[1]);
		else
		{
			PERR("wrong number of args provided");
			throw EXCEPT_GENERIC;
		}

		// Check for valid card data
		for (SectionInfo &s : state.section_info)
		{
			if (s.secs <= 0)
			{
				PERR("invalid card data found");
				throw EXCEPT_GENERIC;
			}
		}
	}
	catch (...)
	{
		// Don't print an error message because one should already have printed
		return 1;
	}

	// Use the specified interface
	switch (state.settings.interface)
	{
	case INTERFACE_ANSI:
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
					std::this_thread::sleep_for(chrono::seconds(state.settings.update_interval));
				}

				base_next_section();
			}
		}
		break;
	case INTERFACE_NCURSES:
		{
			// Color pair ids start at 1 because 0 is unchangeable after the call to use_default_colors()
			enum ColorPair{
				CP_POMOCOM = 1,
				CP_SECTION_WORK,
				CP_SECTION_BREAK,
				CP_TIME,
			};

			try
			{
				// Init ncurses
				if (initscr() == nullptr)
				{
					PERR("failed to initialize ncurses");
					throw EXCEPT_GENERIC;
				}

				// Config input
				if (cbreak() == ERR)
				{
					PERR("ncurses cbreak() call failed");
					throw EXCEPT_GENERIC;
				}
				if (noecho() == ERR)
				{
					PERR("ncurses noecho() call failed");
					throw EXCEPT_GENERIC;
				}

				// Init colors
				if (start_color() == ERR)
				{
					PERR("failed to init ncurses colors");
					throw EXCEPT_GENERIC;
				}
				if (use_default_colors() == ERR)
				{
					PERR("failed to use default ncurses colors");
					throw EXCEPT_GENERIC;
				}

				// Try to call init_pair()
				// On error, print an error message and throw an exception
				#define TRY_INIT_PAIR(n, fg, bg)	do{ if (init_pair(n, fg, bg) == ERR){ PERR("failed to init color pair"); throw EXCEPT_GENERIC; }}while(0)

				// Set colors
				TRY_INIT_PAIR(CP_POMOCOM, COLOR_BLUE, -1);
				TRY_INIT_PAIR(CP_SECTION_WORK, COLOR_YELLOW, -1);
				TRY_INIT_PAIR(CP_SECTION_BREAK, COLOR_GREEN, -1);
				TRY_INIT_PAIR(CP_TIME, -1, -1);
			}
			catch (...)
			{
				// Don't print an error message because one should already have printed
				return 1;
			}

			// Store update interval in milliseconds
			chrono::milliseconds update_interval(1000 * state.settings.update_interval);

			// Alias for clock type
			using Clock = chrono::high_resolution_clock;

			// Start and end time points of timing section
			chrono::time_point<Clock> time_start, time_current, time_end;

			// Repeatedly move through timing sections
			for (;;)
			{
				// Reference to info on the current section
				SectionInfo &si = state.section_info[state.current_section];

				// Pause before starting the section
				if (state.settings.pause_before_section_start)
				{
					// Print info about the upcoming section
					clear();
					attron(COLOR_PAIR(CP_POMOCOM));
					printw("pomocom: %s\n", state.file_name);
					attron(COLOR_PAIR(state.current_section == SECTION_WORK ? CP_SECTION_WORK : CP_SECTION_BREAK));
					printw("next up: %s (%dm%ds)\n", si.name, si.secs / 60, si.secs % 60);
					attron(COLOR_PAIR(CP_TIME));
					printw("press %c to begin.", state.settings.keys.section_begin);
					refresh();

					// Make getch() wait for input before returning
					timeout(-1);

					// Wait until the section begin key is pressed
					while (getch() != state.settings.keys.section_begin)
						;
				}

				// Clear the screen
				clear();
				
				// Print pomocom & pomo file name
				attron(COLOR_PAIR(CP_POMOCOM));
				printw("pomocom: %s\n", state.file_name);

				// Print section name
				attron(COLOR_PAIR(state.current_section == SECTION_WORK ? CP_SECTION_WORK : CP_SECTION_BREAK));
				printw("%s\n", si.name);

				// Set the color for the time
				attron(COLOR_PAIR(CP_TIME));

				// Start the timing section
				time_start = Clock::now();
				time_end = time_start + chrono::seconds(si.secs);

				// Repeatedly update the screen and check for input until section time is over
				while ((time_current = Clock::now()) < time_end)
				{
					// Print the time left in the section
					chrono::seconds time_left = chrono::ceil<chrono::seconds>(time_end - time_current);
					int mins = time_left.count() / 60;
					int secs = time_left.count() % 60;

					// Move to the area of the screen where the time left is displayed
					move(2, 0);

					// Print time left
					clrtobot();
					printw("%dm %ds", mins, secs);
					refresh();

					// Get user input
					auto &keys = state.settings.keys;
					auto time_input_start = Clock::now();

					// Expect to wait update_interval milliseconds for getch() to return
					timeout(update_interval.count());

				l_get_user_input:
					int c = getch();
					if (c == keys.pause)
					{
						// Pause

						// Print pause text
						addstr(" (paused)");
						refresh();

						auto time_pause_start = Clock::now();

						// Make getch() wait for input before returning
						timeout(-1);

						// Wait until pause is pressed again to trigger an unpause
						while (getch() != keys.pause)
							;
						
						// Unpause
						
						// Extend time_end to include the time spent paused
						time_end += Clock::now() - time_pause_start;

						// Go back to updating the screen
						continue;
					}
					else if (c == keys.section_skip)
					{
						// Skip to next section by exiting the loop
						break;
					}
					else if (c == ERR)
					{
						// If getch() returns ERR, the user did not input anything, so we don't need to wait any longer until the time remaining can be reprinted
						continue;
					}
					else
					{
						// The user input an unrecognized key before getch() returned ERR

						// Recalculate the time until the next screen update
						chrono::milliseconds time_until_screen_update = update_interval - chrono::duration_cast<chrono::milliseconds>(Clock::now() - time_input_start);

						if (time_until_screen_update.count() <= 0)
						{
							// A screen update should happen now
							continue;
						}
						else
						{
							// Reset timeout to reflect the change to the time until the next screen update
							timeout(time_until_screen_update.count());

							// Try to get more user input before the next screen update
							goto l_get_user_input;
						}
					}
				}
				base_next_section();
			}
		}

		// Quit ncurses
		endwin();
		break;
	default:
		PERR("unknown interface");
		return 1;
	}

	return 0;
}

namespace pomocom
{
	// Reads sections from the file at *path where *path is unaltered
	static void read_sections_raw(const char *path)
	{
		SmartFilePtr sfp(path, "r");
		auto &fp = sfp.m_fp;

		// Get section data
		for (SectionInfo &s : state.section_info)
		{
			// Read in section name
			try { spdl_readstr(s.name, SECTION_INFO_NAME_LEN, '\n', fp); }
			catch (Exception &e)
			{
				if (e == EXCEPT_OVERRUN)
				{
					PERR("max chars read for section info name (over %d)", SECTION_INFO_NAME_LEN - 1);
					throw e;
				}
			}

			// Read in section command
			try
			{
				int c = fgetc(fp);
				if (c == '+')
				{
					// The program run in the command is in pomocom's bin directory

					// Used to construct the proper path to the script
					std::string buf(state.settings.paths.bin);

					// Add the rest of the command name to buf
					// The max # of chars to read here is shortened because s.cmd needs to contain the path to the script directory
					spdl_readstr(s.cmd, SECTION_INFO_CMD_LEN - buf.size(), '\n', fp);
					buf += s.cmd;

					// Copy the final command from buf to s.cmd
					std::strcpy(s.cmd, buf.c_str());
				}
				else
				{
					// The program run in the command is in the user's $PATH
					ungetc(c, fp);
					spdl_readstr(s.cmd, SECTION_INFO_CMD_LEN, '\n', fp);
				}
			}
			catch (Exception &e)
			{
				if (e == EXCEPT_OVERRUN)
				{
					PERR("max chars read for section info command (over %d)", SECTION_INFO_CMD_LEN - 1);
					throw e;
				}
			}

			// Read in section duration
			int minutes, seconds;
			if (std::fscanf(fp, "%dm%ds\n", &minutes, &seconds) != 2)
				throw EXCEPT_IO;
			s.secs = minutes * 60 + seconds;
		}
	}

	// Reads sections from the file at *path where *path is altered
	static void read_sections(const char *path)
	{
		state.file_name = path;

		// Altering the path
		std::string alt_path("");
		if (std::strlen(path) >= 2 && path[0] == '.' && path[1] == '/')
		{
			// Path is relative
			alt_path += (path + 2);
		}
		else
		{
			// Path is absolute
			alt_path += state.settings.paths.section;
			alt_path += path;
		}
		alt_path += ".pomo";

		// Actually loading the section data with the altered path
		read_sections_raw(alt_path.c_str());
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

	// Handles switching to the next timing section after one finishes
	static void base_next_section()
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
}
