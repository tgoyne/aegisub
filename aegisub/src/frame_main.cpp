// Copyright (c) 2005, Rodrigo Braz Monteiro, Niels Martin Hansen
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

/// @file frame_main.cpp
/// @brief Main window creation and control management
/// @ingroup main_ui
///


///////////////////
// Include headers
#include "config.h"

#ifndef AGI_PRE
#include <wx/filename.h>
#include <wx/image.h>
#include <wx/mimetype.h>
#include <wx/statline.h>
#include <wx/sysopt.h>
#include <wx/tokenzr.h>
#endif

#ifdef __WXMAC__
#include <Carbon/Carbon.h>
#endif

#include "ass_file.h"
#include "audio_box.h"
#include "audio_display.h"
#ifdef WITH_AUTOMATION
#include "auto4_base.h"
#endif
#ifdef WITH_AVISYNTH
#include "avisynth_wrap.h"
#endif
#include "charset_conv.h"
#include "compat.h"
#include "dialog_detached_video.h"
#include "dialog_search_replace.h"
#include "dialog_splash.h"
#include "dialog_styling_assistant.h"
#include "dialog_tip.h"
#include "dialog_version_check.h"
#include "drop.h"
#include "frame_main.h"
#include "help_button.h"
#include "hotkeys.h"
#include "keyframe.h"
#include "libresrc/libresrc.h"
#include "main.h"
#include "options.h"
#include "standard_paths.h"
#include "subs_edit_box.h"
#include "subs_grid.h"
#include "text_file_reader.h"
#include "text_file_writer.h"
#include "utils.h"
#include "version.h"
#include "vfr.h"
#include "video_box.h"
#include "video_context.h"
#include "video_display.h"
#include "video_provider_manager.h"
#include "video_slider.h"


#ifdef WITH_STARTUPLOG

/// DOCME
#define StartupLog(a) MessageBox(0, a, _T("Aegisub startup log"), 0)
#else

/// DOCME
#define StartupLog(a)
#endif

/////////////////////////
// FrameMain constructor


/// @brief DOCME
/// @param args 
///
FrameMain::FrameMain (wxArrayString args)
: wxFrame ((wxFrame*)NULL,-1,_T(""),wxDefaultPosition,wxSize(920,700),wxDEFAULT_FRAME_STYLE | wxCLIP_CHILDREN)
{
	StartupLog(_T("Entering FrameMain constructor"));
#ifdef __WXGTK__
/* XXX HACK XXX
 * Gtk just got initialized. And if we're using the SCIM IME,
 * it just did a setlocale(LC_ALL, ""). so, BOOM.
 */
 	setlocale(LC_ALL, "");
	setlocale(LC_CTYPE, "C");
	setlocale(LC_NUMERIC, "C");
/* XXX HACK XXX */
#endif

	// Set application's frame
	AegisubApp::Get()->frame = this;

	// Initialize flags
	HasSelection = false;
	menuCreated = false;
	blockAudioLoad = false;
	blockAudioLoad = false;

	StartupLog(_T("Install PNG handler"));
	// Create PNG handler
	wxPNGHandler *png = new wxPNGHandler;
	wxImage::AddHandler(png);

	// Storage for subs-file-local scripts
#ifdef WITH_AUTOMATION
	StartupLog(_T("Create local Automation script manager"));
	local_scripts = new Automation4::ScriptManager();
#endif

	// Create menu and tool bars
	StartupLog(_T("Apply saved Maximized state"));
	if (OPT_GET("App/Maximized")->GetBool()) Maximize(true);
	StartupLog(_T("Initialize toolbar"));
	InitToolbar();
	StartupLog(_T("Initialize menu bar"));
	InitMenu();
	
	// Create status bar
	StartupLog(_T("Create status bar"));
	CreateStatusBar(2);

	// Set icon
	StartupLog(_T("Set icon"));
#ifdef _WINDOWS
	SetIcon(wxICON(wxicon));
#else
	wxIcon icon;
	icon.CopyFromBitmap(GETIMAGE(wxicon_misc));
	SetIcon(icon);
#endif

	// Contents
	showVideo = true;
	showAudio = true;
	detachedVideo = NULL;
	stylingAssistant = NULL;
	StartupLog(_T("Initialize inner main window controls"));
	InitContents();
	StartupLog(_T("Display main window"));
	Show();

	// Splash screen
	// It doesn't work properly on wxMac, and the jumping dock icon
	// signals the same as the splash screen either way.
#if !_DEBUG && !__WXMAC__
	if (OPT_GET("App/Splash")->GetBool()) {
		SplashScreen *splash = new SplashScreen(this);
		splash->Show(true);
		splash->Update();
	}
	else
#endif

	wxSafeYield();

	// Set autosave timer
	StartupLog(_T("Set up Auto Save"));
	AutoSave.SetOwner(this,AutoSave_Timer);
	int time = OPT_GET("App/Auto/Save Every Seconds")->GetInt();
	if (time > 0) {
		AutoSave.Start(time*1000);
	}

	// Set accelerator keys
	StartupLog(_T("Install hotkeys"));
	PreviousFocus = NULL;
	SetAccelerators();

	// Set drop target
	StartupLog(_T("Set up drag/drop target"));
	SetDropTarget(new AegisubFileDropTarget(this));

	// Parse arguments
	StartupLog(_T("Initialize empty file"));
	LoadSubtitles(_T(""));
	StartupLog(_T("Load files specified on command line"));
	LoadList(args);

	// Version checker
	StartupLog(_T("Possibly perform automatic updates check"));
	int option = OPT_GET("App/Auto/Check For Updates")->GetInt();
	if (option == -1) {
		int result = wxMessageBox(_("Do you want Aegisub to check for updates whenever it starts? You can still do it manually via the Help menu."),_("Check for updates?"),wxYES_NO);
		option = (result == wxYES);
		OPT_SET("App/Auto/Check For Updates")->SetInt(option);
	}

	PerformVersionCheck(false);

	//ShowFullScreen(true,wxFULLSCREEN_NOBORDER | wxFULLSCREEN_NOCAPTION);
	StartupLog(_T("Leaving FrameMain constructor"));
}



/// @brief FrameMain destructor 
///
FrameMain::~FrameMain () {
	DeInitContents();
#ifdef WITH_AUTOMATION
	delete local_scripts;
#endif
}



