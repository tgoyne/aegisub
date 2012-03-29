// Copyright (c) 2011, Thomas Goyne <plorkyeran@aegisub.org>
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
// Aegisub Project http://www.aegisub.org/
//
// $Id$

/// @file dialog_translation.h
/// @see dialog_translation.cpp
/// @ingroup tools_ui

#ifndef AGI_PRE
#include <wx/dialog.h>
#endif

#include <libaegisub/exception.h>
#include <libaegisub/scoped_ptr.h>
#include <libaegisub/signal.h>

#include "selection_controller.h"

namespace agi { struct Context; }
class AssDialogue;
class PersistLocation;
class ScintillaTextCtrl;
class SubsTextEditCtrl;
class wxStaticText;
class wxCheckBox;

/// @class DialogTranslation
/// @brief Assistant for translating subtitles in one language to another language
///
/// DOCME
class DialogTranslation : public wxDialog, private SelectionListener<AssDialogue> {
	agi::Context *c;

	agi::signal::Connection file_change_connection;

	/// The active line
	AssDialogue *active_line;
	/// Which dialogue block in the active line is currently being translated
	size_t cur_block;

	/// Total number of dialogue lines in the file
	size_t line_count;
	/// Line number of the currently active line
	size_t line_number;

	/// Should active line change announcements be ignored?
	bool switching_lines;

	wxStaticText *line_number_display;
	ScintillaTextCtrl *original_text;
	SubsTextEditCtrl *translated_text;
	wxCheckBox *seek_video;

	agi::scoped_ptr<PersistLocation> persist;

	void OnPlayAudioButton(wxCommandEvent &);
	void OnPlayVideoButton(wxCommandEvent &);
	void OnKeyDown(wxKeyEvent &evt);
	void OnExternalCommit(int commit_type);

	void UpdateDisplay();

	void OnSelectedSetChanged(Selection const&, Selection const&) { }
	void OnActiveLineChanged(AssDialogue *new_line);

public:
	DialogTranslation(agi::Context *context);
	~DialogTranslation();

	bool NextBlock();
	bool PrevBlock();
	void Commit(bool next);
	void InsertOriginal();

	DEFINE_SIMPLE_EXCEPTION_NOINNER(NothingToTranslate, agi::NonFatalException, "dialog/translation/nothing_to_translate");
};
