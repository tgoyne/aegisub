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
#include "subtitle_format_srt.h"
#include "text_file_reader.h"
#include "ass_dialogue.h"


/////////////
// Can read?
bool SRTSubtitleFormatReader::CanReadFile(wxString filename) {
	return (filename.Right(4).Lower() == _T(".srt"));
}


/////////////
// Read file
void SRTSubtitleFormatReader::ReadFile(wxString filename,wxString encoding) {
	using namespace std;

	// Reader
	TextFileReader file(filename,encoding);

	// Default
	LoadDefault();

	// Parse file
	int linen = 1;
	int fileLine = 0;
	int mode = 0;
	long templ;
	AssDialogue *line = NULL;
	while (file.HasMoreLines()) {
		// Reads line
		wxString curLine = file.ReadLineFromFile();
		fileLine++;

		switch (mode) {
			case 0:
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
				break;

			case 1:
				// Read timestamps
				if (curLine.substr(13,3) != _T("-->")) {
					Clear();
					throw wxString::Format(_T("Parse error on entry %i at line %i (expecting timestamps). Possible malformed file."),linen,fileLine);
				}
				line->Start.ParseSRT(curLine.substr(0,12));
				line->End.ParseSRT(curLine.substr(17,12));
				mode = 2;
				break;

			case 2:
				// Checks if it's done
				if (curLine.IsEmpty()) {
					mode = 0;
					linen++;
					line->group = _T("[Events]");
					line->Style = _T("Default");
					line->Comment = false;
					line->UpdateData();
					line->ParseSRTTags();
					line->StartMS = line->Start.GetMS();
					Line->push_back(line);
					break;
				}
				// Append text
				if (line->Text != _T("")) line->Text += _T("\\N");
				line->Text += curLine;
				break;
		}
	}
}
