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

/// @file ass_dialogue.cpp
/// @brief Class for dialogue lines in subtitles
/// @ingroup subs_storage

#include "config.h"

#ifndef AGI_PRE
#include <fstream>
#include <list>
#include <vector>

#include <wx/regex.h>
#include <wx/tokenzr.h>
#endif

#include "ass_dialogue.h"
#include "ass_override.h"
#include "utils.h"

AssDialogue::AssDialogue()
: AssEntry(wxString(), "[Events]")
, Comment(false)
, Layer(0)
, Start(0)
, End(5000)
, Style("Default")
{
	memset(Margin, 0, sizeof Margin);
}

AssDialogue::AssDialogue(AssDialogue const& that)
: AssEntry(wxString(), that.group)
, Comment(that.Comment)
, Layer(that.Layer)
, Start(that.Start)
, End(that.End)
, Style(that.Style)
, Actor(that.Actor)
, Effect(that.Effect)
, Text(that.Text)
{
	memmove(Margin, that.Margin, sizeof Margin);
}

AssDialogue::AssDialogue(wxString const& data)
: AssEntry(wxString(), "[Events]")
, Comment(false)
, Layer(0)
, Start(0)
, End(5000)
, Style("Default")
{
	if (!Parse(data))
		throw "Failed parsing line.";
}

AssDialogue::~AssDialogue () {
	delete_clear(Blocks);
}

void AssDialogue::ClearBlocks() {
	delete_clear(Blocks);
}

bool AssDialogue::Parse(wxString const& rawData) {
	size_t pos = 0;
	wxString temp;

	// Get type
	if (rawData.StartsWith("Dialogue:")) {
		Comment = false;
		pos = 10;
	}
	else if (rawData.StartsWith("Comment:")) {
		Comment = true;
		pos = 9;
	}
	else return false;

	wxStringTokenizer tkn(rawData.Mid(pos),",",wxTOKEN_RET_EMPTY_ALL);
	if (!tkn.HasMoreTokens()) return false;

	// Get first token and see if it has "Marked=" in it
	temp = tkn.GetNextToken().Trim(false).Trim(true);
	bool ssa = temp.Lower().StartsWith("marked=");

	// Get layer number
	if (ssa)
		Layer = 0;
	else {
		long templ;
		temp.ToLong(&templ);
		Layer = templ;
	}

	// Get start time
	if (!tkn.HasMoreTokens()) return false;
	Start = tkn.GetNextToken();

	// Get end time
	if (!tkn.HasMoreTokens()) return false;
	End = tkn.GetNextToken();

	// Get style
	if (!tkn.HasMoreTokens()) return false;
	Style = tkn.GetNextToken();
	Style.Trim(true);
	Style.Trim(false);

	// Get actor
	if (!tkn.HasMoreTokens()) return false;
	Actor = tkn.GetNextToken();
	Actor.Trim(true);
	Actor.Trim(false);

	// Get margins
	for (int i = 0; i < 3; ++i) {
		if (!tkn.HasMoreTokens()) return false;
		SetMarginString(tkn.GetNextToken().Trim(false).Trim(true), i);
	}

	if (!tkn.HasMoreTokens()) return false;
	Effect = tkn.GetNextToken();
	Effect.Trim(true);
	Effect.Trim(false);

	// Get text
	Text = rawData.Mid(pos + tkn.GetPosition());

	return true;
}

wxString AssDialogue::GetData(bool ssa) const {
	wxString s = Style;
	wxString a = Actor;
	wxString e = Effect;
	s.Replace(",",";");
	a.Replace(",",";");
	e.Replace(",",";");

	wxString str = wxString::Format(
		"%s: %s,%s,%s,%s,%s,%d,%d,%d,%s,%s",
		Comment ? "Comment" : "Dialogue",
		ssa ? "Marked=0" : wxString::Format("%01d", Layer),
		Start.GetASSFormated(),
		End.GetASSFormated(),
		s, a,
		Margin[0], Margin[1], Margin[2],
		e,
		Text);

	// Make sure that final has no line breaks
	str.Replace("\n", "");
	str.Replace("\r", "");

	return str;
}

const wxString AssDialogue::GetEntryData() const {
	return GetData(false);
}

wxString AssDialogue::GetSSAText () const {
	return GetData(true);
}

std::vector<AssDialogueBlock*> AssDialogue::ParseTags() const {
	std::vector<AssDialogueBlock*> Blocks;

	// Empty line, make an empty block
	if (Text.empty()) {
		Blocks.push_back(new AssDialogueBlockPlain);
		return Blocks;
	}

	int drawingLevel = 0;

	for (size_t len = Text.size(), cur = 0; cur < len; ) {
		// Overrides block
		if (Text[cur] == '{') {
			++cur;
			// Get contents of block
			wxString work;
			size_t end = Text.find("}", cur);
			if (end == wxString::npos) {
				work = Text.substr(cur);
				cur = len;
			}
			else {
				work = Text.substr(cur, end - cur);
				cur = end + 1;
			}

			if (work.size() && work.Find("\\") == wxNOT_FOUND) {
				//We've found an override block with no backslashes
				//We're going to assume it's a comment and not consider it an override block
				//Currently we'll treat this as a plain text block, but feel free to create a new class
				Blocks.push_back(new AssDialogueBlockPlain("{" + work + "}"));
			}
			else {
				// Create block
				AssDialogueBlockOverride *block = new AssDialogueBlockOverride(work);
				block->ParseTags();
				Blocks.push_back(block);

				// Look for \p in block
				std::vector<AssOverrideTag*>::iterator curTag;
				for (curTag = block->Tags.begin(); curTag != block->Tags.end(); ++curTag) {
					if ((*curTag)->Name == "\\p") {
						drawingLevel = (*curTag)->Params[0]->Get<int>(0);
					}
				}
			}
		}
		// Plain-text/drawing block
		else {
			wxString work;
			size_t end = Text.find("{",cur);
			if (end == wxString::npos) {
				work = Text.substr(cur);
				cur = len;
			}
			else {
				work = Text.substr(cur, end - cur);
				cur = end;
			}

			// Plain-text
			if (drawingLevel == 0) {
				Blocks.push_back(new AssDialogueBlockPlain(work));
			}
			// Drawing
			else {
				Blocks.push_back(new AssDialogueBlockDrawing(work, drawingLevel));
			}
		}
	}

	return Blocks;
}

