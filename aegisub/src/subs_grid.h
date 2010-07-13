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

/// @file subs_grid.h
/// @see subs_grid.cpp
/// @ingroup main_ui
///

#ifndef AGI_PRE
#include <fstream>
#include <iostream>
#include <list>
#include <vector>

#include <wx/grid.h>
#include <wx/wx.h>
#endif

#include "audio_box.h"
#include "audio_display.h"
#include "audio_provider_manager.h"
#include "base_grid.h"

class AssFile;
class AssEntry;
class AssDialogue;
class SubsEditBox;
class FrameMain;
class AudioDisplay;

/// DOCME
typedef std::list<AssEntry*>::iterator entryIter;

/// DOCME
/// @class SubtitlesGrid
/// @brief DOCME
///
/// DOCME
class SubtitlesGrid: public BaseGrid {
private:
	void OnPopupMenu(bool alternate=false);
	void OnKeyDown(wxKeyEvent &event);

	void OnSwap(wxCommandEvent &event);
	void OnDuplicate(wxCommandEvent &event);
	void OnDuplicateNextFrame(wxCommandEvent &event);
	void OnJoinConcat(wxCommandEvent &event);
	void OnJoinReplace(wxCommandEvent &event);
	void OnAdjoin(wxCommandEvent &event);
	void OnAdjoin2(wxCommandEvent &event);
	void OnInsertBefore(wxCommandEvent &event);
	void OnInsertAfter(wxCommandEvent &event);
	void OnInsertBeforeVideo(wxCommandEvent &event);
	void OnInsertAfterVideo(wxCommandEvent &event);
	void OnCopyLines(wxCommandEvent &event);
	void OnCutLines(wxCommandEvent &event);
	void OnPasteLines(wxCommandEvent &event);
	void OnDeleteLines(wxCommandEvent &event);
	void OnSetStartToVideo(wxCommandEvent &event);
	void OnSetEndToVideo(wxCommandEvent &event);
	void OnSetVideoToStart(wxCommandEvent &event);
	void OnSetVideoToEnd(wxCommandEvent &event);
	void OnJoinAsKaraoke(wxCommandEvent &event);
	void OnSplitByKaraoke(wxCommandEvent &event);
	void OnRecombine(wxCommandEvent &event);
	void OnAudioClip(wxCommandEvent &event);
	void OnShowColMenu(wxCommandEvent &event);

public:

	/// DOCME
	AssFile *ass;

	SubtitlesGrid(FrameMain* parentFrame,wxWindow *parent, wxWindowID id, AssFile *subs, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxWANTS_CHARS, const wxString& name = wxPanelNameStr);
	~SubtitlesGrid();

	void LoadDefault();
	void CommitChanges(bool force=false,bool videoOnly=false);

	void UpdateMaps(bool preserve_selected_rows = false);

	void SetVideoToSubs(bool start);
	void SetSubsToVideo(bool start);

	void JoinLines(int first,int last,bool concat=true);
	void JoinAsKaraoke(int first,int last);
	void AdjoinLines(int first,int last,bool setStart);
	void SplitLine(int lineNumber,int splitPosition,int mode,wxString splitText = _T(""));
	bool SplitLineByKaraoke(int lineNumber);
	void DuplicateLines(int first,int last,bool nextFrame=false);

	void SwapLines(int line1,int line2);
	void ShiftLineByTime(int lineNumber,int len,int type);
	void ShiftLineByFrames(int lineNumber,int len,int type);

	void InsertLine(AssDialogue *line,int position,bool insertAfter,bool update=true);
	void DeleteLines(wxArrayInt lines, bool flagModified=true);

	void CopyLines(wxArrayInt lines);
	void CutLines(wxArrayInt lines);
	void PasteLines(int pos,bool over=false);

	std::vector<int> GetAbsoluteSelection();
	void SetSelectionFromAbsolute(std::vector<int> &selection);

	DECLARE_EVENT_TABLE()
};


///////
// IDs
enum {

	/// DOCME
	MENU_GRID_START = 1200,

	/// DOCME
	MENU_INSERT_BEFORE,

	/// DOCME
	MENU_INSERT_AFTER,

	/// DOCME
	MENU_INSERT_BEFORE_VIDEO,

	/// DOCME
	MENU_INSERT_AFTER_VIDEO,

	/// DOCME
	MENU_SWAP,

	/// DOCME
	MENU_DUPLICATE,

	/// DOCME
	MENU_DUPLICATE_NEXT_FRAME,

	/// DOCME
	MENU_SPLIT_BY_KARAOKE,

	/// DOCME
	MENU_COPY,

	/// DOCME
	MENU_PASTE,

	/// DOCME
	MENU_CUT,

	/// DOCME
	MENU_DELETE,

	/// DOCME
	MENU_JOIN_CONCAT,

	/// DOCME
	MENU_JOIN_REPLACE,

	/// DOCME
	MENU_ADJOIN,

	/// DOCME
	MENU_ADJOIN2,

	/// DOCME
	MENU_JOIN_AS_KARAOKE,

	/// DOCME
	MENU_RECOMBINE,

	/// DOCME
	MENU_SET_START_TO_VIDEO,

	/// DOCME
	MENU_SET_END_TO_VIDEO,

	/// DOCME
	MENU_SET_VIDEO_TO_START,

	/// DOCME
	MENU_SET_VIDEO_TO_END,

	/// DOCME
	MENU_GRID_END,

	/// DOCME
	MENU_AUDIOCLIP,

	/// DOCME
	MENU_SHOW_COL = 1250 // Don't put anything after this
};


