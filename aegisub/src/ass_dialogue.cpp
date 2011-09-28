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
: Comment(false)
, Layer(0)
, Start(0)
, End(5000)
, Style("Default")
, Actor("")
, Effect("")
, Text("")
{
	group = "[Events]";
	Valid = true;
	for (int i=0;i<4;i++) Margin[i] = 0;
}

AssDialogue::AssDialogue(AssDialogue const& that)
: AssEntry()
, Comment(that.Comment)
, Layer(that.Layer)
, Start(that.Start)
, End(that.End)
, Style(that.Style)
, Actor(that.Actor)
, Effect(that.Effect)
, Text(that.Text)
{
	group = that.group;
	Valid = that.Valid;
	for (int i=0;i<4;i++) Margin[i] = that.Margin[i];
}

/// @brief DOCME
/// @param _data   
/// @param version 
AssDialogue::AssDialogue(wxString _data,int version)
: Comment(false)
, Layer(0)
, Start(0)
, End(5000)
, Style("Default")
, Actor("")
, Effect("")
, Text("")
{
	group = "[Events]";
	Valid = false;
	// Try parsing in different ways
	int count = 0;
	while (!Valid && count < 3) {
		Valid = Parse(_data,version);
		count++;
		version++;
		if (version > 2) version = 0;
	}

	// Not valid
	if (!Valid) {
		throw "Failed parsing line.";
	}
}

AssDialogue::~AssDialogue () {
	delete_clear(Blocks);
}

void AssDialogue::ClearBlocks() {
	delete_clear(Blocks);
}

