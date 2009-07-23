// Copyright (c) 2007, Rodrigo Braz Monteiro
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

///////////
// Headers
#include "config.h"

#include <wx/intl.h>
#include "dialog_spellchecker.h"
#include "spellchecker_manager.h"
#include "subs_grid.h"
#include "frame_main.h"
#include "ass_file.h"
#include "ass_dialogue.h"
#include "utils.h"
#include "subs_edit_box.h"
#include "options.h"
#include "help_button.h"


///////
// IDs
enum {
	BUTTON_REPLACE = 1720,
	BUTTON_IGNORE,
	BUTTON_REPLACE_ALL,
	BUTTON_IGNORE_ALL,
	BUTTON_ADD,
	LIST_SUGGESTIONS,
	LIST_LANGUAGES
};


///////////////
// Constructor
DialogSpellChecker::DialogSpellChecker(wxFrame *parent)
: wxDialog(parent, -1, _("Spell Checker"), wxDefaultPosition, wxDefaultSize)
{
	// Set icon
	SetIcon(BitmapToIcon(wxBitmap(spellcheck_toolbutton_xpm)));

	// Get spell checker
	spellchecker = SpellCheckerFactoryManager::GetSpellChecker();
	if (!spellchecker) {
		wxMessageBox(_T("No spellchecker available."),_T("Error"),wxICON_ERROR);
		Destroy();
		return;
	}

	// Get languages
	langCodes = spellchecker->GetLanguageList();
	wxArrayString langNames;
	const wxLanguageInfo *info;
	for (size_t i=0;i<langCodes.Count();i++) {
		wxString name;
		info = wxLocale::FindLanguageInfo(langCodes[i]);
		if (info) name = info->Description;
		else name = langCodes[i];
		langNames.Add(name);
	}

	// Get current language
	wxString curLang = Options.AsText(_T("Spell checker language"));
	int curLangPos = langCodes.Index(curLang);
	if (curLangPos == wxNOT_FOUND) {
		curLangPos = langCodes.Index(_T("en"));
		if (curLangPos == wxNOT_FOUND) {
			curLangPos = langCodes.Index(_T("en_US"));
			if (curLangPos == wxNOT_FOUND) {
				curLangPos = 0;
			}
		}
	}

	// Top sizer
	origWord = new wxTextCtrl(this,-1,_("original"),wxDefaultPosition,wxDefaultSize,wxTE_READONLY);
	replaceWord = new wxTextCtrl(this,-1,_("replace with"));
	wxFlexGridSizer *topSizer = new wxFlexGridSizer(2,2,5,5);
	topSizer->Add(new wxStaticText(this,-1,_("Misspelled word:")),0,wxALIGN_CENTER_VERTICAL);
	topSizer->Add(origWord,1,wxEXPAND);
	topSizer->Add(new wxStaticText(this,-1,_("Replace with:")),0,wxALIGN_CENTER_VERTICAL);
	topSizer->Add(replaceWord,1,wxEXPAND);
	topSizer->AddGrowableCol(1,1);

	// List
	suggestList = new wxListBox(this,LIST_SUGGESTIONS,wxDefaultPosition,wxSize(300,150));

	// Actions sizer
	wxSizer *actionsSizer = new wxBoxSizer(wxVERTICAL);
	actionsSizer->Add(new wxButton(this,BUTTON_REPLACE,_("Replace")),0,wxEXPAND | wxBOTTOM,5);
	actionsSizer->Add(new wxButton(this,BUTTON_REPLACE_ALL,_("Replace All")),0,wxEXPAND | wxBOTTOM,5);
	actionsSizer->Add(new wxButton(this,BUTTON_IGNORE,_("Ignore")),0,wxEXPAND | wxBOTTOM,5);
	actionsSizer->Add(new wxButton(this,BUTTON_IGNORE_ALL,_("Ignore all")),0,wxEXPAND | wxBOTTOM,5);
	actionsSizer->Add(new wxButton(this,BUTTON_ADD,_("Add to dictionary")),0,wxEXPAND | wxBOTTOM,5);
	actionsSizer->Add(new HelpButton(this,_T("Spell Checker")),0,wxEXPAND | wxBOTTOM,0);
	actionsSizer->AddStretchSpacer(1);

	// Bottom sizer
	language = new wxComboBox(this,LIST_LANGUAGES,_T(""),wxDefaultPosition,wxDefaultSize,langNames,wxCB_DROPDOWN | wxCB_READONLY);
	language->SetSelection(curLangPos);
	wxFlexGridSizer *botSizer = new wxFlexGridSizer(2,2,5,5);
	botSizer->Add(suggestList,1,wxEXPAND);
	botSizer->Add(actionsSizer,1,wxEXPAND);
	botSizer->Add(language,0,wxEXPAND);
	botSizer->Add(new wxButton(this,wxID_CANCEL),0,wxEXPAND);
	botSizer->AddGrowableCol(0,1);
	botSizer->AddGrowableRow(0,1);
	//SetEscapeId(wxID_CLOSE);

	// Main sizer
	wxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(topSizer,0,wxEXPAND | wxALL,5);
	mainSizer->Add(botSizer,1,wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM,5);
	SetSizer(mainSizer);
	mainSizer->SetSizeHints(this);
	CenterOnParent();

	// Go to first match and show
	if (GetFirstMatch()) ShowModal();
}


