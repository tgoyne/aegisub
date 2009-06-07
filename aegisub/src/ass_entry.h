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
#include <wx/string.h>


//////////////
// Prototypes
class AssDialogue;
class AssStyle;
class AssAttachment;


///////////////////
// Entry type enum
enum ASS_EntryType {
	ENTRY_BASE,
	ENTRY_DIALOGUE,
	ENTRY_STYLE,
	ENTRY_ATTACHMENT
};


////////////////////////////////////
// Base class for each line in file
class AssEntry {
private:
	wxString data;		// Raw data, exactly the same line that appears on the .ass (note that this will be in ass even if source wasn't)
	int StartMS;		// This is only stored for sorting issues, in order to keep non-dialogue lines aligned

public:
	bool Valid;			// Flags as valid or not
	wxString group;		// Group it belongs to, e.g. "[Events]"

	AssEntry();
	AssEntry(wxString data);
	virtual ~AssEntry();

	virtual AssEntry *Clone() const;

	virtual int GetStartMS() const { return StartMS; }
	virtual int GetEndMS() const { return StartMS; }
	virtual void SetStartMS(const int newStart) { StartMS = newStart; }
	virtual void SetEndMS(const int newEnd) { /* do nothing */ (void)newEnd; }

	virtual ASS_EntryType GetType() { return ENTRY_BASE; }
	virtual const wxString GetEntryData() { return data; }
	virtual void SetEntryData(wxString newData) { if (newData.IsEmpty()) data.Clear(); else data = newData; }

	virtual wxString GetSSAText();
	static AssDialogue *GetAsDialogue(AssEntry *base);	// Returns an entry base as a dialogue if it is valid, null otherwise
	static AssStyle *GetAsStyle(AssEntry *base);		// Returns an entry base as a style if it is valid, null otherwise
	static AssAttachment *GetAsAttachment(AssEntry *base);// Returns an entry base as an attachment if it is valid, null otherwise
};

// This operator is for sorting
bool operator < (const AssEntry &t1, const AssEntry &t2);