/// @brief Initialize toolbar 
///
void FrameMain::InitToolbar () {
	// Create toolbar
	wxSystemOptions::SetOption(_T("msw.remap"), 0);
	Toolbar = CreateToolBar(wxTB_FLAT | wxTB_HORIZONTAL,-1,_T("Toolbar"));

	// Subtitle control buttons
	Toolbar->AddTool(Menu_File_New_Subtitles,_("New"),GETIMAGE(new_toolbutton_24),_("New subtitles"));
	Toolbar->AddTool(Menu_File_Open_Subtitles,_("Open"),GETIMAGE(open_toolbutton_24),_("Open subtitles"));
	Toolbar->AddTool(Menu_File_Save_Subtitles,_("Save"),GETIMAGE(save_toolbutton_24),_("Save subtitles"));
	Toolbar->AddSeparator();

	// Video zoom controls
	Toolbar->AddTool(Menu_Video_JumpTo,_("Jump To..."),GETIMAGE(jumpto_button_24),wxNullBitmap,wxITEM_NORMAL,_("Jump video to time/frame"));
	Toolbar->AddTool(Menu_Video_Zoom_In,_("Zoom in"),GETIMAGE(zoom_in_button_24),wxNullBitmap,wxITEM_NORMAL,_("Zoom video in"));
	Toolbar->AddTool(Menu_Video_Zoom_Out,_("Zoom out"),GETIMAGE(zoom_out_button_24),wxNullBitmap,wxITEM_NORMAL,_("Zoom video out"));
	wxArrayString choices;
	for (int i=1;i<=24;i++) {
		wxString toAdd = wxString::Format(_T("%i"),int(i*12.5));
		if (i%2) toAdd += _T(".5");
		toAdd += _T("%");
		choices.Add(toAdd);
	}
	ZoomBox = new wxComboBox(Toolbar,Toolbar_Zoom_Dropdown,_T("75%"),wxDefaultPosition,wxDefaultSize,choices,wxCB_DROPDOWN);
	Toolbar->AddControl(ZoomBox);
	Toolbar->AddSeparator();

	// More video buttons
	Toolbar->AddTool(Menu_Subs_Snap_Video_To_Start,_("Jump video to start"),GETIMAGE(video_to_substart_24),_("Jumps the video to the start frame of current subtitle"));
	Toolbar->AddTool(Menu_Subs_Snap_Video_To_End,_("Jump video to end"),GETIMAGE(video_to_subend_24),_("Jumps the video to the end frame of current subtitle"));
	Toolbar->AddTool(Menu_Subs_Snap_Start_To_Video,_("Snap start to video"),GETIMAGE(substart_to_video_24),_("Set start of selected subtitles to current video frame"));
	Toolbar->AddTool(Menu_Subs_Snap_End_To_Video,_("Snap end to video"),GETIMAGE(subend_to_video_24),_("Set end of selected subtitles to current video frame"));
	Toolbar->AddTool(Menu_Video_Select_Visible,_("Select visible"),GETIMAGE(select_visible_button_24),_("Selects all lines that are currently visible on video frame"));
	Toolbar->AddTool(Menu_Video_Snap_To_Scene,_("Snap subtitles to scene"),GETIMAGE(snap_subs_to_scene_24),_("Snap selected subtitles so they match current scene start/end"));
	Toolbar->AddTool(Menu_Video_Shift_To_Frame,_("Shift subtitles to frame"),GETIMAGE(shift_to_frame_24),_("Shift selected subtitles so first selected starts at this frame"));
	Toolbar->AddSeparator();

	// Property stuff
	Toolbar->AddTool(Menu_Tools_Styles_Manager,_("Styles Manager"),GETIMAGE(style_toolbutton_24),_("Open Styles Manager"));
	Toolbar->AddTool(Menu_Tools_Properties,_("Properties"),GETIMAGE(properties_toolbutton_24),_("Open Properties"));
	Toolbar->AddTool(Menu_Tools_Attachments,_("Attachments"),GETIMAGE(attach_button_24),_("Open Attachment List"));
	Toolbar->AddTool(Menu_Tools_Fonts_Collector,_("Fonts Collector"),GETIMAGE(font_collector_button_24),_("Open Fonts Collector"));
	Toolbar->AddSeparator();

	// Automation
#ifdef WITH_AUTOMATION
	Toolbar->AddTool(Menu_Tools_Automation,_("Automation"),GETIMAGE(automation_toolbutton_24),_("Open Automation manager"));
	Toolbar->AddSeparator();
#endif

	// Tools
	if (HasASSDraw()) {
		Toolbar->AddTool(Menu_Tools_ASSDraw,_T("ASSDraw3"),GETIMAGE(assdraw_24),_("Launches ai-chan's \"ASSDraw3\" tool for vector drawing."));
		Toolbar->AddSeparator();
	}
	Toolbar->AddTool(Menu_Edit_Shift,_("Shift Times"),GETIMAGE(shift_times_toolbutton_24),_("Open Shift Times Dialogue"));
	Toolbar->AddTool(Menu_Tools_Styling,_("Styling Assistant"),GETIMAGE(styling_toolbutton_24),_("Open Styling Assistant"));
	Toolbar->AddTool(Menu_Tools_Translation,_("Translation Assistant"),GETIMAGE(translation_toolbutton_24),_("Open Translation Assistant"));
	Toolbar->AddTool(Menu_Tools_Resample,_("Resample"),GETIMAGE(resample_toolbutton_24),_("Resample Script Resolution"));
	Toolbar->AddTool(Menu_Tools_Timing_Processor,_("Timing Post-Processor"),GETIMAGE(timing_processor_toolbutton_24),_("Open Timing Post-processor dialog"));
	Toolbar->AddTool(Menu_Tools_Kanji_Timer,_("Kanji Timer"),GETIMAGE(kara_timing_copier_24),_("Open Kanji Timer dialog"));
	Toolbar->AddTool(Menu_Tools_SpellCheck,_("Spell Checker"),GETIMAGE(spellcheck_toolbutton_24),_("Open Spell checker"));
	Toolbar->AddSeparator();

	// Options
	Toolbar->AddTool(Menu_Tools_Options,_("Options"),GETIMAGE(options_button_24),_("Configure Aegisub"));
	Toolbar->AddTool(Grid_Toggle_Tags,_("Cycle Tag Hidding Mode"),GETIMAGE(toggle_tag_hiding_24),_("Cycle through tag-hiding modes"));

	// Update
	Toolbar->Realize();
}


/// @brief DOCME
/// @param item_text   
/// @param hotkey_name 
/// @return 
///
wxString MakeHotkeyText(const wxString &item_text, const wxString &hotkey_name) {
	return item_text + wxString(_T("\t")) + Hotkeys.GetText(hotkey_name);
 }


