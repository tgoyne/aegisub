// Copyright (c) 2005, Rodrigo Braz Monteiro
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

/// @file frame_main_events.cpp
/// @brief Event handlers for controls in main window
/// @ingroup main_ui
///


///////////////////
// Include headers
#include "config.h"

#ifndef AGI_PRE
#include <wx/clipbrd.h>
#include <wx/filename.h>
#include <wx/mimetype.h>
#include <wx/rawbmp.h>
#include <wx/stdpaths.h>
#include <wx/sysopt.h>
#include <wx/tglbtn.h>
#endif

#include "ass_dialogue.h"
#include "ass_file.h"
#include "audio_box.h"
#include "audio_display.h"
#ifdef WITH_AUTOMATION
#include "auto4_base.h"
#endif
#include "charset_conv.h"
#include "compat.h"
#include "dialog_about.h"
#include "dialog_attachments.h"
#include "dialog_automation.h"
#include "dialog_dummy_video.h"
#include "dialog_export.h"
#include "dialog_fonts_collector.h"
#include "dialog_jumpto.h"
#include "dialog_kara_timing_copy.h"
#include "dialog_progress.h"
#include "dialog_properties.h"
#include "dialog_resample.h"
#include "dialog_search_replace.h"
#include "dialog_selection.h"
#include "dialog_shift_times.h"
#include "dialog_spellchecker.h"
#include "dialog_style_manager.h"
#include "dialog_styling_assistant.h"
#include "dialog_timing_processor.h"
#include "dialog_translation.h"
#include "dialog_version_check.h"
#include "dialog_video_details.h"
#include "frame_main.h"
#include "hotkeys.h"
#include "keyframe.h"
#include "libresrc/libresrc.h"
#include "main.h"
#include "options.h"
#include "preferences.h"
#include "standard_paths.h"
#include "subs_edit_box.h"
#include "subs_grid.h"
#include "toggle_bitmap.h"
#include "utils.h"
#include "vfr.h"
#include "video_box.h"
#include "video_context.h"
#include "video_display.h"
#include "video_slider.h"

#ifdef __APPLE__
extern "C" {
#include "libosxutil/libosxutil.h"
}
#endif

////////////////////
// Menu event table
BEGIN_EVENT_TABLE(FrameMain, wxFrame)
	EVT_TIMER(AutoSave_Timer, FrameMain::OnAutoSave)
	EVT_TIMER(StatusClear_Timer, FrameMain::OnStatusClear)

	EVT_CLOSE(FrameMain::OnCloseWindow)

	EVT_MENU_OPEN(FrameMain::OnMenuOpen)
	EVT_MENU_RANGE(Menu_File_Recent,Menu_File_Recent+99, FrameMain::OnOpenRecentSubs)
	EVT_MENU_RANGE(Menu_Video_Recent,Menu_Video_Recent+99, FrameMain::OnOpenRecentVideo)
	EVT_MENU_RANGE(Menu_Audio_Recent,Menu_Audio_Recent+99, FrameMain::OnOpenRecentAudio)
	EVT_MENU_RANGE(Menu_Timecodes_Recent,Menu_Timecodes_Recent+99, FrameMain::OnOpenRecentTimecodes)
	EVT_MENU_RANGE(Menu_Keyframes_Recent,Menu_Keyframes_Recent+99, FrameMain::OnOpenRecentKeyframes)
	EVT_MENU_RANGE(Menu_Automation_Macro,Menu_Automation_Macro+99, FrameMain::OnAutomationMacro)

	EVT_MENU_RANGE(MENU_GRID_START+1,MENU_GRID_END-1,FrameMain::OnGridEvent)
	EVT_MENU(Menu_File_New_Window, FrameMain::OnNewWindow)
	EVT_MENU(Menu_File_Exit, FrameMain::OnExit)
	EVT_MENU(Menu_File_Open_Video, FrameMain::OnOpenVideo)
	EVT_MENU(Menu_File_Close_Video, FrameMain::OnCloseVideo)
	EVT_MENU(Menu_File_Open_Subtitles, FrameMain::OnOpenSubtitles)
	EVT_MENU(Menu_File_Open_Subtitles_Charset, FrameMain::OnOpenSubtitlesCharset)
	EVT_MENU(Menu_File_Open_Subtitles_From_Video, FrameMain::OnOpenSubtitlesVideo)
	EVT_MENU(Menu_File_New_Subtitles, FrameMain::OnNewSubtitles)
	EVT_MENU(Menu_File_Save_Subtitles, FrameMain::OnSaveSubtitles)
	EVT_MENU(Menu_File_Save_Subtitles_As, FrameMain::OnSaveSubtitlesAs)
	EVT_MENU(Menu_File_Save_Subtitles_With_Charset, FrameMain::OnSaveSubtitlesCharset)
	EVT_MENU(Menu_File_Export_Subtitles, FrameMain::OnExportSubtitles)
	EVT_MENU(Menu_File_Open_VFR, FrameMain::OnOpenVFR)
	EVT_MENU(Menu_File_Save_VFR, FrameMain::OnSaveVFR)
	EVT_MENU(Menu_File_Close_VFR, FrameMain::OnCloseVFR)
	EVT_MENU(Menu_Video_Load_Keyframes, FrameMain::OnOpenKeyframes)
	EVT_MENU(Menu_Video_Save_Keyframes, FrameMain::OnSaveKeyframes)
	EVT_MENU(Menu_Video_Close_Keyframes, FrameMain::OnCloseKeyframes)

	EVT_MENU(Menu_View_Zoom_50, FrameMain::OnSetZoom50)
	EVT_MENU(Menu_View_Zoom_100, FrameMain::OnSetZoom100)
	EVT_MENU(Menu_View_Zoom_200, FrameMain::OnSetZoom200)
	EVT_COMBOBOX(Toolbar_Zoom_Dropdown, FrameMain::OnSetZoom)
	EVT_TEXT_ENTER(Toolbar_Zoom_Dropdown, FrameMain::OnSetZoom)
	EVT_MENU(Video_Frame_Play, FrameMain::OnVideoPlay)
	EVT_MENU(Menu_Video_Zoom_In, FrameMain::OnZoomIn)
	EVT_MENU(Menu_Video_Zoom_Out, FrameMain::OnZoomOut)
	EVT_MENU(Menu_Video_AR_Default, FrameMain::OnSetARDefault)
	EVT_MENU(Menu_Video_AR_Full, FrameMain::OnSetARFull)
	EVT_MENU(Menu_Video_AR_Wide, FrameMain::OnSetARWide)
	EVT_MENU(Menu_Video_AR_235, FrameMain::OnSetAR235)
	EVT_MENU(Menu_Video_AR_Custom, FrameMain::OnSetARCustom)
	EVT_MENU(Menu_Video_JumpTo, FrameMain::OnJumpTo)
	EVT_MENU(Menu_Video_Select_Visible, FrameMain::OnSelectVisible)
	EVT_MENU(Menu_Video_Detach, FrameMain::OnDetachVideo)
	EVT_MENU(Menu_Video_Dummy, FrameMain::OnDummyVideo)
	EVT_MENU(Menu_Video_Overscan, FrameMain::OnOverscan)
	EVT_MENU(Menu_Video_Details, FrameMain::OnOpenVideoDetails)

	EVT_MENU(Menu_Audio_Open_File, FrameMain::OnOpenAudio)
	EVT_MENU(Menu_Audio_Open_From_Video, FrameMain::OnOpenAudioFromVideo)
	EVT_MENU(Menu_Audio_Close, FrameMain::OnCloseAudio)	
#ifdef _DEBUG
	EVT_MENU(Menu_Audio_Open_Dummy, FrameMain::OnOpenDummyAudio)
	EVT_MENU(Menu_Audio_Open_Dummy_Noise, FrameMain::OnOpenDummyNoiseAudio)
