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
// -----------------------------------------------------------------------------
//
// AEGISUB
//
// Website: http://aegisub.cellosoft.com
// Contact: mailto:zeratul@cellosoft.com
//


#pragma once


///////////
// Headers
#include <wx/wxprec.h>


//////////////
// Prototypes
class AssDialogue;
class AssStyle;


///////////////////
// Entry type enum
enum ASS_EntryType {
	ENTRY_BASE,
	ENTRY_DIALOGUE,
	ENTRY_STYLE
};


////////////////////////////////////
// Base class for each line in file
class AssEntry {
public:
	int StartMS;		// This is only stored for sorting issues, in order to keep non-dialogue lines aligned
	bool Valid;			// Flags as valid or not
	wxString data;		// Raw data, exactly the same line that appears on the .ass (note that this will be in ass even if source wasn't)
	wxString group;		// Group it belongs to, e.g. "[Events]"
	ASS_EntryType Type;	// Defines if this is a dialogue, style, or unknown line

	AssEntry();
	AssEntry(wxString data);
	virtual ~AssEntry();

	virtual wxString GetSSAText();
	static AssDialogue *GetAsDialogue(AssEntry *base);	// Returns an entry base as a dialogue if it is valid, null otherwise
	static AssStyle *GetAsStyle(AssEntry *base);		// Returns an entry base as a style if it is valid, null otherwise
};

// This operator is for sorting
bool operator < (const AssEntry &t1, const AssEntry &t2);
