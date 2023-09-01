/*
 * wx.cc contains code for the wxWidgets interface.
 *
 * This interface utilizes a wxTimer object to periodically display the time remaining, and wxButton objects to implement basic controls
 */

#include <chrono>
#include <iostream>
#include <sstream>
#include <string>

#include <wx/wx.h>

#include "../pomocom.hh" // For SectionInfo
#include "../state.hh"
#include "all.hh"
#include "base.hh"

namespace chrono = std::chrono;

namespace pomocom
{
	using Clock = chrono::high_resolution_clock;

	// String literals
	constexpr const char *BTN_START_LABEL = "start";
	constexpr const char *BTN_PAUSE_LABEL = "pause";
	constexpr const char *BTN_RESUME_LABEL = "resume";
	constexpr const char *BTN_SKIP_LABEL = "skip";
	constexpr const char *STATUS_TIME_STARTED = "time started";
	constexpr const char *STATUS_TIME_PAUSED = "time paused";
	constexpr const char *STATUS_TIME_RESUMED = "time resumed";
	constexpr const char *STATUS_TIME_UP = "time up!";
	constexpr const char *STATUS_INITIAL = "welcome to pomocom!";
	constexpr const char *TXT_TIME_INITIAL_LABEL = "time left goes here";
	constexpr const char *TXT_TIME_UP_LABEL = STATUS_TIME_UP;
	constexpr const char *TXT_SECTION_INITIAL_LABEL = "section name goes here";
	
	enum WindowId{
		ID_BTN_PAUSE = 2,
		ID_BTN_QUIT,
		ID_TXT_TIME,
		ID_TXT_SECTION,
		ID_MENU_ABOUT,
		ID_MENU_EXIT,
	};
	
	enum TimerState{
		// The timing section has not started yet
		TSTATE_START,
		
		// The wxTimer is running
		TSTATE_TIMER_RUNNING,
		
		// The wxTimer not running
		TSTATE_TIMER_NOT_RUNNING,
	};
	
	// Timing section data
	struct TimerData{
		TimerState state;
		
		// Start of the timing section
		chrono::time_point<Clock> start;
		
		// End of the timing section
		chrono::time_point<Clock> end;
		
		// When the wxTimer was last paused at
		chrono::time_point<Clock> pause_start;
	};

	// Frame created when the app starts
	struct MainFrame : public wxFrame{
	private:
		TimerData m_timer_data;
		
		// Info on the current section
		SectionInfo *m_si;

		// Used to handle periodic text updating
		int m_timer_interval;
		wxTimer m_timer;

		// Button that controls starting the timing section, pausing, and unpausing
		wxButton *m_btn_pause;

		// Displays the time left in the timing section
		wxStaticText *m_txt_time;

		// Displays the name of the timing section
		wxStaticText *m_txt_section;

		// Updates m_txt_time to show the time left in the timing section
		void update_txt_time(chrono::time_point<Clock> &time_current);

		// Runs when m_btn_pause is clicked
		void on_btn_pause(wxCommandEvent &e);

		// Runs every m_timer_interval milliseconds when the timer is running
		// Calls update_txt_time if time isn't up yet
		void on_timer(wxTimerEvent &e);

		// Runs when the wxTimer fails to start
		void on_timer_error();

		void on_about(wxCommandEvent &e);
		void on_exit(wxCommandEvent &e);
	public:
		MainFrame();
	};

			
	// Updates m_txt_time to show the time left in the timing section
	void MainFrame::update_txt_time(chrono::time_point<Clock> &time_current)
	{
		// Time left in the section in seconds
		int time_left = chrono::ceil<chrono::seconds>(m_timer_data.end - time_current).count();
		
		// Minutes and seconds left
		int mins = time_left / 60;
		int secs = time_left % 60;
		
		// Set the new label for the time left widget
		std::stringstream label;
		label << mins << "m " << secs << "s left";
		m_txt_time->SetLabel(label.str());
	}

	// Runs when m_btn_pause is clicked
	void MainFrame::on_btn_pause([[maybe_unused]] wxCommandEvent &e)
	{
		switch (m_timer_data.state)
		{
		case TSTATE_START:
			// Start the timing section
			
			// Set the start and end times of the section
			m_timer_data.start = Clock::now();
			m_timer_data.end = m_timer_data.start + chrono::seconds(m_si->secs);
			
			// Start the wxTimer
			if (m_timer.Start(m_timer_interval) == false)
				on_timer_error();
			
			// Update the UI
			update_txt_time(m_timer_data.start);
			m_txt_section->SetLabel(m_si->name);
			m_btn_pause->SetLabel(BTN_PAUSE_LABEL);
			SetStatusText(STATUS_TIME_STARTED);
			
			// Update the timer state
			m_timer_data.state = TSTATE_TIMER_RUNNING;
			break;
		case TSTATE_TIMER_RUNNING:
			// Pause the timer
			
			m_timer_data.pause_start = Clock::now();
			
			// Stop the wxTimer
			m_timer.Stop();
			
			// Update the UI
			m_btn_pause->SetLabel(BTN_RESUME_LABEL);
			SetStatusText(STATUS_TIME_PAUSED);
			
			// Update the timer state
			m_timer_data.state = TSTATE_TIMER_NOT_RUNNING;
			break;
		case TSTATE_TIMER_NOT_RUNNING:
			// Resume the timer
			
			// Add the time spent paused to the end time
			m_timer_data.end += Clock::now() - m_timer_data.pause_start;
			
			// Restart the wxTimer
			if (m_timer.Start(m_timer_interval) == false)
				on_timer_error();
			
			// Update the UI
			m_btn_pause->SetLabel(BTN_PAUSE_LABEL);
			SetStatusText(STATUS_TIME_RESUMED);
			
			// Update the timer state
			m_timer_data.state = TSTATE_TIMER_RUNNING;
			break;
		default:
			wxLogMessage("error: unknown timer state");
			Close(true);
			break;
		}
	}
	
