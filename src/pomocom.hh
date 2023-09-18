/*
 * pomocom.hh contains section types.
 */

#pragma once

namespace pomocom
{
	#define	POMOCOM_VERSION	"0.0.0"
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
}
