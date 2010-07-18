// Copyright (c) 2010, Amar Takhar <verm@aegisub.org>
//
// Permission to use, copy, modify, and distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//
// $Id$

/// @file preferences.h
/// @brief Preferences dialogue
/// @see preferences.cpp
/// @ingroup configuration_ui

#ifndef AGI_PRE
#include <wx/dialog.h>
#include <wx/event.h>
#include <wx/treebook.h>
#include <wx/listctrl.h>
#endif

#include "browse_button.h"

class General;
class Subtitles;
class Audio;
//class Video;
//class Interface;
//class Interface_Colours;
//class Interface_Hotkeys;
//class Paths;
//class File_Associations;
//class Backup;
//class Automation;
//class Advanced;
//class Advanced_Interface;
//class Advanced_Audio;
//class Advanced_Video;

class Preferences: public wxDialog {
	wxTreebook *book;

	void OnOK(wxCommandEvent &event);
	void OnCancel(wxCommandEvent &event);
	void OnApply(wxCommandEvent &event);

	General *general;
	Subtitles *subtitles;
	Audio *audio;
//	Video *video;
//	Interface *interface;
//	Interface_Colours *interface_colours;
//	Interface_Hotkeys *interface_hotkeys;
//	Paths *paths;
//	File_Associations *file_associations;
//	Backup *backup;
//	Automation *automation;
//	Advanced *advanced;
//	Advanced_Interface *advanced_interface;
//	Advanced_Audio *advanced_audio;
//	Advanced_Video *advanced_video;

//	void OptionAdd(wxPanel *parent, wxFlexGridSizer *flex, const wxString &name, const char *opt_name);
	void OptionAdd(wxPanel *parent, wxFlexGridSizer *flex, const wxString &name, const char *opt_name, double min=0, double max=100, double inc=1);
	void OptionChoice(wxPanel *parent, wxFlexGridSizer *flex, const wxString &name, const wxArrayString &choices, const char *opt_name);
	void OptionBrowse(wxPanel *parent, wxFlexGridSizer *flex, const wxString &name, BrowseType browse_type, const char *opt_name);

	void Video(wxTreebook *book);
	void Interface(wxTreebook *book);
	void Interface_Colours(wxTreebook *book);
	void Interface_Hotkeys(wxTreebook *book);
	void Paths(wxTreebook *book);
	void File_Associations(wxTreebook *book);
	void Backup(wxTreebook *book);
	void Automation(wxTreebook *book);
	void Advanced(wxTreebook *book);
	void Advanced_Interface(wxTreebook *book);
	void Advanced_Audio(wxTreebook *book);
	void Advanced_Video(wxTreebook *book);

public:
	Preferences(wxWindow *parent);
	~Preferences();

	DECLARE_EVENT_TABLE()
};

