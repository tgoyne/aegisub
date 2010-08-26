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

/// @file preferences.cpp
/// @brief Preferences dialogue
/// @ingroup configuration_ui


#ifndef AGI_PRE
#include <iterator>

#include <wx/checkbox.h>
#include <wx/combobox.h>
#include <wx/filefn.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/stdpaths.h>
#include <wx/treebook.h>
#endif

#include <libaegisub/exception.h>

#include "colour_button.h"
#include "compat.h"
#include "libresrc/libresrc.h"
#include "preferences.h"
#include "main.h"
#include "include/aegisub/audio_player.h"
#include "include/aegisub/audio_provider.h"
#include "include/aegisub/subtitles_provider.h"
#include "video_provider_manager.h"

/// Define make all platform-specific options visible in a single view.
#define SHOW_ALL 1

#include "preferences_base.h"

/// General preferences page
General::General(wxTreebook *book): OptionPage(book, _("General")) {

	wxFlexGridSizer *startup = PageSizer(_("Startup"));
	OptionAdd(startup, _("Check for updates"), "App/Auto/Check For Updates");
	OptionAdd(startup, _("Show Splash Screen"), "App/Splash");

	wxFlexGridSizer *recent = PageSizer(_("Recently Used Lists"));
	OptionAdd(recent, _("Files"), "Limits/MRU");
	OptionAdd(recent, _("Find/Replace"), "Limits/Find Replace");
	sizer->AddSpacer(15);

	wxFlexGridSizer *undo = PageSizer(_("Undo / Redo Settings"));
	OptionAdd(undo, _("Undo Levels"), "Limits/MRU");

	SetSizerAndFit(sizer);
}


/// Subtitles preferences page
Subtitles::Subtitles(wxTreebook *book): OptionPage(book, _("Subtitles")) {

	wxFlexGridSizer *general = PageSizer(_("Options"));
	OptionAdd(general, _("Enable call tips"), "App/Call Tips");
	OptionAdd(general, _("Enable syntax highlighting"), "Subtitle/Highlight/Syntax");
	OptionAdd(general, _("Link commiting of times"), "Subtitle/Edit Box/Link Time Boxes Commit");
	OptionAdd(general, _("Overwrite-Insertion in time boxes"), "Subtitle/Time Edit/Insert Mode");

	wxFlexGridSizer *grid = PageSizer(_("Grid"));
	OptionAdd(grid, _("Allow grid to take focus"), "Subtitle/Grid/Focus Allow");
	OptionAdd(grid, _("Highlight visible subtitles"), "Subtitle/Grid/Highlight Subtitles in Frame");

	SetSizerAndFit(sizer);
}


/// Audio preferences page
Audio::Audio(wxTreebook *book): OptionPage(book, _("Audio")) {

	wxFlexGridSizer *general = PageSizer(_("Options"));
	OptionAdd(general, _("Grab times from line upon selection"), "Audio/Grab Times on Select");
	OptionAdd(general, _("Default mouse wheel to zoom"), "Audio/Wheel Default to Zoom");
	OptionAdd(general, _("Lock scroll on cursor"), "Audio/Lock Scroll on Cursor");
	OptionAdd(general, _("Snap to keyframes"), "Audio/Display/Snap/Keyframes");
	OptionAdd(general, _("Snap to adjacent lines"), "Audio/Display/Snap/Other Lines");
	OptionAdd(general, _("Auto-focus on mouse over"), "Audio/Auto/Focus");
	OptionAdd(general, _("Play audio when stepping in video"), "Audio/Plays When Stepping Video");
	CellSkip(general);
	OptionAdd(general, _("Default timing length"), "Timing/Default Duration", 0, 36000);
	OptionAdd(general, _("Default lead-in length"), "Audio/Lead/IN", 0, 36000);
	OptionAdd(general, _("Default lead-out length"), "Audio/Lead/OUT", 0, 36000);

	const wxString dtl_arr[3] = { _("Don't show"), _("Show previous"), _("Show all") };
	wxArrayString choice_dtl(3, dtl_arr);
	OptionChoice(general, _("Show inactive lines"), choice_dtl, "Audio/Inactive Lines Display Mode");
	OptionAdd(general, _("Start-marker drag sensitivity"), "Audio/Start Drag Sensitivity", 1, 15);

	wxFlexGridSizer *display = PageSizer(_("Display Visual Options"));
	OptionAdd(display, _("Secondary lines"), "Audio/Display/Draw/Secondary Lines");
	OptionAdd(display, _("Selection background"), "Audio/Display/Draw/Selection Background");
	OptionAdd(display, _("Timeline"), "Audio/Display/Draw/Timeline");
	OptionAdd(display, _("Cursor time"), "Audio/Display/Draw/Cursor Time");
	OptionAdd(display, _("Keyframes"), "Audio/Display/Draw/Keyframes");
	OptionAdd(display, _("Video position"), "Audio/Display/Draw/Video Position");

	SetSizerAndFit(sizer);
}