bool AssDialogue::Parse(wxString rawData, int version) {
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
	if (temp.Lower().StartsWith("marked=")) version = 0;
	else if (version == 0) version = 1;

	// Get layer number
	if (version == 0) Layer = 0;
	else {
		long templ;
		temp.ToLong(&templ);
		Layer = templ;
	}

	// Get start time
	if (!tkn.HasMoreTokens()) return false;
	Start.ParseASS(tkn.GetNextToken());

	// Get end time
	if (!tkn.HasMoreTokens()) return false;
	End.ParseASS(tkn.GetNextToken());

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

	// Get left margin
	if (!tkn.HasMoreTokens()) return false;
	SetMarginString(tkn.GetNextToken().Trim(false).Trim(true),0);

	// Get right margin
	if (!tkn.HasMoreTokens()) return false;
	SetMarginString(tkn.GetNextToken().Trim(false).Trim(true),1);

	// Get top margin
	if (!tkn.HasMoreTokens()) return false;
	temp = tkn.GetNextToken().Trim(false).Trim(true);
	SetMarginString(temp,2);
	if (version == 1) SetMarginString(temp,3);

	// Get bottom margin
	bool rollBack = false;
	if (version == 2) {
		if (!tkn.HasMoreTokens()) return false;
		wxString oldTemp = temp;
		temp = tkn.GetNextToken().Trim(false).Trim(true);
		if (!temp.IsNumber()) {
			version = 1;
			rollBack = true;
		}
	}

	// Get effect
	if (!rollBack) {
		if (!tkn.HasMoreTokens()) return false;
		temp = tkn.GetNextToken();
	}
	Effect = temp;
	Effect.Trim(true);
	Effect.Trim(false);

	// Get text
	Text = rawData.Mid(pos+tkn.GetPosition());

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

void AssDialogue::ParseSRTTags () {
	// Search and replace
	size_t total = 0;
	total += Text.Replace("<i>","{\\i1}");
	total += Text.Replace("</i>","{\\i0}");
	total += Text.Replace("<b>","{\\b1}");
	total += Text.Replace("</b>","{\\b0}");
	total += Text.Replace("<u>","{\\u1}");
	total += Text.Replace("</u>","{\\u0}");
	total += Text.Replace("<s>","{\\s1}");
	total += Text.Replace("</s>","{\\s0}");

	// Process <font> tag
	wxString work = Text;
	work.UpperCase();
	size_t pos_open = 0;
	size_t pos_close = 0;
	size_t pos = 0;
	size_t end = 0;
	size_t start = 0;
	bool isOpen;

	// Iterate
	pos_open = work.find("<FONT",0);
	pos_close = work.find("</FONT",0);
	while (pos_open != wxString::npos || pos_close != wxString::npos) {
		// Determine if it's an open or close tag
		if (pos_open < pos_close) {
			start = pos_open;
			isOpen = true;
		}
		else {
			start = pos_close;
			isOpen = false;
		}
		end = work.find(">",start)+1;
		//if (end == wxString::npos) continue;

		// Open tag
		if (isOpen) {
			wxString replaced;

			// Color tag
			if ((pos = work.find("COLOR=\"",start)) != wxString::npos) {
				if (pos < end) {
					pos += 7;
					size_t end_tag = Text.find("\"",pos);
					if (end_tag != wxString::npos) {
						if (end_tag-pos == 7) {
							replaced += "{\\c&H";
							replaced += work.substr(pos+5,2);
							replaced += work.substr(pos+3,2);
							replaced += work.substr(pos+1,2);
							replaced += "&}";
							total++;
						}
					}
				}
			}

			// Face tag
			if ((pos = work.find("FACE=\"",start)) != wxString::npos) {
				if (pos < end) {
					pos += 6;
					size_t end_tag = work.find("\"",pos);
					if (end_tag != wxString::npos) {
						replaced += "{\\fn";
						replaced += work.substr(pos,end_tag-pos);
						replaced += "}";
						total++;
					}
				}
			}

			// Size tag
			if ((pos = work.find("SIZE=\"",start)) != wxString::npos) {
				if (pos < end) {
					pos += 6;
					size_t end_tag = Text.find("\"",pos);
					if (end_tag != wxString::npos) {
						replaced += "{\\fs";
						replaced += work.substr(pos,end_tag-pos);
						replaced += "}";
						total++;
					}
				}
			}

			// Replace whole tag
			//Text = Text.substr(0,start) + replaced + Text.substr(end);
			Text = Text.substr(0, start);
			Text << replaced << Text.substr(end);
			total++;
		}

		// Close tag
		else {
			// Find if it's italic, bold, underline, and strikeout
			wxString prev = Text.Left(start);
			bool isItalic=false,isBold=false,isUnder=false,isStrike=false;
			if (CountMatches(prev,"{\\i1}") > CountMatches(prev,"{\\i0}")) isItalic = true;
			if (CountMatches(prev,"{\\b1}") > CountMatches(prev,"{\\b0}")) isBold = true;
			if (CountMatches(prev,"{\\u1}") > CountMatches(prev,"{\\u0}")) isUnder = true;
			if (CountMatches(prev,"{\\s1}") > CountMatches(prev,"{\\s0}")) isStrike = true;

			// Generate new tag, by reseting and then restoring flags
			wxString replaced = "{\\r";
			if (isItalic) replaced += "\\i1";
			if (isBold) replaced += "\\b1";
			if (isUnder) replaced += "\\u1";
			if (isStrike) replaced += "\\s1";
			replaced += "}";

			// Replace
			//Text = Text.substr(0,start) + replaced + Text.substr(end);
			Text = Text.substr(0, start);
			Text << replaced << Text.substr(end);
			total++;
		}

		// Get next
		work = Text;
		work.UpperCase();
		pos_open = work.find("<FONT",0);
		pos_close = work.find("</FONT",0);
	}

	// Remove double tagging
	Text.Replace("}{","");
}

void AssDialogue::ParseASSTags () {
	ClearBlocks();

	int drawingLevel = 0;

	const size_t len = Text.size();
	size_t cur = 0;
	size_t end = 0;
	while (cur < len) {
		// Overrides block
		if (Text[cur] == '{') {
			// Get contents of block
			wxString work;
			end = Text.find("}",cur);
			if (end == wxString::npos) {
				work = Text.substr(cur);
				end = len;
			}
			else work = Text.substr(cur,end-cur+1);
			
			if (work.Find("\\") == wxNOT_FOUND) {
				//We've found an override block with no backslashes
				//We're going to assume it's a comment and not consider it an override block
				//Currently we'll treat this as a plain text block, but feel free to create a new class
				AssDialogueBlockPlain *block = new AssDialogueBlockPlain;
				block->text = work;
				Blocks.push_back(block);
			}

			else {
				work = work.substr(1,work.Len()-2); // trim { and }
				// Create block
				AssDialogueBlockOverride *block = new AssDialogueBlockOverride;
				block->parent = this;
				block->text = work;
				block->ParseTags();
				Blocks.push_back(block);

				// Look for \p in block
				std::vector<AssOverrideTag*>::iterator curTag;
				for (curTag = block->Tags.begin();curTag != block->Tags.end();curTag++) {
					if ((*curTag)->Name == "\\p") {
						drawingLevel = (*curTag)->Params[0]->Get<int>(0);
					}
				}
			}

			// Increase
			cur = end+1;
		}

		// Plain-text/drawing block
		else {
			wxString work;
			end = Text.find("{",cur);
			if (end == wxString::npos) {
				work = Text.substr(cur);
				end = len;
			}
			else work = Text.substr(cur,end-cur);

			// Plain-text
			if (drawingLevel == 0) {
				AssDialogueBlockPlain *block = new AssDialogueBlockPlain;
				block->text = work;
				Blocks.push_back(block);
			}

			// Drawing
			else {
				AssDialogueBlockDrawing *block = new AssDialogueBlockDrawing;
				block->text = work;
				block->Scale = drawingLevel;
				Blocks.push_back(block);
			}

			cur = end;
		}
	}

	// Empty line, make an empty block
	if (len == 0) {
		AssDialogueBlockPlain *block = new AssDialogueBlockPlain;
		block->text = "";
		Blocks.push_back(block);
	}
}

void AssDialogue::StripTags () {
	Text = GetStrippedText();
}

void AssDialogue::StripTag (wxString tagName) {
	using std::list;
	using std::vector;
	ParseASSTags();
	wxString final;

	// Look for blocks
	for (vector<AssDialogueBlock*>::iterator cur=Blocks.begin();cur!=Blocks.end();cur++) {
		if ((*cur)->GetType() == BLOCK_OVERRIDE) {
			AssDialogueBlockOverride *over = dynamic_cast<AssDialogueBlockOverride*>(*cur);
			wxString temp;
			for (size_t i=0;i<over->Tags.size();i++) {
				if (over->Tags[i]->Name != tagName) temp += *over->Tags[i];
			}

			// Insert
			if (!temp.IsEmpty()) final += "{" + temp + "}";
		}
		else final += (*cur)->GetText();
	}

	ClearBlocks();
	Text = final;
}

void AssDialogue::ConvertTagsToSRT () {
	using std::list;
	using std::vector;
	AssDialogueBlockOverride* curBlock;
	AssDialogueBlockPlain *curPlain;
	AssOverrideTag* curTag;
	wxString final = "";
	bool isItalic=false,isBold=false,isUnder=false,isStrike=false;
	bool temp;

	// Iterate through blocks
	ParseASSTags();
	for (size_t i=0;i<Blocks.size();i++) {
		curBlock = dynamic_cast<AssDialogueBlockOverride*>(Blocks.at(i));
		if (curBlock) {
			// Iterate through overrides
			for (size_t j=0;j<curBlock->Tags.size();j++) {
				curTag = curBlock->Tags.at(j);
				if (curTag->IsValid()) {
					// Italics
					if (curTag->Name == "\\i") {
						temp = curTag->Params.at(0)->Get<bool>();
						if (temp && !isItalic) {
							isItalic = true;
							final += "<i>";
						}
						if (!temp && isItalic) {
							isItalic = false;
							final += "</i>";
						}
					}

					// Underline
					if (curTag->Name == "\\u") {
						temp = curTag->Params.at(0)->Get<bool>();
						if (temp && !isUnder) {
							isUnder = true;
							final += "<u>";
						}
						if (!temp && isUnder) {
							isUnder = false;
							final += "</u>";
						}
					}

					// Strikeout
					if (curTag->Name == "\\s") {
						temp = curTag->Params.at(0)->Get<bool>();
						if (temp && !isStrike) {
							isStrike = true;
							final += "<s>";
						}
						if (!temp && isStrike) {
							isStrike = false;
							final += "</s>";
						}
					}

					// Bold
					if (curTag->Name == "\\b") {
						temp = curTag->Params.at(0)->Get<bool>();
						if (temp && !isBold) {
							isBold = true;
							final += "<b>";
						}
						if (!temp && isBold) {
							isBold = false;
							final += "</b>";
						}
					}
				}
			}
		}

		// Plain text
		else {
			curPlain = dynamic_cast<AssDialogueBlockPlain*>(Blocks.at(i));
			if (curPlain) {
				final += curPlain->GetText();
			}
		}
	}

	// Ensure all tags are closed
	if (isBold)
		final += "</b>";
	if (isItalic)
		final += "</i>";
	if (isUnder)
		final += "</u>";
	if (isStrike)
		final += "</s>";

	Text = final;
	ClearBlocks();
}

void AssDialogue::UpdateText () {
	if (Blocks.empty()) return;
	Text.clear();
	for (std::vector<AssDialogueBlock*>::iterator cur=Blocks.begin();cur!=Blocks.end();cur++) {
		if ((*cur)->GetType() == BLOCK_OVERRIDE) {
			Text += "{";
			Text += (*cur)->GetText();
			Text += "}";
		}
		else Text += (*cur)->GetText();
	}
}

void AssDialogue::SetMarginString(const wxString origvalue,int which) {
	if (which < 0 || which >= 4) throw Aegisub::InvalidMarginIdError();

	// Make it numeric
	wxString strvalue = origvalue;
	if (!strvalue.IsNumber()) {
		strvalue = "";
		for (size_t i=0;i<origvalue.Length();i++) {
			if (origvalue.Mid(i,1).IsNumber()) {
				strvalue += origvalue.Mid(i,1);
			}
		}
	}

	// Get value
	long value;
	strvalue.ToLong(&value);

	// Cap it
	if (value < 0) value = 0;
	if (value > 9999) value = 9999;

	// Assign
	Margin[which] = value;
}

wxString AssDialogue::GetMarginString(int which,bool pad) const {
	if (which < 0 || which >= 4) throw Aegisub::InvalidMarginIdError();
	int value = Margin[which];
	if (pad) return wxString::Format("%04i",value);
	else return wxString::Format("%i",value);
}

void AssDialogue::ProcessParameters(AssDialogueBlockOverride::ProcessParametersCallback callback,void *userData) {
	// Apply for all override blocks
	AssDialogueBlockOverride *curBlock;
	//ParseASSTags();
	for (std::vector<AssDialogueBlock*>::iterator cur=Blocks.begin();cur!=Blocks.end();cur++) {
		if ((*cur)->GetType() == BLOCK_OVERRIDE) {
			curBlock = static_cast<AssDialogueBlockOverride*> (*cur);
			curBlock->ProcessParameters(callback,userData);
		}
	}
	//ClearBlocks();
}

bool AssDialogue::CollidesWith(AssDialogue *target) {
	if (!target) return false;
	int a = Start.GetMS();
	int b = End.GetMS();
	int c = target->Start.GetMS();
	int d = target->End.GetMS();
	return ((a < c) ? (c < b) : (a < d));
}

wxString AssDialogue::GetStrippedText() const {
	static wxRegEx reg("\\{[^\\{]*\\}",wxRE_ADVANCED);
	wxString txt(Text);
	reg.Replace(&txt,"");
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
