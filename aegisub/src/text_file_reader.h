// Copyright (c) 2010, Thomas Goyne
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

/// @file text_file_reader.h
/// @see text_file_reader.cpp
/// @ingroup utility
///

#pragma once

#ifndef AGI_PRE
#include <fstream>
#include <memory>

#include <wx/dynarray.h>
#include <wx/string.h>
#endif

#include <libaegisub/line_iterator.h>

/// @class TextFileReader
/// @brief A line-based text file reader
class TextFileReader {
	std::auto_ptr<std::ifstream> file;
	bool trim;
	bool isBinary;
	agi::line_iterator<wxString> iter;

	TextFileReader(const TextFileReader&);
	TextFileReader& operator=(const TextFileReader&);

public:
	/// @brief Constructor
	/// @param filename File to open
	/// @param enc      Encoding to use, or empty to autodetect
	/// @param trim     Whether to trim whitespace from lines read
	TextFileReader(wxString const& filename,wxString encoding=L"", bool trim=true);
	/// @brief Destructor
	~TextFileReader();

	/// @brief Read a line from the file
	/// @return The line, possibly trimmed
	wxString ReadLineFromFile();
	/// @brief Check if there are any more lines to read
	bool HasMoreLines() const { return iter != agi::line_iterator<wxString>(); }
	bool IsBinary() const { return isBinary; }
};
