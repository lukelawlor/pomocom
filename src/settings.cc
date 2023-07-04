/*
 * settings.cc contains the settings_map object.
 */

#include <cstddef>	// For std::ptrdiff_t and std::byte
#include <cstdlib>	// For std::atoll()
#include <stdexcept>	// For std::out_of_range
#include <type_traits>	// For std::is_same
#include <unordered_map>

#include "error.hh"
#include "exceptions.hh"
#include "settings.hh"

namespace pomocom
{
	// Returns the enum counterpart to setting type T
	template <typename T>
	consteval SettingType get_setting_type_from_variable_type()
	{
		if constexpr (std::is_same_v<T, SettingBool>) return ST_BOOL;
		if constexpr (std::is_same_v<T, SettingBool>) return ST_BOOL;
		if constexpr (std::is_same_v<T, SettingChar>) return ST_CHAR;
		if constexpr (std::is_same_v<T, SettingInt>) return ST_INT;
		if constexpr (std::is_same_v<T, SettingLong>) return ST_LONG;
		if constexpr (std::is_same_v<T, SettingString>) return ST_STRING;

		// Cause a compilation error because the variable type cannot be converted to a setting type
		throw EXCEPT_GENERIC;
	}

	// Defines setting_name as a setting by creating an entry in settings_map
	// Used in the initializer for settings_map
	#define ADD_SETTING(setting_name)	{ \
							#setting_name, \
							{ \
								.type = get_setting_type_from_variable_type<decltype(ProgramSettings::setting_name)>(), \
								.offset = offsetof(ProgramSettings, setting_name) \
							} \
						},

	// Map of settings
	const std::unordered_map<const char *, SettingDef> settings_map = {
		ADD_SETTING(interface)
		ADD_SETTING(update_interval)
		ADD_SETTING(show_controls)
		ADD_SETTING(pause_before_section_start)
		ADD_SETTING(breaks_until_long_reset)
		ADD_SETTING(keys.pause)
		ADD_SETTING(keys.section_begin)
		ADD_SETTING(keys.section_skip)
		ADD_SETTING(paths.config)
		ADD_SETTING(paths.section)
		ADD_SETTING(paths.bin)
	};

	// Map of keywords that translate into SettingInt values
	// IMPORTANT: the keys should not start with a number because that will be seen as an indication that a setting value is a number
	const std::unordered_map<const char *, SettingInt> settings_keyword_map = {
		{"true", 1},
		{"false", 0},
		{"ncurses", INTERFACE_NCURSES},
		{"ansi", INTERFACE_ANSI},
	};

	// Set a setting
	void setting_set(ProgramSettings &s, const char *setting_name, const char *setting_value)
	{
		// Get setting definition
		const SettingDef *setting_def;
		try{ setting_def = &settings_map.at(setting_name); }
		catch (std::out_of_range &e)
		{
			// Setting with name *setting_name doesn't exist
			PERR("no setting named \"%s\" exists", setting_name);
			throw EXCEPT_IO;
		}

		// Pointer to setting variable
		std::byte *setting_ptr = reinterpret_cast<std::byte *>(&s);
		setting_ptr += setting_def->offset;

		// Set the setting
		switch (setting_def->type)
		{
		case ST_STRING:
			{
				// The setting is a string
				SettingString *p = reinterpret_cast<SettingString *>(setting_ptr);
				*p = setting_value;
			}
			break;
		case ST_CHAR:
			{
				// The setting is a char
				SettingChar *p = reinterpret_cast<SettingChar *>(setting_ptr);
				*p = setting_value[0];
			}
			break;
		default:
			{
				// The setting is a number
				
				// Number that *setting_value will be converted into
				SettingLong setting_value_number;
				
				if (isdigit(*setting_value))
				{
					// If the first character of *setting_value is a digit, assume that *setting_value is a number in string form
					setting_value_number = std::atoll(setting_value);
				}
				else
				{
					// Assume that *setting_value is a keyword
					try{ setting_value_number = settings_keyword_map.at(setting_value); }
					catch (std::out_of_range &e)
					{
						// No keyword exists, so the conversion failed
						PERR("cannot convert setting value \"%s\" to a number", setting_value);
						throw EXCEPT_IO;
					}
				}

				// Set the setting
				switch (setting_def->type)
				{
				case ST_BOOL:
					{
						SettingBool *p = reinterpret_cast<SettingBool *>(setting_ptr);
						*p = static_cast<SettingBool>(setting_value_number);
						break;
					}
				case ST_INT:
					{
						SettingInt *p = reinterpret_cast<SettingInt *>(setting_ptr);
						*p = static_cast<SettingInt>(setting_value_number);
						break;
					}
				case ST_LONG:
					{
						SettingLong *p = reinterpret_cast<SettingLong *>(setting_ptr);
						*p = setting_value_number;
						break;
					}
				default:
					PERR("unknown number setting type for setting \"%s\"", setting_name);
					throw EXCEPT_IO;
				}
			}
		}
	}

	// Read pomocom.conf file
	void settings_read()
	{
	}
}