//////////////
// Destructor
DialogSpellChecker::~DialogSpellChecker() {
	if (spellchecker) delete spellchecker;
}


///////////////////
// Find next match
bool DialogSpellChecker::FindNext(int startLine,int startPos) {
	// Set start
	if (startLine != -1) lastLine = startLine;
	if (startPos != -1) lastPos = 0;

	// Get grid
	SubtitlesGrid *grid = ((FrameMain*)GetParent())->SubsBox;
	int rows = grid->GetRows();

	// Loop through lines
	for (int i=lastLine;i<rows+firstLine;i++) {
		startFindNextOuterLoop:
		// Get dialogue
		int curLine = i % rows;
		AssDialogue *diag = grid->GetDialogue(curLine);

		// Find list of words in it
		IntPairVector results;
		GetWordBoundaries(diag->Text,results);

		// Look for spelling mistakes
		for (size_t j=0;j<results.size();j++) {
			// Get word
			int s = results[j].first;
			if (s < lastPos) continue;
			int e = results[j].second;
			wxString word = diag->Text.Mid(s,e-s);

			// Check if it's on auto ignore
			if (autoIgnore.Index(word) != wxNOT_FOUND) continue;

			// Mistake
			if (!spellchecker->CheckWord(word)) {
				// Set word
				wordStart = s;
				wordEnd = e;
				lastLine = i;
				lastPos = e;

				// Auto replace?
				if (autoReplace.find(word) != autoReplace.end()) {
					// lol mad h4x
					replaceWord->SetValue(autoReplace[word]);
					Replace();
					goto startFindNextOuterLoop;
				}

				// Proceed normally
				SetWord(word);
				return true;
			}
		}

		// Go to next
		lastPos = 0;
	}

	// None found
	return false;
}


////////////
// Set word
void DialogSpellChecker::SetWord(wxString word) {
	// Get list of suggestions
	wxArrayString sugs = spellchecker->GetSuggestions(word);

	// Set fields
	origWord->SetValue(word);
	replaceWord->SetValue((sugs.Count()>0)? sugs[0] : word);

	// Set suggestions list
	suggestList->Clear();
	for (size_t i=0;i<sugs.Count();i++) suggestList->Append(sugs[i]);

	// Show word on the main program interface
	SubtitlesGrid *grid = ((FrameMain*)GetParent())->SubsBox;
	int line = lastLine % grid->GetRows();
	grid->SelectRow(line,false);
	grid->MakeCellVisible(line,0);
	grid->editBox->SetToLine(line);
	grid->editBox->TextEdit->SetSelectionU(wordStart,wordEnd);
	grid->EndBatch();
}