/// @brief Initialize menu bar 
///
void FrameMain::InitMenu() {
	// Deinit menu if needed
	if (menuCreated) {
		SetMenuBar(NULL);
		MenuBar->Destroy();
	}
	
#ifdef __WXMAC__
	// Make sure special menu items are placed correctly on Mac
	wxApp::s_macAboutMenuItemId = Menu_Help_About;
	wxApp::s_macExitMenuItemId = Menu_File_Exit;
	wxApp::s_macPreferencesMenuItemId = Menu_Tools_Options;
	wxApp::s_macHelpMenuTitleName = _("&Help");
#endif

	// Generate menubar
	MenuBar = new wxMenuBar();

	// Create recent subs submenus
	RecentSubs = new wxMenu();
	RecentVids = new wxMenu();
	RecentAuds = new wxMenu();
	RecentTimecodes = new wxMenu();
	RecentKeyframes = new wxMenu();

	// Create file menu
	fileMenu = new wxMenu();
	AppendBitmapMenuItem(fileMenu,Menu_File_New_Subtitles, MakeHotkeyText(_("&New Subtitles"), _T("New Subtitles")), _("New subtitles"),GETIMAGE(new_toolbutton_16));
	AppendBitmapMenuItem(fileMenu,Menu_File_Open_Subtitles, MakeHotkeyText(_("&Open Subtitles..."), _T("Open Subtitles")), _("Opens a subtitles file"),GETIMAGE(open_toolbutton_16));
	AppendBitmapMenuItem(fileMenu,Menu_File_Open_Subtitles_Charset, _("&Open Subtitles with Charset..."), _("Opens a subtitles file with a specific charset"),GETIMAGE(open_with_toolbutton_16));
	fileMenu->Append(Menu_File_Open_Subtitles_From_Video, _("Open Subtitles from &Video"), _("Opens the subtitles from the current video file"));
	AppendBitmapMenuItem(fileMenu,Menu_File_Save_Subtitles, MakeHotkeyText(_("&Save Subtitles"), _T("Save Subtitles")), _("Saves subtitles"),GETIMAGE(save_toolbutton_16));
	AppendBitmapMenuItem(fileMenu,Menu_File_Save_Subtitles_As, _("Save Subtitles as..."), _("Saves subtitles with another name"), GETIMAGE(save_as_toolbutton_16));
	AppendBitmapMenuItem(fileMenu,Menu_File_Export_Subtitles, _("Export Subtitles..."), _("Saves a copy of subtitles with processing applied to it."), GETIMAGE(export_menu_16));
	wxMenuItem *RecentParent = new wxMenuItem(fileMenu, Menu_File_Recent_Subs_Parent, _("Recent"), _T(""), wxITEM_NORMAL, RecentSubs);
#ifndef __APPLE__
	RecentParent->SetBitmap(GETIMAGE(blank_button_16));
#endif
	fileMenu->Append(RecentParent);
	fileMenu->AppendSeparator();
	AppendBitmapMenuItem (fileMenu,Menu_Tools_Properties, _("&Properties..."), _("Open script properties window"),GETIMAGE(properties_toolbutton_16));
	AppendBitmapMenuItem (fileMenu,Menu_Tools_Attachments, _("&Attachments..."), _("Open the attachment list"), GETIMAGE(attach_button_16));
	AppendBitmapMenuItem (fileMenu,Menu_Tools_Fonts_Collector, _("&Fonts Collector..."),_("Open fonts collector"), GETIMAGE(font_collector_button_16));
	fileMenu->AppendSeparator();
#ifndef __APPLE__
	// Doesn't work on Mac, only one instance is ever allowed there from OS side
	AppendBitmapMenuItem(fileMenu,Menu_File_New_Window, _("New Window"), _("Open a new application window"),GETIMAGE(new_window_menu_16));
#endif
	AppendBitmapMenuItem(fileMenu,Menu_File_Exit, MakeHotkeyText(_("E&xit"), _T("Exit")), _("Exit the application"),GETIMAGE(exit_button_16));
	MenuBar->Append(fileMenu, _("&File"));

	// Create Edit menu
	// NOTE: Undo and Redo are actually controlled in frame_main_events, OnMenuOpen(). They will always be the first two items.
	editMenu = new wxMenu();
	AppendBitmapMenuItem(editMenu,Menu_Edit_Undo, MakeHotkeyText(_("&Undo"), _T("Undo")), _("Undoes last action"),GETIMAGE(undo_button_16));
	AppendBitmapMenuItem(editMenu,Menu_Edit_Redo, MakeHotkeyText(_("&Redo"), _T("Redo")), _("Redoes last action"),GETIMAGE(redo_button_16));
	editMenu->AppendSeparator();
	AppendBitmapMenuItem(editMenu,Menu_Edit_Cut, MakeHotkeyText(_("Cut Lines"), _T("Cut")), _("Cut subtitles"), GETIMAGE(cut_button_16));
	AppendBitmapMenuItem(editMenu,Menu_Edit_Copy, MakeHotkeyText(_("Copy Lines"), _T("Copy")), _("Copy subtitles"), GETIMAGE(copy_button_16));
	AppendBitmapMenuItem(editMenu,Menu_Edit_Paste, MakeHotkeyText(_("Paste Lines"), _T("Paste")), _("Paste subtitles"), GETIMAGE(paste_button_16));
	AppendBitmapMenuItem(editMenu,Menu_Edit_Paste_Over, MakeHotkeyText(_("Paste Lines Over..."), _T("Paste Over")) , _("Paste subtitles over others"),GETIMAGE(paste_over_button_16));
	editMenu->AppendSeparator();
	AppendBitmapMenuItem(editMenu,Menu_Edit_Find, MakeHotkeyText(_("&Find..."), _T("Find")), _("Find words in subtitles"),GETIMAGE(find_button_16));
	AppendBitmapMenuItem(editMenu,Menu_Edit_Find_Next, MakeHotkeyText(_("Find Next"), _T("Find Next")), _("Find next match of last word"),GETIMAGE(find_next_menu_16));
	AppendBitmapMenuItem(editMenu,Menu_Edit_Replace, MakeHotkeyText(_("Search and &Replace..."), _T("Replace")) , _("Find and replace words in subtitles"),GETIMAGE(find_replace_menu_16));
	MenuBar->Append(editMenu, _("&Edit"));

	// Create subtitles menu
	subtitlesMenu = new wxMenu();
	wxMenu *InsertMenu = new wxMenu;
	wxMenuItem *InsertParent = new wxMenuItem(subtitlesMenu,Menu_Subtitles_Insert,_("&Insert Lines"),_T(""),wxITEM_NORMAL,InsertMenu);
#ifndef __APPLE__
	InsertParent->SetBitmap(GETIMAGE(blank_button_16));
#endif
	AppendBitmapMenuItem (subtitlesMenu,Menu_Tools_Styles_Manager, _("&Styles Manager..."), _("Open styles manager"), GETIMAGE(style_toolbutton_16));
	AppendBitmapMenuItem (subtitlesMenu,Menu_Tools_Styling, _("St&yling Assistant..."), _("Open styling assistant"), GETIMAGE(styling_toolbutton_16));
	AppendBitmapMenuItem (subtitlesMenu,Menu_Tools_Translation, _("&Translation Assistant..."),_("Open translation assistant"), GETIMAGE(translation_toolbutton_16));
	AppendBitmapMenuItem (subtitlesMenu,Menu_Tools_Resample,_("Resample Resolution..."), _("Changes resolution and modifies subtitles to conform to change"), GETIMAGE(resample_toolbutton_16));
	AppendBitmapMenuItem (subtitlesMenu,Menu_Tools_SpellCheck, _("Spe&ll Checker..."),_("Open spell checker"), GETIMAGE(spellcheck_toolbutton_16));
	if (HasASSDraw()) {
		subtitlesMenu->AppendSeparator();
		AppendBitmapMenuItem (subtitlesMenu,Menu_Tools_ASSDraw,_T("ASSDraw3..."),_("Launches ai-chan's \"ASSDraw3\" tool for vector drawing."), GETIMAGE(assdraw_16));
	}
	subtitlesMenu->AppendSeparator();
	AppendBitmapMenuItem(InsertMenu,MENU_INSERT_BEFORE,_("&Before Current"),_("Inserts a line before current"),GETIMAGE(blank_button_16));
	AppendBitmapMenuItem(InsertMenu,MENU_INSERT_AFTER,_("&After Current"),_("Inserts a line after current"),GETIMAGE(blank_button_16));
	AppendBitmapMenuItem(InsertMenu,MENU_INSERT_BEFORE_VIDEO,_("Before Current, at Video Time"),_("Inserts a line before current, starting at video time"),GETIMAGE(blank_button_16));
	AppendBitmapMenuItem(InsertMenu,MENU_INSERT_AFTER_VIDEO,_("After Current, at Video Time"),_("Inserts a line after current, starting at video time"),GETIMAGE(blank_button_16));
	subtitlesMenu->Append(InsertParent);
	AppendBitmapMenuItem(subtitlesMenu,MENU_DUPLICATE,MakeHotkeyText(_("&Duplicate Lines"), _T("Grid duplicate rows")),_("Duplicate the selected lines"),GETIMAGE(blank_button_16));
	AppendBitmapMenuItem(subtitlesMenu,MENU_DUPLICATE_NEXT_FRAME,MakeHotkeyText(_("&Duplicate and Shift by 1 Frame"), _T("Grid duplicate and shift one frame")),_("Duplicate lines and shift by one frame"),GETIMAGE(blank_button_16));
	AppendBitmapMenuItem(subtitlesMenu,MENU_DELETE,MakeHotkeyText(_("Delete Lines"), _T("Grid delete rows")),_("Delete currently selected lines"),GETIMAGE(delete_button_16));
	subtitlesMenu->AppendSeparator();
	wxMenu *JoinMenu = new wxMenu;
	wxMenuItem *JoinParent = new wxMenuItem(subtitlesMenu,Menu_Subtitles_Join,_("Join Lines"),_T(""),wxITEM_NORMAL,JoinMenu);
#ifndef __APPLE__
	JoinParent->SetBitmap(GETIMAGE(blank_button_16));
#endif
	AppendBitmapMenuItem(JoinMenu,MENU_JOIN_CONCAT,_("&Concatenate"),_("Joins selected lines in a single one, concatenating text together"),GETIMAGE(blank_button_16));
	AppendBitmapMenuItem(JoinMenu,MENU_JOIN_REPLACE,_("Keep &First"),_("Joins selected lines in a single one, keeping text of first and discarding remaining"),GETIMAGE(blank_button_16));
	AppendBitmapMenuItem(JoinMenu,MENU_JOIN_AS_KARAOKE,_("As &Karaoke"),_("Joins selected lines in a single one, as karaoke"),GETIMAGE(blank_button_16));
	subtitlesMenu->Append(JoinParent);
	AppendBitmapMenuItem(subtitlesMenu,MENU_RECOMBINE,_("Recombine Lines"),_("Recombine subtitles when they have been split and merged"),GETIMAGE(blank_button_16));
	AppendBitmapMenuItem(subtitlesMenu,MENU_SPLIT_BY_KARAOKE,_("Split Lines (by karaoke)"),_("Uses karaoke timing to split line into multiple smaller lines"),GETIMAGE(blank_button_16));
	subtitlesMenu->AppendSeparator();
	wxMenu *SortMenu = new wxMenu;
	wxMenuItem *SortParent = new wxMenuItem(subtitlesMenu,Menu_Subtitles_Sort_Start,_("Sort Lines"),_T(""),wxITEM_NORMAL,SortMenu);
#ifndef __APPLE__
	SortParent->SetBitmap(GETIMAGE(sort_times_button_16));
#endif
	AppendBitmapMenuItem(SortMenu,Menu_Subtitles_Sort_Start,_("&Start Time"),_("Sort all subtitles by their start times"),GETIMAGE(blank_button_16));
	AppendBitmapMenuItem(SortMenu,Menu_Subtitles_Sort_End,_("&End Time"),_("Sort all subtitles by their end times"),GETIMAGE(blank_button_16));
	AppendBitmapMenuItem(SortMenu,Menu_Subtitles_Sort_Style,_("St&yle Name"),_("Sort all subtitles by their style names"),GETIMAGE(blank_button_16));
	subtitlesMenu->Append(SortParent);
	AppendBitmapMenuItem(subtitlesMenu,MENU_SWAP,_("Swap Lines"),_("Swaps the two selected lines"),GETIMAGE(arrow_sort_16));
	AppendBitmapMenuItem (subtitlesMenu,Menu_Edit_Select, MakeHotkeyText(_("Select Lines..."), _T("Select lines")), _("Selects lines based on defined criterea"),GETIMAGE(select_lines_button_16));
	MenuBar->Append(subtitlesMenu, _("&Subtitles"));

	// Create timing menu
	timingMenu = new wxMenu();
	AppendBitmapMenuItem(timingMenu,Menu_Edit_Shift, MakeHotkeyText(_("S&hift Times..."), _T("Shift times")), _("Shift subtitles by time or frames"),GETIMAGE(shift_times_toolbutton_16));
	AppendBitmapMenuItem(timingMenu,Menu_Tools_Timing_Processor,_("Timing Post-Processor..."), _("Runs a post-processor for timing to deal with lead-ins, lead-outs, scene timing and etc."), GETIMAGE(timing_processor_toolbutton_16));
	AppendBitmapMenuItem (timingMenu,Menu_Tools_Kanji_Timer,_("Kanji Timer..."),_("Open Kanji timer"),GETIMAGE(kara_timing_copier_16));
	timingMenu->AppendSeparator();
	AppendBitmapMenuItem(timingMenu,Menu_Subs_Snap_Start_To_Video, MakeHotkeyText(_("Snap Start to Video"), _T("Set Start To Video")), _("Set start of selected subtitles to current video frame"), GETIMAGE(substart_to_video_16));
	AppendBitmapMenuItem(timingMenu,Menu_Subs_Snap_End_To_Video, MakeHotkeyText(_("Snap End to Video"), _T("Set End to Video")), _("Set end of selected subtitles to current video frame"), GETIMAGE(subend_to_video_16));
	AppendBitmapMenuItem(timingMenu,Menu_Video_Snap_To_Scene, MakeHotkeyText(_("Snap to Scene"), _T("Snap to Scene")), _("Set start and end of subtitles to the keyframes around current video frame"), GETIMAGE(snap_subs_to_scene_16));
	AppendBitmapMenuItem(timingMenu,Menu_Video_Shift_To_Frame, MakeHotkeyText(_("Shift to Current Frame"), _T("Shift by Current Time")), _("Shift selection so first selected line starts at current frame"), GETIMAGE(shift_to_frame_16));
	timingMenu->AppendSeparator();
	wxMenu *ContinuousMenu = new wxMenu;
	wxMenuItem *ContinuousParent = new wxMenuItem(subtitlesMenu,-1,_("Make Times Continuous"),_T(""),wxITEM_NORMAL,ContinuousMenu);
#ifndef __APPLE__
	ContinuousParent->SetBitmap(GETIMAGE(blank_button_16));
#endif
	AppendBitmapMenuItem(ContinuousMenu,MENU_ADJOIN,_("Change &Start"),_("Changes times of subs so start times begin on previous's end time"),GETIMAGE(blank_button_16));
	AppendBitmapMenuItem(ContinuousMenu,MENU_ADJOIN2,_("Change &End"),_("Changes times of subs so end times begin on next's start time"),GETIMAGE(blank_button_16));
	timingMenu->Append(ContinuousParent);
	MenuBar->Append(timingMenu, _("&Timing"));

	// Create video menu
	videoMenu = new wxMenu();
	AppendBitmapMenuItem(videoMenu, Menu_File_Open_Video, _("&Open Video..."), _("Opens a video file"), GETIMAGE(open_video_menu_16));
	AppendBitmapMenuItem(videoMenu, Menu_File_Close_Video, _("&Close Video"), _("Closes the currently open video file"), GETIMAGE(close_video_menu_16));
	wxMenuItem *RecentVidParent = new wxMenuItem(videoMenu, Menu_File_Recent_Vids_Parent, _("Recent"), _T(""), wxITEM_NORMAL, RecentVids);
	videoMenu->Append(RecentVidParent);
	AppendBitmapMenuItem(videoMenu, Menu_Video_Dummy, _("Use Dummy Video..."), _("Opens a video clip with solid colour"), GETIMAGE(use_dummy_video_menu_16));
	AppendBitmapMenuItem(videoMenu, Menu_Video_Details, _("Show Video Details..."), _("Shows video details"), GETIMAGE(show_video_details_menu_16));
	videoMenu->AppendSeparator();
	AppendBitmapMenuItem(videoMenu, Menu_File_Open_VFR, _("Open Timecodes File..."), _("Opens a VFR timecodes v1 or v2 file"), GETIMAGE(open_timecodes_menu_16));
	AppendBitmapMenuItem(videoMenu, Menu_File_Save_VFR, _("Save Timecodes File..."), _("Saves a VFR timecodes v2 file"), GETIMAGE(save_timecodes_menu_16));
	AppendBitmapMenuItem(videoMenu, Menu_File_Close_VFR, _("Close Timecodes File"), _("Closes the currently open timecodes file"), GETIMAGE(close_timecodes_menu_16))->Enable(false);
	wxMenuItem *RecentTimesParent = new wxMenuItem(videoMenu, Menu_File_Recent_Timecodes_Parent, _("Recent"), _T(""), wxITEM_NORMAL, RecentTimecodes);
	videoMenu->Append(RecentTimesParent);
	videoMenu->AppendSeparator();
	AppendBitmapMenuItem(videoMenu, Menu_Video_Load_Keyframes, _("Open Keyframes..."), _("Opens a keyframe list file"), GETIMAGE(open_keyframes_menu_16));
	AppendBitmapMenuItem(videoMenu, Menu_Video_Save_Keyframes, _("Save Keyframes..."), _("Saves the current keyframe list"), GETIMAGE(save_keyframes_menu_16))->Enable(false);
	AppendBitmapMenuItem(videoMenu, Menu_Video_Close_Keyframes, _("Close Keyframes"), _("Closes the currently open keyframes list"), GETIMAGE(close_keyframes_menu_16))->Enable(false);
	wxMenuItem *RecentKeyframesParent = new wxMenuItem(videoMenu, Menu_File_Recent_Keyframes_Parent, _("Recent"), _T(""), wxITEM_NORMAL, RecentKeyframes);
	videoMenu->Append(RecentKeyframesParent);
	videoMenu->AppendSeparator();
	AppendBitmapMenuItem(videoMenu, Menu_Video_Detach, _("Detach Video"), _("Detach video, displaying it in a separate Window."), GETIMAGE(detach_video_menu_16));
	wxMenu *ZoomMenu = new wxMenu;
	wxMenuItem *ZoomParent = new wxMenuItem(subtitlesMenu,Menu_View_Zoom,_("Set Zoom"),_T(""),wxITEM_NORMAL,ZoomMenu);
#ifndef __APPLE__
	ZoomParent->SetBitmap(GETIMAGE(set_zoom_menu_16));
#endif
	ZoomMenu->Append(Menu_View_Zoom_50, MakeHotkeyText(_T("&50%"), _T("Zoom 50%")), _("Set zoom to 50%"));
	ZoomMenu->Append(Menu_View_Zoom_100, MakeHotkeyText(_T("&100%"), _T("Zoom 100%")), _("Set zoom to 100%"));
	ZoomMenu->Append(Menu_View_Zoom_200, MakeHotkeyText(_T("&200%"), _T("Zoom 200%")), _("Set zoom to 200%"));
	videoMenu->Append(ZoomParent);
	wxMenu *AspectMenu = new wxMenu;
	wxMenuItem *AspectParent = new wxMenuItem(subtitlesMenu,Menu_Video_AR,_("Override Aspect Ratio"),_T(""),wxITEM_NORMAL,AspectMenu);
#ifndef __APPLE__
	AspectParent->SetBitmap(GETIMAGE(override_aspect_menu_16));
#endif
	AspectMenu->AppendCheckItem(Menu_Video_AR_Default, _("&Default"), _("Leave video on original aspect ratio"));
	AspectMenu->AppendCheckItem(Menu_Video_AR_Full, _("&Fullscreen (4:3)"), _("Forces video to 4:3 aspect ratio"));
	AspectMenu->AppendCheckItem(Menu_Video_AR_Wide, _("&Widescreen (16:9)"), _("Forces video to 16:9 aspect ratio"));
	AspectMenu->AppendCheckItem(Menu_Video_AR_235, _("&Cinematic (2.35)"), _("Forces video to 2.35 aspect ratio"));
	AspectMenu->AppendCheckItem(Menu_Video_AR_Custom, _("Custom..."), _("Forces video to a custom aspect ratio"));
	videoMenu->Append(AspectParent);
	videoMenu->AppendCheckItem(Menu_Video_Overscan, _("Show Overscan Mask"), _("Show a mask over the video, indicating areas that might get cropped off by overscan on televisions."));
//  This is broken as you can't use Check() on a menu item that has a bitmap.
//	AppendBitmapMenuItem(videoMenu, Menu_Video_Overscan, _("Show Overscan Mask"), _("Show a mask over the video, indicating areas that might get cropped off by overscan on televisions."), GETIMAGE(show_overscan_menu_checked_16));
	videoMenu->AppendSeparator();
	AppendBitmapMenuItem(videoMenu,Menu_Video_JumpTo, MakeHotkeyText(_("&Jump to..."), _T("Video Jump")), _("Jump to frame or time"), GETIMAGE(jumpto_button_16));
	AppendBitmapMenuItem(videoMenu,Menu_Subs_Snap_Video_To_Start, MakeHotkeyText(_("Jump Video to Start"), _T("Jump Video To Start")), _("Jumps the video to the start frame of current subtitle"), GETIMAGE(video_to_substart_16));
	AppendBitmapMenuItem(videoMenu,Menu_Subs_Snap_Video_To_End, MakeHotkeyText(_("Jump Video to End"), _T("Jump Video To End")), _("Jumps the video to the end frame of current subtitle"), GETIMAGE(video_to_subend_16));
	MenuBar->Append(videoMenu, _("&Video"));

	// Create audio menu
	audioMenu = new wxMenu();
	AppendBitmapMenuItem(audioMenu, Menu_Audio_Open_File, _("&Open Audio File..."), _("Opens an audio file"), GETIMAGE(open_audio_menu_16));
	AppendBitmapMenuItem(audioMenu, Menu_Audio_Open_From_Video, _("Open Audio from &Video"), _("Opens the audio from the current video file"), GETIMAGE(open_audio_from_video_menu_16));
	AppendBitmapMenuItem(audioMenu, Menu_Audio_Close, _("&Close Audio"), _("Closes the currently open audio file"), GETIMAGE(close_audio_menu_16));
	wxMenuItem *RecentAudParent = new wxMenuItem(audioMenu, Menu_File_Recent_Auds_Parent, _("Recent"), _T(""), wxITEM_NORMAL, RecentAuds);
	audioMenu->Append(RecentAudParent);
#ifdef _DEBUG
	audioMenu->AppendSeparator();
	audioMenu->Append(Menu_Audio_Open_Dummy, _T("Open 2h30 Blank Audio"), _T("Open a 150 minutes blank audio clip, for debugging"));
	audioMenu->Append(Menu_Audio_Open_Dummy_Noise, _T("Open 2h30 Noise Audio"), _T("Open a 150 minutes noise-filled audio clip, for debugging"));
#endif
	MenuBar->Append(audioMenu, _("&Audio"));

	// Create Automation menu
#ifdef WITH_AUTOMATION
	automationMenu = new wxMenu();
	AppendBitmapMenuItem (automationMenu,Menu_Tools_Automation, _("&Automation..."),_("Open automation manager"), GETIMAGE(automation_toolbutton_16));
	automationMenu->AppendSeparator();
	MenuBar->Append(automationMenu, _("&Automation"));
#endif

	// Create view menu
	viewMenu = new wxMenu();
	AppendBitmapMenuItem(viewMenu,Menu_View_Language, _T("&Language..."), _("Select Aegisub interface language"), GETIMAGE(languages_menu_16));
	AppendBitmapMenuItem(viewMenu,Menu_Tools_Options, MakeHotkeyText(_("&Options..."), _T("Options")), _("Configure Aegisub"), GETIMAGE(options_button_16));
	viewMenu->AppendSeparator();
	viewMenu->AppendRadioItem(Menu_View_Subs, _("Subs Only View"), _("Display subtitles only"));
	viewMenu->AppendRadioItem(Menu_View_Video, _("Video+Subs View"), _("Display video and subtitles only"));
	viewMenu->AppendRadioItem(Menu_View_Audio, _("Audio+Subs View"), _("Display audio and subtitles only"));
	viewMenu->AppendRadioItem(Menu_View_Standard, _("Full view"), _("Display audio, video and subtitles"));
	viewMenu->AppendSeparator();
	viewMenu->AppendRadioItem(Menu_View_FullTags, _("Show Tags"), _("Show full override tags in the subtitle grid"));
	viewMenu->AppendRadioItem(Menu_View_ShortTags, _("Simplify Tags"), _("Replace override tags in the subtitle grid with a simplified placeholder"));
	viewMenu->AppendRadioItem(Menu_View_NoTags, _("Hide Tags"), _("Hide override tags in the subtitle grid"));
	MenuBar->Append(viewMenu, _("Vie&w"));

	// Create help menu
	helpMenu = new wxMenu();
	AppendBitmapMenuItem (helpMenu,Menu_Help_Contents, MakeHotkeyText(_("&Contents..."), _T("Help")), _("Help topics"), GETIMAGE(contents_button_16));
#ifdef __WXMAC__
	AppendBitmapMenuItem (helpMenu,Menu_Help_Files, MakeHotkeyText(_("&All Files") + _T("..."), _T("Help")), _("Help topics"), GETIMAGE(contents_button_16));
#endif
	helpMenu->AppendSeparator();
	AppendBitmapMenuItem(helpMenu,Menu_Help_Website, _("&Website..."), _("Visit Aegisub's official website"),GETIMAGE(website_button_16));
	AppendBitmapMenuItem(helpMenu,Menu_Help_Forums, _("&Forums..."), _("Visit Aegisub's forums"),GETIMAGE(forums_button_16));
	AppendBitmapMenuItem(helpMenu,Menu_Help_BugTracker, _("&Bug Tracker..."), _("Visit Aegisub's bug tracker to report bugs and request new features"),GETIMAGE(bugtracker_button_16));
	AppendBitmapMenuItem (helpMenu,Menu_Help_IRCChannel, _("&IRC Channel..."), _("Visit Aegisub's official IRC channel"), GETIMAGE(irc_button_16));
#ifndef __WXMAC__
	helpMenu->AppendSeparator();
#endif
	AppendBitmapMenuItem(helpMenu,Menu_Help_Check_Updates, _("&Check for Updates..."), _("Check to see if there is a new version of Aegisub available"),GETIMAGE(blank_button_16));
	AppendBitmapMenuItem(helpMenu,Menu_Help_About, _("&About..."), _("About Aegisub"),GETIMAGE(about_menu_16));
	AppendBitmapMenuItem(helpMenu,Menu_Help_Log, _("&Log window..."), _("Aegisub event log"),GETIMAGE(about_menu_16));
	MenuBar->Append(helpMenu, _("&Help"));

	// Set the bar as this frame's
	SetMenuBar(MenuBar);

	// Set menu created flag
	menuCreated = true;
}



