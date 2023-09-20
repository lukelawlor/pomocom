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
#include <wx/hyperlink.h>
#include <wx/artprov.h>

#include "../pomocom.hh" // For SectionInfo
#include "../state.hh"
#include "all.hh"
#include "base.hh"

namespace chrono = std::chrono;

namespace pomocom
{
	using Clock = chrono::high_resolution_clock;

	// String literals
	
	// Image filenames
	constexpr auto S_FILE_ICON_SMALL = "icon_16x16.png";
	constexpr auto S_FILE_ICON_LARGE = "icon_48x64.png";

	// Window titles
	constexpr auto S_TITLE_DEFAULT = "pomocom";
	constexpr auto S_TITLE_ABOUT = "About pomocom";
	
	// Hyperlinks
	constexpr auto S_LINK_CODEBERG_LABEL = "Codeberg Repository";
	constexpr auto S_LINK_CODEBERG_URL = "https://codeberg.org/lukelawlor/pomocom";
	constexpr auto S_LINK_GITHUB_LABEL = "GitHub Repository";
	constexpr auto S_LINK_GITHUB_URL = "https://github.com/lukelawlor/pomocom";

	// Labels of buttons
	constexpr auto S_BTN_START = "Start";
	constexpr auto S_BTN_PAUSE = "Pause";
	constexpr auto S_BTN_RESUME = "Resume";
	constexpr auto S_BTN_SKIP = "Skip";

	// About window text
	constexpr auto S_ABOUT_NAME = "pomocom";
	constexpr auto S_ABOUT_VERSION = "v" POMOCOM_VERSION;
	constexpr auto S_ABOUT_DESC = "a lightweight and configurable pomodoro timer";
	constexpr auto S_ABOUT_COPYRIGHT = "Copyright (c) 2023 by Luke Lawlor <lklawlor1@gmail.com>";

	// Status bar text
	constexpr auto S_STATUS_TIME_STARTED = "Time started";
	constexpr auto S_STATUS_TIME_PAUSED = "Time paused";
	constexpr auto S_STATUS_TIME_RESUMED = "Time resumed";
	constexpr auto S_STATUS_TIME_UP = "Time up!";
	constexpr auto S_STATUS_INIT = "Welcome to pomocom!";

	// Text widget text
	constexpr auto S_TXT_TIME_INIT = "time left goes here";
	constexpr auto S_TXT_TIME_UP = S_STATUS_TIME_UP;
	constexpr auto S_TXT_SECTION_INIT = "section name goes here";
	
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

	// About window
	struct AboutWin : public wxDialog{
		AboutWin();
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

		// About window
		AboutWin m_about_win;

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
			m_btn_pause->SetLabel(S_BTN_PAUSE);
			SetStatusText(S_STATUS_TIME_STARTED);
			
			// Update the timer state
			m_timer_data.state = TSTATE_TIMER_RUNNING;
			break;
		case TSTATE_TIMER_RUNNING:
			// Pause the timer
			
			m_timer_data.pause_start = Clock::now();
			
			// Stop the wxTimer
			m_timer.Stop();
			
			// Update the UI
			m_btn_pause->SetLabel(S_BTN_RESUME);
			SetStatusText(S_STATUS_TIME_PAUSED);
			
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
			m_btn_pause->SetLabel(S_BTN_PAUSE);
			SetStatusText(S_STATUS_TIME_RESUMED);
			
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
			m_txt_time->SetLabel(S_TXT_TIME_UP);
			m_txt_section->SetLabel(m_si->name);
			m_btn_pause->SetLabel(S_BTN_START);
			SetStatusText(S_STATUS_TIME_UP);
			
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
		m_about_win.Centre();
		m_about_win.ShowModal();
	}
	
	void MainFrame::on_exit(wxCommandEvent &e)
	{
		Close();
	}
	