///////////////
// Event table
BEGIN_EVENT_TABLE(DialogSpellChecker,wxDialog)
	EVT_BUTTON(wxID_CANCEL,DialogSpellChecker::OnClose)
	EVT_BUTTON(BUTTON_REPLACE,DialogSpellChecker::OnReplace)
	EVT_BUTTON(BUTTON_REPLACE_ALL,DialogSpellChecker::OnReplaceAll)
	EVT_BUTTON(BUTTON_IGNORE,DialogSpellChecker::OnIgnore)
	EVT_BUTTON(BUTTON_IGNORE_ALL,DialogSpellChecker::OnIgnoreAll)
	EVT_BUTTON(BUTTON_ADD,DialogSpellChecker::OnAdd)

	EVT_COMBOBOX(LIST_LANGUAGES,DialogSpellChecker::OnChangeLanguage)
	EVT_LISTBOX(LIST_SUGGESTIONS,DialogSpellChecker::OnChangeSuggestion)
	EVT_LISTBOX_DCLICK(LIST_SUGGESTIONS,DialogSpellChecker::OnTakeSuggestion)
END_EVENT_TABLE()


/////////
// Close
void DialogSpellChecker::OnClose(wxCommandEvent &event) {
	Destroy();
}


///////////
// Replace
void DialogSpellChecker::OnReplace(wxCommandEvent &event) {
	Replace();
	FindOrDie();
}


//////////////////////
// Replace all errors
void DialogSpellChecker::OnReplaceAll(wxCommandEvent &event) {
	// Add word to autoreplace list
	autoReplace[origWord->GetValue()] = replaceWord->GetValue();

	// Replace
	Replace();
	FindOrDie();
}


/////////////////////
// Ignore this error
void DialogSpellChecker::OnIgnore(wxCommandEvent &event) {
	// Next
	FindOrDie();
}


/////////////////////
// Ignore all errors
void DialogSpellChecker::OnIgnoreAll(wxCommandEvent &event) {
	// Add word to autoignore list
	autoIgnore.Add(origWord->GetValue());

	// Next
	FindOrDie();
}


/////////////////////
// Add to dictionary
void DialogSpellChecker::OnAdd(wxCommandEvent &event) {
	spellchecker->AddWord(origWord->GetValue());
	FindOrDie();
}


///////////////////////////////////////////////
// Goes to next... if it can't find one, close
bool DialogSpellChecker::FindOrDie() {
	if (!FindNext()) {
		wxMessageBox(_("Aegisub has finished checking spelling of this script."),_("Spell checking complete."));
		Destroy();
		return false;
	}
	return true;
}


///////////
// Replace
void DialogSpellChecker::Replace() {
	// Get dialog
	SubtitlesGrid *grid = ((FrameMain*)GetParent())->SubsBox;
	AssDialogue *diag = grid->GetDialogue(lastLine % grid->GetRows());

	// Replace
	diag->Text = diag->Text.Left(wordStart) + replaceWord->GetValue() + diag->Text.Mid(wordEnd);
	lastPos = wordStart + replaceWord->GetValue().Length();

	// Commit
	grid->ass->FlagAsModified(_("Spell check replace"));
	grid->CommitChanges();
}


///////////////////
// Change language
void DialogSpellChecker::OnChangeLanguage(wxCommandEvent &event) {
	// Change language code
	wxString code = langCodes[language->GetSelection()];
	spellchecker->SetLanguage(code);
	Options.SetText(_T("Spell checker language"),code);
	Options.Save();

	// Go back to first match
	GetFirstMatch();
}


/////////////////////
// Change suggestion
void DialogSpellChecker::OnChangeSuggestion(wxCommandEvent &event) {
	replaceWord->SetValue(suggestList->GetStringSelection());
}


/////////////////////////////////
// Suggestion box double clicked
void DialogSpellChecker::OnTakeSuggestion(wxCommandEvent &event) {
	// First line should be unnecessary due to event above, but you never know...
	replaceWord->SetValue(suggestList->GetStringSelection());
	Replace();
	FindOrDie();
}


///////////////
// First match
bool DialogSpellChecker::GetFirstMatch() {
	// Get selection
	SubtitlesGrid *grid = ((FrameMain*)GetParent())->SubsBox;
	wxArrayInt sel = grid->GetSelection();
	firstLine = (sel.Count()>0) ? sel[0] : 0;
	bool hasTypos = FindNext(firstLine,0);

	// File is already OK
	if (!hasTypos) {
		wxMessageBox(_("Aegisub has found no spelling mistakes in this script."),_("Spell checking complete."));
		Destroy();
		return false;
	}

	// OK
	return true;
}