/// @brief Initialize contents 
///
void FrameMain::InitContents() {
	// Set a background panel
	StartupLog(_T("Create background panel"));
	Panel = new wxPanel(this,-1,wxDefaultPosition,wxDefaultSize,wxTAB_TRAVERSAL | wxCLIP_CHILDREN);

	// Initialize sizers
	StartupLog(_T("Create main sizers"));
	MainSizer = new wxBoxSizer(wxVERTICAL);
	TopSizer = new wxBoxSizer(wxHORIZONTAL);
	BottomSizer = new wxBoxSizer(wxHORIZONTAL);

	// Video area;
	StartupLog(_T("Create video box"));
	videoBox = new VideoBox(Panel, false);
	TopSizer->Add(videoBox,0,wxEXPAND,0);
	videoBox->videoDisplay->zoomBox = ZoomBox;

	// Subtitles area
	StartupLog(_T("Create subtitles grid"));
	SubsBox = new SubtitlesGrid(this,Panel,-1,wxDefaultPosition,wxSize(600,100),wxWANTS_CHARS | wxSUNKEN_BORDER,_T("Subs grid"));
	BottomSizer->Add(SubsBox,1,wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM,0);
	StartupLog(_T("Reset undo stack"));
	AssFile::StackReset();
	videoBox->videoSlider->grid = SubsBox;
	VideoContext::Get()->grid = SubsBox;
	StartupLog(_T("Reset video zoom"));
	videoBox->videoDisplay->SetZoom(OPT_GET("Video/Default Zoom")->GetInt() * .125 + .125);
	Search.grid = SubsBox;

	// Audio area
	StartupLog(_T("Create audio box"));
	audioBox = new AudioBox(Panel);
	audioBox->frameMain = this;
	VideoContext::Get()->audio = audioBox->audioDisplay;

	// Top sizer
	StartupLog(_T("Create subtitle editing box"));
	EditBox = new SubsEditBox(Panel,SubsBox);
	EditBox->audio = audioBox->audioDisplay;
	StartupLog(_T("Arrange controls in sizers"));
	ToolSizer = new wxBoxSizer(wxVERTICAL);
	ToolSizer->Add(audioBox,0,wxEXPAND | wxBOTTOM,5);
	ToolSizer->Add(EditBox,1,wxEXPAND,5);
	TopSizer->Add(ToolSizer,1,wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM,5);

	// Set sizers/hints
	StartupLog(_T("Arrange main sizers"));
	MainSizer->Add(new wxStaticLine(Panel),0,wxEXPAND | wxALL,0);
	MainSizer->Add(TopSizer,0,wxEXPAND | wxALL,0);
	MainSizer->Add(BottomSizer,1,wxEXPAND | wxALL,0);
	Panel->SetSizer(MainSizer);
	//MainSizer->SetSizeHints(Panel);
	//SetSizer(MainSizer);

	// Set display
	StartupLog(_T("Perform layout"));
	Layout();
	StartupLog(_T("Set focus to edting box"));
	EditBox->TextEdit->SetFocus();
	StartupLog(_T("Leaving InitContents"));
}



