// Copyright (c) 2010, Thomas Goyne <plorkyeran@aegisub.org>
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

/// @file font_file_lister.h
/// @see font_file_lister.cpp
/// @ingroup font_collector
///

#ifndef AGI_PRE
#include <list>
#include <tr1/functional>

#include <wx/string.h>
#endif

class AssEntry;
class AssDialogue;

/// @class FontFileLister
/// @brief Font lister base class
class FontFileLister {
	struct StyleInfo {
		wxString facename;
		int bold;
		bool italic;
		bool operator<(StyleInfo const& rgt) const;
	};

	/// A (non-unique) list of each combination of styles used in the file
	std::list<StyleInfo> dialogueChunks;
	/// Style name -> ASS style definition
	std::map<wxString, StyleInfo> styles;
	/// Style -> Path to font, or empty string if the font could not be found
	std::map<StyleInfo, wxString> results;

	void ProcessDialogueLine(AssDialogue *line);
	void ProcessChunk(StyleInfo const& style);
	std::vector<wxString> Run(std::list<AssEntry*> const& file);

	/// @brief Get the path to the font with the given styles
	/// @param facename Name of font face
	/// @param bold ASS font weight
	/// @param italic Italic?
	/// @return Path to the matching font file
	///
	/// This is only non-abstract due to a bug in the version of boost::bind
	/// used for std::tr1::bind in MSVC, and should always be overridden
	virtual wxString GetFontPath(wxString const& facename, int bold, bool italic) { return ""; };

protected:
	/// Message callback provider by caller
	std::tr1::function<void (wxString, int)> statusCallback;

	FontFileLister(std::tr1::function<void (wxString, int)> statusCallback) : statusCallback(statusCallback) { }

public:
	/// @brief Get a list of the locations of all font files used in the file
	/// @param file Lines in the subtitle file to check
	/// @param status Callback function for messages
	/// @return List of paths to fonts
	static std::vector<wxString> GetFontPaths(std::list<AssEntry*> const& file, std::tr1::function<void (wxString, int)> status);
};
