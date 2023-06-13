/*
 * pomocom.hh contains section types and other miscellaenous types.
 */

#pragma once

namespace pomocom
{
	constexpr int SECTION_INFO_NAME_LEN = 100;
	constexpr int SECTION_INFO_CMD_LEN = 100;

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
}
