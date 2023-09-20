/*
 * settings.hh contains types used for settings and functions for manipulating settings.
 *
 * All program settings are stored in a ProgramSettings object, which is contained in the global state object (see state.hh). Each member of the ProgramSettings object are of a special type designated for settings. In settings.cc, a macro is used to specify which of these members will be included the settings_map map, which keeps track of all settings that end users can modify.
 *
 * The key for settings_map is a string that identifies the name of a setting. The value is a SettingDef object which stores the type of setting in an enum and a memory offset of the setting variable relative to the first byte of a ProgramSettings object. This data is needed to know how to modify the value of each setting, which is done in setting_set().
 *
 * Each setting type is an integer of varying widths except for SettingString. SettingString variables are assigned their own unique C string in memory to point to. Each time the value of a string setting is changed using setting_set(), the old value of the setting is passed to std::free() and new memory is allocated for the new string value. When pomocom exits, all string settings are freed in settings_free_strings().
 *
 * Keywords, or strings that can be converted into numbers, are stored in the settings_keyword_map map.
 */

#pragma once

#include <cstdint>
#include <cstddef>	// For std::ptrdiff_t
#include <string>
#include <unordered_map>

#include "pomocom.hh"

#define SETTING_LONG_ASSERT_MSG	"SettingLong must be the widest of all setting types besides SettingString"

namespace pomocom
{
	// Program interface types
	enum ProgramInterface{
		// Interface that uses ANSI terminal escape codes
		INTERFACE_ANSI,

		// Interface using the ncurses library
		INTERFACE_NCURSES,

		// Interface using wxWidgets
		INTERFACE_WX,
	};

	// Setting type IDs
	enum SettingType{
		ST_CHAR,
		ST_STRING,
		ST_BOOL,
		ST_INT,
		ST_SHORT,
		ST_LONG,
	};

	// Type definitions for settings
	typedef char SettingChar;
	typedef const char *SettingString;
	typedef std::uint8_t SettingBool;
	typedef std::int8_t SettingInt;
	typedef std::int16_t SettingShort;
	typedef std::int64_t SettingLong;

	// Assert that SettingLong is the largest of the setting types besides SettingString
	static_assert(sizeof(SettingLong) > sizeof(SettingBool), SETTING_LONG_ASSERT_MSG);
	static_assert(sizeof(SettingLong) > sizeof(SettingChar), SETTING_LONG_ASSERT_MSG);
	static_assert(sizeof(SettingLong) > sizeof(SettingInt), SETTING_LONG_ASSERT_MSG);
	static_assert(sizeof(SettingLong) > sizeof(SettingShort), SETTING_LONG_ASSERT_MSG);

	// Program settings
	struct ProgramSettings{
		// Meant to hold a value of type ProgramInterface
		SettingInt interface;

		// # of seconds to wait between screen updates
		SettingLong update_interval;

		SettingBool pause_before_section_start;

		// Sets the terminal title to "pomocom - (pomo file name)"
		SettingBool set_terminal_title;

		// On screen updates, sets the terminal title to "(mins & secs) - pomocom - (pomo file name) - (section name)"
		SettingBool set_terminal_title_countdown;

		// Number of breaks until a long break
		SettingInt breaks_until_long_reset;

		// Keyboard controls
		struct Key{
			SettingChar quit;
			SettingChar pause;
			SettingChar section_begin;
			SettingChar section_skip;
		} key;

		// Paths
		// These should all be C strings that end in with '/'
		// IMPORTANT: each path should point to unique memory because std::free() will be called on all of them when pomocom exits
		struct Path{
			// Path to directory where config files are stored
			SettingString config;

			// Path to directory where pomo files are stored in
			SettingString section;

			// Path to directory where script files are stored in
			SettingString bin;

			// Path to directory where resource files are stored in (e.g. images)
			SettingString res;
		} path;

		struct Ncurses{
			struct Color{
				struct ColorPair{
					// Foreground color
					SettingShort fg;

					// Background color
					SettingShort bg;
				};

				// Used for the first line of text containing "pomocom:"
				ColorPair pomocom;

				// Used for the name of the work section
				ColorPair section_work;

				// Used for the name of the break sections
				ColorPair section_break;

				// Used for the time remaining in the section
				ColorPair time;
			} color;
		} ncurses;

		struct Wx{
			SettingBool show_menu_bar;
			SettingBool show_resize_symbol;
		} wx;

		// Sets default settings values
		ProgramSettings();
	};

	// Definition for a setting of a specified type with a memory address at the address of a ProgramSettings object + offset
	struct SettingDef{
		SettingType type;
		std::ptrdiff_t offset;
	};

	// Map of settings
	// Key:   string containing name of setting
	// Value: SettingDef object
	extern const std::unordered_map<std::string_view, SettingDef> settings_map;

	// Map of keywords that translate into SettingInt values
	// Key:   string containing name of keyword
	// Value: SettingInt object
	extern const std::unordered_map<std::string_view, SettingInt> settings_keyword_map;

	// Set default values for path settings
	void settings_set_default_paths(ProgramSettings::Path &path);

	// Set the setting with name *setting_name to *setting_value
	void setting_set(ProgramSettings &s, const char *setting_name, const char *setting_value);

	// Read settings file
	void settings_read(ProgramSettings &s);

	// Free all string settings
	void settings_free_strings(ProgramSettings &s);
}
