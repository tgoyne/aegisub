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


////////////
// Includes

#include "config.h"

#include <wx/tokenzr.h>
#include "font_file_lister.h"
#include "text_file_writer.h"
#include "text_file_reader.h"
#include "standard_paths.h"
#if defined(__WINDOWS__) || defined(__APPLE__)
#ifdef WITH_FREETYPE2
#include "font_file_lister_freetype.h"
#define FontListerClass FreetypeFontFileLister
#endif
#else
#include "font_file_lister_fontconfig.h"
#define FontListerClass FontConfigFontFileLister
#endif


////////////////////
// Static instances
FontFileLister *FontFileLister::instance = NULL;


///////////////
// Constructor
FontFileLister::FontFileLister() {
}


//////////////
// Destructor
FontFileLister::~FontFileLister() {
}


////////////////
// Get instance
void FontFileLister::GetInstance() {
#ifdef FontListerClass
	if (!instance) instance = new FontListerClass();
#endif
}


////////////////////////////////////
// Redirect statics to the instance
wxArrayString FontFileLister::GetFilesWithFace(wxString facename) {
	GetInstance();
	return instance->DoGetFilesWithFace(facename);
}
void FontFileLister::Initialize() {
	GetInstance();
	instance->DoInitialize();
}
void FontFileLister::ClearData() {
	GetInstance();
	instance->DoClearData();
}


////////////////////////////////////////////////
// Get list of files that match a specific face
wxArrayString FontFileLister::CacheGetFilesWithFace(wxString facename) {
	FontMap::iterator iter = fontTable.find(facename);
	if (iter != fontTable.end()) return iter->second;
	else {
		iter = fontTable.find(_T("*")+facename);
		if (iter != fontTable.end()) return iter->second;
		return wxArrayString();
	}
}


//////////////
// Clear data
void FontFileLister::ClearCache() {
	fontFiles.clear();
	fontTable.clear();
}


////////////
// Add font
void FontFileLister::AddFont(wxString filename,wxString facename) {
	// See if it's a valid facename
	facename.Trim(true).Trim(false);
	if (facename.IsEmpty()) return;
	if (facename.Lower().StartsWith(_T("copyright "))) return;

	// Add filename to general list
	if (fontFiles.Index(filename) == wxNOT_FOUND) {
		fontFiles.Add(filename);
	}

	// Add filename to mapping of this face
	wxArrayString &arr = fontTable[facename];
	if (arr.Index(filename) == wxNOT_FOUND) arr.Add(filename);
}


/////////////////////////////////
// Check if a filename is cached
bool FontFileLister::IsFilenameCached(wxString filename) {
	return fontFiles.Index(filename) != wxNOT_FOUND;
}


//////////////
// Save cache
void FontFileLister::SaveCache() {
	try {
		// Open file
		TextFileWriter file(StandardPaths::DecodePath(_T("?user/fontscache.dat")));

		// For each face...
		for (FontMap::iterator iter = fontTable.begin();iter!=fontTable.end();iter++) {
			// Write face name
			wxString line = iter->first + _T("?");
			size_t len = iter->second.Count();

			// Write file names
			for (size_t i=0;i<len;i++) {
				line += iter->second[i];
				if (i != len-1) line += _T("|");
			}

			// Write line
			file.WriteLineToFile(line);
		}
	}
	catch (...) {
	}
}


//////////////
// Load cache
void FontFileLister::LoadCache() {
	try {
		// Load cache
		TextFileReader file(StandardPaths::DecodePath(_T("?user/fontscache.dat")));

		// Read each line
		while (file.HasMoreLines()) {
			// Read line
			wxString line = file.ReadLineFromFile();
			int pos = line.Find(_T('?'));

			// Get face name
			wxString face = line.Left(pos);
			if (face.IsEmpty()) continue;

			// Get files
			wxStringTokenizer tkn(line.Mid(pos+1),_T("|"));
			while (tkn.HasMoreTokens()) {
				wxString file = tkn.GetNextToken();
				if (!file.IsEmpty()) {
					AddFont(file,face);
				}
			}
		}
	}
	catch (...) {
	}
}