#endif

	EVT_MENU(Menu_Edit_Undo, FrameMain::OnUndo)
	EVT_MENU(Menu_Edit_Redo, FrameMain::OnRedo)
	EVT_MENU(Menu_Edit_Cut, FrameMain::OnCut)
	EVT_MENU(Menu_Edit_Copy, FrameMain::OnCopy)
	EVT_MENU(Menu_Edit_Paste, FrameMain::OnPaste)
	EVT_MENU(Menu_Edit_Paste_Over, FrameMain::OnPasteOver)
	EVT_MENU(Menu_Edit_Find, FrameMain::OnFind)
	EVT_MENU(Menu_Edit_Find_Next, FrameMain::OnFindNext)
	EVT_MENU(Menu_Edit_Replace, FrameMain::OnReplace)
	EVT_MENU(Menu_Edit_Shift, FrameMain::OnShift)
	EVT_MENU(Menu_Edit_Select, FrameMain::OnSelect)

	EVT_MENU(Menu_Subtitles_Sort_Start, FrameMain::OnSortStart)
	EVT_MENU(Menu_Subtitles_Sort_End, FrameMain::OnSortEnd)
	EVT_MENU(Menu_Subtitles_Sort_Style, FrameMain::OnSortStyle)

	EVT_MENU(Menu_Tools_Properties, FrameMain::OnOpenProperties)
	EVT_MENU(Menu_Tools_Styles_Manager, FrameMain::OnOpenStylesManager)
	EVT_MENU(Menu_Tools_Attachments, FrameMain::OnOpenAttachments)
	EVT_MENU(Menu_Tools_Translation, FrameMain::OnOpenTranslation)
	EVT_MENU(Menu_Tools_SpellCheck, FrameMain::OnOpenSpellCheck)
	EVT_MENU(Menu_Tools_Fonts_Collector, FrameMain::OnOpenFontsCollector)
	EVT_MENU(Menu_Tools_Automation, FrameMain::OnOpenAutomation)
	EVT_MENU(Menu_Tools_Styling, FrameMain::OnOpenStylingAssistant)
	EVT_MENU(Menu_Tools_Resample, FrameMain::OnOpenResample)
	EVT_MENU(Menu_Tools_Timing_Processor, FrameMain::OnOpenTimingProcessor)
	EVT_MENU(Menu_Tools_Kanji_Timer, FrameMain::OnOpenKanjiTimer)
	EVT_MENU(Menu_Tools_Options, FrameMain::OnOpenPreferences)
	EVT_MENU(Menu_Tools_ASSDraw, FrameMain::OnOpenASSDraw)
	
	EVT_MENU(Menu_Subs_Snap_Start_To_Video, FrameMain::OnSnapSubsStartToVid)
	EVT_MENU(Menu_Subs_Snap_End_To_Video, FrameMain::OnSnapSubsEndToVid)
	EVT_MENU(Menu_Subs_Snap_Video_To_Start, FrameMain::OnSnapVidToSubsStart)
	EVT_MENU(Menu_Subs_Snap_Video_To_End, FrameMain::OnSnapVidToSubsEnd)
	EVT_MENU(Menu_Video_Snap_To_Scene, FrameMain::OnSnapToScene)
	EVT_MENU(Menu_Video_Shift_To_Frame, FrameMain::OnShiftToFrame)

	EVT_MENU(Menu_Help_Contents, FrameMain::OnContents)
	EVT_MENU(Menu_Help_Files, FrameMain::OnFiles)
	EVT_MENU(Menu_Help_Website, FrameMain::OnWebsite)
	EVT_MENU(Menu_Help_Forums, FrameMain::OnForums)
	EVT_MENU(Menu_Help_BugTracker, FrameMain::OnBugTracker)
	EVT_MENU(Menu_Help_IRCChannel, FrameMain::OnIRCChannel)
	EVT_MENU(Menu_Help_Check_Updates, FrameMain::OnCheckUpdates)
	EVT_MENU(Menu_Help_About, FrameMain::OnAbout)

	EVT_MENU(Menu_View_Language, FrameMain::OnChooseLanguage)
	EVT_MENU(Menu_View_Standard, FrameMain::OnViewStandard)
	EVT_MENU(Menu_View_Audio, FrameMain::OnViewAudio)
	EVT_MENU(Menu_View_Video, FrameMain::OnViewVideo)
	EVT_MENU(Menu_View_Subs, FrameMain::OnViewSubs)
	EVT_MENU(Menu_View_FullTags, FrameMain::OnSetTags)
	EVT_MENU(Menu_View_ShortTags, FrameMain::OnSetTags)
	EVT_MENU(Menu_View_NoTags, FrameMain::OnSetTags)

	EVT_MENU(Video_Prev_Frame,FrameMain::OnPrevFrame)
	EVT_MENU(Video_Next_Frame,FrameMain::OnNextFrame)
	EVT_MENU(Video_Focus_Seek,FrameMain::OnFocusSeek)
	EVT_MENU(Grid_Next_Line,FrameMain::OnNextLine)
	EVT_MENU(Grid_Prev_Line,FrameMain::OnPrevLine)
	EVT_MENU(Grid_Toggle_Tags,FrameMain::OnToggleTags)
	EVT_MENU(Edit_Box_Commit,FrameMain::OnEditBoxCommit)

	EVT_MENU(Medusa_Play, FrameMain::OnMedusaPlay)
	EVT_MENU(Medusa_Stop, FrameMain::OnMedusaStop)
	EVT_MENU(Medusa_Play_After, FrameMain::OnMedusaPlayAfter)
	EVT_MENU(Medusa_Play_Before, FrameMain::OnMedusaPlayBefore)
	EVT_MENU(Medusa_Next, FrameMain::OnMedusaNext)
	EVT_MENU(Medusa_Prev, FrameMain::OnMedusaPrev)
	EVT_MENU(Medusa_Shift_Start_Forward, FrameMain::OnMedusaShiftStartForward)
	EVT_MENU(Medusa_Shift_Start_Back, FrameMain::OnMedusaShiftStartBack)
	EVT_MENU(Medusa_Shift_End_Forward, FrameMain::OnMedusaShiftEndForward)
	EVT_MENU(Medusa_Shift_End_Back, FrameMain::OnMedusaShiftEndBack)
	EVT_MENU(Medusa_Enter, FrameMain::OnMedusaEnter)

#ifdef __WXMAC__
   EVT_MENU(wxID_ABOUT, FrameMain::OnAbout)
   EVT_MENU(wxID_EXIT, FrameMain::OnExit)
#endif
END_EVENT_TABLE()



/// @brief Redirect grid events to grid 
/// @param event 
///
void FrameMain::OnGridEvent (wxCommandEvent &event) {
	SubsBox->GetEventHandler()->ProcessEvent(event);
}



/// @brief Rebuild recent list 
/// @param listName 
/// @param menu     
/// @param startID  
///
void FrameMain::RebuildRecentList(wxString listName,wxMenu *menu,int startID) {
	// Wipe previous list
	int count = (int)menu->GetMenuItemCount();
	for (int i=count;--i>=0;) {
		menu->Destroy(menu->FindItemByPosition(i));
	}

	// Rebuild
	int added = 0;
	wxString n;
	wxArrayString entries = lagi_MRU_wxAS(listName);
	for (size_t i=0;i<entries.Count();i++) {
		n = wxString::Format(_T("%i"),i+1);
		if (i < 9) n = _T("&") + n;
		wxFileName shortname(entries[i]);
		wxString filename = shortname.GetFullName();
		menu->Append(startID+i,n + _T(" ") + filename);
		added++;
	}

	// Nothing added, add an empty placeholder
	if (added == 0) menu->Append(startID,_("Empty"))->Enable(false);
}



