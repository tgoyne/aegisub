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

/// @file ass_dialogue.h
/// @see ass_dialogue.cpp
/// @ingroup subs_storage
///

#ifndef AGI_PRE
#include <vector>
#endif

#include "ass_entry.h"
#include "ass_time.h"

#include <libaegisub/exception.h>

enum AssBlockType {
	BLOCK_BASE,
	BLOCK_PLAIN,
	BLOCK_OVERRIDE,
	BLOCK_DRAWING
};

class AssOverrideParameter;
class AssOverrideTag;

/// @class AssDialogueBlock
/// @brief AssDialogue Blocks
///
/// A block is each group in the text field of an AssDialogue
/// @verbatim
///  Yes, I {\i1}am{\i0} here.
///
/// Gets split in five blocks:
///  "Yes, I " (Plain)
///  "\\i1"     (Override)
///  "am"      (Plain)
///  "\\i0"     (Override)
///  " here."  (Plain)
///
/// Also note how {}s are discarded.
/// Override blocks are further divided in AssOverrideTags.
///
/// The GetText() method generates a new value for the "text" field from
/// the other fields in the specific class, and returns the new value.
/// @endverbatim
class AssDialogueBlock {
protected:
	/// Text of this block
	wxString text;
public:
	AssDialogueBlock(wxString const& text) : text(text) { }
	virtual ~AssDialogueBlock() { }

	virtual AssBlockType GetType() const = 0;
	virtual wxString GetText() { return text; }
};

class AssDialogueBlockPlain : public AssDialogueBlock {
public:
	AssBlockType GetType() const { return BLOCK_PLAIN; }
	AssDialogueBlockPlain(wxString const& text = wxString()) : AssDialogueBlock(text) { }
};

class AssDialogueBlockDrawing : public AssDialogueBlock {
public:
	int Scale;

	AssBlockType GetType() const { return BLOCK_DRAWING; }
	AssDialogueBlockDrawing(wxString const& text, int scale) : AssDialogueBlock(text), Scale(scale) { }
	void TransformCoords(int trans_x,int trans_y,double mult_x,double mult_y);
};

class AssDialogueBlockOverride : public AssDialogueBlock {
public:
	AssDialogueBlockOverride(wxString const& text = wxString()) : AssDialogueBlock(text) { }
	~AssDialogueBlockOverride();

	std::vector<AssOverrideTag*> Tags;

	AssBlockType GetType() const { return BLOCK_OVERRIDE; }
	wxString GetText();
	void ParseTags();
	void AddTag(wxString const& tag);

	/// Type of callback function passed to ProcessParameters
	typedef void (*ProcessParametersCallback)(wxString,int,AssOverrideParameter*,void *);
	/// @brief Process parameters via callback
	/// @param callback The callback function to call per tag paramer
	/// @param userData User data to pass to callback function
	void ProcessParameters(ProcessParametersCallback callback,void *userData);
};

class AssDialogue : public AssEntry {
	wxString GetData(bool ssa) const;
public:
	/// Contains information about each block of text
	std::vector<AssDialogueBlock*> Blocks;

	/// Unique ID of this line. Copies of the line for Undo/Redo purposes
	/// preserve the unique ID, so that the equivalent lines can be found in
	/// the different versions of the file.
	const int Id;

	/// Is this a comment line?
	bool Comment;
	/// Layer number
	int Layer;
	/// Margins: 0 = Left, 1 = Right, 2 = Top (Vertical)
	int Margin[3];
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

	AssEntryType GetType() const { return ENTRY_DIALOGUE; }

	/// @brief Parse raw ASS data into everything else
	/// @param data ASS line
	/// @return Did it successfully parse?
	bool Parse(wxString const& data);

	/// Parse text as ASS to generate block information
	void ParseAssTags();

	/// Parse text as ASS and return block information
	std::vector<AssDialogueBlock*> ParseTags() const;

	/// Clear all blocks, ALWAYS call this after you're done processing tags
	void ClearBlocks();

	/// @brief Process parameters via callback
	/// @param callback The callback function to call per tag parameter
	/// @param userData User data to pass to callback function
	void ProcessParameters(AssDialogueBlockOverride::ProcessParametersCallback callback,void *userData=nullptr);
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
	void SetMarginString(wxString const& value) { SetMarginString(value, which);}
	/// @brief Set a margin
	/// @param value New value of the margin
	/// @param which 0 = left, 1 = right, 2 = vertical
	void SetMarginString(wxString const& value, int which);
	/// @brief Get a margin
	/// @param which 0 = left, 1 = right, 2 = vertical
	/// @param pad Pad the number to four digits
	wxString GetMarginString(int which, bool pad=true) const;
	/// Get the line as SSA rather than ASS
	wxString GetSSAText() const;
	/// Does this line collide with the passed line?
	bool CollidesWith(const AssDialogue *target) const;

	AssEntry *Clone() const;

	AssDialogue();
	AssDialogue(AssDialogue const&);
	AssDialogue(wxString const& data);
	~AssDialogue();
};

class InvalidMarginIdError : public agi::InternalError {
public:
	InvalidMarginIdError() : InternalError("Invalid margin id", 0) { }
	const char *GetName() const { return "internal_error/invalid_margin_id"; }
};