/// @brief Deinitialize controls 
///
void FrameMain::DeInitContents() {
	if (detachedVideo) detachedVideo->Destroy();
	if (stylingAssistant) stylingAssistant->Destroy();
	AssFile::StackReset();
	delete AssFile::top;
	delete EditBox;
	delete videoBox;
	HelpButton::ClearPages();
}



/// @brief Update toolbar 
///
void FrameMain::UpdateToolbar() {
	// Collect flags
	bool isVideo = VideoContext::Get()->IsLoaded();
	HasSelection = true;
	int selRows = SubsBox->GetNumberSelection();

	// Update
	wxToolBar* toolbar = GetToolBar();
	toolbar->FindById(Menu_Video_JumpTo)->Enable(isVideo);
	toolbar->FindById(Menu_Video_Zoom_In)->Enable(isVideo && !detachedVideo);
	toolbar->FindById(Menu_Video_Zoom_Out)->Enable(isVideo && !detachedVideo);
	ZoomBox->Enable(isVideo && !detachedVideo);
	toolbar->FindById(Menu_Subs_Snap_Start_To_Video)->Enable(isVideo && selRows > 0);
	toolbar->FindById(Menu_Subs_Snap_End_To_Video)->Enable(isVideo && selRows > 0);
	toolbar->FindById(Menu_Subs_Snap_Video_To_Start)->Enable(isVideo && selRows == 1);
	toolbar->FindById(Menu_Subs_Snap_Video_To_End)->Enable(isVideo && selRows == 1);
	toolbar->FindById(Menu_Video_Select_Visible)->Enable(isVideo);
	toolbar->FindById(Menu_Video_Snap_To_Scene)->Enable(isVideo && selRows > 0);
	toolbar->FindById(Menu_Video_Shift_To_Frame)->Enable(isVideo && selRows > 0);
	toolbar->Realize();
}



