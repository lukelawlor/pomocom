/*
 * pomocom.cc contains the main function and section and timing code.
 */

#include <cstring>	// For std::strcpy() and std::strlen()
#include <iostream>

#include "error.hh"
#include "fileio.hh"
#include "interface/all.hh"
#include "pomocom.hh"
#include "state.hh"

namespace pomocom
{
	// Reads sections from the file at *path where *path is altered
	static void read_sections(const char *path);

	// Reads sections from the file at *path where *path is unaltered
	static void read_sections_raw(const char *path);
}

int main(int argc, char **argv)
{
	using namespace pomocom;

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

		// Use the specified interface
		switch (state.settings.interface)
		{
		case INTERFACE_ANSI:
			interface_ansi_loop();
			break;
		case INTERFACE_NCURSES:
			interface_ncurses_loop();
			break;
		default:
			PERR("unknown interface");
			return EXIT_FAILURE;
		}
	}
	catch (...)
	{
		// Don't print an error message because one should already have printed
		return EXIT_FAILURE;
	}

	// Cleanup and exit
	settings_free_paths(state.settings.paths);

	// Bye bye
	std::cout << "Hey thanks for using pomocom.\n";

	return EXIT_SUCCESS;
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
}
