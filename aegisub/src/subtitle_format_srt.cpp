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
#include "config.h"

#include "subtitle_format_srt.h"
#include "text_file_reader.h"
#include "text_file_writer.h"
#include "ass_dialogue.h"
#include "ass_file.h"


/////////////
// Can read?
bool SRTSubtitleFormat::CanReadFile(wxString filename) {
	return (filename.Right(4).Lower() == _T(".srt"));
}


//////////////
// Can write?
bool SRTSubtitleFormat::CanWriteFile(wxString filename) {
	return (filename.Right(4).Lower() == _T(".srt"));
}


////////////
// Get name
wxString SRTSubtitleFormat::GetName() {
	return _T("SubRip");
}


//////////////////////
// Get read wildcards
wxArrayString SRTSubtitleFormat::GetReadWildcards() {
	wxArrayString formats;
	formats.Add(_T("srt"));
	return formats;
}


///////////////////////
// Get write wildcards
wxArrayString SRTSubtitleFormat::GetWriteWildcards() {
	return GetReadWildcards();
}


/////////////
// Read file
void SRTSubtitleFormat::ReadFile(wxString filename,wxString encoding) {
	using namespace std;

	// Reader
	TextFileReader file(filename,encoding);

	// Default
	LoadDefault(false);

	// Parse file
	int linen = 1;
	int fileLine = 0;
	int mode = 0;
	int lines = 0;
	long templ;
	AssDialogue *line = NULL;
	while (file.HasMoreLines()) {
		// Reads line
		wxString curLine = file.ReadLineFromFile();
		fileLine++;

		if (mode == 0) {
			// Checks if there is anything to read
			if (curLine.IsEmpty()) continue;

			// Check if it's a line number
			if (!curLine.IsNumber()) {
				Clear();
				throw wxString::Format(_T("Parse error on entry %i at line %i (expecting line number). Possible malformed file."),linen,fileLine);
			}

			// Read line number
			curLine.ToLong(&templ);
			if (templ != linen) {
				linen = templ;
			}
			line = new AssDialogue();
			mode = 1;
		}

		else if (mode == 1) {
			// Read timestamps
			if (curLine.substr(13,3) != _T("-->")) {
				Clear();
				throw wxString::Format(_T("Parse error on entry %i at line %i (expecting timestamps). Possible malformed file."),linen,fileLine);
			}
			line->Start.ParseSRT(curLine.substr(0,12));
			line->End.ParseSRT(curLine.substr(17,12));
			mode = 2;
		}

		else if (mode == 2) {
			// Checks if it's done
			bool eof = !file.HasMoreLines();
			bool isDone = curLine.IsEmpty();

			// Append text
			if (!isDone) {
				if (line->Text != _T("")) line->Text += _T("\\N");
				line->Text += curLine;
			}

			// Done
			if (isDone || eof) {
				mode = 0;
				linen++;
				line->group = _T("[Events]");
				line->Style = _T("Default");
				line->Comment = false;
				line->UpdateData();
				line->ParseSRTTags();
				line->FixStartMS();
				Line->push_back(line);
				lines++;
			}
		}
	}

	// No lines?
	if (lines == 0) {
		AssDialogue *line = new AssDialogue();
		line->group = _T("[Events]");
		line->Style = _T("Default");
		line->SetStartMS(0);
		line->SetEndMS(5000);
		Line->push_back(line);
	}
}


//////////////
// Write file
void SRTSubtitleFormat::WriteFile(wxString _filename,wxString encoding) {
	// Open file
	TextFileWriter file(_filename,encoding);

	// Convert to SRT
	CreateCopy();
	SortLines();
	StripComments();
	// Tags must be converted in two passes
	// First ASS style overrides are converted to SRT but linebreaks are kept
	ConvertTags(2,_T("\\N"));
	// Then we can recombine overlaps, this requires ASS style linebreaks
	RecombineOverlaps();
	MergeIdentical();
	// And finally convert linebreaks
	ConvertTags(0,_T("\r\n"));
	// Otherwise unclosed overrides might affect lines they shouldn't, see bug #809 for example

	// Write lines
	int i=1;
	using std::list;
	for (list<AssEntry*>::iterator cur=Line->begin();cur!=Line->end();cur++) {
		AssDialogue *current = AssEntry::GetAsDialogue(*cur);
		if (current && !current->Comment) {
			// Write line
			file.WriteLineToFile(wxString::Format(_T("%i"),i));
			file.WriteLineToFile(current->Start.GetSRTFormated() + _T(" --> ") + current->End.GetSRTFormated());
			file.WriteLineToFile(current->Text);
			file.WriteLineToFile(_T(""));

			i++;
		}
	}

	// Clean up
	ClearCopy();
}
