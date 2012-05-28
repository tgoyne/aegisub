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

/// @file subs_edit_ctrl.h
/// @see subs_edit_ctrl.cpp
/// @ingroup main_ui
///

#ifndef AGI_PRE
#include <string>
#include <vector>
#endif

#include "scintilla_text_ctrl.h"

#include <libaegisub/scoped_ptr.h>

class SpellChecker;
class SubsEditBox;
class Thesaurus;
namespace agi { struct Context; }

/// @class SubsTextEditCtrl
/// @brief A Scintilla control with spell checking and syntax highlighting
class SubsTextEditCtrl : public ScintillaTextCtrl {
	/// Backend spellchecker to use
	agi::scoped_ptr<SpellChecker> spellchecker;

	/// Backend thesaurus to use
	agi::scoped_ptr<Thesaurus> thesaurus;

	/// Project context, for splitting lines
	agi::Context *context;

	/// The word right-clicked on, used for spellchecker replacing
	wxString currentWord;

	/// The beginning of the word right-clicked on, for spellchecker replacing
	int currentWordPos;

	/// Spellchecker suggestions for the last right-clicked word
	wxArrayString sugs;

	/// Thesaurus suggestions for the last right-clicked word
	std::vector<std::string> thesSugs;

	/// Calltip prototypes
	wxArrayString proto;

	/// Last show calltip, to avoid flickering the currently active one
	int tipProtoN;

	void OnContextMenu(wxContextMenuEvent &);
	void OnAddToDictionary(wxCommandEvent &event);
	void OnUseSuggestion(wxCommandEvent &event);
	void OnSetDicLanguage(wxCommandEvent &event);
	void OnSetThesLanguage(wxCommandEvent &event);
	void OnLoseFocus(wxFocusEvent &event);
	void OnKeyDown(wxKeyEvent &event);

	void SetSyntaxStyle(int id, wxFont &font, std::string const& name);
	void Subscribe(std::string const& name);

	void StyleSpellCheck();
	void UpdateCallTip(wxStyledTextEvent &);
	void SetStyles();

	void UpdateStyle();

	/// Split the line at the current cursor position
	/// @param estimateTimes Adjust the times based on the lengths of the halves
	void SplitLine(bool estimateTimes);

	/// Add the thesaurus suggestions to a menu
	void AddThesaurusEntries(wxMenu &menu);

	/// Add the spell checker suggestions to a menu
	void AddSpellCheckerEntries(wxMenu &menu);

	/// Generate a languages submenu from a list of locales and a current language
	/// @param base_id ID to use for the first menu item
	/// @param curLang Currently selected language
	/// @param lang Full list of languages
	wxMenu *GetLanguagesMenu(int base_id, wxString const& curLang, wxArrayString const& langs);

public:
	SubsTextEditCtrl(wxWindow* parent, wxSize size, long style, agi::Context *context);
	~SubsTextEditCtrl();

	void SetTextTo(wxString text);
	void Paste();

	DECLARE_EVENT_TABLE()
};