/// Video preferences page
Video::Video(wxTreebook *book): OptionPage(book, _("Video")) {

	wxFlexGridSizer *general = PageSizer(_("Options"));
	OptionAdd(general, _("Show keyframes in slider"), "Video/Slider/Show Keyframes");
	OptionAdd(general, _("Always show visual tools"), "Tool/Visual/Always Show");

	const wxString cres_arr[3] = { _("Never"), _("Ask"), _("Always") };
	wxArrayString choice_res(3, cres_arr);
	OptionChoice(general, _("Match video resolution on open"), choice_res, "Video/Check Script Res");

	const wxString czoom_arr[24] = { _T("12.5%"), _T("25%"), _T("37.5%"), _T("50%"), _T("62.5%"), _T("75%"), _T("87.5%"), _T("100%"), _T("112.5%"), _T("125%"), _T("137.5%"), _T("150%"), _T("162.5%"), _T("175%"), _T("187.5%"), _T("200%"), _T("212.5%"), _T("225%"), _T("237.5%"), _T("250%"), _T("262.5%"), _T("275%"), _T("287.5%"), _T("300%") };
	wxArrayString choice_zoom(24, czoom_arr);
	OptionChoice(general, _("Default Zoom"), choice_zoom, "Video/Default Zoom");

	OptionAdd(general, _("Fast jump step in frames"), "Video/Slider/Fast Jump Step");

	const wxString cscr_arr[3] = { _("?video"), _("?script"), _(".") };
	wxArrayString scr_res(3, cscr_arr);
	OptionChoice(general, _("Screenshot save path"), scr_res, "Path/Screenshot");

	SetSizerAndFit(sizer);
}


/// Interface preferences page
Interface::Interface(wxTreebook *book): OptionPage(book, _("Interface")) {

	wxFlexGridSizer *grid = PageSizer(_("Subtitle Grid"));
	OptionBrowse(grid, _("Font face"), BROWSE_FONT, "Subtitle/Grid/Font Face");
	OptionAdd(grid, _("Font size"), "Subtitle/Grid/Font Size", 3, 42);

	OptionAdd(grid, _("Hide overrides symbol"), "Subtitle/Grid/Hide Overrides Char");

	SetSizerAndFit(sizer);
}


/// Interface Colours preferences subpage
Interface_Colours::Interface_Colours(wxTreebook *book): OptionPage(book, _("Colours"), PAGE_SCROLL|PAGE_SUB) {

	wxFlexGridSizer *general = PageSizer(_("General"));
	OptionAdd(general, _("Modified Background"), "Colour/Background/Modified");

	wxFlexGridSizer *audio = PageSizer(_("Audio Display"));
	OptionAdd(audio, _("Play cursor"), "Colour/Audio Display/Play Cursor");
	OptionAdd(audio, _("Background"), "Colour/Audio Display/Background/Background");
	OptionAdd(audio, _("Selection background"), "Colour/Audio Display/Background/Selection");
	OptionAdd(audio, _("Selection background modified"), "Colour/Audio Display/Background/Selection Modified");
	OptionAdd(audio, _("Seconds boundaries"), "Colour/Audio Display/Seconds Boundaries");
	OptionAdd(audio, _("Waveform"), "Colour/Audio Display/Waveform");
	OptionAdd(audio, _("Waveform selected"), "Colour/Audio Display/Waveform Selected");
	OptionAdd(audio, _("Waveform Modified"), "Colour/Audio Display/Waveform Modified");
	OptionAdd(audio, _("Waveform Inactive"), "Colour/Audio Display/Waveform Inactive");
	OptionAdd(audio, _("Line boundary start"), "Colour/Audio Display/Line boundary Start");
	OptionAdd(audio, _("Line boundary end"), "Colour/Audio Display/Line boundary End");
	OptionAdd(audio, _("Line boundary inactive line"), "Colour/Audio Display/Line Boundary Inactive Line");
	OptionAdd(audio, _("Syllable text"), "Colour/Audio Display/Syllable Text");
	OptionAdd(audio, _("Syllable boundaries"), "Colour/Audio Display/Syllable Boundaries");

	wxFlexGridSizer *syntax = PageSizer(_("Syntax Highlighting"));
	OptionAdd(syntax, _("Normal"), "Colour/Subtitle/Syntax/Normal");
	OptionAdd(syntax, _("Brackets"), "Colour/Subtitle/Syntax/Brackets");
	OptionAdd(syntax, _("Slashes and Parentheses"), "Colour/Subtitle/Syntax/Slashes");
	OptionAdd(syntax, _("Tags"), "Colour/Subtitle/Syntax/Highlight Tags");
	OptionAdd(syntax, _("Parameters"), "Colour/Subtitle/Syntax/Parameters");
	OptionAdd(syntax, _("Error"), "Colour/Subtitle/Syntax/Error");
	OptionAdd(syntax, _("Error Background"), "Colour/Subtitle/Syntax/Background/Error");
	OptionAdd(syntax, _("Line Break"), "Colour/Subtitle/Syntax/Line Break");
	OptionAdd(syntax, _("Karaoke templates"), "Colour/Subtitle/Syntax/Karaoke Template");

	wxFlexGridSizer *grid = PageSizer(_("Subtitle Grid"));
	OptionAdd(grid, _("Standard foreground"), "Colour/Subtitle Grid/Standard");
	OptionAdd(grid, _("Standard background"), "Colour/Subtitle Grid/Background/Background");
	OptionAdd(grid, _("Selection foreground"), "Colour/Subtitle Grid/Selection");
	OptionAdd(grid, _("Selection background"), "Colour/Subtitle Grid/Background/Selection");
	OptionAdd(grid, _("Comment background"), "Colour/Subtitle Grid/Background/Comment");
	OptionAdd(grid, _("Selected comment background"), "Colour/Subtitle Grid/Background/Selected Comment");
	OptionAdd(grid, _("Left Column"), "Colour/Subtitle Grid/Left Column");
	OptionAdd(grid, _("Active Line Border"), "Colour/Subtitle Grid/Active Border");
	OptionAdd(grid, _("Lines"), "Colour/Subtitle Grid/Lines");

	SetSizerAndFit(sizer);
}


