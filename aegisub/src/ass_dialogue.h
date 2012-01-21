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
// Aegisub Project http://www.aegisub.org/
//
// $Id$

/// @file ass_dialogue.h
/// @see ass_dialogue.cpp
/// @ingroup subs_storage
///

#ifndef AGI_PRE
#include <vector>
#endif

#include "ass_entry.h"
#include "ass_time.h"

/// DOCME
/// @class AssDialogue
/// @brief DOCME
///
/// DOCME
class AssDialogue : public AssEntry {
	wxString GetData(bool ssa) const;
public:

	/// Is this a comment line?
	bool Comment;
	/// Layer number
	int Layer;
	/// Margins: 0 = Left, 1 = Right, 2 = Top (Vertical), 3 = Bottom
	int Margin[4];
	/// Starting time
	AssTime Start;
	/// Ending time
	AssTime End;
	/// Style name
	wxString Style;
	/// Actor name
	wxString Actor;
	/// Effect name
	wxString Effect;
	/// Raw text data
	wxString Text;

	ASS_EntryType GetType() const { return ENTRY_DIALOGUE; }

	/// @brief Parse raw ASS data into everything else
	/// @param data ASS line
	/// @param version ASS version to try first (4, 4+, ASS2)
	/// @return Did it successfully parse?
	bool Parse(wxString data,int version=1);
	/// Parse text as ASS to generate block information
	void ParseASSTags();
	/// Clear all blocks, ALWAYS call this after you're done processing tags
	void ClearBlocks();

	/// @brief Process parameters via callback 
	/// @param callback The callback function to call per tag parameter
	/// @param userData User data to pass to callback function
	void ProcessParameters(AssDialogueBlockOverride::ProcessParametersCallback callback,void *userData=NULL);
	/// Strip all ASS tags from the text
	void StripTags();
	/// Strip a specific ASS tag from the text
	void StripTag(wxString tagName);
	/// Get text without tags
	wxString GetStrippedText() const;

	/// If blocks have been parsed, update the text from their current value
	void UpdateText();
	const wxString GetEntryData() const;
	/// Do nothing
	void SetEntryData(wxString const&) { }

	template<int which>
	void SetMarginString(const wxString value) { SetMarginString(value, which);}
	/// @brief Set a margin 
	/// @param value New value of the margin
	/// @param which 0 = left, 1 = right, 2 = vertical/top, 3 = bottom
	void SetMarginString(const wxString value,int which);
	/// @brief Get a margin
	/// @param which 0 = left, 1 = right, 2 = vertical/top, 3 = bottom
	/// @param pad Pad the number to four digits
	wxString GetMarginString(int which,bool pad=true) const;
	/// Get the line as SSA rather than ASS
	wxString GetSSAText() const;
	/// Does this line collide with the passed line?
	bool CollidesWith(AssDialogue *target);

	AssEntry *Clone() const;

	AssDialogue();
	AssDialogue(wxString data,int version=1);
	~AssDialogue();
};