	MainFrame::MainFrame()
		: wxFrame(nullptr, wxID_ANY, S_TITLE_DEFAULT)
	{
		// Initialize the timer interval based on settings
		m_timer_interval = 1000 * state.settings.update_interval;
		
		// Get info the current timing section
		m_si = &state.section_info[state.current_section];
		
		// Frame settings
		this->SetClientSize(540, 280);
		this->Centre();
		if (!state.settings.wx.show_resize_symbol)
			this->SetWindowStyle(wxDEFAULT_FRAME_STYLE & ~(wxRESIZE_BORDER));
		
		// Set frame name
		std::stringstream title;
		title << "pomocom - " << state.file_name;
		this->SetTitle(title.str());
		
		// Set frame icon
		std::stringstream icon_path;
		icon_path << state.settings.path.res << S_FILE_ICON_SMALL;
		wxIcon icon(icon_path.str(), wxBITMAP_TYPE_ANY, 16, 16);
		SetIcon(icon);

		// Menus
		if (state.settings.wx.show_menu_bar)
		{
			auto menu_file = new wxMenu;
			menu_file->Append(ID_MENU_EXIT, "&Exit", "Exit pomocom");
			
			auto menu_help = new wxMenu;
			menu_help->Append(ID_MENU_ABOUT, "&About", "About pomocom");
			
			auto menu_bar = new wxMenuBar;
			menu_bar->Append(menu_file, "&File");
			menu_bar->Append(menu_help, "&Help");
			SetMenuBar(menu_bar);
		}
		
		// Widget creation and layout
		auto panel = new wxPanel(this);
		auto btn_skip = new wxButton(panel, ID_BTN_QUIT, S_BTN_SKIP, wxPoint(0, 24), wxSize(80, 24));
		m_btn_pause = new wxButton(panel, ID_BTN_PAUSE, S_BTN_START, wxPoint(0, 0), wxSize(80, 24));
		m_txt_time = new wxStaticText(panel, ID_TXT_TIME, S_TXT_TIME_INIT, wxPoint(90, 0), wxSize(200, 24));
		m_txt_section = new wxStaticText(panel, ID_TXT_SECTION, S_TXT_SECTION_INIT, wxPoint(90, 25), wxSize(200, 24));
		
		// Status bar
		CreateStatusBar();
		SetStatusText(S_STATUS_INIT);
		
		// Event binding
		m_btn_pause->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &MainFrame::on_btn_pause, this);
		m_timer.Bind(wxEVT_TIMER, &MainFrame::on_timer, this);
		Bind(wxEVT_MENU, &MainFrame::on_about, this, ID_MENU_ABOUT);
		Bind(wxEVT_MENU, &MainFrame::on_exit, this, ID_MENU_EXIT);
	}

	AboutWin::AboutWin()
		: wxDialog(nullptr, wxID_ANY, S_TITLE_ABOUT)
	{
		// Window settings
		this->Centre();

		// Pomocom icon
		std::stringstream icon_path;
		icon_path << state.settings.path.res << S_FILE_ICON_LARGE;
		wxBitmap bmp(icon_path.str(), wxBITMAP_TYPE_ANY);

		if (!bmp.IsOk())
		{
			// Use a missing art image if the bitmap fails
			bmp = wxArtProvider::GetBitmap(wxART_MISSING_IMAGE);
		}

		auto bmp_pomocom_icon = new wxStaticBitmap(this, wxID_ANY, bmp);

		// Text
		wxFont font;
		auto txt_name = new wxStaticText(this, wxID_ANY, S_ABOUT_NAME);
		font = txt_name->GetFont();
		font.SetWeight(wxFONTWEIGHT_BOLD);
		txt_name->SetFont(font);
		auto txt_version = new wxStaticText(this, wxID_ANY, S_ABOUT_VERSION);
		auto txt_desc = new wxStaticText(this, wxID_ANY, S_ABOUT_DESC);
		auto txt_copyright = new wxStaticText(this, wxID_ANY, S_ABOUT_COPYRIGHT);
		font = txt_copyright->GetFont();
		font.SetPointSize(10);
		txt_copyright->SetFont(font);

		// Links
		auto link_codeberg = new wxHyperlinkCtrl(this, wxID_ANY, S_LINK_CODEBERG_LABEL, S_LINK_CODEBERG_URL);
		auto link_github = new wxHyperlinkCtrl(this, wxID_ANY, S_LINK_GITHUB_LABEL, S_LINK_GITHUB_URL);

		wxBoxSizer *sizer_link = new wxBoxSizer(wxHORIZONTAL);
		sizer_link->Add(link_codeberg, 0, wxALIGN_CENTRE);
		sizer_link->AddSpacer(10);
		sizer_link->Add(link_github, 0, wxALIGN_CENTRE);
		
		wxBoxSizer *sizer_vert = new wxBoxSizer(wxVERTICAL);
		sizer_vert->Add(bmp_pomocom_icon, 0, wxALIGN_CENTRE | wxALL, 20);
		sizer_vert->Add(txt_name, 0, wxALIGN_CENTRE | wxBOTTOM, 20);
		sizer_vert->Add(txt_version, 0, wxALIGN_CENTRE | wxBOTTOM, 20);
		sizer_vert->Add(txt_desc, 0, wxALIGN_CENTRE | wxBOTTOM, 20);
		sizer_vert->Add(sizer_link, 0, wxALIGN_CENTRE | wxBOTTOM, 20);
		sizer_vert->Add(txt_copyright, 0, wxALIGN_CENTRE | wxALL, 20);
		sizer_vert->Add(CreateButtonSizer(wxCLOSE), 0, wxALIGN_CENTRE | wxALL, 20);
		SetSizerAndFit(sizer_vert);
	}
	
	// wxWidgets app
	struct App : public wxApp{
		bool OnInit() override
		{
			// Allow PNG image loading
			wxImage::AddHandler(new wxPNGHandler);

			auto main_frame = new MainFrame;
			main_frame->Show();
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
