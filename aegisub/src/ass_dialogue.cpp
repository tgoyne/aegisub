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

/// @file ass_dialogue.cpp
/// @brief Class for dialogue lines in subtitles
/// @ingroup subs_storage

#include "config.h"

#include "ass_dialogue.h"
#include "compat.h"
#include "subtitle_format.h"
#include "utils.h"

#include <libaegisub/of_type_adaptor.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

using namespace boost::adaptors;

AssDialogue::AssDialogue()
: Comment(false)
, Layer(0)
, Start(0)
, End(5000)
, Style("Default")
{
	memset(Margin, 0, sizeof Margin);
}

AssDialogue::AssDialogue(AssDialogue const& that)
: Comment(that.Comment)
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

AssDialogue::AssDialogue(std::string const& data)
: Comment(false)
, Layer(0)
, Start(0)
, End(5000)
, Style("Default")
{
	Parse(data);
}

AssDialogue::~AssDialogue () {
}

typedef boost::iterator_range<std::string::const_iterator> string_range;

class tokenizer {
	string_range str;
	boost::char_separator<char> sep;
	boost::tokenizer<boost::char_separator<char>> tkn;
	boost::tokenizer<boost::char_separator<char>>::iterator pos;

public:
	tokenizer(string_range const& str)
	: str(str)
	, sep(",", "", boost::keep_empty_tokens)
	, tkn(str, sep)
	, pos(tkn.begin())
	{
	}

	string_range next_tok() {
		if (pos == tkn.end())
			throw SubtitleFormatParseError("Failed parsing line: " + std::string(str.begin(), str.end()), 0);
		return *pos++;
	}

	string_range next_tok_trim() {
		return trim_copy(next_tok());
	}

	std::string next_str_trim() {
		auto range = next_tok_trim();
		return std::string(begin(range), end(range));
	}
};

void AssDialogue::Parse(std::string const& rawData) {
	boost::iterator_range<std::string::const_iterator> str;
	if (boost::starts_with(rawData, "Dialogue:")) {
		Comment = false;
		str = boost::make_iterator_range(rawData.begin() + 10, rawData.end());
	}
	else if (boost::starts_with(rawData, "Comment:")) {
		Comment = true;
		str = boost::make_iterator_range(rawData.begin() + 9, rawData.end());
	}
	else
		throw SubtitleFormatParseError("Failed parsing line: " + rawData, 0);

	tokenizer tkn(str);

	// Get first token and see if it has "Marked=" in it
	string_range tmp = tkn.next_tok_trim();
	bool ssa = boost::istarts_with(tmp, "marked=");

	// Get layer number
	if (ssa)
		Layer = 0;
	else
		Layer = boost::lexical_cast<int>(tmp);

	Start = tkn.next_str_trim();
	End = tkn.next_str_trim();
	Style = tkn.next_str_trim();
	Actor = tkn.next_str_trim();
	for (int& margin : Margin)
		margin = mid(0, boost::lexical_cast<int>(tkn.next_tok()), 9999);
	Effect = tkn.next_str_trim();
	Text = std::string(tkn.next_tok().begin(), str.end());
}

std::string AssDialogue::GetData(bool ssa) const {
	wxString s = to_wx(Style);
	wxString a = to_wx(Actor);
	wxString e = to_wx(Effect);
	s.Replace(",",";");
	a.Replace(",",";");
	e.Replace(",",";");

	wxString str = wxString::Format(
		"%s: %s,%s,%s,%s,%s,%d,%d,%d,%s,%s",
		Comment ? "Comment" : "Dialogue",
		ssa ? "Marked=0" : wxString::Format("%01d", Layer),
		Start.GetAssFormated(),
		End.GetAssFormated(),
		s, a,
		Margin[0], Margin[1], Margin[2],
		e,
		to_wx(Text.get()));

	// Make sure that final has no line breaks
	str.Replace("\n", "");
	str.Replace("\r", "");

	return from_wx(str);
}

const std::string AssDialogue::GetEntryData() const {
	return GetData(false);
}

std::string AssDialogue::GetSSAText() const {
	return GetData(true);
}

