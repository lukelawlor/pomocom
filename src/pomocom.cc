/*
 * pomocom.cc contains the main function and section and timing code.
 */

#include <chrono>	// For sleeping for a specified duration
#include <cstdlib>	// For std::time()
#include <cstring>	// For std::strcpy() and std::strlen()
#include <cstdio>	// For std::FILE and std::fopen
#include <iostream>
#include <thread>	// For sleeping

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
}

int main(int argc, char **argv)
{
	// Times of current timing section
	std::time_t time_start;
	std::time_t time_current;
	std::time_t time_end;

	using namespace pomocom;

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
		// Don't print any error messages if an uncaught exception occurs because in such case an error message will already be printed
		return 1;
	}

	// Breaks left until a long break
	switch (state.settings.interface)
	{
	case INTERFACE_ANSI:
		for (;;)
		{
			// Reference to info on the current section
			SectionInfo &si = state.section_info[state.current_section];

			// Print the pomocom text
			std::cout << AT_CLEAR;
			std::cout << "pomocom:\n";
			
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
		break;
	case INTERFACE_NCURSES:
		PERR("ncurses interface not implemented yet");
		abort();
	default:
		PERR("unknown interface");
		abort();
	}

	return 0;
}
