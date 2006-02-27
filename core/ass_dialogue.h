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


////////////
// Includes
#include <vector>
#include "ass_entry.h"
#include "ass_time.h"
#ifndef NO_FEX
class FexTracker;
class FexMovement;
#endif


//////////////
// Prototypes
class AssOverrideParameter;
class AssOverrideTag;
class AssDialogueBlockPlain;
class AssDialogueBlockOverride;
class AssDialogueBlockDrawing;


///////////////////
// Block type enum
enum ASS_BlockType {
	BLOCK_BASE,
	BLOCK_PLAIN,
	BLOCK_OVERRIDE,
	BLOCK_DRAWING
};


//////////////////////
// AssDialogue Blocks
// ------------------
// A block is each group in the text field of an AssDialogue
// For example:
//  Yes, I {\i1}am{\i0} here.
//
// Gets split in five blocks:
//  "Yes, I " (Plain)
//  "\i1"     (Override)
//  "am"      (Plain)
//  "\i0"     (Override)
//  " here."  (Plain)
//
// Also note how {}s are discarded.
// Override blocks are further divided in AssOverrideTag's.
//
// The GetText() method generates a new value for the "text" field from
// the other fields in the specific class, and returns the new value.
//
// TODO: Support for {\p#}...{\p0}
//
class AssDialogueBlock {
public:
	wxString text;
	ASS_BlockType type;
	AssDialogue *parent;

	AssDialogueBlock();
	virtual ~AssDialogueBlock();

	virtual wxString GetText() = 0; // make the class abstract
	static AssDialogueBlockPlain *GetAsPlain(AssDialogueBlock *base);		// Returns a block base as a plain block if it is valid, null otherwise
	static AssDialogueBlockOverride *GetAsOverride(AssDialogueBlock *base);	// Returns a block base as an override block if it is valid, null otherwise
	static AssDialogueBlockDrawing *GetAsDrawing(AssDialogueBlock *base);	// Returns a block base as a drawing block if it is valid, null otherwise
};


/////////////////////////////
// Plain dialogue block text
// -------------------------
// This is used for standard text
// type = BLOCK_PLAIN
// 
class AssDialogueBlockPlain : public AssDialogueBlock {
public:
	AssDialogueBlockPlain();
	wxString GetText();
};


/////////////////////////////
// Plain dialogue block text
// -------------------------
// This is used for drawing commands
// type = BLOCK_DRAWING
// 
class AssDialogueBlockDrawing : public AssDialogueBlock {
public:
	int Scale;

	AssDialogueBlockDrawing();
	void MultiplyCoords(double x,double y);
	wxString GetText();
};


//////////////////////
// Override tag block
// ------------------
// Used to store ASS overrides
// Type = BLOCK_OVERRIDE
//
class AssDialogueBlockOverride : public AssDialogueBlock {
public:
	AssDialogueBlockOverride();
	~AssDialogueBlockOverride();
	std::vector<AssOverrideTag*> Tags;

	wxString GetText();
	void ParseTags();		// Parses tags
	void ProcessParameters(void (*callback)(wxString,int,AssOverrideParameter*,void *),void *userData);
};


////////////////////////////////////////
// Class for Dialogue and Comment lines
class AssDialogue : public AssEntry {
private:
	wxString MakeData();

public:
	static bool keepData;

	std::vector<AssDialogueBlock*> Blocks;	// Contains information about each block of text

	bool Comment;					// Is this a comment line?
	int Layer;						// Layer number
	int MarginR;					// Right margin
	int MarginL;					// Left margin
	int MarginV;					// Vertical margin
	AssTime Start;					// Starting time
	AssTime End;					// Ending time
	wxString Style;					// Style name
	wxString Actor;					// Actor name
	wxString Effect;				// Effect name
	wxString Text;					// Raw text data
#ifndef NO_FEX
	FexTracker *Tracker;			// Point tracker
	FexMovement *Movement;			// Point tracker generated movement
#endif

	ASS_EntryType GetType() { return ENTRY_DIALOGUE; }

	bool Parse(wxString data,bool IsSSA=false);	// Parses raw ASS data into everything else
	void ParseASSTags();			// Parses text to generate block information (doesn't update data)
	void ParseSRTTags();			// Converts tags to ass format and calls ParseASSTags+UpdateData

	void UpdateData();				// Updates raw data from current values + text
	void UpdateText();				// Generates text from the override tags
	const wxString GetEntryData();
	void SetEntryData(wxString newData);

	void ConvertTagsToSRT();		// Converts tags to SRT format
	void StripTags();				// Strips all tags from the text
	void Clear();					// Wipes all data
	void SetMarginString(const wxString value,int which);	// Set string to a margin value (1 = left, 2 = right, 3 = vertical)
	wxString GetMarginString(int which);					// Returns the string of a margin value (1 = left, 2 = right, 3 = vertical)
	void ProcessParameters(void (*callback)(wxString,int,AssOverrideParameter*,void *userData),void *userData=NULL);	// Callback to process parameters
	wxString GetSSAText();
	bool CollidesWith(AssDialogue *target);					// Checks if two lines collide
	void ClearBlocks();

	AssEntry *Clone();

	AssDialogue();
	AssDialogue(wxString data,bool IsSSA=false);
	~AssDialogue();
};