/// @brief Open subtitles 
/// @param filename 
/// @param charset  
/// @return 
///
void FrameMain::LoadSubtitles (wxString filename,wxString charset) {
	// First check if there is some loaded
	if (AssFile::top && AssFile::top->loaded) {
		if (TryToCloseSubs() == wxCANCEL) return;
	}

	// Setup
	bool isFile = (filename != _T(""));
	bool isBinary = false;

	// Load
	try {
		// File exists?
		if (isFile) {
			wxFileName fileCheck(filename);
			if (!fileCheck.FileExists()) throw _T("Selected file does not exist.");

			// Make sure that file isn't actually a timecode file
			try {
				TextFileReader testSubs(filename,charset);
				charset = testSubs.GetCurrentEncoding();
				isBinary = charset == _T("binary");
				if (!isBinary && testSubs.HasMoreLines()) {
					wxString cur = testSubs.ReadLineFromFile();
					if (cur.Left(10) == _T("# timecode")) {
						LoadVFR(filename);
						OPT_SET("Path/Last/Timecodes")->SetString(STD_STR(fileCheck.GetPath()));
						return;
					}
				}
			}
			catch (...) {
				// if trying to load the file as timecodes fails it's fairly safe to assume that
				// it is in fact not a timecode file
			}
		}

		// Proceed into loading
		SubsBox->Clear();
		AssFile::StackReset();
		if (isFile) {
			AssFile::top->Load(filename,charset);
			SubsBox->LoadFromAss(AssFile::top,false,true);
			wxFileName fn(filename);
			StandardPaths::SetPathValue(_T("?script"),fn.GetPath());
			OPT_SET("Path/Last/Subtitles")->SetString(STD_STR(fn.GetPath()));
		}
		else {
			SubsBox->LoadDefault(AssFile::top);
			StandardPaths::SetPathValue(_T("?script"),_T(""));
		}
	}
	catch (const wchar_t *err) {
		wxMessageBox(wxString(err), _T("Error"), wxOK | wxICON_ERROR, NULL);
		return;
	}
	catch (wxString err) {
		wxMessageBox(err, _T("Error"), wxOK | wxICON_ERROR, NULL);
		return;
	}
	catch (...) {
		wxMessageBox(_T("Unknown error"), _T("Error"), wxOK | wxICON_ERROR, NULL);
		return;
	}

	// Save copy
	wxFileName origfile(filename);
	if (!isBinary && OPT_GET("App/Auto/Backup")->GetBool() && origfile.FileExists()) {
		// Get path
		wxString path = lagi_wxString(OPT_GET("Path/Auto/Backup")->GetString());
		if (path.IsEmpty()) path = origfile.GetPath();
		wxFileName dstpath(path);
		if (!dstpath.IsAbsolute()) path = StandardPaths::DecodePathMaybeRelative(path, _T("?user/"));
		path += _T("/");
		dstpath.Assign(path);
		if (!dstpath.DirExists()) wxMkdir(path);

		// Save
		wxString backup = path + origfile.GetName() + _T(".ORIGINAL.") + origfile.GetExt();
		wxCopyFile(filename,backup,true);
	}

	// Sync
	SynchronizeProject(true);

	// Update title bar
	UpdateTitle();
}



/// @brief Save subtitles 
/// @param saveas      
/// @param withCharset 
/// @return 
///
bool FrameMain::SaveSubtitles(bool saveas,bool withCharset) {
	// Try to get filename from file
	wxString filename;
	if (saveas == false && AssFile::top->CanSave()) filename = AssFile::top->filename;

	// Failed, ask user
	if (filename.IsEmpty()) {
		VideoContext::Get()->Stop();
		wxString path = lagi_wxString(OPT_GET("Path/Last/Subtitles")->GetString());
		wxFileName origPath(AssFile::top->filename);
		filename = 	wxFileSelector(_("Save subtitles file"),path,origPath.GetName() + _T(".ass"),_T("ass"),AssFile::GetWildcardList(1),wxFD_SAVE | wxFD_OVERWRITE_PROMPT,this);
	}

	// Actually save
	if (!filename.empty()) {
		// Store path
		wxFileName filepath(filename);
		OPT_SET("Path/Last/Subtitles")->SetString(STD_STR(filepath.GetPath()));

		// Fix me, ghetto hack for correct relative path generation in SynchronizeProject()
		AssFile::top->filename = filename;

		// Synchronize
		SynchronizeProject();

		// Get charset
		wxString charset = _T("");
		if (withCharset) {
			wxArrayString choices = AegisubCSConv::GetEncodingsList();
			charset = wxGetSingleChoice(_("Choose charset code:"), _T("Charset"),choices,this,-1, -1,true,250,200);
			if (charset.IsEmpty()) return false;
		}

		// Save
		try {
			AssFile::top->Save(filename,true,true,charset);
			UpdateTitle();
		}
		catch (const wchar_t *err) {
			wxMessageBox(wxString(err), _T("Error"), wxOK | wxICON_ERROR, NULL);
			return false;
		}
		catch (...) {
			wxMessageBox(_T("Unknown error"), _T("Error"), wxOK | wxICON_ERROR, NULL);
			return false;
		}
		return true;
	}
	return false;
}



/// @brief Try to close subtitles 
/// @param enableCancel 
/// @return 
///
int FrameMain::TryToCloseSubs(bool enableCancel) {
	AssFile *ass = AssFile::top;
	if (ass->IsModified()) {
		int flags = wxYES_NO;
		if (enableCancel) flags |= wxCANCEL;
		int result = wxMessageBox(_("Save before continuing?"), _("Unsaved changes"), flags,this);
		if (result == wxYES) {
			// If it fails saving, return cancel anyway
			if (SaveSubtitles(false)) return wxYES;
			else return wxCANCEL;
		}
		return result;
	}
	else return wxYES;
}



/// @brief Set the video and audio display visibilty
/// @param video -1: leave unchanged; 0: hide; 1: show
/// @param audio -1: leave unchanged; 0: hide; 1: show
void FrameMain::SetDisplayMode(int video, int audio) {
	if (!IsShownOnScreen()) return;

	bool sv = false, sa = false;

	if (video == -1) sv = showVideo;
	else if (video)  sv = VideoContext::Get()->IsLoaded() && !detachedVideo;

	if (audio == -1) sa = showAudio;
	else if (audio)  sa = audioBox->loaded;

	// See if anything changed
	if (sv == showVideo && sa == showAudio) return;

	showVideo = sv;
	showAudio = sa;

	// Stop
	Freeze();
	VideoContext::Get()->Stop();

	// Set display
	TopSizer->Show(videoBox,showVideo,true);
	ToolSizer->Show(audioBox,showAudio,true);

	// Update
	UpdateToolbar();
	EditBox->SetSplitLineMode();
	MainSizer->CalcMin();
	MainSizer->RecalcSizes();
	MainSizer->Layout();
	Layout();
	Show(true);
	if (showVideo) VideoContext::Get()->UpdateDisplays(true);
	Thaw();
}



/// @brief Update title bar 
///
void FrameMain::UpdateTitle() {
	// Determine if current subs are modified
	bool subsMod = AssFile::top->IsModified();
	
	// Create ideal title
	wxString newTitle = _T("");
#ifndef __WXMAC__
	if (subsMod) newTitle << _T("* ");
	if (AssFile::top->filename != _T("")) {
		wxFileName file (AssFile::top->filename);
		newTitle << file.GetFullName();
	}
	else newTitle << _("Untitled");
	newTitle << _T(" - Aegisub ") << GetAegisubLongVersionString();
#else
	// Apple HIG says "untitled" should not be capitalised
	// and the window is a document window, it shouldn't contain the app name
	// (The app name is already present in the menu bar)
	if (AssFile::top->filename != _T("")) {
		wxFileName file (AssFile::top->filename);
		newTitle << file.GetFullName();
	}
	else newTitle << _("untitled");
#endif

#ifdef __WXMAC__
	// On Mac, set the mark in the close button
	WindowRef wnd = (WindowRef)GetHandle();
	SetWindowModified(wnd, subsMod);
#endif

	// Get current title
	wxString curTitle = GetTitle();
	if (curTitle != newTitle) SetTitle(newTitle);
}