/// Interface Hotkeys preferences subpage
Interface_Hotkeys::Interface_Hotkeys(wxTreebook *book): OptionPage(book, _("Hotkeys"), PAGE_SUB) {

	wxFlexGridSizer *hotkeys = PageSizer(_("Hotkeys"));
	hotkeys->Add(new wxStaticText(this, wxID_ANY, _T("To be added after hotkey rewrite.")), 0, wxALL, 5);
	SetSizerAndFit(sizer);
}


/// Paths preferences page class Paths: public OptionPage { public:
Paths::Paths(wxTreebook *book): OptionPage(book, _("Paths")) {

	wxFlexGridSizer *general = PageSizer(_("General"));
	general->Add(new wxStaticText(this, wxID_ANY, _T("TBD..")), 0, wxALL, 5);

	SetSizerAndFit(sizer);
}


/// File Associations preferences page
File_Associations::File_Associations(wxTreebook *book): OptionPage(book, _("File Associations")) {

	wxFlexGridSizer *assoc = PageSizer(_("General"));
	assoc->Add(new wxStaticText(this, wxID_ANY, _T("TBD..")), 0, wxALL, 5);

	SetSizerAndFit(sizer);
}


/// Backup preferences page
Backup::Backup(wxTreebook *book): OptionPage(book, _("Backup")) {
	wxFlexGridSizer *save = PageSizer(_("Automatic Save"));
	OptionAdd(save, _("Enable"), "App/Auto/Backup");
	CellSkip(save);
	OptionAdd(save, _("Interval in seconds."), "App/Auto/Save Every Seconds");
	OptionBrowse(save, _("Path"), BROWSE_FOLDER, "Path/Auto/Save");

	wxFlexGridSizer *backup = PageSizer(_("Automatic Backup"));
	CellSkip(backup);
	OptionAdd(backup, _("Enable"), "App/Auto/Backup");
	OptionBrowse(backup, _("Path"), BROWSE_FOLDER, "Path/Auto/Backup");

	SetSizerAndFit(sizer);
}



/// Automation preferences page
Automation::Automation(wxTreebook *book): OptionPage(book, _("Automation")) {
	wxFlexGridSizer *general = PageSizer(_("General"));

	OptionAdd(general, _("Base path"), "Path/Automation/Base");
	OptionAdd(general, _("Include path"), "Path/Automation/Include");
	OptionAdd(general, _("Auto-load path"), "Path/Automation/Autoload");

	const wxString tl_arr[6] = { _("Fatal"), _("Error"), _("Warning"), _("Hint"), _("Debug"), _("Trace") };
	wxArrayString tl_choice(6, tl_arr);
	OptionChoice(general, _("Trace level"), tl_choice, "Automation/Trace Level");

	const wxString tp_arr[3] = { _("Normal"), _("Below Normal (recommended)"), _("Lowest") };
	wxArrayString tp_choice(3, tp_arr);
	OptionChoice(general, _("Thread priority"), tp_choice, "Automation/Lua/Thread Priority");

	const wxString ar_arr[4] = { _("No scripts"), _("Subtitle-local scripts"), _("Global autoload scripts"), _("All scripts") };
	wxArrayString ar_choice(4, ar_arr);
	OptionChoice(general, _("Autoreload on Export"), ar_choice, "Automation/Autoreload Mode");

	SetSizerAndFit(sizer);
}


