/*
 * ncurses.cc contains functions for using the ncurses interface.
 */

#include <chrono>		// For std::chrono::high_resolution_clock and more

#include <ncurses.h>

#include "../error.hh"
#include "../pomocom.hh"
#include "../settings.hh"
#include "../state.hh"
#include "base.hh"

namespace chrono = std::chrono;

namespace pomocom
{
	// Color pair ids start at 1 because 0 is unchangeable after the call to use_default_colors()
	enum ColorPair{
		CP_POMOCOM = 1,
		CP_SECTION_WORK,
		CP_SECTION_BREAK,
		CP_TIME,
	};

	static inline void interface_ncurses_init();
	static inline void interface_ncurses_exit();

	// Print info about the upcoming section
	static void print_upcoming_section(SectionInfo &si);

	// Print the first line of text, which contains "pomocom:"
	static void print_pomocom();

	// Print the timing section name
	static void print_section();

	// Print the time left in a section
	static void print_time_left(int mins, int secs);

	// Re-print the screen when KEY_RESIZE is detected during a timing section
	static void reprint_timing_screen(int mins, int secs);

	// Using attron(), activate the color pair for the section name text depending on the type of current section
	static inline void activate_section_color();

	// Calls init_pair() and throws an exception on error
	static inline void try_init_pair(short pair, ProgramSettings::Ncurses::Color::ColorPair cp);

	void interface_ncurses_loop()
	{
		interface_ncurses_init();

		// Alias for clock type
		using Clock = chrono::high_resolution_clock;

		// Alias for key settings
		auto &key = state.settings.key;

		// Store update interval in milliseconds
		chrono::milliseconds update_interval(1000 * state.settings.update_interval);

		// Start and end time points of timing section
		chrono::time_point<Clock> time_start, time_current, time_end;

		// Repeatedly move through timing sections
		for (;;)
		{
			// Reference to info on the current section
			SectionInfo &si = state.section_info[state.current_section];

			// Pause before starting the section
			if (state.settings.pause_before_section_start)
			{
				print_upcoming_section(si);

				// Make getch() wait for input before returning
				timeout(-1);

				// Wait until the section begin key is pressed or the user quits
				for (;;)
				{
					int c = getch();
					if (c == key.section_begin)
						break;
					if (c == key.section_skip)
						goto l_section_end;
					if (c == key.quit)
						goto l_exit;
					if (c == KEY_RESIZE)
					{
						// Prevent KEY_RESIZE from being read infinitely
						flushinp();
						print_upcoming_section(si);
					}
				}
			}

			clear();
			print_pomocom();
			print_section();

			// Start the timing section
			time_start = Clock::now();
			time_end = time_start + chrono::seconds(si.secs);

			// Repeatedly update the screen and check for input until section time is over
			while ((time_current = Clock::now()) < time_end)
			{
				// Print the time left in the section
				chrono::seconds time_left = chrono::ceil<chrono::seconds>(time_end - time_current);
				int mins = time_left.count() / 60;
				int secs = time_left.count() % 60;

				print_time_left(mins, secs);
				refresh();

				// Get user input
				auto time_input_start = Clock::now();

				// Expect to wait update_interval milliseconds for getch() to return
				timeout(update_interval.count());

			l_get_user_input:
				int c = getch();
				if (c == key.pause)
				{
					// Pause

					// Print pause text
					addstr(" (paused)");
					refresh();

					auto time_pause_start = Clock::now();

					// Make getch() wait for input before returning
					timeout(-1);

					// Wait until pause is pressed again to trigger an unpause or the user quits
					for (;;)
					{
						c = getch();
						if (c == key.pause)
							break;
						if (c == key.quit)
							goto l_exit;
					}
					
					// Unpause
					
					// Extend time_end to include the time spent paused
					time_end += Clock::now() - time_pause_start;

					// Go back to updating the screen
					continue;
				}
				else if (c == key.section_skip)
				{
					// Skip to next section by exiting the loop
					break;
				}
				else if (c == key.quit)
				{
					// Quit
					goto l_exit;
				}
				else if (c == ERR)
				{
					// If getch() returns ERR, the user did not input anything, so we don't need to wait any longer until the time remaining can be reprinted
					continue;
				}
				else if (c == KEY_RESIZE)
				{
					reprint_timing_screen(mins, secs);

					// Prevent KEY_RESIZE from being read infinitely
					flushinp();
				}

				// If code execution reaches here, either the user has input an unrecognized key or performed an action that doesn't cause a continue, break, or goto.
				// In both such cases, getch() has not returned ERR, so the time until the next screen update has not yet passed

				// Recalculate the time until the next screen update
				chrono::milliseconds time_until_screen_update = update_interval - chrono::duration_cast<chrono::milliseconds>(Clock::now() - time_input_start);

				if (time_until_screen_update.count() <= 0)
				{
					// A screen update should happen now
					continue;
				}
				else
				{
					// Reset timeout to reflect the change to the time until the next screen update
					timeout(time_until_screen_update.count());

					// Try to get more user input before the next screen update
					goto l_get_user_input;
				}
			}
		l_section_end:
			base_next_section();
		}
	l_exit:
		interface_ncurses_exit();
	}

