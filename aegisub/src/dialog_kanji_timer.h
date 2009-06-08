// Copyright (c) 2006-2009, Dan Donovan (Dansolo), Niels Martin Hansen
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


#ifndef DIALOG_KANJITIMER_H
#define DIALOG_KANJITIMER_H


///////////
// Headers
#include <wx/wxprec.h>
#include <wx/listctrl.h>
#include <wx/regex.h>
#include <wx/checkbox.h>
#include <wx/dialog.h>
#include <wx/combobox.h>
#include <vector>
#include "options.h"
#include "kana_table.h"
#include "ass_file.h"
#include "ass_entry.h"


//////////////
// Prototypes
class SubtitlesGrid;
class AssOverrideParameter;
class KaraokeLineMatchDisplay;


/////////
// Class
class DialogKanjiTimer : public wxDialog {
	SubtitlesGrid *grid;
	AssFile *subs;

	KaraokeLineMatchDisplay *display;
	wxComboBox *SourceStyle, *DestStyle;
	wxCheckBox *Interpolate;

	std::vector<std::pair<entryIter,wxString> > LinesToChange;
	entryIter currentSourceLine;
	entryIter currentDestinationLine;

	void OnClose(wxCommandEvent &event);
	void OnStart(wxCommandEvent &event);
	void OnLink(wxCommandEvent &event);
	void OnUnlink(wxCommandEvent &event);
	void OnSkipSource(wxCommandEvent &event);
	void OnSkipDest(wxCommandEvent &event);
	void OnGoBack(wxCommandEvent &event);
	void OnAccept(wxCommandEvent &event);
	inline void OnKeyEnter(wxCommandEvent &event);

	void ResetForNewLine();
	void TryAutoMatch();

	entryIter FindNextStyleMatch(entryIter search_from, const wxString &search_style);
	entryIter FindPrevStyleMatch(entryIter search_from, const wxString &search_style);

public:
	DialogKanjiTimer(wxWindow *parent, SubtitlesGrid *grid);
	void OnKeyDown(wxKeyEvent &event);
	DECLARE_EVENT_TABLE()
};


///////
// IDs
enum {
	BUTTON_KTSTART = 2500,
	BUTTON_KTLINK,
	BUTTON_KTUNLINK,
	BUTTON_KTSKIPSOURCE,
	BUTTON_KTSKIPDEST,
	BUTTON_KTGOBACK,
	BUTTON_KTACCEPT,
	TEXT_SOURCE,
	TEXT_DEST
};


#endif