/// Advanced preferences page
Advanced::Advanced(wxTreebook *book): OptionPage(book, _("Advanced")) {
	wxFlexGridSizer *general = PageSizer(_("General"));

	wxStaticText *warning = new wxStaticText(this, wxID_ANY ,_("Changing these settings might result in bugs and/or crashes.  Do not touch these unless you know what you're doing."));
	warning->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	sizer->Fit(this);
	warning->Wrap(400);
	general->Add(warning, 0, wxALL, 5);

	SetSizerAndFit(sizer);
}


/// Advanced Interface preferences subpage
Advanced_Interface::Advanced_Interface(wxTreebook *book): OptionPage(book, _("Backup"), PAGE_SUB) {
	wxFlexGridSizer *interface_ = PageSizer(_("Interface"));

	interface_->Add(new wxStaticText(this, wxID_ANY, _T("TBD..")), 0, wxALL, 5);

	SetSizerAndFit(sizer);
}

static wxArrayString vec_to_arrstr(std::vector<std::string> const& vec) {
	wxArrayString arrstr;
	std::copy(vec.begin(), vec.end(), std::back_inserter(arrstr));
	return arrstr;
}

/// Advanced Audio preferences subpage
Advanced_Audio::Advanced_Audio(wxTreebook *book): OptionPage(book, _("Audio"), PAGE_SUB) {
	wxFlexGridSizer *expert = PageSizer(_("Expert"));

	wxArrayString ap_choice = vec_to_arrstr(AudioProviderFactory::GetClasses());
	OptionChoice(expert, _("Audio provider"), ap_choice, "Audio/Provider");

	wxArrayString apl_choice = vec_to_arrstr(AudioPlayerFactory::GetClasses());
	OptionChoice(expert, _("Audio player"), apl_choice, "Audio/Player");

	wxFlexGridSizer *cache = PageSizer(_("Cache"));
	const wxString ct_arr[3] = { _("None (NOT RECOMMENDED)"), _("RAM"), _("Hard Disk") };
	wxArrayString ct_choice(3, ct_arr);
	OptionChoice(cache, _("Cache type"), ct_choice, "Audio/Cache/Type");

	OptionBrowse(cache, _("Path"), BROWSE_FOLDER, "Audio/Cache/HD/Location");
	OptionAdd(cache, _("File name"), "Audio/Cache/HD/Name");

	wxFlexGridSizer *spectrum = PageSizer(_("Spectrum"));

	OptionAdd(spectrum, _("Cutoff"), "Audio/Renderer/Spectrum/Cutoff");

	const wxString sq_arr[4] = { _("Regular quality"), _("Better quality"), _("High quality"), _("Insane quality") };
	wxArrayString sq_choice(4, sq_arr);
	OptionChoice(spectrum, _("Quality"), sq_choice, "Audio/Renderer/Spectrum/Quality");

	OptionAdd(spectrum, _("Cache memory max (MB)"), "Audio/Renderer/Spectrum/Memory Max", 2, 1024);

#if defined(WIN32) || defined(SHOW_ALL)
	wxFlexGridSizer *windows = PageSizer(_("Windows Specific"));
	const wxString adm_arr[3] = { _T("ConvertToMono"), _T("GetLeftChannel"), _T("GetRightChannel") };
	wxArrayString adm_choice(3, adm_arr);
	OptionChoice(windows, _("Avisynth down-mixer"), adm_choice, "Audio/Downmixer");
#endif

	SetSizerAndFit(sizer);
}


/// Advanced Video preferences subpage
Advanced_Video::Advanced_Video(wxTreebook *book): OptionPage(book, _("Video"), PAGE_SUB) {
	wxFlexGridSizer *expert = PageSizer(_("Expert"));

	wxArrayString vp_choice = vec_to_arrstr(VideoProviderFactory::GetClasses());
	OptionChoice(expert, _("Video provider"), vp_choice, "Video/Provider");

	wxArrayString sp_choice = vec_to_arrstr(SubtitlesProviderFactory::GetClasses());
	OptionChoice(expert, _("Subtitle provider"), sp_choice, "Subtitle/Provider");


#if defined(WIN32) || defined(SHOW_ALL)
	wxFlexGridSizer *windows = PageSizer(_("Windows Specific"));
	OptionAdd(windows, _("Allow pre-2.56a Avisynth"), "Provider/Avisynth/Allow Ancient");
	CellSkip(windows);
	OptionAdd(windows, _("Avisynth memory limit"), "Provider/Avisynth/Memory Max");
#endif

	SetSizerAndFit(sizer);
}
