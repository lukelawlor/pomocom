/*
 * ncurses.cc contains functions for using the ncurses interface.
 */

#include <chrono>		// For std::chrono::high_resolution_clock and more

#include <ncurses.h>

#include "../error.hh"
#include "../pomocom.hh"
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

	// Calls init_pair() and throws an exception on error
	static inline void try_init_pair(short pair, short f, short b);

	void interface_ncurses_loop()
	{
		interface_ncurses_init();

		// Store update interval in milliseconds
		chrono::milliseconds update_interval(1000 * state.settings.update_interval);

		// Alias for clock type
		using Clock = chrono::high_resolution_clock;

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
				// Print info about the upcoming section
				clear();
				attron(COLOR_PAIR(CP_POMOCOM));
				printw("pomocom: %s\n", state.file_name);
				attron(COLOR_PAIR(state.current_section == SECTION_WORK ? CP_SECTION_WORK : CP_SECTION_BREAK));
				printw("next up: %s (%dm%ds)\n", si.name, si.secs / 60, si.secs % 60);
				attron(COLOR_PAIR(CP_TIME));
				printw("press %c to begin.", state.settings.keys.section_begin);
				refresh();

				// Make getch() wait for input before returning
				timeout(-1);

				// Wait until the section begin key is pressed or the user quits
				for (;;)
				{
					int c = getch();
					if (c == state.settings.keys.section_begin)
						break;
					if (c == state.settings.keys.quit)
						goto l_exit;
				}
			}

			// Clear the screen
			clear();
			
			// Print pomocom & pomo file name
			attron(COLOR_PAIR(CP_POMOCOM));
			printw("pomocom: %s\n", state.file_name);

			// Print section name
			attron(COLOR_PAIR(state.current_section == SECTION_WORK ? CP_SECTION_WORK : CP_SECTION_BREAK));
			printw("%s\n", si.name);

			// Set the color for the time
			attron(COLOR_PAIR(CP_TIME));

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

				// Move to the area of the screen where the time left is displayed
				move(2, 0);

				// Print time left
				clrtobot();
				printw("%dm %ds", mins, secs);
				refresh();

				// Get user input
				auto &keys = state.settings.keys;
				auto time_input_start = Clock::now();

				// Expect to wait update_interval milliseconds for getch() to return
				timeout(update_interval.count());

			l_get_user_input:
				int c = getch();
				if (c == keys.pause)
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
						if (c == keys.pause)
							break;
						if (c == keys.quit)
							goto l_exit;
					}
					
					// Unpause
					
					// Extend time_end to include the time spent paused
					time_end += Clock::now() - time_pause_start;

					// Go back to updating the screen
					continue;
				}
				else if (c == keys.section_skip)
				{
					// Skip to next section by exiting the loop
					break;
				}
				else if (c == keys.quit)
				{
					// Quit
					goto l_exit;
				}
				else if (c == ERR)
				{
					// If getch() returns ERR, the user did not input anything, so we don't need to wait any longer until the time remaining can be reprinted
					continue;
				}
				else
				{
					// The user input an unrecognized key before getch() returned ERR

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
			}
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

		// Set colors
		try_init_pair(CP_POMOCOM, COLOR_BLUE, -1);
		try_init_pair(CP_SECTION_WORK, COLOR_YELLOW, -1);
		try_init_pair(CP_SECTION_BREAK, COLOR_GREEN, -1);
		try_init_pair(CP_TIME, -1, -1);
	}

	static inline void interface_ncurses_exit()
	{
		// Exit ncurses
		endwin();
	}

	// Calls init_pair() and throws an exception on error
	static inline void try_init_pair(short pair, short f, short b)
	{
		if (init_pair(pair, f, b) == ERR)
		{
			PERR("failed to init color pair %d", pair);
			throw EXCEPT_GENERIC;
		}
	}
}
