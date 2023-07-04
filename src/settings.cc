/*
 * settings.cc contains the settings_map object.
 */

#include <cstddef>	// For std::ptrdiff_t and std::byte
#include <cstdlib>	// For std::atoll() and offsetof
#include <stdexcept>	// For std::out_of_range
#include <type_traits>	// For std::is_same_v
#include <unordered_map>

#include "error.hh"
#include "exceptions.hh"
#include "fileio.hh"	// For pomocom::SmartFilePtr
#include "settings.hh"

namespace pomocom
{
	// Maximum length of a token in the setting file
	// A token in this case is any string without whitespace
	constexpr int MAX_SETTING_TOKEN_LEN = 100;

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
	#define ADD_SETTING(setting_name)	{\
							#setting_name,\
							{\
								.type = get_setting_type_from_variable_type<decltype(ProgramSettings::setting_name)>(),\
								.offset = offsetof(ProgramSettings, setting_name)\
							}\
						},

	// Map of settings
	const std::unordered_map<std::string_view, SettingDef> settings_map = {
		ADD_SETTING(interface)
		ADD_SETTING(update_interval)
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
	const std::unordered_map<std::string_view, SettingInt> settings_keyword_map = {
		{"true", 1},
		{"false", 0},
		{"ncurses", INTERFACE_NCURSES},
		{"ansi", INTERFACE_ANSI},
	};

	// Set the setting with name *setting_name to *setting_value
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

	// Read settings file
	void settings_read(ProgramSettings &s)
	{
		// Get the path to the settings file (pomocom.conf)
		std::string path_to_pomocom_conf(s.paths.config);
		path_to_pomocom_conf += "pomocom.conf";

		// Open the file
		SmartFilePtr sfp(path_to_pomocom_conf.c_str(), "r");
		auto &fp = sfp.m_fp;

		struct Token{
			// String
			char m_str[MAX_SETTING_TOKEN_LEN];

			// Length
			int m_len;

			// Initialize length with 0
			Token() : m_len(0) {}

			// Adds c to the string
			void add(const int &c)
			{
				if (m_len >= MAX_SETTING_TOKEN_LEN - 1)
				{
					// m_str has max length, don't add to the string
					return;
				}
				m_str[m_len] = c;
				++m_len;
			}

			// Resets length to 0
			void reset(){ m_len = 0; }

			void null_terminate(){ m_str[m_len] = '\0'; }
		};

		// Tokens being read
		Token setting_name;
		Token setting_value;

		// Line number being read in the settings file
		int line_number = 1;

		// Character being read
		int c;

		// Start reading file data
		for (;;)
		{
		l_read_setting_name:
			// Read setting name
			switch (c = std::fgetc(fp))
			{
			case ' ':
				// Read setting value
				for (;;)
				{
					switch (c = std::fgetc(fp))
					{
					case '\n':
						// End of line found
						setting_name.null_terminate();
						setting_value.null_terminate();
						try{ setting_set(s, setting_name.m_str, setting_value.m_str); }
						catch (Exception &e){ PERR("error in pomocom.conf at line %d", line_number); }
						setting_name.reset();
						setting_value.reset();
						++line_number; 
						goto l_read_setting_name;
					default:
						// Collect char in setting_value
						setting_value.add(c);
						break;
					}
				}
			case EOF:
				// End of file found, exit the function
				return;
			default:
				// Collect char in setting_name
				setting_name.add(c);
				break;
			}
		}
	}
}
