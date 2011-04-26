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
// Aegisub Project http://www.aegisub.org/
//
// $Id$

/// @file thesaurus_myspell.cpp
/// @brief MySpell-based thesaurus implementation
/// @ingroup thesaurus
///

#include "config.h"

#ifndef AGI_PRE
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/log.h>
#endif

#include <libaegisub/log.h>

#include "compat.h"
#include "mythes.hxx"
#include "main.h"
#include "standard_paths.h"
#include "thesaurus_myspell.h"
#include "utils.h"

MySpellThesaurus::MySpellThesaurus()
: mythes(0)
, conv(0)
{
	SetLanguage(lagi_wxString(OPT_GET("Tool/Thesaurus/Language")->GetString()));
}

MySpellThesaurus::~MySpellThesaurus() {
	delete mythes;
	delete conv;
}

void MySpellThesaurus::Lookup(wxString const& word, ThesaurusEntryArray &result) {
	if (!mythes) return;

	mentry *me;
	wxCharBuffer buf = word.Lower().mb_str(*conv);
	if (!buf) return;
	int n = mythes->Lookup(buf, strlen(buf), &me);

	for (int i = 0; i < n; i++) {
		ThesaurusEntry entry;
		entry.name = wxString(me[i].defn, *conv);
		for (int j = 0; j < me[i].count; j++)
			entry.words.Add(wxString(me[i].psyns[j], *conv));
		result.push_back(entry);
	}

	mythes->CleanUpAfterLookup(&me, n);
}

wxArrayString MySpellThesaurus::GetLanguageList() {
	if (!languages.empty()) return languages;

	wxArrayString idx, dat;

	// Get list of dictionaries
	wxString path = StandardPaths::DecodePath("?data/dictionaries/");
	if (wxFileName::DirExists(path)) {
		wxDir::GetAllFiles(path, &idx, "th_*.idx", wxDIR_FILES);
		wxDir::GetAllFiles(path, &dat, "th_*.dat", wxDIR_FILES);
	}
	path = StandardPaths::DecodePath(lagi_wxString(OPT_GET("Path/Dictionary")->GetString()) + "/");
	if (wxFileName::DirExists(path)) {
		wxDir::GetAllFiles(path, &idx, "th_*.idx", wxDIR_FILES);
		wxDir::GetAllFiles(path, &dat, "th_*.dat", wxDIR_FILES);
	}
	if (idx.empty() || dat.empty()) return languages;

	idx.Sort();
	dat.Sort();

	// Drop extensions and the th_ prefix
	for (size_t i = 0; i < idx.size(); ++i) idx[i] = idx[i].Mid(3, idx[i].size() - 7);
	for (size_t i = 0; i < dat.size(); ++i) dat[i] = dat[i].Mid(3, dat[i].size() - 7);

	// Verify that each idx has a dat
	for (size_t i = 0, j = 0; i < idx.size() && j < dat.size(); ) {
		int cmp = idx[i].Cmp(dat[j]);
		if (cmp < 0) ++i;
		else if (cmp > 0) ++j;
		else {
			// Don't insert a language twice if it's in both the user dir and
			// the app's dir
			wxString name = wxFileName(dat[j]).GetName().Mid(3);
			if (languages.empty() || name != languages.back())
				languages.push_back(name);
			++i;
			++j;
		}
	}
	return languages;
}

void MySpellThesaurus::SetLanguage(wxString const& language) {
	delete mythes;
	mythes = 0;
	delete conv;
	conv = 0;

	if (language.empty()) return;

	wxString path = StandardPaths::DecodePath(lagi_wxString(OPT_GET("Path/Dictionary")->GetString()) + "/");

	// Get affix and dictionary paths
	wxString idxpath = wxString::Format("%s/th_%s.idx", path, language);
	wxString datpath = wxString::Format("%s/th_%s.dat", path, language);

	// If they aren't in the user dictionary path, check the application directory
	if (!wxFileExists(idxpath) || !wxFileExists(datpath)) {
		path = StandardPaths::DecodePath("?data/dictionaries/");
		idxpath = wxString::Format("%s/th_%s.idx", path, language);
		datpath = wxString::Format("%s/th_%s.dat", path, language);

		if (!wxFileExists(idxpath) || !wxFileExists(datpath)) return;
	}

	LOG_I("thesaurus/file") << "Using thesaurus: " << datpath.c_str();

	// Load
	mythes = new MyThes(idxpath.mb_str(wxConvLocal),datpath.mb_str(wxConvLocal));
	if (mythes) conv = new wxCSConv(wxString(mythes->get_th_encoding(),wxConvUTF8));
}
