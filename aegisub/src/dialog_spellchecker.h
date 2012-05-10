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

/// @file dialog_spellchecker.h
/// @see dialog_spellchecker.cpp
/// @ingroup spelling
///

#ifndef AGI_PRE
#include <map>
#include <set>

#include <wx/dialog.h>
#endif

#include <libaegisub/scoped_ptr.h>

namespace agi { struct Context; }
class AssDialogue;
class SpellChecker;
class wxButton;
class wxCheckBox;
class wxComboBox;
class wxListBox;
class wxTextCtrl;

/// DOCME
/// @class DialogSpellChecker
/// @brief DOCME
///
/// DOCME
class DialogSpellChecker : public wxDialog {
	agi::Context *context; ///< The project context
	agi::scoped_ptr<SpellChecker> spellchecker; ///< The spellchecking engine

	/// Words which the user has indicated should always be corrected
	std::map<wxString,wxString> auto_replace;

	/// Words which the user has temporarily added to the dictionary
	std::set<wxString> auto_ignore;

	/// Dictionaries available
	wxArrayString dictionary_lang_codes;

	int word_start; ///< Start index of the current misspelled word
	int word_end;   ///< End index of the current misspelled word

	wxTextCtrl *orig_word;    ///< The word being corrected
	wxTextCtrl *replace_word; ///< The replacement that will be used if "Replace" is clicked
	wxListBox *suggest_list;  ///< The list of suggested replacements

	wxComboBox *language;      ///< The list of available languages
	wxButton *add_button;      ///< Add word to currently active dictionary
	wxCheckBox *skip_comments; ///< Skip over commented lines

	AssDialogue *start_line;  ///< The first line checked
	AssDialogue *active_line; ///< The most recently checked line
	bool has_looped;          ///< Has the search already looped from the end to beginning?

	/// Find the next misspelled word and close the dialog if there are none
	/// @return Are there any more misspelled words?
	bool FindNext();

	/// Check a single line for misspellings
	/// @param active_line Line to check
	/// @param start_pos Index in the line to start at
	/// @param[in,out] commit_id Commit id for coalescing autoreplace commits
	/// @return Was a misspelling found?
	bool CheckLine(AssDialogue *active_line, int start_pos, int *commit_id);

	/// Set the current word to be corrected
	void SetWord(wxString const& word);
	/// Correct the currently selected word
	void Replace();

	void OnChangeLanguage(wxCommandEvent&);
	void OnChangeSuggestion(wxCommandEvent&);

	void OnReplace(wxCommandEvent&);
	void OnReplaceAll(wxCommandEvent&);
	void OnIgnore(wxCommandEvent&);
	void OnIgnoreAll(wxCommandEvent&);
	void OnAdd(wxCommandEvent&);

public:
	DialogSpellChecker(agi::Context *context);
	~DialogSpellChecker();
};