/// @brief Menu is being opened 
/// @param event 
///
void FrameMain::OnMenuOpen (wxMenuEvent &event) {
	// Get menu
	MenuBar->Freeze();
	wxMenu *curMenu = event.GetMenu();

	// File menu
	if (curMenu == fileMenu) {
		// Rebuild recent
		RebuildRecentList(_T("Subtitle"),RecentSubs,Menu_File_Recent);

		MenuBar->Enable(Menu_File_Open_Subtitles_From_Video,VideoContext::Get()->HasSubtitles());
	}

	// View menu
	else if (curMenu == viewMenu) {
		// Flags
		bool aud = audioBox->audioDisplay->loaded;
		bool vid = VideoContext::Get()->IsLoaded() && !detachedVideo;

		// Set states
		MenuBar->Enable(Menu_View_Audio,aud);
		MenuBar->Enable(Menu_View_Video,vid);
		MenuBar->Enable(Menu_View_Standard,aud && vid);

		// Select option
		if (!showVideo && !showAudio) MenuBar->Check(Menu_View_Subs,true);
		else if (showVideo && !showAudio) MenuBar->Check(Menu_View_Video,true);
		else if (showAudio && showVideo) MenuBar->Check(Menu_View_Standard,true);
		else MenuBar->Check(Menu_View_Audio,true);

		MenuBar->Check(OPT_GET("Subtitle/Grid/Hide Overrides")->GetInt() + Menu_View_FullTags, true);
	}

	// Video menu
	else if (curMenu == videoMenu) {
		bool state = VideoContext::Get()->IsLoaded();
		bool attached = state && !detachedVideo;

		// Set states
		MenuBar->Enable(Menu_Video_JumpTo,state);
		MenuBar->Enable(Menu_Subs_Snap_Video_To_Start,state);
		MenuBar->Enable(Menu_Subs_Snap_Video_To_End,state);
		MenuBar->Enable(Menu_View_Zoom,attached);
		MenuBar->Enable(Menu_View_Zoom_50,attached);
		MenuBar->Enable(Menu_View_Zoom_100,attached);
		MenuBar->Enable(Menu_View_Zoom_200,attached);
		MenuBar->Enable(Menu_File_Close_Video,state);
		MenuBar->Enable(Menu_Video_AR,attached);
		MenuBar->Enable(Menu_Video_AR_Default,attached);
		MenuBar->Enable(Menu_Video_AR_Full,attached);
		MenuBar->Enable(Menu_Video_AR_Wide,attached);
		MenuBar->Enable(Menu_Video_AR_235,attached);
		MenuBar->Enable(Menu_Video_AR_Custom,attached);
		MenuBar->Enable(Menu_Video_Detach,state);
		MenuBar->Enable(Menu_File_Save_VFR,VFR_Output.GetFrameRateType() == VFR);
		MenuBar->Enable(Menu_File_Close_VFR,VFR_Output.GetFrameRateType() == VFR);
		MenuBar->Enable(Menu_Video_Close_Keyframes,VideoContext::Get()->OverKeyFramesLoaded());
		MenuBar->Enable(Menu_Video_Save_Keyframes,VideoContext::Get()->KeyFramesLoaded());
		MenuBar->Enable(Menu_Video_Details,state);
		MenuBar->Enable(Menu_Video_Overscan,state);

		// Set AR radio
		int arType = VideoContext::Get()->GetAspectRatioType();
		MenuBar->Check(Menu_Video_AR_Default,false);
		MenuBar->Check(Menu_Video_AR_Full,false);
		MenuBar->Check(Menu_Video_AR_Wide,false);
		MenuBar->Check(Menu_Video_AR_235,false);
		MenuBar->Check(Menu_Video_AR_Custom,false);
		switch (arType) {
			case 0: MenuBar->Check(Menu_Video_AR_Default,true); break;
			case 1: MenuBar->Check(Menu_Video_AR_Full,true); break;
			case 2: MenuBar->Check(Menu_Video_AR_Wide,true); break;
			case 3: MenuBar->Check(Menu_Video_AR_235,true); break;
			case 4: MenuBar->Check(Menu_Video_AR_Custom,true); break;
		}

		// Set overscan mask
		MenuBar->Check(Menu_Video_Overscan,OPT_GET("Video/Overscan Mask")->GetBool());

		// Rebuild recent lists
		RebuildRecentList(_T("Video"),RecentVids,Menu_Video_Recent);
		RebuildRecentList(_T("Timecodes"),RecentTimecodes,Menu_Timecodes_Recent);
		RebuildRecentList(_T("Keyframes"),RecentKeyframes,Menu_Keyframes_Recent);
	}

	// Audio menu
	else if (curMenu == audioMenu) {
		bool state = audioBox->loaded;
		bool vidstate = VideoContext::Get()->IsLoaded();

		MenuBar->Enable(Menu_Audio_Open_From_Video,vidstate);
		MenuBar->Enable(Menu_Audio_Close,state);

		// Rebuild recent
		RebuildRecentList(_T("Audio"),RecentAuds,Menu_Audio_Recent);
	}

	// Subtitles menu
	else if (curMenu == subtitlesMenu) {
		// Variables
		bool continuous;
		wxArrayInt sels = SubsBox->GetSelection(&continuous);
		int count = sels.Count();
		bool state,state2;

		// Entries
		state = count > 0;
		MenuBar->Enable(MENU_INSERT_BEFORE,state);
		MenuBar->Enable(MENU_INSERT_AFTER,state);
		MenuBar->Enable(MENU_SPLIT_BY_KARAOKE,state);
		MenuBar->Enable(MENU_DELETE,state);
		state2 = count > 0 && VideoContext::Get()->IsLoaded();
		MenuBar->Enable(MENU_INSERT_BEFORE_VIDEO,state2);
		MenuBar->Enable(MENU_INSERT_AFTER_VIDEO,state2);
		MenuBar->Enable(Menu_Subtitles_Insert,state);
		state = count > 0 && continuous;
		MenuBar->Enable(MENU_DUPLICATE,state);
		state = count > 0 && continuous && VFR_Output.IsLoaded();
		MenuBar->Enable(MENU_DUPLICATE_NEXT_FRAME,state);
		state = count == 2;
		MenuBar->Enable(MENU_SWAP,state);
		state = count >= 2 && continuous;
		MenuBar->Enable(MENU_JOIN_CONCAT,state);
		MenuBar->Enable(MENU_JOIN_REPLACE,state);
		MenuBar->Enable(MENU_JOIN_AS_KARAOKE,state);
		MenuBar->Enable(Menu_Subtitles_Join,state);
		state = (count == 2 || count == 3) && continuous;
		MenuBar->Enable(MENU_RECOMBINE,state);
	}

	// Timing menu
	else if (curMenu == timingMenu) {
		// Variables
		bool continuous;
		wxArrayInt sels = SubsBox->GetSelection(&continuous);
		int count = sels.Count();

		// Video related
		bool state = VideoContext::Get()->IsLoaded();
		MenuBar->Enable(Menu_Subs_Snap_Start_To_Video,state);
		MenuBar->Enable(Menu_Subs_Snap_End_To_Video,state);
		MenuBar->Enable(Menu_Video_Snap_To_Scene,state);
		MenuBar->Enable(Menu_Video_Shift_To_Frame,state);

		// Other
		state = count >= 2 && continuous;
		MenuBar->Enable(MENU_ADJOIN,state);
		MenuBar->Enable(MENU_ADJOIN2,state);
	}

	// Edit menu
	else if (curMenu == editMenu) {
		// Undo state
		wxMenuItem *item;
		wxString undo_text = _("&Undo") + wxString(_T(" ")) + AssFile::GetUndoDescription() + wxString(_T("\t")) + Hotkeys.GetText(_T("Undo"));
		item = editMenu->FindItem(Menu_Edit_Undo);
		item->SetItemLabel(undo_text);
		item->Enable(!AssFile::IsUndoStackEmpty());

		// Redo state
		wxString redo_text = _("&Redo") + wxString(_T(" ")) + AssFile::GetRedoDescription() + wxString(_T("\t")) + Hotkeys.GetText(_T("Redo"));
		item = editMenu->FindItem(Menu_Edit_Redo);
		item->SetItemLabel(redo_text);
		item->Enable(!AssFile::IsRedoStackEmpty());

		// Copy/cut/paste
		wxArrayInt sels = SubsBox->GetSelection();
		bool can_copy = (sels.Count() > 0);

		bool can_paste = true;
		if (wxTheClipboard->Open()) {
			can_paste = wxTheClipboard->IsSupported(wxDF_TEXT);
			wxTheClipboard->Close();
		}

		MenuBar->Enable(Menu_Edit_Cut,can_copy);
		MenuBar->Enable(Menu_Edit_Copy,can_copy);
		MenuBar->Enable(Menu_Edit_Paste,can_paste);
		MenuBar->Enable(Menu_Edit_Paste_Over,can_copy&&can_paste);
	}

	// Automation menu
#ifdef WITH_AUTOMATION
	else if (curMenu == automationMenu) {
		// Remove old macro items
		for (unsigned int i = 0; i < activeMacroItems.size(); i++) {
			wxMenu *p = 0;
			wxMenuItem *it = MenuBar->FindItem(Menu_Automation_Macro + i, &p);
			if (it)
				p->Delete(it);
		}
		activeMacroItems.clear();

		// Add new ones
		int added = 0;
		added += AddMacroMenuItems(automationMenu, wxGetApp().global_scripts->GetMacros());
		added += AddMacroMenuItems(automationMenu, local_scripts->GetMacros());

		// If none were added, show a ghosted notice
		if (added == 0) {
			automationMenu->Append(Menu_Automation_Macro, _("No Automation macros loaded"))->Enable(false);
			activeMacroItems.push_back(0);
		}
	}
#endif

	MenuBar->Thaw();
}



/// @brief Macro menu creation helper 
/// @param menu   
/// @param macros 
/// @return 
///
int FrameMain::AddMacroMenuItems(wxMenu *menu, const std::vector<Automation4::FeatureMacro*> &macros) {
#ifdef WITH_AUTOMATION
	if (macros.empty()) {
		return 0;
	}

	int id = activeMacroItems.size();;
	for (std::vector<Automation4::FeatureMacro*>::const_iterator i = macros.begin(); i != macros.end(); ++i) {
		wxMenuItem * m = menu->Append(Menu_Automation_Macro + id, (*i)->GetName(), (*i)->GetDescription());
		m->Enable((*i)->Validate(SubsBox->ass, SubsBox->GetAbsoluteSelection(), SubsBox->GetFirstSelRow()));
		activeMacroItems.push_back(*i);
		id++;
	}

	return macros.size();
#else
	return 0;
#endif
}



/// @brief Open recent subs menu entry 
/// @param event 
///
void FrameMain::OnOpenRecentSubs(wxCommandEvent &event) {
	int number = event.GetId()-Menu_File_Recent;
	LoadSubtitles(AegisubApp::Get()->mru->GetEntry("Subtitle", number));
}