/// @brief Updates subs with video/whatever data 
/// @param fromSubs 
///
void FrameMain::SynchronizeProject(bool fromSubs) {
	// Gather current data
	AssFile *subs = AssFile::top;

	// Retrieve data from subs
	if (fromSubs) {
		// Reset the state
		long videoPos = 0;
		long videoAr = 0;
		double videoArValue = 0.0;
		double videoZoom = 0.;

		// Get AR
		wxString arString = subs->GetScriptInfo(_T("Video Aspect Ratio"));
		if (arString.Left(1) == _T("c")) {
			videoAr = 4;
			arString = arString.Mid(1);
			arString.ToDouble(&videoArValue);
		}
		else if (arString.IsNumber()) arString.ToLong(&videoAr);

		// Get new state info
		subs->GetScriptInfo(_T("Video Position")).ToLong(&videoPos);
		subs->GetScriptInfo(_T("Video Zoom Percent")).ToDouble(&videoZoom);
		wxString curSubsVideo = DecodeRelativePath(subs->GetScriptInfo(_T("Video File")),AssFile::top->filename);
		wxString curSubsVFR = DecodeRelativePath(subs->GetScriptInfo(_T("VFR File")),AssFile::top->filename);
		wxString curSubsKeyframes = DecodeRelativePath(subs->GetScriptInfo(_T("Keyframes File")),AssFile::top->filename);
		wxString curSubsAudio = DecodeRelativePath(subs->GetScriptInfo(_T("Audio File")),AssFile::top->filename);
		wxString AutoScriptString = subs->GetScriptInfo(_T("Automation Scripts"));

		// Check if there is anything to change
		int autoLoadMode = OPT_GET("App/Auto/Load Linked Files")->GetInt();
		bool hasToLoad = false;
		if (curSubsAudio != audioBox->audioName ||
			curSubsVFR != VFR_Output.GetFilename() ||
			curSubsVideo != VideoContext::Get()->videoName ||
			curSubsKeyframes != VideoContext::Get()->GetKeyFramesName()
#ifdef WITH_AUTOMATION
			|| !AutoScriptString.IsEmpty() || local_scripts->GetScripts().size() > 0
#endif
			) {
			hasToLoad = true;
		}

		// Decide whether to load or not
		bool doLoad = false;
		if (hasToLoad) {
			if (autoLoadMode == 1) doLoad = true;
			else if (autoLoadMode == 2) {
				int result = wxMessageBox(_("Do you want to load/unload the associated files?"),_("(Un)Load files?"),wxYES_NO);
				if (result == wxYES) doLoad = true;
			}
		}

		if (doLoad) {
			// Variable frame rate
			LoadVFR(curSubsVFR);

			// Video
			if (curSubsVideo != VideoContext::Get()->videoName) {
				//if (curSubsVideo != _T("")) {
				LoadVideo(curSubsVideo);
				if (VideoContext::Get()->IsLoaded()) {
					VideoContext::Get()->SetAspectRatio(videoAr,videoArValue);
					videoBox->videoDisplay->SetZoom(videoZoom);
					VideoContext::Get()->JumpToFrame(videoPos);
				}
				//}
			}

			// Keyframes
			if (curSubsKeyframes != _T("")) {
				KeyFrameFile::Load(curSubsKeyframes);
			}

			// Audio
			if (curSubsAudio != audioBox->audioName) {
				if (curSubsAudio == _T("?video")) LoadAudio(_T(""),true);
				else LoadAudio(curSubsAudio);
			}

			// Automation scripts
#ifdef WITH_AUTOMATION
			local_scripts->RemoveAll();
			wxStringTokenizer tok(AutoScriptString, _T("|"), wxTOKEN_STRTOK);
			wxFileName subsfn(subs->filename);
			wxString autobasefn(lagi_wxString(OPT_GET("Path/Automation/Base")->GetString()));
			while (tok.HasMoreTokens()) {
				wxString sfnames = tok.GetNextToken().Trim(true).Trim(false);
				wxString sfnamel = sfnames.Left(1);
				sfnames.Remove(0, 1);
				wxString basepath;
				if (sfnamel == _T("~")) {
					basepath = subsfn.GetPath();
				} else if (sfnamel == _T("$")) {
					basepath = autobasefn;
				} else if (sfnamel == _T("/")) {
					basepath = _T("");
				} else {
					wxLogWarning(_T("Automation Script referenced with unknown location specifier character.\nLocation specifier found: %s\nFilename specified: %s"),
						sfnamel.c_str(), sfnames.c_str());
					continue;
				}
				wxFileName sfname(sfnames);
				sfname.MakeAbsolute(basepath);
				if (sfname.FileExists()) {
					sfnames = sfname.GetFullPath();
					local_scripts->Add(Automation4::ScriptFactory::CreateFromFile(sfnames, true));
				} else {
					wxLogWarning(_T("Automation Script referenced could not be found.\nFilename specified: %s%s\nSearched relative to: %s\nResolved filename: %s"),
						sfnamel.c_str(), sfnames.c_str(), basepath.c_str(), sfname.GetFullPath().c_str());
				}
			}
#endif
		}

		// Display
		SetDisplayMode(1,1);
	}

	// Store data on subs
	else {
		// Setup
		wxString seekpos = _T("0");
		wxString ar = _T("0");
		wxString zoom = _T("6");
		if (VideoContext::Get()->IsLoaded()) {
			seekpos = wxString::Format(_T("%i"),videoBox->videoDisplay->GetFrame());
			zoom = wxString::Format(_T("%f"),videoBox->videoDisplay->GetZoom());

			int arType = VideoContext::Get()->GetAspectRatioType();
			if (arType == 4) ar = wxString(_T("c")) + AegiFloatToString(VideoContext::Get()->GetAspectRatioValue());
			else ar = wxString::Format(_T("%i"),arType);
		}
		
		// Store audio data
		subs->SetScriptInfo(_T("Audio File"),MakeRelativePath(audioBox->audioName,AssFile::top->filename));

		// Store video data
		subs->SetScriptInfo(_T("Video File"),MakeRelativePath(VideoContext::Get()->videoName,AssFile::top->filename));
		subs->SetScriptInfo(_T("Video Aspect Ratio"),ar);
		subs->SetScriptInfo(_T("Video Zoom Percent"),zoom);
		subs->SetScriptInfo(_T("Video Position"),seekpos);
		subs->SetScriptInfo(_T("VFR File"),MakeRelativePath(VFR_Output.GetFilename(),AssFile::top->filename));
		subs->SetScriptInfo(_T("Keyframes File"),MakeRelativePath(VideoContext::Get()->GetKeyFramesName(),AssFile::top->filename));

		// Store Automation script data
		// Algorithm:
		// 1. If script filename has Automation Base Path as a prefix, the path is relative to that (ie. "$")
		// 2. Otherwise try making it relative to the subs filename
		// 3. If step 2 failed, or absolut path is shorter than path relative to subs, use absolute path ("/")
		// 4. Otherwise, use path relative to subs ("~")
#ifdef WITH_AUTOMATION
		wxString scripts_string;
		wxString autobasefn(lagi_wxString(OPT_GET("Path/Automation/Base")->GetString()));

		const std::vector<Automation4::Script*> &scripts = local_scripts->GetScripts();
		for (unsigned int i = 0; i < scripts.size(); i++) {
			Automation4::Script *script = scripts[i];

			if (i != 0)
				scripts_string += _T("|");

			wxString autobase_rel, subsfile_rel;
			wxString scriptfn(script->GetFilename());
			autobase_rel = MakeRelativePath(scriptfn, autobasefn);
			subsfile_rel = MakeRelativePath(scriptfn, AssFile::top->filename);

			if (autobase_rel.size() <= scriptfn.size() && autobase_rel.size() <= subsfile_rel.size()) {
				scriptfn = _T("$") + autobase_rel;
			} else if (subsfile_rel.size() <= scriptfn.size() && subsfile_rel.size() <= autobase_rel.size()) {
				scriptfn = _T("~") + subsfile_rel;
			} else {
				scriptfn = _T("/") + wxFileName(scriptfn).GetFullPath(wxPATH_UNIX);
			}

			scripts_string += scriptfn;
		}
		subs->SetScriptInfo(_T("Automation Scripts"), scripts_string);
#endif
	}
}



/// @brief Loads video 
/// @param file     
/// @param autoload 
/// @return 
///
void FrameMain::LoadVideo(wxString file,bool autoload) {
	if (blockVideoLoad) return;
	Freeze();
	VideoContext::Get()->Stop();
	try {
		if (VideoContext::Get()->IsLoaded()) {
			if (VFR_Output.GetFrameRateType() == VFR) {
				if (!autoload) {
					int result = wxMessageBox(_("You have timecodes loaded currently. Would you like to unload them?"), _("Unload timecodes?"), wxYES_NO, this);
					if (result == wxYES) {
						VFR_Output.Unload();
					}
				}
			}
			else {
				VFR_Output.Unload();
			}
		}
		VideoContext::Get()->SetVideo(file);
	}
	catch (const wchar_t *error) {
		wxString err(error);
		wxMessageBox(err, _T("Error opening video file"), wxOK | wxICON_ERROR, this);
	}
	catch (...) {
		wxMessageBox(_T("Unknown error"), _T("Error opening video file"), wxOK | wxICON_ERROR, this);
	}

	if (VideoContext::Get()->IsLoaded()) {
		int vidx = VideoContext::Get()->GetWidth(), vidy = VideoContext::Get()->GetHeight();

		// Set zoom level based on video resolution and window size
		double target_zoom = 1.;
		wxSize windowSize = GetSize();
		if (vidx*3 > windowSize.GetX()*2 || vidy*4 > windowSize.GetY()*3)
			target_zoom = .5;
		if (vidx*3 > windowSize.GetX()*4 || vidy*4 > windowSize.GetY()*6)
			target_zoom = .25;
		videoBox->videoDisplay->SetZoom(target_zoom);

		// Check that the video size matches the script video size specified
		int scriptx = SubsBox->ass->GetScriptInfoAsInt(_T("PlayResX"));
		int scripty = SubsBox->ass->GetScriptInfoAsInt(_T("PlayResY"));
		if (scriptx != vidx || scripty != vidy) {
			switch (OPT_GET("Video/Check Script Res")->GetInt()) {
				case 1:
					// Ask to change on mismatch
					if (wxMessageBox(wxString::Format(_("The resolution of the loaded video and the resolution specified for the subtitles don't match.\n\nVideo resolution:\t%d x %d\nScript resolution:\t%d x %d\n\nChange subtitles resolution to match video?"), vidx, vidy, scriptx, scripty), _("Resolution mismatch"), wxYES_NO, this) != wxYES)
						break;
					// Fallthrough to case 2
				case 2:
					// Always change script res
					SubsBox->ass->SetScriptInfo(_T("PlayResX"), wxString::Format(_T("%d"), vidx));
					SubsBox->ass->SetScriptInfo(_T("PlayResY"), wxString::Format(_T("%d"), vidy));
					SubsBox->ass->FlagAsModified(_("Change script resolution"));
					SubsBox->CommitChanges();
					break;
				case 0:
				default:
					// Never change
					break;
			}
		}
	}

	SubsBox->CommitChanges(true);
	SetDisplayMode(1,-1);
	EditBox->UpdateFrameTiming();

	DetachVideo(VideoContext::Get()->IsLoaded() && OPT_GET("Video/Detached/Enabled")->GetBool());
	Thaw();
}



