/*
 * pomocom.cc contains the main function and section and timing code.
 */

#include <chrono>	// For sleeping for a specified duration
#include <cstdlib>	// For std::time()
#include <cstring>	// For std::strcpy() and std::strlen()
#include <cstdio>	// For std::FILE and std::fopen
#include <iostream>
#include <mutex>
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

	// Returns nonzero on error
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
	void switch_section(Section new_section)
	{
		// Change section
		state.current_section = new_section;

		// Call section command
		char *cmd = state.section_info[new_section].cmd;
		int exit_code = std::system(cmd);
		if (exit_code)
			PERR("section command \"%s\" exited with nonzero exit code %d", cmd, exit_code);
	}

	// Handles switching to the next section after a time section
	void base_next_section()
	{
		if (state.current_section == SECTION_WORK)
		{
			if (--state.breaks_until_long == 0)
			{
				state.breaks_until_long = state.settings.breaks_until_long_reset;
				switch_section(SECTION_BREAK_LONG);
			}
			else
				switch_section(SECTION_BREAK);
		}
		else
			switch_section(SECTION_WORK);
	}
}

int main(int argc, char **argv)
{
	using namespace pomocom;

	// Init pomocom
	try
	{
		// Set strings to paths that pomocom looks for files in
		set_paths();

		// Read in settings
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

	// Breaks left until a long break
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
					std::this_thread::sleep_for(std::chrono::seconds(state.settings.update_interval));
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

			// Set the maximum # of milliseconds to wait for input after getch() is called by calling timeout()
			std::chrono::milliseconds timeout_duration(1000 * state.settings.update_interval);
			timeout(timeout_duration.count());

			// Time points of current timing section
			std::chrono::time_point<std::chrono::high_resolution_clock> time_start, time_current, time_end;

			for (;;)
			{
				// Reference to info on the current section
				SectionInfo &si = state.section_info[state.current_section];

				// Pause before starting the section
				if (state.settings.pause_before_section_start)
				{
					clear();
					attron(COLOR_PAIR(CP_POMOCOM));
					printw("pomocom: %s\n", state.file_name);
					attron(COLOR_PAIR(state.current_section == SECTION_WORK ? CP_SECTION_WORK : CP_SECTION_BREAK));
					printw("next up: %s (%dm%ds)\n", si.name, si.secs / 60, si.secs % 60);
					attron(COLOR_PAIR(CP_TIME));
					printw("press %c to begin.", state.settings.keys.section_begin);
					refresh();

					// Wait until the section begin key is pressed
					while (getch() != state.settings.keys.section_begin)
						;
				}

				// Clear the screen
				clear();
				
				// Print pomocom
				attron(COLOR_PAIR(CP_POMOCOM));
				printw("pomocom: %s\n", state.file_name);

				// Print section name
				attron(COLOR_PAIR(state.current_section == SECTION_WORK ? CP_SECTION_WORK : CP_SECTION_BREAK));
				printw("%s\n", si.name);

				// Set the color for the time
				attron(COLOR_PAIR(CP_TIME));

				// Start the timing section
				time_start = std::chrono::high_resolution_clock::now();
				time_end = time_start + std::chrono::milliseconds(1000 * si.secs);

				for (;;)
				{
				l_print_time:
					// Set time_current and check if time is up
					time_current = std::chrono::high_resolution_clock::now();
					if (time_current >= time_end)
					{
						// Time is up
						break;
					}

					// Print the time remaining
					std::chrono::seconds time_left = std::chrono::ceil<std::chrono::seconds>(time_end - time_current);
					int mins = time_left.count() / 60;
					int secs = time_left.count() % 60;

					// Move to the area of the screen where the time remaining is displayed
					move(2, 0);

					clrtobot();
					printw("%dm %ds", mins, secs);
					refresh();

					// Get user input
					auto &keys = state.settings.keys;
					auto time_input_start = std::chrono::high_resolution_clock::now();
					int c = getch();
					if (c == keys.pause)
					{
						auto time_pause_start = std::chrono::high_resolution_clock::now();
						addstr(" (paused)");
						refresh();
						for (;;)
						{
							if (getch() == keys.pause)
							{
								// Unpause
								
								// Extend time_end to include the time spent paused
								auto time_pause_end = std::chrono::high_resolution_clock::now();
								time_end += (time_pause_end - time_pause_start);

								goto l_print_time;
							}
						}
					}
					else if (c == keys.section_skip)
					{
						// Skip to next section by altering time_end
						time_end = time_current;
						goto l_print_time;
					}
					else if (c == ERR)
					{
						// If getch() returns ERR, the user did not input anything, so we don't need to wait any longer until the time remaining can be reprinted
						goto l_print_time;
					}

					// Code execution reaches here if the user has input something
					// This means getch() will have returned before timeout_duration passed
					// We will wait for however many milliseconds is needed until timeout_duration has passed from the moment getch() was called
					auto time_input_end = std::chrono::high_resolution_clock::now();
					std::chrono::milliseconds timeout_time_left(timeout_duration - std::chrono::duration_cast<std::chrono::milliseconds>(time_input_end - time_input_start));
					if (timeout_time_left.count() > 0)
						std::this_thread::sleep_for(timeout_time_left);
				}

				base_next_section();
			}
		}

		// Quit ncurses
		endwin();
		break;
	default:
		PERR("unknown interface");
		abort();
	}

	return 0;
}