/// @brief Open recent video menu entry 
/// @param event 
///
void FrameMain::OnOpenRecentVideo(wxCommandEvent &event) {
	int number = event.GetId()-Menu_Video_Recent;
	LoadVideo(AegisubApp::Get()->mru->GetEntry("Video", number));
}



/// @brief Open recent timecodes entry 
/// @param event 
///
void FrameMain::OnOpenRecentTimecodes(wxCommandEvent &event) {
	int number = event.GetId()-Menu_Timecodes_Recent;
	LoadVFR(AegisubApp::Get()->mru->GetEntry("Timecodes", number));
}



/// @brief Open recent Keyframes entry 
/// @param event 
///
void FrameMain::OnOpenRecentKeyframes(wxCommandEvent &event) {
	int number = event.GetId()-Menu_Keyframes_Recent;
	KeyFrameFile::Load(AegisubApp::Get()->mru->GetEntry("Keyframes", number));
	videoBox->videoSlider->Refresh();
	audioBox->audioDisplay->Update();
	Refresh();
}



/// @brief Open recent audio menu entry 
/// @param event 
///
void FrameMain::OnOpenRecentAudio(wxCommandEvent &event) {
	int number = event.GetId()-Menu_Audio_Recent;
	LoadSubtitles(AegisubApp::Get()->mru->GetEntry("Audio", number));
}



/// @brief Open new Window 
/// @param event 
///
void FrameMain::OnNewWindow(wxCommandEvent& WXUNUSED(event)) {
	RestartAegisub();
	//wxStandardPaths stand;
	//wxExecute(stand.GetExecutablePath());
}



/// @brief Exit 
///
void FrameMain::OnExit(wxCommandEvent& WXUNUSED(event)) {
	Close();
}



/// @brief Open about box 
/// @param event 
///
void FrameMain::OnAbout(wxCommandEvent &event) {
	AboutScreen About(this);
	About.ShowModal();
}



/// @brief Open check updates
/// @param event 
///
void FrameMain::OnCheckUpdates(wxCommandEvent &event) {
	PerformVersionCheck(true);
}



/// @brief Open help topics 
/// @param event 
///
void FrameMain::OnContents(wxCommandEvent& WXUNUSED(event)) {
	OpenHelp(_T(""));
}

/// @brief Open help files on OSX.
/// @param event
///
void FrameMain::OnFiles(wxCommandEvent& WXUNUSED(event)) {
#ifdef __WXMAC__
	char *shared_path = OSX_GetBundleSharedSupportDirectory();
	wxString help_path = wxString::Format(_T("%s/doc"), wxString(shared_path, wxConvUTF8).c_str());
	OSX_OpenLocation(help_path.c_str());
#endif
}



/// @brief Open website 
/// @param event 
///
void FrameMain::OnWebsite(wxCommandEvent& WXUNUSED(event)) {
	AegisubApp::OpenURL(_T("http://www.aegisub.org/"));
}



/// @brief Open forums 
/// @param event 
///
void FrameMain::OnForums(wxCommandEvent& WXUNUSED(event)) {
	AegisubApp::OpenURL(_T("http://forum.aegisub.org/"));
}



/// @brief Open bugtracker 
/// @param event 
///
void FrameMain::OnBugTracker(wxCommandEvent& WXUNUSED(event)) {
	if (wxGetMouseState().CmdDown()) {
		if (wxGetMouseState().ShiftDown()) {
			wxMessageBox(_T("Now crashing with an access violation..."));
			for (char *foo = (char*)0;;) *foo++ = 42;
		}
		else {
			wxMessageBox(_T("Now crashing with an unhandled exception..."));
			throw this;
		}
	}

	AegisubApp::OpenURL(_T("http://devel.aegisub.org/"));
}



/// @brief Open IRC channel 
/// @param event 
///
void FrameMain::OnIRCChannel(wxCommandEvent& WXUNUSED(event)) {
	AegisubApp::OpenURL(_T("irc://irc.rizon.net/aegisub"));
}



/// @brief Play video 
/// @param event 
///
void FrameMain::OnVideoPlay(wxCommandEvent &event) {
	VideoContext::Get()->Play();
}




