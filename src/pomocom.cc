/*
 * pomocom.cc contains main() and pomo file reading code.
 */

#include <cstring>	// For std::strcpy() and std::strlen()
#include <iostream>
#include <sstream>

#include "error.hh"
#include "fileio.hh"
#include "interface/all.hh"
#include "pomocom.hh"
#include "state.hh"
#include "terminal_title.hh"

namespace pomocom
{
	// Reads sections from the file at *path where *path is altered
	static void read_sections(const char *path);

	// Reads sections from the file at *path where *path is unaltered
	static void read_sections_raw(const char *path);

	static constexpr const char *DEFAULT_POMO_FILE = "standard";
}

int main(int argc, char **argv)
{
	using namespace pomocom;

	int exit_code = EXIT_SUCCESS;

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
			read_sections(DEFAULT_POMO_FILE);
		else if (argc > 1)
		{
			bool pomo_file_was_specified = false;

			// Loop through args
			for (int i = 1; i < argc; ++i)
			{
				char *arg = argv[i];

				if (arg[0] == '-')
				{
					if (arg[1] == '-')
					{
						// The argument starts with "--"
						// Assume the argument contains a setting name after the "--"
						char *setting_name = arg + 2;

						// The next argument should be the setting value

						if (i + 1 == argc)
						{
							// This is the last argument, so there is no next argument containing a value
							PERR("no setting value specified for setting \"%s\"", setting_name);
							break;
						}

						// There is a next argument, so we can increment i without going out of bounds
						++i;
						char *setting_value = argv[i];
						try{ setting_set(state.settings, setting_name, setting_value); }
						catch (Exception &e)
						{
							
							if (e != EXCEPT_IO)
							{
								// A memory allocation failed, so we can't handle it
								throw e;
							}
						}
					}
					else
					{
						// The argument starts with "-"
						switch (arg[1])
						{
						case 'b':
							// Start with short break section
							state.current_section = SECTION_BREAK;
							break;
						case 'B':
							// Start with long break section
							state.current_section = SECTION_BREAK_LONG;
							break;
						case 'q':
							// Quick pomo file setup
							// usage: -q (mins of work section) (mins of break section) (mins of long break section)
							{
								if (i + 3 >= argc)
								{
									PERR("not enough arguments following \"-q\"");
									throw EXCEPT_BAD_SETTING;
								}

								// Use the names and commands from the default pomo file
								pomo_file_was_specified = true;
								read_sections(DEFAULT_POMO_FILE);

								// Overwrite the length of each section based on the args after -q
								for (SectionInfo &si : state.section_info)
									si.secs = std::atoi(argv[++i]) * 60;
							}
							break;
						default:
							PERR("unknown argument \"%s\"", arg);
							break;
						}
					}
				}
				else
				{
					// The argument doesn't start with "-"
					// Assume the argument is the name of a pomo file
					pomo_file_was_specified = true;
					read_sections(arg);
				}
			}

			if (!pomo_file_was_specified)
				read_sections(DEFAULT_POMO_FILE);
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

		if ((state.settings.interface == INTERFACE_ANSI ||
		     state.settings.interface == INTERFACE_NCURSES) &&
		    state.settings.set_terminal_title)
		{
			std::stringstream title;
			title << "pomocom - " << state.file_name;
			set_terminal_title(title.view());
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
		case INTERFACE_WX:
			interface_wx_loop();
			break;
		default:
			PERR("unknown interface");
			throw EXCEPT_BAD_SETTING;
		}
	}
	catch (...)
	{
		// Don't print an error message because one should already have printed
		exit_code = EXIT_FAILURE;
	}

	// Cleanup and exit
	settings_free_strings(state.settings);

	// Bye bye
	if (exit_code == EXIT_SUCCESS)
		std::cout << "Hey thanks for using pomocom.\n";

	return exit_code;
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
					std::string buf(state.settings.path.bin);

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
			alt_path += state.settings.path.section;
			alt_path += path;
		}
		alt_path += ".pomo";

		// Actually loading the section data with the altered path
		read_sections_raw(alt_path.c_str());
	}
}