	// Runs every m_timer_interval milliseconds when the timer is running
	// Calls update_txt_time if time isn't up yet
	void MainFrame::on_timer([[maybe_unused]] wxTimerEvent &e)
	{
		auto time_current = Clock::now();
		
		if (time_current >= m_timer_data.end)
		{
			// Move to the next timing section
			base_next_section();
			m_si = &state.section_info[state.current_section];
			
			// Stop the wxTimer
			m_timer.Stop();
			
			// Update the UI
			m_txt_time->SetLabel(TXT_TIME_UP_LABEL);
			m_txt_section->SetLabel(m_si->name);
			m_btn_pause->SetLabel(BTN_START_LABEL);
			SetStatusText(STATUS_TIME_UP);
			
			// Update the timer state
			m_timer_data.state = TSTATE_START;
		}
		else
			update_txt_time(time_current);
	}
	
	// Runs when the wxTimer fails to start
	void MainFrame::on_timer_error()
	{
		wxLogMessage("error: failed to start wxTimer");
		Close(true);
	}
	
	void MainFrame::on_about(wxCommandEvent &e)
	{
		wxMessageBox("placeholder about", "About pomocom", wxOK | wxICON_INFORMATION);
	}
	
	void MainFrame::on_exit(wxCommandEvent &e)
	{
		Close();
	}
	
	MainFrame::MainFrame()
		: wxFrame(nullptr, wxID_ANY, "pomocom")
	{
		// Initialize the timer interval based on settings
		m_timer_interval = 1000 * state.settings.update_interval;
		
		// Get info the current timing section
		m_si = &state.section_info[state.current_section];
		
		// Frame settings
		this->SetClientSize(540, 280);
		this->Centre();
		this->Show();
		
		// Set frame name
		if (state.settings.set_terminal_title)
		{
			std::stringstream title;
			title << "pomocom - " << state.file_name;
			this->SetTitle(title.str());
		}

		// Menus
		auto menu_file = new wxMenu;
		menu_file->Append(ID_MENU_EXIT, "&Exit", "exit pomocom");

		auto menu_help = new wxMenu;
		menu_help->Append(ID_MENU_ABOUT, "&About", "about pomocom");

		auto menu_bar = new wxMenuBar;
		menu_bar->Append(menu_file, "&File");
		menu_bar->Append(menu_help, "&Help");
		SetMenuBar(menu_bar);
		
		// Widget creation and layout
		auto panel = new wxPanel(this);
		auto btn_skip = new wxButton(panel, ID_BTN_QUIT, BTN_SKIP_LABEL, wxPoint(0, 25), wxSize(80, 24));
		m_btn_pause = new wxButton(panel, ID_BTN_PAUSE, BTN_START_LABEL, wxPoint(0, 0), wxSize(80, 24));
		m_txt_time = new wxStaticText(panel, ID_TXT_TIME, TXT_TIME_INITIAL_LABEL, wxPoint(90, 0), wxSize(200, 24));
		m_txt_section = new wxStaticText(panel, ID_TXT_SECTION, TXT_SECTION_INITIAL_LABEL, wxPoint(90, 25), wxSize(200, 24));
		
		// Status bar
		CreateStatusBar();
		SetStatusText(STATUS_INITIAL);
		
		// Event binding
		m_btn_pause->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &MainFrame::on_btn_pause, this);
		m_timer.Bind(wxEVT_TIMER, &MainFrame::on_timer, this);
		Bind(wxEVT_MENU, &MainFrame::on_about, this, ID_MENU_ABOUT);
		Bind(wxEVT_MENU, &MainFrame::on_exit, this, ID_MENU_EXIT);
	}
	
	// wxWidgets app
	struct App : public wxApp{
		bool OnInit() override
		{
			new MainFrame;
			return true;
		}
	};
	
	// Runs the wxWidgets app
	// Technically not a loop, but it's named that way to fit in with the other functions in all.hh
	void interface_wx_loop()
	{
		// Arguments aren't used, but they need to be created to run wxEntry()
		int argc = 0;
		wxChar **argv = nullptr;
		
		wxApp::SetInstance(new App());
		wxEntry(argc, argv);
		wxEntryCleanup();
	}
}
