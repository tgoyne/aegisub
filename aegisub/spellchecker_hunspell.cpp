// Copyright (c) 2006, Rodrigo Braz Monteiro
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
#include "setup.h"
#if USE_HUNSPELL == 1
#include <hunspell/hunspell.hxx>
#include <wx/wxprec.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include "spellchecker_hunspell.h"
#include "main.h"
#include "utils.h"
#include "options.h"


///////////////
// Constructor
HunspellSpellChecker::HunspellSpellChecker() {
	hunspell = NULL;
	conv = NULL;
	SetLanguage(Options.AsText(_T("Spell checker Language")));
}


//////////////
// Destructor
HunspellSpellChecker::~HunspellSpellChecker() {
	Reset();
}


/////////
// Reset
void HunspellSpellChecker::Reset() {
	delete hunspell;
	hunspell = NULL;
	delete conv;
	conv = NULL;
	affpath.Clear();
	dicpath.Clear();
}


//////////////////////////
// Can add to dictionary?
bool HunspellSpellChecker::CanAddWord(wxString word) {
	if (!hunspell) return false;
	return (word.mb_str(*conv) != NULL);
}


//////////////////////////
// Add word to dictionary
void HunspellSpellChecker::AddWord(wxString word) {
	// Dictionary OK?
	if (!hunspell) return;

	// Add to currently loaded file
	hunspell->put_word(word.mb_str(*conv));

	// Load dictionary
	wxArrayString dic;
	wxString curLine;
	if (!dicpath.IsEmpty()) {			// Even if you ever want to remove this "if", keep the braces, so the stream closes at the end
		bool first = true;
		bool added = false;
		wxFileInputStream in(dicpath);
		if (!in.IsOk()) return;
		wxTextInputStream textIn(in,_T(" \t"),*conv);

		// Read it
		while (in.CanRead() && !in.Eof()) {
			// Read line
			curLine = textIn.ReadLine();
			curLine.Trim();

			// First
			if (first) {
				first = false;
				if (curLine.IsNumber()) continue;
			}

			// See if word to be added goes here
			if (!added && curLine.Lower() > word.Lower()) {
				dic.Add(word);
				added = true;
			}

			// Add to memory dictionary
			dic.Add(curLine);
		}
	}
	
	// Write back to disk
	wxFileOutputStream out(dicpath);
	if (!out.IsOk()) return;
	wxTextOutputStream textOut(out,wxEOL_UNIX,*conv);
	textOut.WriteString(wxString::Format(_T("%i"),dic.Count())+_T("\n"));
	for (unsigned int i=0;i<dic.Count();i++) textOut.WriteString(dic[i]+_T("\n"));
}


//////////////////////////////
// Check if the word is valid
bool HunspellSpellChecker::CheckWord(wxString word) {
	if (!hunspell) return true;
	wxCharBuffer buf = word.mb_str(*conv);
	if (buf) return (hunspell->spell(buf) == 1);
	return false;
}


////////////////////////////
// Get suggestions for word
wxArrayString HunspellSpellChecker::GetSuggestions(wxString word) {
	// Array
	wxArrayString suggestions;

	// Get suggestions
	if (hunspell) {
		// Word
		wxCharBuffer buf = word.mb_str(*conv);
		if (!buf) return suggestions;

		// Grab raw from Hunspell
		char **results;
		int n = hunspell->suggest(&results,buf);

		// Convert each
		for (int i=0;i<n;i++) {
			wxString current(results[i],*conv);
			suggestions.Add(current);
			delete results[i];
		}

		// Delete
		delete results;
	}

	// Return them
	return suggestions;
}


//////////////////////////////////////
// Get list of available dictionaries
wxArrayString HunspellSpellChecker::GetLanguageList() {
	// Get dir name
	wxString path = DecodeRelativePath(Options.AsText(_T("Dictionaries path")),AegisubApp::folderName) + _T("/");
	wxArrayString list;

	// Get file lists
	wxArrayString dic;
	wxDir::GetAllFiles(path,&dic,_T("*.dic"),wxDIR_FILES);
	wxArrayString aff;
	wxDir::GetAllFiles(path,&aff,_T("*.aff"),wxDIR_FILES);

	// For each dictionary match, see if it can find the corresponding .aff
	for (unsigned int i=0;i<dic.Count();i++) {
		wxString curAff = dic[i].Left(MAX(0,dic[i].Length()-4)) + _T(".aff");
		for (unsigned int j=0;j<aff.Count();j++) {
			// Found match
			if (curAff == aff[j]) {
				wxFileName fname(curAff);
				list.Add(fname.GetName());
				break;
			}
		}
	}

	// Return list
	return list;
}


////////////////
// Set language
void HunspellSpellChecker::SetLanguage(wxString language) {
	// Unload
	Reset();
	if (language.IsEmpty()) return;

	// Get dir name
	wxString path = DecodeRelativePath(Options.AsText(_T("Dictionaries path")),AegisubApp::folderName) + _T("/");

	// Get affix and dictionary paths
	affpath = path + language + _T(".aff");
	dicpath = path + language + _T(".dic");

	// Check if language is available
	if (!wxFileExists(affpath) || !wxFileExists(dicpath)) return;

	// Load
	hunspell = new Hunspell(affpath.mb_str(wxConvLocal),dicpath.mb_str(wxConvLocal));
	conv = NULL;
	if (hunspell) conv = new wxCSConv(wxString(hunspell->get_dic_encoding(),wxConvUTF8));
}

#endif
