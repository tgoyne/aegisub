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
// -----------------------------------------------------------------------------
//
// AEGISUB
//
// Website: http://aegisub.cellosoft.com
// Contact: mailto:zeratul@cellosoft.com
//


#pragma once


////////////
// Includes
#include <wx/wx.h>
#include <wx/grid.h>
#include <vector>
#include <list>
#include "base_grid.h"


//////////////
// Prototypes
class AssFile;
class AssEntry;
class AssDialogue;
class VideoDisplay;
class SubsEditBox;
class FrameMain;
class AudioDisplay;


////////////
// Typedefs
typedef std::list<AssEntry*>::iterator entryIter;


//////////////
// Main class
class SubtitlesGrid: public BaseGrid {
private:
	wxString tempfile;
	bool ready;

	void OnCellLeftClick(wxGridEvent &event);
	void OnCellChange(wxGridEvent &event);
	void OnSelectCell(wxGridEvent &event);
	void OnPopupMenu();
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
	void On1122Recombine(wxCommandEvent &event);
	void On122Recombine(wxCommandEvent &event);
	void On112Recombine(wxCommandEvent &event);

public:
	AssFile *ass;

	SubtitlesGrid(FrameMain* parentFrame,wxWindow *parent, wxWindowID id, VideoDisplay* video, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxWANTS_CHARS, const wxString& name = wxPanelNameStr);
	~SubtitlesGrid();

	void LoadDefault(AssFile *ass=NULL);
	void LoadFromAss(AssFile *ass=NULL,bool keepSelection=false,bool dontModify=false);
	void CommitChanges(bool force=false);

	void SetVideoToSubs(bool start);
	void SetSubsToVideo(bool start);

	void SwapLines(int n1,int n2);
	void DuplicateLines(int n1,int n2,bool nextFrame=false);
	void DeleteLines(int n1,int n2,bool sel);
	void JoinLines(int n1,int n2,bool concat=true);
	void JoinAsKaraoke(int n1,int n2);
	void AdjoinLines(int n1,int n2,bool setStart);
	void InsertLine(AssDialogue *line,int n,bool after,bool update=true);
	void ShiftLineByTime(int n,int len,int type);
	void ShiftLineByFrames(int n,int len,int type);
	void SplitLine(int n,int pos,int mode);

	void CopyLines();
	void CutLines();
	void PasteLines(int n);

	wxString GetTempWorkFile ();

	DECLARE_EVENT_TABLE()
};


///////
// IDs
enum {
	MENU_INSERT_BEFORE = 1200,
	MENU_INSERT_AFTER,
	MENU_INSERT_BEFORE_VIDEO,
	MENU_INSERT_AFTER_VIDEO,
	MENU_SWAP,
	MENU_DUPLICATE,
	MENU_DUPLICATE_NEXT_FRAME,
	MENU_COPY,
	MENU_PASTE,
	MENU_CUT,
	MENU_DELETE,
	MENU_JOIN_CONCAT,
	MENU_JOIN_REPLACE,
	MENU_ADJOIN,
	MENU_ADJOIN2,
	MENU_JOIN_AS_KARAOKE,
	MENU_1_12_2_RECOMBINE,
	MENU_1_12_RECOMBINE,
	MENU_12_2_RECOMBINE,
	MENU_SET_START_TO_VIDEO,
	MENU_SET_END_TO_VIDEO,
	MENU_SET_VIDEO_TO_START,
	MENU_SET_VIDEO_TO_END
};