/// @brief Loads audio 
/// @param filename  
/// @param FromVideo 
/// @return 
///
void FrameMain::LoadAudio(wxString filename,bool FromVideo) {
	if (blockAudioLoad) return;
	VideoContext::Get()->Stop();
	try {
		audioBox->SetFile(filename,FromVideo);
		SetDisplayMode(-1,1);
	}
	catch (const wchar_t *error) {
		wxString err(error);
		wxMessageBox(err, _T("Error opening audio file"), wxOK | wxICON_ERROR, this);
	}
	#ifdef WITH_AVISYNTH
	catch (AvisynthError err) {
		wxMessageBox (wxString(_T("AviSynth error: ")) + wxString(err.msg,wxConvUTF8), _T("Error loading audio"), wxOK | wxICON_ERROR);
		return;
	}
	#endif
	catch (...) {
		wxMessageBox(_T("Unknown error"), _T("Error opening audio file"), wxOK | wxICON_ERROR, this);
	}
}



/// @brief Loads VFR 
/// @param filename 
///
void FrameMain::LoadVFR(wxString filename) {
	VideoContext::Get()->Stop();
	if (filename != _T("")) {
		try {
			VFR_Output.Load(filename);
			SubsBox->Refresh(false);
		}

		// Fail
		catch (const wchar_t *error) {
			wxString err(error);
			wxMessageBox(err, _T("Error opening timecodes file"), wxOK | wxICON_ERROR, this);
		}
		catch (...) {
			wxMessageBox(_T("Unknown error"), _T("Error opening timecodes file"), wxOK | wxICON_ERROR, this);
		}
	}

	else {
		VFR_Output.Unload();
		if (VideoContext::Get()->IsLoaded() && !VFR_Output.IsLoaded()) {
			VFR_Output.SetCFR(VideoContext::Get()->GetFPS());
		}
	}

	SubsBox->CommitChanges();
	EditBox->UpdateFrameTiming();
}



/// @brief Saves VFR 
/// @param filename 
///
void FrameMain::SaveVFR(wxString filename) {
	VFR_Output.Save(filename);
}



/// @brief Open help 
/// @param page 
///
void FrameMain::OpenHelp(wxString page) {
	HelpButton::OpenPage(_T("Main"));
}



/// @brief Detach video window 
/// @param detach 
void FrameMain::DetachVideo(bool detach) {
	if (detach) {
		if (!detachedVideo) {
			detachedVideo = new DialogDetachedVideo(this, videoBox->videoDisplay->GetClientSize());
			detachedVideo->Show();
			VideoContext::Get()->UpdateDisplays(true);
		}
	}
	else if (detachedVideo) {
		detachedVideo->Destroy();
		detachedVideo = NULL;
		SetDisplayMode(1,-1);
	}
	UpdateToolbar();
}



/// @brief Sets status and clear after n miliseconds 
/// @param text 
/// @param ms   
///
void FrameMain::StatusTimeout(wxString text,int ms) {
	SetStatusText(text,1);
	StatusClear.SetOwner(this,StatusClear_Timer);
	StatusClear.Start(ms,true);
}



/// @brief Setup accelerator table 
///
void FrameMain::SetAccelerators() {
	std::vector<wxAcceleratorEntry> entry;
	entry.reserve(32);

	// Standard
	entry.push_back(Hotkeys.GetAccelerator(_T("Video global prev frame"),Video_Prev_Frame));
	entry.push_back(Hotkeys.GetAccelerator(_T("Video global next frame"),Video_Next_Frame));
	entry.push_back(Hotkeys.GetAccelerator(_T("Video global focus seek"),Video_Focus_Seek));
	entry.push_back(Hotkeys.GetAccelerator(_T("Grid global prev line"),Grid_Prev_Line));
	entry.push_back(Hotkeys.GetAccelerator(_T("Grid global next line"),Grid_Next_Line));
	entry.push_back(Hotkeys.GetAccelerator(_T("Save Subtitles Alt"),Menu_File_Save_Subtitles));
	entry.push_back(Hotkeys.GetAccelerator(_T("Video global zoom in"),Menu_Video_Zoom_In));
	entry.push_back(Hotkeys.GetAccelerator(_T("Video global zoom out"),Menu_Video_Zoom_Out));
	entry.push_back(Hotkeys.GetAccelerator(_T("Video global play"),Video_Frame_Play));
	entry.push_back(Hotkeys.GetAccelerator(_T("Edit box commit"),Edit_Box_Commit));

	// Medusa
	bool medusaPlay = OPT_GET("Audio/Medusa Timing Hotkeys")->GetBool();
	if (medusaPlay && audioBox->audioDisplay->loaded) {
		entry.push_back(Hotkeys.GetAccelerator(_T("Audio Medusa Play"),Medusa_Play));
		entry.push_back(Hotkeys.GetAccelerator(_T("Audio Medusa Stop"),Medusa_Stop));
		entry.push_back(Hotkeys.GetAccelerator(_T("Audio Medusa Play Before"),Medusa_Play_Before));
		entry.push_back(Hotkeys.GetAccelerator(_T("Audio Medusa Play After"),Medusa_Play_After));
		entry.push_back(Hotkeys.GetAccelerator(_T("Audio Medusa Next"),Medusa_Next));
		entry.push_back(Hotkeys.GetAccelerator(_T("Audio Medusa Previous"),Medusa_Prev));
		entry.push_back(Hotkeys.GetAccelerator(_T("Audio Medusa Shift Start Forward"),Medusa_Shift_Start_Forward));
		entry.push_back(Hotkeys.GetAccelerator(_T("Audio Medusa Shift Start Back"),Medusa_Shift_Start_Back));
		entry.push_back(Hotkeys.GetAccelerator(_T("Audio Medusa Shift End Forward"),Medusa_Shift_End_Forward));
		entry.push_back(Hotkeys.GetAccelerator(_T("Audio Medusa Shift End Back"),Medusa_Shift_End_Back));
		entry.push_back(Hotkeys.GetAccelerator(_T("Audio Medusa Enter"),Medusa_Enter));
	}

	// Set table
	wxAcceleratorTable table(entry.size(),&entry[0]);
	SetAcceleratorTable(table);
}



/// @brief Load list of files 
/// @param list 
/// @return 
///
bool FrameMain::LoadList(wxArrayString list) {
	// Build list
	wxArrayString List;
	for (size_t i=0;i<list.Count();i++) {
		wxFileName file(list[i]);
		if (file.IsRelative()) file.MakeAbsolute();
		if (file.FileExists()) List.Add(file.GetFullPath());
	}

	// Video formats
	wxArrayString videoList;
	videoList.Add(_T("avi"));
	videoList.Add(_T("mkv"));
	videoList.Add(_T("mp4"));
	videoList.Add(_T("d2v"));
	videoList.Add(_T("mpg"));
	videoList.Add(_T("mpeg"));
	videoList.Add(_T("ogm"));
	videoList.Add(_T("avs"));
	videoList.Add(_T("wmv"));
	videoList.Add(_T("asf"));
	videoList.Add(_T("mov"));
	videoList.Add(_T("rm"));
	videoList.Add(_T("y4m"));
	videoList.Add(_T("yuv"));

	// Subtitle formats
	wxArrayString subsList;
	subsList.Add(_T("ass"));
	subsList.Add(_T("ssa"));
	subsList.Add(_T("srt"));
	subsList.Add(_T("sub"));
	subsList.Add(_T("txt"));
	subsList.Add(_T("ttxt"));

	// Audio formats
	wxArrayString audioList;
	audioList.Add(_T("wav"));
	audioList.Add(_T("mp3"));
	audioList.Add(_T("ogg"));
	audioList.Add(_T("wma"));
	audioList.Add(_T("ac3"));
	audioList.Add(_T("aac"));
	audioList.Add(_T("mpc"));
	audioList.Add(_T("ape"));
	audioList.Add(_T("flac"));
	audioList.Add(_T("mka"));
	audioList.Add(_T("m4a"));

	// Scan list
	wxString audio = _T("");
	wxString video = _T("");
	wxString subs = _T("");
	wxString ext;
	for (size_t i=0;i<List.Count();i++) {
		wxFileName file(List[i]);
		ext = file.GetExt().Lower();

		if (subs.IsEmpty() && subsList.Index(ext) != wxNOT_FOUND) subs = List[i];
		if (video.IsEmpty() && videoList.Index(ext) != wxNOT_FOUND) video = List[i];
		if (audio.IsEmpty() && audioList.Index(ext) != wxNOT_FOUND) audio = List[i];
	}

	// Set blocking
	blockAudioLoad = (audio != _T(""));
	blockVideoLoad = (video != _T(""));

	// Load files
	if (subs != _T("")) {
		LoadSubtitles(subs);
	}
	if (blockVideoLoad) {
		blockVideoLoad = false;
		LoadVideo(video);
	}
	if (blockAudioLoad) {
		blockAudioLoad = false;
		LoadAudio(audio);
	}

	// Result
	return ((subs != _T("")) || (audio != _T("")) || (video != _T("")));
}




/// @brief Sets the descriptions for undo/redo 
///
void FrameMain::SetUndoRedoDesc() {
	editMenu->SetHelpString(0,_T("Undo ")+AssFile::GetUndoDescription());
	editMenu->SetHelpString(1,_T("Redo ")+AssFile::GetRedoDescription());
}



/// @brief Check if ASSDraw is available 
///
bool FrameMain::HasASSDraw() {
#ifdef __WINDOWS__
	wxFileName fn(StandardPaths::DecodePath(_T("?data/ASSDraw3.exe")));
	return fn.FileExists();
#else
	return false;
#endif
}