void AssDialogue::ParseASSTags() {
	ClearBlocks();
	Blocks = ParseTags();
}

void AssDialogue::StripTags () {
	Text = GetStrippedText();
}

void AssDialogue::StripTag (wxString tagName) {
	ParseASSTags();
	wxString final;

	// Look for blocks
	for (std::vector<AssDialogueBlock*>::iterator cur = Blocks.begin(); cur != Blocks.end(); ++cur) {
		if ((*cur)->GetType() != BLOCK_OVERRIDE) {
			final += (*cur)->GetText();
			continue;
		}

		AssDialogueBlockOverride *over = dynamic_cast<AssDialogueBlockOverride*>(*cur);
		wxString temp;
		for (size_t i = 0; i < over->Tags.size(); ++i) {
			if (over->Tags[i]->Name != tagName)
				temp += *over->Tags[i];
		}

		if (!temp.empty())
			final += "{" + temp + "}";
	}

	ClearBlocks();
	Text = final;
}

void AssDialogue::UpdateText () {
	if (Blocks.empty()) return;
	Text.clear();
	for (std::vector<AssDialogueBlock*>::iterator cur = Blocks.begin(); cur != Blocks.end(); ++cur) {
		if ((*cur)->GetType() == BLOCK_OVERRIDE) {
			Text += "{";
			Text += (*cur)->GetText();
			Text += "}";
		}
		else Text += (*cur)->GetText();
	}
}

void AssDialogue::SetMarginString(wxString const& origvalue, int which) {
	if (which < 0 || which > 2) throw Aegisub::InvalidMarginIdError();

	// Make it numeric
	wxString strvalue = origvalue;
	if (!strvalue.IsNumber()) {
		strvalue.clear();
		for (size_t i = 0; i < origvalue.Length(); ++i) {
			if (origvalue.Mid(i, 1).IsNumber()) {
				strvalue += origvalue.Mid(i, 1);
			}
		}
	}

	// Get value
	long value = 0;
	strvalue.ToLong(&value);
	Margin[which] = mid<int>(0, value, 9999);
}

wxString AssDialogue::GetMarginString(int which, bool pad) const {
	if (which < 0 || which > 2) throw Aegisub::InvalidMarginIdError();
	return wxString::Format(pad ? "%04d" : "%d", Margin[which]);
}

void AssDialogue::ProcessParameters(AssDialogueBlockOverride::ProcessParametersCallback callback,void *userData) {
	// Apply for all override blocks
	for (std::vector<AssDialogueBlock*>::iterator cur = Blocks.begin(); cur != Blocks.end(); ++cur) {
		if ((*cur)->GetType() == BLOCK_OVERRIDE) {
			static_cast<AssDialogueBlockOverride*>(*cur)->ProcessParameters(callback, userData);
		}
	}
}

bool AssDialogue::CollidesWith(const AssDialogue *target) const {
	if (!target) return false;
	return ((Start < target->Start) ? (target->Start < End) : (Start < target->End));
}

wxString AssDialogue::GetStrippedText() const {
	static const wxRegEx reg("\\{[^\\{]*\\}", wxRE_ADVANCED);
	wxString txt(Text);
	reg.Replace(&txt, "");
	return txt;
}

AssEntry *AssDialogue::Clone() const {
	return new AssDialogue(*this);
}

void AssDialogueBlockDrawing::TransformCoords(int mx,int my,double x,double y) {
	// HACK: Implement a proper parser ffs!!
	// Could use Spline but it'd be slower and this seems to work fine
	wxStringTokenizer tkn(GetText()," ",wxTOKEN_DEFAULT);
	wxString cur;
	wxString final;
	bool isX = true;
	long temp;

	// Process tokens
	while (tkn.HasMoreTokens()) {
		cur = tkn.GetNextToken().Lower();

		// Number, process it
		if (cur.IsNumber()) {
			// Transform it
			cur.ToLong(&temp);
			if (isX) temp = (long int)((temp+mx)*x + 0.5);
			else temp = (long int)((temp+my)*y + 0.5);

			// Write back to list
			final += wxString::Format("%i ",temp);

			// Toggle X/Y
			isX = !isX;
		}

		// Text
		else {
			if (cur == "m" || cur == "n" || cur == "l" || cur == "b" || cur == "s" || cur == "p" || cur == "c") isX = true;
			final += cur + " ";
		}
	}

	// Write back final
	final = final.Left(final.Length()-1);
	text = final;
}
