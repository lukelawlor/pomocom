/*
 * settings.hh contains the settings struct type.
 */

#pragma once

#include <cstdint>
#include <cstddef>	// For std::ptrdiff_t
#include <unordered_map>

#include "pomocom.hh"

namespace pomocom
{
	// Program interface types
	enum ProgramInterface{
		// Interface that uses ANSI terminal escape codes
		INTERFACE_ANSI,

		// Interface using the ncurses library
		INTERFACE_NCURSES,
	};

	// Type definitions for settings
	typedef char SettingChar;
	typedef const char *SettingString;
	typedef std::uint8_t SettingBool;
	typedef std::int8_t SettingInt;
	typedef std::int64_t SettingLong;

	#define SETTING_LONG_ASSERT_MSG	"SettingLong must be the widest of all setting types besides SettingString"
	static_assert(sizeof(SettingLong) > sizeof(SettingBool), SETTING_LONG_ASSERT_MSG);
	static_assert(sizeof(SettingLong) > sizeof(SettingChar), SETTING_LONG_ASSERT_MSG);
	static_assert(sizeof(SettingLong) > sizeof(SettingInt), SETTING_LONG_ASSERT_MSG);

	enum SettingType{
		ST_CHAR,
		ST_STRING,
		ST_BOOL,
		ST_INT,
		ST_LONG,
	};

	// Program settings
	struct ProgramSettings{
		SettingInt interface;

		// # of seconds to wait between screen updates
		SettingLong update_interval;

		SettingBool show_controls;
		SettingBool pause_before_section_start;

		// Number of breaks until a long break
		SettingInt breaks_until_long_reset;

		// Keyboard controls
		struct SettingsKeys{
			SettingChar pause;
			SettingChar section_begin;
			SettingChar section_skip;
		} keys;

		// Paths
		// These should all be C strings that end in with '/'
		struct SettingsPaths{
			// Path to directory where config files are stored
			SettingString config;

			// Path to directory that section files are stored in
			SettingString section;

			// Path to directory that script files are stored in
			SettingString bin;

		} paths;
	};

	// Defines a setting of a specified type with a memory address at the address of a ProgramSettings object + offset
	struct SettingDef{
		SettingType type;
		ptrdiff_t offset;
	};

	// Map of settings
	// Key = string containing name of setting
	// Value = SettingDef object
	extern const std::unordered_map<const char *, SettingDef> settings_map;

	// Map of keywords that translate into SettingInt values
	// Key = string containing name of keyword
	// Value = SettingInt object
	extern const std::unordered_map<const char *, SettingInt> settings_keyword_map;

	// Set a setting
	void setting_set(ProgramSettings &s, const char *setting_name, const char *setting_value);

	// Read pomocom.conf file
	void settings_read();
}