	static inline void interface_ncurses_init()
	{
		// Init ncurses
		if (initscr() == nullptr)
		{
			PERR("ncurses initscr() call failed");
			throw EXCEPT_GENERIC;
		}

		// Config input
		if (cbreak() == ERR)
		{
			PERR("ncurses cbreak() call failed");
			throw EXCEPT_GENERIC;
		}
		if (noecho() == ERR)
		{
			PERR("ncurses noecho() call failed");
			throw EXCEPT_GENERIC;
		}

		// Init colors
		if (start_color() == ERR)
		{
			PERR("failed to init ncurses colors");
			throw EXCEPT_GENERIC;
		}
		if (use_default_colors() == ERR)
		{
			PERR("failed to use default ncurses colors");
			throw EXCEPT_GENERIC;
		}

		// Alias for ncurses color settings
		auto &color = state.settings.ncurses.color;

		// Set colors
		try_init_pair(CP_POMOCOM,	color.pomocom);
		try_init_pair(CP_SECTION_WORK,	color.section_work);
		try_init_pair(CP_SECTION_BREAK, color.section_break);
		try_init_pair(CP_TIME,		color.time);
	}

	static inline void interface_ncurses_exit()
	{
		// Exit ncurses
		endwin();
	}

	// Print info about the upcoming section
	static void print_upcoming_section(SectionInfo &si)
	{
		clear();
		print_pomocom();

		// Print the upcoming section name and duration
		move(1, 0);
		activate_section_color();
		printw("next up: %s (%dm%ds)\n", si.name, si.secs / 60, si.secs % 60);

		// Print section begin key
		attron(COLOR_PAIR(CP_TIME));
		printw("press %c to begin.", state.settings.key.section_begin);

		refresh();
	}

	// Re-print the screen when KEY_RESIZE is detected during a timing section
	static void reprint_timing_screen(int mins, int secs)
	{
		clear();
		print_pomocom();
		print_section();
		print_time_left(mins, secs);
		refresh();
	}

	// Print the first line of text, which contains "pomocom:"
	static void print_pomocom()
	{
		move(0, 0);
		attron(COLOR_PAIR(CP_POMOCOM));
		printw("pomocom: %s", state.file_name);
	}

	// Print the timing section name
	static void print_section()
	{
		move(1, 0);
		activate_section_color();
		printw("%s", state.section_info[state.current_section].name);
	}

	// Print the time left in a section
	static void print_time_left(int mins, int secs)
	{
		move(2, 0);

		// Clear the previous time left on the screen
		clrtoeol();

		attron(COLOR_PAIR(CP_TIME));
		printw("%dm %ds", mins, secs);
	}

	// Using attron(), activate the color pair for the section name text depending on the type of current section
	static inline void activate_section_color()
	{
		attron(COLOR_PAIR(state.current_section == SECTION_WORK ? CP_SECTION_WORK : CP_SECTION_BREAK));
	}

	// Calls init_pair() and throws an exception on error
	static inline void try_init_pair(short pair, ProgramSettings::Ncurses::Color::ColorPair cp)
	{
		if (init_pair(pair, cp.fg, cp.bg) == ERR)
		{
			PERR("failed to init color pair %d", pair);
			throw EXCEPT_GENERIC;
		}
	}
}