/// @brief Open video 
/// @param event 
///
void FrameMain::OnOpenVideo(wxCommandEvent& WXUNUSED(event)) {
	wxString path = lagi_wxString(OPT_GET("Path/Last/Video")->GetString());
	wxString str = wxString(_("Video Formats")) + _T(" (*.avi,*.mkv,*.mp4,*.avs,*.d2v,*.ogm,*.mpeg,*.mpg,*.vob,*.mov)|*.avi;*.avs;*.d2v;*.mkv;*.ogm;*.mp4;*.mpeg;*.mpg;*.vob;*.mov|")
				 + _("All Files") + _T(" (*.*)|*.*");
	wxString filename = wxFileSelector(_("Open video file"),path,_T(""),_T(""),str,wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (!filename.empty()) {
		LoadVideo(filename);
		OPT_SET("Path/Last/Video")->SetString(STD_STR(filename));
	}
}



/// @brief Close video 
/// @param event 
///
void FrameMain::OnCloseVideo(wxCommandEvent& WXUNUSED(event)) {
	LoadVideo(_T(""));
}



/// @brief Open Audio 
/// @param event 
///
void FrameMain::OnOpenAudio (wxCommandEvent& WXUNUSED(event)) {
	wxString path = lagi_wxString(OPT_GET("Path/Last/Audio")->GetString());
	wxString str = wxString(_("Audio Formats")) + _T(" (*.wav,*.mp3,*.ogg,*.flac,*.mp4,*.ac3,*.aac,*.mka,*.m4a,*.w64)|*.wav;*.mp3;*.ogg;*.flac;*.mp4;*.ac3;*.aac;*.mka;*.m4a;*.w64|")
		         + _("Video Formats") + _T(" (*.avi,*.mkv,*.ogm,*.mpg,*.mpeg)|*.avi;*.mkv;*.ogm;*.mp4;*.mpeg;*.mpg|")
				 + _("All files") + _T(" (*.*)|*.*");
	wxString filename = wxFileSelector(_("Open audio file"),path,_T(""),_T(""),str,wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (!filename.empty()) {
		LoadAudio(filename);
		OPT_SET("Path/Last/Audio")->SetString(STD_STR(filename));
	}
}



/// @brief DOCME
/// @param event 
///
void FrameMain::OnOpenAudioFromVideo (wxCommandEvent& WXUNUSED(event)) {
	LoadAudio(_T(""),true);
}



/// @brief DOCME
/// @param event 
///
void FrameMain::OnCloseAudio (wxCommandEvent& WXUNUSED(event)) {
	LoadAudio(_T(""));
}

#ifdef _DEBUG

/// @brief DOCME
/// @param event 
///
void FrameMain::OnOpenDummyAudio (wxCommandEvent& WXUNUSED(event)) {
	LoadAudio(_T("?dummy"));
}

/// @brief DOCME
/// @param event 
///
void FrameMain::OnOpenDummyNoiseAudio (wxCommandEvent& WXUNUSED(event)) {
	LoadAudio(_T("?noise"));
}
#endif



/// @brief Open subtitles 
/// @param event 
///
void FrameMain::OnOpenSubtitles(wxCommandEvent& WXUNUSED(event)) {
	wxString path = lagi_wxString(OPT_GET("Path/Last/Subtitles")->GetString());	
	wxString filename = wxFileSelector(_("Open subtitles file"),path,_T(""),_T(""),AssFile::GetWildcardList(0),wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (!filename.empty()) {
		LoadSubtitles(filename);
		wxFileName filepath(filename);
		OPT_SET("Path/Last/Subtitles")->SetString(STD_STR(filepath.GetPath()));
	}
}



/// @brief Open subtitles with specific charset 
/// @param event 
///
void FrameMain::OnOpenSubtitlesCharset(wxCommandEvent& WXUNUSED(event)) {
	// Initialize charsets
	wxArrayString choices = AegisubCSConv::GetEncodingsList();
	wxString path = lagi_wxString(OPT_GET("Path/Last/Subtitles")->GetString());

	// Get options and load
	wxString filename = wxFileSelector(_("Open subtitles file"),path,_T(""),_T(""),AssFile::GetWildcardList(0),wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (!filename.empty()) {
		wxString charset = wxGetSingleChoice(_("Choose charset code:"), _("Charset"),choices,this,-1, -1,true,250,200);
		if (!charset.empty()) {
			LoadSubtitles(filename,charset);
		}
		OPT_SET("Path/Last/Subtitles")->SetString(STD_STR(filename));
	}
}

/// @brief Open subtitles from the currently open video file
void FrameMain::OnOpenSubtitlesVideo(wxCommandEvent&) {
	LoadSubtitles(VideoContext::Get()->videoName);
}



/// @brief Save subtitles as 
/// @param event 
///
void FrameMain::OnSaveSubtitlesAs(wxCommandEvent& WXUNUSED(event)) {
	SaveSubtitles(true);
}



/// @brief Save subtitles 
/// @param event 
///
void FrameMain::OnSaveSubtitles(wxCommandEvent& WXUNUSED(event)) {
	SaveSubtitles(false);
}



/// @brief Save subtitles with specific charset 
/// @param event 
///
void FrameMain::OnSaveSubtitlesCharset(wxCommandEvent& WXUNUSED(event)) {
	SaveSubtitles(true,true);
}



/// @brief Close subtitles 
/// @param event 
///
void FrameMain::OnNewSubtitles(wxCommandEvent& WXUNUSED(event)) {
	LoadSubtitles(_T(""));
}



/// @brief Export subtitles 
/// @param event 
///
void FrameMain::OnExportSubtitles(wxCommandEvent & WXUNUSED(event)) {
#ifdef WITH_AUTOMATION
	int autoreload = OPT_GET("Automation/Autoreload Mode")->GetInt();
	if (autoreload & 1) {
		// Local scripts
		const std::vector<Automation4::Script*> scripts = local_scripts->GetScripts();
		for (size_t i = 0; i < scripts.size(); ++i) {
			try {
				scripts[i]->Reload();
			}
			catch (const wchar_t *e) {
				wxLogError(_T("Error while reloading Automation scripts before export: %s"), e);
			}
			catch (...) {
				wxLogError(_T("An unknown error occurred reloading Automation script '%s'."), scripts[i]->GetName().c_str());
			}
		}
	}
	if (autoreload & 2) {
		// Global scripts
		wxGetApp().global_scripts->Reload();
	}
#endif

	DialogExport exporter(this);
	exporter.ShowModal();
}



/// @brief Open VFR tags 
/// @param event 
///
void FrameMain::OnOpenVFR(wxCommandEvent &event) {
	wxString path = lagi_wxString(OPT_GET("Path/Last/Timecodes")->GetString());
	wxString str = wxString(_("All Supported Types")) + _T("(*.txt)|*.txt|")
		           + _("All Files") + _T(" (*.*)|*.*");
	wxString filename = wxFileSelector(_("Open timecodes file"),path,_T(""),_T(""),str,wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (!filename.empty()) {
		LoadVFR(filename);
		OPT_SET("Path/Last/Timecodes")->SetString(STD_STR(filename));
	}
}



/// @brief Save VFR tags 
/// @param event 
///
void FrameMain::OnSaveVFR(wxCommandEvent &event) {
	wxString path = lagi_wxString(OPT_GET("Path/Last/Timecodes")->GetString());
	wxString str = wxString(_("All Supported Types")) + _T("(*.txt)|*.txt|")
		           + _("All Files") + _T(" (*.*)|*.*");
	wxString filename = wxFileSelector(_("Save timecodes file"),path,_T(""),_T(""),str,wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (!filename.empty()) {
		SaveVFR(filename);
		OPT_SET("Path/Last/Timecodes")->SetString(STD_STR(filename));
	}
}




/// @brief Close VFR tags 
/// @param event 
///
void FrameMain::OnCloseVFR(wxCommandEvent &event) {
	LoadVFR(_T(""));
}



/// @brief Open keyframes 
/// @param event 
/// @return 
///
void FrameMain::OnOpenKeyframes (wxCommandEvent &event) {
	// Pick file
	wxString path = lagi_wxString(OPT_GET("Path/Last/Keyframes")->GetString());
	wxString filename = wxFileSelector(_T("Select the keyframes file to open"),path,_T(""),_T(".txt"),_T("All supported formats (*.txt, *.pass, *.stats, *.log)|*.txt;*.pass;*.stats;*.log|All files (*.*)|*.*"),wxFD_FILE_MUST_EXIST | wxFD_OPEN);
	if (filename.IsEmpty()) return;
	OPT_SET("Path/Last/Keyframes")->SetString(STD_STR(filename));

	// Load
	KeyFrameFile::Load(filename);
	videoBox->videoSlider->Refresh();
	audioBox->audioDisplay->Update();
	Refresh();
}



/// @brief Close keyframes 
/// @param event 
///
void FrameMain::OnCloseKeyframes (wxCommandEvent &event) {
	VideoContext::Get()->CloseOverKeyFrames();
	videoBox->videoSlider->Refresh();
	audioBox->audioDisplay->Update();
	Refresh();
}



/// @brief Save keyframes 
/// @param event 
/// @return 
///
void FrameMain::OnSaveKeyframes (wxCommandEvent &event) {
	// Pick file
	wxString path = lagi_wxString(OPT_GET("Path/Last/Keyframes")->GetString());
	wxString filename = wxFileSelector(_T("Select the Keyframes file to open"),path,_T(""),_T("*.key.txt"),_T("Text files (*.txt)|*.txt"),wxFD_OVERWRITE_PROMPT | wxFD_SAVE);
	if (filename.IsEmpty()) return;
	OPT_SET("Path/Last/Keyframes")->SetString(STD_STR(filename));

	// Save
	KeyFrameFile::Save(filename);
}



/// @brief Zoom levels 
/// @param event 
///
void FrameMain::OnSetZoom50(wxCommandEvent& WXUNUSED(event)) {
	VideoContext::Get()->Stop();
	videoBox->videoDisplay->SetZoom(.5);
}


/// @brief DOCME
/// @param event 
///
void FrameMain::OnSetZoom100(wxCommandEvent& WXUNUSED(event)) {
	VideoContext::Get()->Stop();
	videoBox->videoDisplay->SetZoom(1.);
}


/// @brief DOCME
/// @param event 
///
void FrameMain::OnSetZoom200(wxCommandEvent& WXUNUSED(event)) {
	VideoContext::Get()->Stop();
	videoBox->videoDisplay->SetZoom(2.);
}


/// @brief DOCME
/// @param event 
///
void FrameMain::OnZoomIn (wxCommandEvent &event) {
	VideoContext::Get()->Stop();
	videoBox->videoDisplay->SetZoom(videoBox->videoDisplay->GetZoom() + .125);
}


/// @brief DOCME
/// @param event 
///
void FrameMain::OnZoomOut (wxCommandEvent &event) {
	VideoContext::Get()->Stop();
	videoBox->videoDisplay->SetZoom(videoBox->videoDisplay->GetZoom() - .125);
}


/// @brief DOCME
/// @param event 
///
void FrameMain::OnSetZoom(wxCommandEvent &event) {
	videoBox->videoDisplay->SetZoomFromBox();
}



/// @brief Detach video 
/// @param event 
///
void FrameMain::OnDetachVideo(wxCommandEvent &event) {
	DetachVideo(!detachedVideo);
}



/// @brief Use dummy video 
/// @param event 
///
void FrameMain::OnDummyVideo (wxCommandEvent &event) {
	wxString fn;
	if (DialogDummyVideo::CreateDummyVideo(this, fn)) {
		LoadVideo(fn);
	}
}



/// @brief Overscan toggle 
/// @param event 
///
void FrameMain::OnOverscan (wxCommandEvent &event) {
	OPT_SET("Video/Overscan Mask")->SetBool(event.IsChecked());
	VideoContext::Get()->Stop();
	videoBox->videoDisplay->Render();
}



/// @brief Show video details 
/// @param event 
///
void FrameMain::OnOpenVideoDetails (wxCommandEvent &event) {
	VideoContext::Get()->Stop();
	DialogVideoDetails videodetails(this);
	videodetails.ShowModal();	
}



/// @brief Open jump to dialog 
/// @param event 
///
void FrameMain::OnJumpTo(wxCommandEvent& WXUNUSED(event)) {
	VideoContext::Get()->Stop();
	if (VideoContext::Get()->IsLoaded()) {
		DialogJumpTo JumpTo(this);
		JumpTo.ShowModal();
		videoBox->videoSlider->SetFocus();
	}
}



/// @brief Open shift dialog 
/// @param event 
///
void FrameMain::OnShift(wxCommandEvent& WXUNUSED(event)) {
	VideoContext::Get()->Stop();
	DialogShiftTimes Shift(this,SubsBox);
	Shift.ShowModal();
}



/// @brief Open properties 
/// @param event 
///
void FrameMain::OnOpenProperties (wxCommandEvent &event) {
	VideoContext::Get()->Stop();
	DialogProperties Properties(this);
	int res = Properties.ShowModal();
	if (res) {
		SubsBox->CommitChanges();
	}
}



/// @brief Open styles manager 
/// @param event 
///
void FrameMain::OnOpenStylesManager(wxCommandEvent& WXUNUSED(event)) {
	VideoContext::Get()->Stop();
	DialogStyleManager StyleManager(this,SubsBox);
	StyleManager.ShowModal();
	EditBox->UpdateGlobals();
	SubsBox->CommitChanges();
}



/// @brief Open attachments 
/// @param event 
///
void FrameMain::OnOpenAttachments(wxCommandEvent& WXUNUSED(event)) {
	VideoContext::Get()->Stop();
	DialogAttachments attachments(this);
	attachments.ShowModal();
}



/// @brief Open translation assistant 
/// @param event 
///
void FrameMain::OnOpenTranslation(wxCommandEvent& WXUNUSED(event)) {
	VideoContext::Get()->Stop();
	int start = SubsBox->GetFirstSelRow();
	if (start == -1) start = 0;
	DialogTranslation Trans(this,AssFile::top,SubsBox,start,true);
	Trans.ShowModal();
}



/// @brief Open Spell Checker 
/// @param event 
///
void FrameMain::OnOpenSpellCheck (wxCommandEvent &event) {
	VideoContext::Get()->Stop();
	new DialogSpellChecker(this);
	//DialogSpellChecker *spell = 
	//spell->ShowModal();
}



/// @brief Open Fonts Collector 
/// @param event 
///
void FrameMain::OnOpenFontsCollector (wxCommandEvent &event) {
	VideoContext::Get()->Stop();
	DialogFontsCollector Collector(this);
	Collector.ShowModal();
}



/// @brief Open Resolution Resampler 
/// @param event 
///
void FrameMain::OnOpenResample (wxCommandEvent &event) {
	VideoContext::Get()->Stop();
	DialogResample diag(this, SubsBox);
	diag.ShowModal();
}



/// @brief Open Timing post-processor dialog 
/// @param event 
///
void FrameMain::OnOpenTimingProcessor (wxCommandEvent &event) {
	DialogTimingProcessor timing(this,SubsBox);
	timing.ShowModal();
}

/// @brief Open Kanji Timer dialog 
/// @param event 
///
void FrameMain::OnOpenKanjiTimer (wxCommandEvent &event) {
	DialogKanjiTimer kanjitimer(this,SubsBox);
	kanjitimer.ShowModal();
}



/// @brief Open Options dialog 
/// @param event 
///
void FrameMain::OnOpenPreferences (wxCommandEvent &event) {
	try {
		Preferences pref(this);
		pref.ShowModal();

	} catch (agi::Exception& e) {
		wxPrintf("Caught agi::Exception: %s -> %s\n", e.GetName(), e.GetMessage());
	}
}



/// @brief Launch ASSDraw 
/// @param event 
///
void FrameMain::OnOpenASSDraw (wxCommandEvent &event) {
	wxExecute(_T("\"") + StandardPaths::DecodePath(_T("?data/ASSDraw3.exe")) + _T("\""));
}



/// @brief Open Automation 
/// @param event 
///
void FrameMain::OnOpenAutomation (wxCommandEvent &event) {
#ifdef WITH_AUTOMATION
#ifdef __APPLE__
	if (wxGetMouseState().CmdDown()) {
#else
	if (wxGetMouseState().ControlDown()) {
#endif
		wxGetApp().global_scripts->Reload();
		if (wxGetMouseState().ShiftDown()) {
			const std::vector<Automation4::Script*> scripts = local_scripts->GetScripts();
			for (size_t i = 0; i < scripts.size(); ++i) {
				try {
					scripts[i]->Reload();
				}
				catch (const wchar_t *e) {
					wxLogError(e);
				}
				catch (...) {
					wxLogError(_T("An unknown error occurred reloading Automation script '%s'."), scripts[i]->GetName().c_str());
				}
			}

			StatusTimeout(_("Reloaded all Automation scripts"));
		}
		else {
			StatusTimeout(_("Reloaded autoload Automation scripts"));
		}
	}
	else {
		VideoContext::Get()->Stop();
		DialogAutomation dlg(this, local_scripts);
		dlg.ShowModal();
	}
#endif
}



/// @brief General handler for all Automation-generated menu items 
/// @param event 
///
void FrameMain::OnAutomationMacro (wxCommandEvent &event) {
#ifdef WITH_AUTOMATION
	SubsBox->BeginBatch();
	// First get selection data
	// This much be done before clearing the maps, since selection data are lost during that
	std::vector<int> selected_lines = SubsBox->GetAbsoluteSelection();
	int first_sel = SubsBox->GetFirstSelRow();
	// Clear all maps from the subs grid before running the macro
	// The stuff done by the macro might invalidate some of the iterators held by the grid, which will cause great crashing
	SubsBox->Clear();
	// Run the macro...
	activeMacroItems[event.GetId()-Menu_Automation_Macro]->Process(SubsBox->ass, selected_lines, first_sel, this);
	// Have the grid update its maps, this properly refreshes it to reflect the changed subs
	SubsBox->UpdateMaps();
	SubsBox->SetSelectionFromAbsolute(selected_lines);
	SubsBox->CommitChanges(true, false);
	SubsBox->AdjustScrollbar();
	SubsBox->EndBatch();
#endif
}



/// @brief Snap subs to video 
/// @param event 
///
void FrameMain::OnSnapSubsStartToVid (wxCommandEvent &event) {
	if (VideoContext::Get()->IsLoaded() && SubsBox->GetSelection().Count() > 0) {
		SubsBox->SetSubsToVideo(true);
	}
}


/// @brief DOCME
/// @param event 
///
void FrameMain::OnSnapSubsEndToVid (wxCommandEvent &event) {
	if (VideoContext::Get()->IsLoaded() && SubsBox->GetSelection().Count() > 0) {
		SubsBox->SetSubsToVideo(false);
	}
}



/// @brief Jump video to subs 
/// @param event 
///
void FrameMain::OnSnapVidToSubsStart (wxCommandEvent &event) {
	if (VideoContext::Get()->IsLoaded() && SubsBox->GetSelection().Count() > 0) {
		SubsBox->SetVideoToSubs(true);
	}
}


/// @brief DOCME
/// @param event 
///
void FrameMain::OnSnapVidToSubsEnd (wxCommandEvent &event) {
	if (VideoContext::Get()->IsLoaded() && SubsBox->GetSelection().Count() > 0) {
		SubsBox->SetVideoToSubs(false);
	}
}



/// @brief Snap to scene 
/// @param event 
///
void FrameMain::OnSnapToScene (wxCommandEvent &event) {
	if (VideoContext::Get()->IsLoaded()) {
		// Get frames
		wxArrayInt sel = SubsBox->GetSelection();
		int curFrame = VideoContext::Get()->GetFrameN();
		int prev = 0;
		int next = 0;
		int frame = 0;
		wxArrayInt keyframes = VideoContext::Get()->GetKeyFrames();
		size_t n = keyframes.Count();
		bool found = false;
		for (size_t i=0;i<n;i++) {
			frame = keyframes[i];

			if (frame == curFrame) {
				prev = frame;
				if (i < n-1) next = keyframes[i+1];
				else next = VideoContext::Get()->GetLength();
				found = true;
				break;
			}

			if (frame > curFrame) {
				if (i != 0) prev = keyframes[i-1];
				else prev = 0;
				next = frame;
				found = true;
				break;
			}
		}

		// Last section?
		if (!found) {
			if (n > 0) prev = keyframes[n-1];
			else prev = 0;
			next = VideoContext::Get()->GetLength();
		}

		// Get times
		int start_ms = VFR_Output.GetTimeAtFrame(prev,true);
		int end_ms = VFR_Output.GetTimeAtFrame(next-1,false);
		AssDialogue *cur;

		// Update rows
		for (size_t i=0;i<sel.Count();i++) {
			cur = SubsBox->GetDialogue(sel[i]);
			cur->Start.SetMS(start_ms);
			cur->End.SetMS(end_ms);
			cur->UpdateData();
		}

		// Commit
		SubsBox->editBox->Update(true);
		SubsBox->ass->FlagAsModified(_("snap to scene"));
		SubsBox->CommitChanges();
	}
}



/// @brief Shift to frame 
/// @param event 
/// @return 
///
void FrameMain::OnShiftToFrame (wxCommandEvent &event) {
	if (VideoContext::Get()->IsLoaded()) {
		// Get selection
		wxArrayInt sels = SubsBox->GetSelection();
		size_t n=sels.Count();
		if (n == 0) return;

		// Get shifting in ms
		AssDialogue *cur = SubsBox->GetDialogue(sels[0]);
		if (!cur) return;
		int shiftBy = VFR_Output.GetTimeAtFrame(VideoContext::Get()->GetFrameN(),true) - cur->Start.GetMS();

		// Update
		for (size_t i=0;i<n;i++) {
			cur = SubsBox->GetDialogue(sels[i]);
			if (cur) {
				cur->Start.SetMS(cur->Start.GetMS()+shiftBy);
				cur->End.SetMS(cur->End.GetMS()+shiftBy);
				cur->UpdateData();
			}
		}

		// Commit
		SubsBox->ass->FlagAsModified(_("shift to frame"));
		SubsBox->CommitChanges();
		SubsBox->editBox->Update(true,false);
	}
}



/// @brief Undo 
/// @param event 
/// @return 
///
void FrameMain::OnUndo(wxCommandEvent& WXUNUSED(event)) {
	// Block if it's on a editbox
	//wxWindow *focused = wxWindow::FindFocus();
	//if (focused && focused->IsKindOf(CLASSINFO(wxTextCtrl))) return;

	VideoContext::Get()->Stop();
	AssFile::StackPop();
	SubsBox->LoadFromAss(AssFile::top,true);
	AssFile::Popping = false;
}



/// @brief Redo 
/// @param event 
///
void FrameMain::OnRedo(wxCommandEvent& WXUNUSED(event)) {
	VideoContext::Get()->Stop();
	AssFile::StackRedo();
	SubsBox->LoadFromAss(AssFile::top,true);
	AssFile::Popping = false;
}



/// @brief Find 
/// @param event 
///
void FrameMain::OnFind(wxCommandEvent &event) {
	VideoContext::Get()->Stop();
	Search.OpenDialog(false);
}



/// @brief Find next 
/// @param event 
///
void FrameMain::OnFindNext(wxCommandEvent &event) {
	VideoContext::Get()->Stop();
	Search.FindNext();
}



/// @brief Find & replace 
/// @param event 
///
void FrameMain::OnReplace(wxCommandEvent &event) {
	VideoContext::Get()->Stop();
	Search.OpenDialog(true);
}



/// @brief Change aspect ratio to default 
/// @param event 
///
void FrameMain::OnSetARDefault (wxCommandEvent &event) {
	VideoContext::Get()->Stop();
	VideoContext::Get()->SetAspectRatio(0);
	SetDisplayMode(1,-1);
}



/// @brief Change aspect ratio to fullscreen 
/// @param event 
///
void FrameMain::OnSetARFull (wxCommandEvent &event) {
	VideoContext::Get()->Stop();
	VideoContext::Get()->SetAspectRatio(1);
	SetDisplayMode(1,-1);
}



/// @brief Change aspect ratio to widescreen 
/// @param event 
///
void FrameMain::OnSetARWide (wxCommandEvent &event) {
	VideoContext::Get()->Stop();
	VideoContext::Get()->SetAspectRatio(2);
	SetDisplayMode(1,-1);
}



/// @brief Change aspect ratio to 2:35 
/// @param event 
///
void FrameMain::OnSetAR235 (wxCommandEvent &event) {
	VideoContext::Get()->Stop();
	VideoContext::Get()->SetAspectRatio(3);
	SetDisplayMode(1,-1);
}



/// @brief Change aspect ratio to a custom value 
/// @param event 
/// @return 
///
void FrameMain::OnSetARCustom (wxCommandEvent &event) {
	// Get text
	VideoContext::Get()->Stop();

	wxString value = wxGetTextFromUser(_("Enter aspect ratio in either:\n  decimal (e.g. 2.35)\n  fractional (e.g. 16:9)\n  specific resolution (e.g. 853x480)"),_("Enter aspect ratio"),AegiFloatToString(VideoContext::Get()->GetAspectRatioValue()));
	if (value.IsEmpty()) return;

	value.MakeLower();

	// Process text
	double numval;
	if (value.ToDouble(&numval)) {
		//Nothing to see here, move along
	}
	else {
		double a,b;
		int pos=0;
		bool scale=false;

		//Why bloat using Contains when we can just check the output of Find?
		pos = value.Find(':');
		if (pos==wxNOT_FOUND) pos = value.Find('/');
		if (pos==wxNOT_FOUND&&value.Contains(_T('x'))) {
			pos = value.Find('x');
			scale=true;
		}

		if (pos>0) {
			wxString num = value.Left(pos);
			wxString denum = value.Mid(pos+1);
			if (num.ToDouble(&a) && denum.ToDouble(&b) && b!=0) {
				numval = a/b;
				if (scale) videoBox->videoDisplay->SetZoom(b / VideoContext::Get()->GetHeight());
			}
		}
		else numval = 0.0;
	}

	// Sanity check
	if (numval < 0.5 || numval > 5.0) wxMessageBox(_("Invalid value! Aspect ratio must be between 0.5 and 5.0."),_("Invalid Aspect Ratio"),wxICON_ERROR|wxOK);

	// Set value
	else {
		VideoContext::Get()->SetAspectRatio(4,numval);
		SetDisplayMode(1,-1);
	}
}



/// @brief Window is attempted to be closed 
/// @param event 
///
void FrameMain::OnCloseWindow (wxCloseEvent &event) {
	// Stop audio and video
	VideoContext::Get()->Stop();
	audioBox->audioDisplay->Stop();

	// Ask user if he wants to save first
	bool canVeto = event.CanVeto();
	int result = TryToCloseSubs(canVeto);

	// Store maximization state
	OPT_SET("App/Maximized")->SetBool(IsMaximized());

	// Abort/destroy
	if (canVeto) {
		if (result == wxCANCEL) event.Veto();
		else Destroy();
	}
	else Destroy();
}



/// @brief Cut/copy/paste 
/// @param event 
/// @return 
///
void FrameMain::OnCut (wxCommandEvent &event) {
	if (FindFocus() == EditBox->TextEdit) {
		EditBox->TextEdit->Cut();
		return;
	}
	SubsBox->CutLines(SubsBox->GetSelection());
}


/// @brief DOCME
/// @param event 
/// @return 
///
void FrameMain::OnCopy (wxCommandEvent &event) {
	if (FindFocus() == EditBox->TextEdit) {
		EditBox->TextEdit->Copy();
		return;
	}
	SubsBox->CopyLines(SubsBox->GetSelection());
}


/// @brief DOCME
/// @param event 
/// @return 
///
void FrameMain::OnPaste (wxCommandEvent &event) {
	if (FindFocus() == EditBox->TextEdit) {
		EditBox->TextEdit->Paste();
		return;
	}
	SubsBox->PasteLines(SubsBox->GetFirstSelRow());
}



/// @brief Paste over 
/// @param event 
///
void FrameMain::OnPasteOver (wxCommandEvent &event) {
	SubsBox->PasteLines(SubsBox->GetFirstSelRow(),true);
}



/// @brief Select visible lines 
/// @param event 
///
void FrameMain::OnSelectVisible (wxCommandEvent &event) {
	VideoContext::Get()->Stop();
	SubsBox->SelectVisible();
}



/// @brief Open select dialog 
/// @param event 
///
void FrameMain::OnSelect (wxCommandEvent &event) {
	VideoContext::Get()->Stop();
	DialogSelection select(this, SubsBox);
	select.ShowModal();
}

/// @brief Sort subtitles by start time
void FrameMain::OnSortStart (wxCommandEvent &) {
	AssFile::top->Sort();
	AssFile::top->FlagAsModified(_("sort"));
	SubsBox->UpdateMaps();
	SubsBox->CommitChanges();
}
/// @brief Sort subtitles by end time
void FrameMain::OnSortEnd (wxCommandEvent &) {
	AssFile::top->Sort(AssFile::CompEnd);
	AssFile::top->FlagAsModified(_("sort"));
	SubsBox->UpdateMaps();
	SubsBox->CommitChanges();
}
/// @brief Sort subtitles by style name
void FrameMain::OnSortStyle (wxCommandEvent &) {
	AssFile::top->Sort(AssFile::CompStyle);
	AssFile::top->FlagAsModified(_("sort"));
	SubsBox->UpdateMaps();
	SubsBox->CommitChanges();
}

/// @brief Open styling assistant 
/// @param event 
///
void FrameMain::OnOpenStylingAssistant (wxCommandEvent &event) {
	VideoContext::Get()->Stop();
	if (!stylingAssistant) stylingAssistant = new DialogStyling(this,SubsBox);
	stylingAssistant->Show(true);
}



/// @brief Autosave the currently open file, if any
/// @param event Unused
///
void FrameMain::OnAutoSave(wxTimerEvent &event) {
	// Auto Save
	try {
		if (AssFile::top->loaded) {
			// Set path
			wxFileName origfile(AssFile::top->filename);
			wxString path = lagi_wxString(OPT_GET("Path/Auto/Save")->GetString());
			if (path.IsEmpty()) path = origfile.GetPath();
			wxFileName dstpath(path);
			if (!dstpath.IsAbsolute()) path = StandardPaths::DecodePathMaybeRelative(path, _T("?user/"));
			dstpath.AssignDir(path);
			if (!dstpath.DirExists()) wxMkdir(path);

			wxString name = origfile.GetName();
			if (name.IsEmpty()) {
				dstpath.SetFullName("Untitled.AUTOSAVE.ass");
			}
			else {
				dstpath.SetFullName(name + L".AUTOSAVE.ass");
			}

			wxFileName temp = dstpath;
			temp.SetName(dstpath.GetName() + ".temp");

			AssFile::top->Save(temp.GetFullPath(),false,false);
			wxRenameFile(temp.GetFullPath(), dstpath.GetFullPath());

			// Set status bar
			StatusTimeout(_("File backup saved as \"") + dstpath.GetFullPath() + _T("\"."));
		}
	}
	catch (wxString err) {
		StatusTimeout(_T("Exception when attempting to autosave file: ") + err);
	}
	catch (const wchar_t *err) {
		StatusTimeout(_T("Exception when attempting to autosave file: ") + wxString(err));
	}
	catch (...) {
		StatusTimeout(_T("Unhandled exception when attempting to autosave file."));
	}
}



/// @brief Clear statusbar 
/// @param event 
///
void FrameMain::OnStatusClear(wxTimerEvent &event) {
	SetStatusText(_T(""),1);
}

/// @brief Next frame hotkey 
/// @param event 
///
void FrameMain::OnNextFrame(wxCommandEvent &event) {
	videoBox->videoSlider->NextFrame();
}



/// @brief Previous frame hotkey 
/// @param event 
///
void FrameMain::OnPrevFrame(wxCommandEvent &event) {
	videoBox->videoSlider->PrevFrame();
}



/// @brief Toggle focus between seek bar and whatever else 
/// @param event 
///
void FrameMain::OnFocusSeek(wxCommandEvent &event) {
	wxWindow *curFocus = wxWindow::FindFocus();
	if (curFocus == videoBox->videoSlider) {
		if (PreviousFocus) PreviousFocus->SetFocus();
	}
	else {
		PreviousFocus = curFocus;
		videoBox->videoSlider->SetFocus();
	}
}



/// @brief Previous line hotkey 
/// @param event 
/// @return 
///
void FrameMain::OnPrevLine(wxCommandEvent &event) {
	int next = EditBox->linen-1;
	if (next < 0) return;
	SubsBox->SelectRow(next);
	SubsBox->MakeCellVisible(next,0);
	EditBox->SetToLine(next);
}



/// @brief Next line hotkey 
/// @param event 
/// @return 
///
void FrameMain::OnNextLine(wxCommandEvent &event) {
	int nrows = SubsBox->GetRows();
	int next = EditBox->linen+1;
	if (next >= nrows) return;
	SubsBox->SelectRow(next);
	SubsBox->MakeCellVisible(next,0);
	EditBox->SetToLine(next);
}



/// @brief Cycle through tag hiding modes 
void FrameMain::OnToggleTags(wxCommandEvent &) {
	int tagMode = OPT_GET("Subtitle/Grid/Hide Overrides")->GetInt();

	// Cycle to next
	tagMode = (tagMode+1)%3;

	// Show on status bar
	wxString message = _("ASS Override Tag mode set to ");
	if (tagMode == 0) message += _("show full tags.");
	if (tagMode == 1) message += _("simplify tags.");
	if (tagMode == 2) message += _("hide tags.");
	StatusTimeout(message,10000);

	// Set option
	OPT_SET("Subtitle/Grid/Hide Overrides")->SetInt(tagMode);

	// Refresh grid
	SubsBox->Refresh(false);
}
void FrameMain::OnSetTags(wxCommandEvent &event) {
	OPT_SET("Subtitle/Grid/Hide Overrides")->SetInt(event.GetId() - Menu_View_FullTags);
	SubsBox->Refresh(false);
}


/// @brief Commit Edit Box's changes 
/// @param event 
/// @return 
///
void FrameMain::OnEditBoxCommit(wxCommandEvent &event) {
	// Find focus
	wxWindow *focus = FindFocus();
	if (!focus) return;

	// Is the text edit
	if (focus == EditBox->TextEdit) {
		EditBox->Commit(true);
		EditBox->Update(true);
	}

	// Other window
	else {
		//wxKeyEvent keyevent;
		//keyevent.m_keyCode = WXK_RETURN;
		//keyevent.m_controlDown = true;
		//keyevent.SetEventType(wxEVT_KEY_DOWN);
		wxCommandEvent keyevent(wxEVT_COMMAND_TEXT_ENTER,focus->GetId());
		focus->GetEventHandler()->AddPendingEvent(keyevent);
	}
}



/// @brief Choose a different language 
/// @param event 
///
void FrameMain::OnChooseLanguage (wxCommandEvent &event) {
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
				if (Close()) {
					RestartAegisub();
					//wxStandardPaths stand;
					//wxExecute(_T("\"") + stand.GetExecutablePath() + _T("\""));
				}
			}
		}
	}
}



/// @brief View standard 
/// @param event 
/// @return 
///
void FrameMain::OnViewStandard (wxCommandEvent &event) {
	SetDisplayMode(1,1);
}



/// @brief View video 
/// @param event 
/// @return 
///
void FrameMain::OnViewVideo (wxCommandEvent &event) {
	SetDisplayMode(1,0);
}



/// @brief View audio 
/// @param event 
/// @return 
///
void FrameMain::OnViewAudio (wxCommandEvent &event) {
	SetDisplayMode(0,1);
}



/// @brief View subs 
/// @param event 
///
void FrameMain::OnViewSubs (wxCommandEvent &event) {
	SetDisplayMode(0,0);
}



/// @brief Medusa shortcuts 
/// @param event 
///
void FrameMain::OnMedusaPlay(wxCommandEvent &event) {
	int start=0,end=0;
	audioBox->audioDisplay->GetTimesSelection(start,end);
	audioBox->audioDisplay->Play(start,end);
}

/// @brief DOCME
/// @param event 
///
void FrameMain::OnMedusaStop(wxCommandEvent &event) {
	// Playing, stop
	if (audioBox->audioDisplay->player->IsPlaying()) {
		audioBox->audioDisplay->Stop();
		audioBox->audioDisplay->Refresh();
	}

	// Otherwise, play the last 500 ms
	else {
		int	start=0,end=0;
		audioBox->audioDisplay->GetTimesSelection(start,end);
		audioBox->audioDisplay->Play(end-500,end);
	}
}

/// @brief DOCME
/// @param event 
///
void FrameMain::OnMedusaShiftStartForward(wxCommandEvent &event) {
	audioBox->audioDisplay->curStartMS += 10;
	audioBox->audioDisplay->Update();
	audioBox->audioDisplay->wxWindow::Update();
	audioBox->audioDisplay->UpdateTimeEditCtrls();
}

/// @brief DOCME
/// @param event 
///
void FrameMain::OnMedusaShiftStartBack(wxCommandEvent &event) {
	audioBox->audioDisplay->curStartMS -= 10;
	audioBox->audioDisplay->Update();
	audioBox->audioDisplay->wxWindow::Update();
	audioBox->audioDisplay->UpdateTimeEditCtrls();
}

/// @brief DOCME
/// @param event 
///
void FrameMain::OnMedusaShiftEndForward(wxCommandEvent &event) {
	audioBox->audioDisplay->curEndMS += 10;
	audioBox->audioDisplay->Update();
	audioBox->audioDisplay->wxWindow::Update();
	audioBox->audioDisplay->UpdateTimeEditCtrls();
}

/// @brief DOCME
/// @param event 
///
void FrameMain::OnMedusaShiftEndBack(wxCommandEvent &event) {
	audioBox->audioDisplay->curEndMS -= 10;
	audioBox->audioDisplay->Update();
	audioBox->audioDisplay->wxWindow::Update();
	audioBox->audioDisplay->UpdateTimeEditCtrls();
}

/// @brief DOCME
/// @param event 
///
void FrameMain::OnMedusaPlayBefore(wxCommandEvent &event) {
	int start=0,end=0;
	audioBox->audioDisplay->GetTimesSelection(start,end);
	audioBox->audioDisplay->Play(start-500,start);
}

/// @brief DOCME
/// @param event 
///
void FrameMain::OnMedusaPlayAfter(wxCommandEvent &event) {
	int start=0,end=0;
	audioBox->audioDisplay->GetTimesSelection(start,end);
	audioBox->audioDisplay->Play(end,end+500);
}

/// @brief DOCME
/// @param event 
///
void FrameMain::OnMedusaNext(wxCommandEvent &event) {
	audioBox->audioDisplay->Next(false);
}

/// @brief DOCME
/// @param event 
///
void FrameMain::OnMedusaPrev(wxCommandEvent &event) {
	audioBox->audioDisplay->Prev(false);
}

/// @brief DOCME
/// @param event 
///
void FrameMain::OnMedusaEnter(wxCommandEvent &event) {
	audioBox->audioDisplay->CommitChanges(true);
}


