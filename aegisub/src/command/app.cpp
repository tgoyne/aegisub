// Copyright (c) 2005-2010, Niels Martin Hansen
// Copyright (c) 2005-2010, Rodrigo Braz Monteiro
// Copyright (c) 2010, Amar Takhar
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of the Aegisub Group nor the names of its contributors
//     may be used to endorse or promote products derived from this software
//     without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Aegisub Project http://www.aegisub.org/
//
// $Id$

/// @file app.cpp
/// @brief app/ commands.
/// @ingroup command
///

#include "config.h"

#ifndef AGI_PRE
#endif

#include "command.h"

#include "aegisub/context.h"
#include "main.h"

#include "dialog_about.h"
#include "audio_controller.h"
#include "frame_main.h"
#include "video_context.h"
#include "utils.h"
#include "dialog_log.h"
#include "preferences.h"
#include "dialog_version_check.h"


namespace cmd {
/// @defgroup cmd-app Application related
/// @{

/// Launch about dialogue.
class app_about: public Command {
public:
	CMD_NAME("app/about")
	STR_MENU("&About..")
	STR_DISP("About")
	STR_HELP("About Aegisub.")

	void operator()(agi::Context *c) {
		AboutScreen About(c->parent);
		About.ShowModal();
	}
};


/// Display audio and subtitles.
class app_display_audio_subs: public Command {
public:
	CMD_NAME("app/display/audio_subs")
	STR_MENU("Audio+Subs View")
	STR_DISP("Audio+Subs View")
	STR_HELP("Display audio and subtitles only.")

	void operator()(agi::Context *c) {
		if (!c->audioController->IsAudioOpen()) return;
		wxGetApp().frame->SetDisplayMode(0,1);
	}
};


/// Display audio, video and subtitles.
class app_display_full: public Command {
public:
	CMD_NAME("app/display/full")
	STR_MENU("Full view")
	STR_DISP("Full view")
	STR_HELP("Display audio, video and subtitles.")

	void operator()(agi::Context *c) {
	if (!c->audioController->IsAudioOpen() || !VideoContext::Get()->IsLoaded()) return;
		wxGetApp().frame->SetDisplayMode(1,1);
	}
};


/// Display subtitles only.
class app_display_subs: public Command {
public:
	CMD_NAME("app/display/subs")
	STR_MENU("Subs Only View")
	STR_DISP("Subs Only View")
	STR_HELP("Display subtitles only.")

	void operator()(agi::Context *c) {
		wxGetApp().frame->SetDisplayMode(0,0);
	}
};


/// Display video and subtitles only.
class app_display_video_subs: public Command {
public:
	CMD_NAME("app/display/video_subs")
	STR_MENU("Video+Subs View")
	STR_DISP("Video+Subs View")
	STR_HELP("Display video and subtitles only.")

	void operator()(agi::Context *c) {
		wxGetApp().frame->SetDisplayMode(1,0);
	}
};


/// Exit the application.
class app_exit: public Command {
public:
	CMD_NAME("app/exit")
	STR_MENU("E&xit")
	STR_DISP("Exit")
	STR_HELP("Exit the application.")

	void operator()(agi::Context *c) {
		printf("XXX: not working yet\n");
	}
};


/// Select Aegisub interface language
class app_language: public Command {
public:
	CMD_NAME("app/language")
	STR_MENU("&Language...")
	STR_DISP("Language")
	STR_HELP("Select Aegisub interface language")

	void operator()(agi::Context *c) {
		// Get language
		AegisubApp *app = (AegisubApp*) wxTheApp;
		int old = app->locale.curCode;
		int newCode = app->locale.PickLanguage();
		// Is OK?
		if (newCode != -1) {
			// Set code
			OPT_SET("App/Locale")->SetInt(newCode);

			// Language actually changed?
			if (newCode != old) {
				// Ask to restart program
				int result = wxMessageBox(_T("Aegisub needs to be restarted so that the new language can be applied. Restart now?"),_T("Restart Aegisub?"),wxICON_QUESTION | wxYES_NO);
				if (result == wxYES) {
					// Restart Aegisub
					if (wxGetApp().frame->Close()) {
						RestartAegisub();
						//wxStandardPaths stand;
						//wxExecute(_T("\"") + stand.GetExecutablePath() + _T("\""));
					}
				}
			}
		}
	}
};


/// Event log.
class app_log: public Command {
public:
	CMD_NAME("app/log")
	STR_MENU("&Log window...")
	STR_DISP("Log window")
	STR_HELP("Event log.")

	void operator()(agi::Context *c) {
		LogWindow *log = new LogWindow(c->parent);
		log->Show(1);
	}
};


/// Open a new application window.
class app_new_window: public Command {
public:
	CMD_NAME("app/new_window")
	STR_MENU("New Window")
	STR_DISP("New Window")
	STR_HELP("Open a new application window.")

	void operator()(agi::Context *c) {
		RestartAegisub();
	}
};


/// Configure Aegisub.
class app_options: public Command {
public:
	CMD_NAME("app/options")
	STR_MENU("&Options..")
	STR_DISP("Options")
	STR_HELP("Configure Aegisub.")

	void operator()(agi::Context *c) {
		try {
			Preferences pref(c->parent);
			pref.ShowModal();
		} catch (agi::Exception& e) {
			LOG_E("config/init") << "Caught exception: " << e.GetName() << " -> " << e.GetMessage();
		}
	}
};


/// Check to see if there is a new version of Aegisub available.
class app_updates: public Command {
public:
	CMD_NAME("app/updates")
	STR_MENU("&Check for Updates..")
	STR_DISP("Check for Updates")
	STR_HELP("Check to see if there is a new version of Aegisub available.")

	void operator()(agi::Context *c) {
		PerformVersionCheck(true);
	}
};

/// @}

/// Init app/ commands
void init_app(CommandManager *cm) {
	cm->reg(new app_about());
	cm->reg(new app_display_audio_subs());
	cm->reg(new app_display_full());
	cm->reg(new app_display_subs());
	cm->reg(new app_display_video_subs());
	cm->reg(new app_exit());
	cm->reg(new app_language());
	cm->reg(new app_log());
	cm->reg(new app_new_window());
	cm->reg(new app_options());
	cm->reg(new app_updates());
}

} // namespace cmd

