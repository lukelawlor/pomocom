/*
 * pomocom.cc does everything. It contains the main function.
 */

#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <thread>

#include "ansi_term.hh"

#define	SECTION_INFO_NAME_LEN	20
#define	SECTION_INFO_CMD_LEN	20

namespace pomocom
{
	// Generic exception codes
	enum Exception{
		EXCEPT_GENERIC,

		// Input/output error
		EXCEPT_IO,
	};

	// Timing sections
	enum Section{
		SECTION_WORK,
		SECTION_BREAK,
		SECTION_BREAK_LONG,
		SECTION_MAX,
	};

	// Info on each timing section
	struct SectionInfo{
		char name[SECTION_INFO_NAME_LEN];
		char cmd[SECTION_INFO_CMD_LEN];
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
		// section_info left uninitialized because it will be set when files are read
	};

	// Function from SoupDL 06 (spdl)
	// Writes chars from *stream (including \0) into *dest
	// Stops when the delim character is found, and doesn't include the delim in the string
	// Returns the number of chars written to *dest minus 1
	int spdl_readstr(char *dest, const size_t len_max, const int delim, std::FILE *stream)
	{
		// Index of *dest to access next
		int i = 0;

		// Stop reading when i reaches this value
		const int i_stop = len_max - 1;

		// Current character being processed
		int c;

		// Reading loop
		for (;;)
		{
			c = std::fgetc(stream);
			if (c == delim)
			{
				dest[i] = '\0';
				break;
			}
			if (c == EOF)
			{
				// Reading error
				throw EXCEPT_IO;
			}
			dest[i] = c;
			if (++i == i_stop)
			{
				// The max amount of chars was read and the end of the string was not found
				dest[i] = '\0';
				throw EXCEPT_GENERIC;
			}
		}

		// Success
		return i;
	}

	// TODO: get the proper home directory with code
	const char *section_directory = "/home/cak/projects/pomocom/data/";

	// Reads sections from the file at *path where *path is unaltered
	void read_sections_raw(const char *path)
	{
		std::FILE *fp = std::fopen(path, "r");
		if (fp == nullptr)
		{
			std::fprintf(stderr, "couldn't open pomo file \"%s\"\n", path);
			throw EXCEPT_IO;
		}

		// Get section data
		for (SectionInfo &s : state.section_info)
		{
			// Read in section data
			spdl_readstr(s.name, SECTION_INFO_NAME_LEN, '\n', fp);
			spdl_readstr(s.cmd, SECTION_INFO_CMD_LEN, '\n', fp);
			int minutes, seconds;
			std::fscanf(fp, "%dm%ds\n", &minutes, &seconds);
			s.secs = minutes * 60 + seconds;
		}
	}

	// Returns nonzero on error
	// Reads sections from the file at *path where *path is altered
	void read_sections(const char *path)
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
			alt_path += section_directory;
			alt_path += path;
		}
		alt_path += ".pomo";

		// Actually loading the section data with the altered path
		read_sections_raw(alt_path.c_str());
	}
}

int main(int argc, char **argv)
{
	// Times of current timing section
	std::time_t time_start;
	std::time_t time_current;
	std::time_t time_end;

	using namespace pomocom;

	// Read in settings
	if (argc == 1)
	{
		read_sections("standard");
	}
	else if (argc == 2)
	{
		read_sections(argv[1]);
	}
	else
	{
		std::fprintf(stderr, "wrong number of args provided\n");
		return 1;
	}

	for (SectionInfo &s : state.section_info)
	{
		if (s.secs <= 0)
		{
			std::fprintf(stderr, "invalid card data found\n");
			return 1;
		}
	}

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