std::auto_ptr<boost::ptr_vector<AssDialogueBlock>> AssDialogue::ParseTags() const {
	boost::ptr_vector<AssDialogueBlock> Blocks;

	// Empty line, make an empty block
	if (Text.get().empty()) {
		Blocks.push_back(new AssDialogueBlockPlain);
		return Blocks.release();
	}

	int drawingLevel = 0;
	std::string const& text(Text.get());

	for (size_t len = text.size(), cur = 0; cur < len; ) {
		// Overrides block
		if (text[cur] == '{') {
			size_t end = text.find('}', cur);

			// VSFilter requires that override blocks be closed, while libass
			// does not. We match VSFilter here.
			if (end == std::string::npos)
				goto plain;

			++cur;
			// Get contents of block
			std::string work = text.substr(cur, end - cur);
			cur = end + 1;

			if (work.size() && work.find('\\') == std::string::npos) {
				//We've found an override block with no backslashes
				//We're going to assume it's a comment and not consider it an override block
				Blocks.push_back(new AssDialogueBlockComment(work));
			}
			else {
				// Create block
				AssDialogueBlockOverride *block = new AssDialogueBlockOverride(work);
				block->ParseTags();
				Blocks.push_back(block);

				// Look for \p in block
				for (auto const& tag : block->Tags) {
					if (tag.Name == "\\p")
						drawingLevel = tag.Params[0].Get<int>(0);
				}
			}

			continue;
		}

		// Plain-text/drawing block
plain:
		std::string work;
		size_t end = text.find('{', cur + 1);
		if (end == std::string::npos) {
			work = text.substr(cur);
			cur = len;
		}
		else {
			work = text.substr(cur, end - cur);
			cur = end;
		}

		if (drawingLevel == 0)
			Blocks.push_back(new AssDialogueBlockPlain(work));
		else
			Blocks.push_back(new AssDialogueBlockDrawing(work, drawingLevel));
	}

	return Blocks.release();
}

void AssDialogue::StripTags() {
	Text = GetStrippedText();
}

static std::string get_text(AssDialogueBlock &d) { return d.GetText(); }
void AssDialogue::UpdateText(boost::ptr_vector<AssDialogueBlock>& blocks) {
	if (blocks.empty()) return;
	Text = join(blocks | transformed(get_text), "");
}

void AssDialogue::SetMarginString(std::string const& origvalue, int which) {
	if (which < 0 || which > 2) throw InvalidMarginIdError();
	Margin[which] = mid<int>(0, atoi(origvalue.c_str()), 9999);
}

std::string AssDialogue::GetMarginString(int which) const {
	if (which < 0 || which > 2) throw InvalidMarginIdError();
	return std::to_string(Margin[which]);
}

bool AssDialogue::CollidesWith(const AssDialogue *target) const {
	if (!target) return false;
	return ((Start < target->Start) ? (target->Start < End) : (Start < target->End));
}

static std::string get_text_p(AssDialogueBlock *d) { return d->GetText(); }
std::string AssDialogue::GetStrippedText() const {
	boost::ptr_vector<AssDialogueBlock> blocks(ParseTags());
	return join(blocks | agi::of_type<AssDialogueBlockPlain>() | transformed(get_text_p), "");
}

AssEntry *AssDialogue::Clone() const {
	return new AssDialogue(*this);
}

void AssDialogueBlockDrawing::TransformCoords(int mx, int my, double x, double y) {
	// HACK: Implement a proper parser ffs!!
	// Could use Spline but it'd be slower and this seems to work fine
	bool is_x = true;
	std::string final;

	boost::char_separator<char> sep(" ");
	for (auto const& cur : boost::tokenizer<boost::char_separator<char>>(text, sep)) {
		if (std::all_of(begin(cur), end(cur), isdigit)) {
			int val = boost::lexical_cast<int>(cur);
			if (is_x)
				val = (int)((val + mx) * x + .5);
			else
				val = (int)((val + my) * y + .5);
			final += std::to_string(val);
			final += ' ';
		}
		else if (cur.size() == 1) {
			char c = tolower(cur[0]);
			if (c == 'm' || c == 'n' || c == 'l' || c == 'b' || c == 's' || c == 'p' || c == 'c') {
				is_x = true;
				final += c;
				final += ' ';
			}
		}
	}

	// Write back final
	final.pop_back();
	text = final;
}
