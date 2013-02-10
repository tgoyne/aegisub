// Copyright (c) 2013, Thomas Goyne <plorkyeran@aegisub.org>
//
// Permission to use, copy, modify, and distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//
// Aegisub Project http://www.aegisub.org/

#include "config.h"

#include "ass_dialogue.h"

#include "subtitle_format.h"
#include "utils.h"

#include <libaegisub/split.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_int.hpp>

using namespace boost::adaptors;

namespace {
int next_id = 0;

class tokenizer {
	agi::StringRange str;
	boost::split_iterator<agi::StringRange::const_iterator> pos;

public:
	tokenizer(agi::StringRange const& str) : str(str) , pos(agi::Split(str, ',')) { }

	agi::StringRange next_tok() {
		if (pos.eof())
			throw SubtitleFormatParseError("Failed parsing line: " + std::string(str.begin(), str.end()), 0);
		return *pos++;
	}

	std::string next_str() { return agi::str(next_tok()); }
	std::string next_str_trim() { return agi::str(boost::trim_copy(next_tok())); }
};
}

AssDialogue::AssDialogue()
: Id(++next_id)
, Comment(false)
, Layer(0)
, Start(0)
, End(5000)
, Style("Default")
{
	memset(Margin, 0, sizeof Margin);
}

AssDialogue::AssDialogue(AssDialogue const& that)
: Id(++next_id)
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

AssDialogue::AssDialogue(std::string const& raw)
: Id(++next_id)
{
	agi::StringRange str;
	if (boost::starts_with(raw, "Dialogue:")) {
		Comment = false;
		str = agi::StringRange(raw.begin() + 10, raw.end());
	}
	else if (boost::starts_with(raw, "Comment:")) {
		Comment = true;
		str = agi::StringRange(raw.begin() + 9, raw.end());
	}
	else
		throw SubtitleFormatParseError("Failed parsing line: " + raw, 0);

	tokenizer tkn(str);

	// Get first token and see if it has "Marked=" in it
	auto tmp = tkn.next_str_trim();
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
		margin = mid(0, boost::lexical_cast<int>(tkn.next_str()), 9999);
	Effect = tkn.next_str_trim();
	Text = std::string(tkn.next_tok().begin(), str.end());
}

namespace {
void append_int(std::string &str, int v) {
	boost::spirit::karma::generate(back_inserter(str), boost::spirit::karma::int_, v);
	str += ',';
}

void append_str(std::string &out, std::string const& str) {
	out += str;
	out += ',';
}

void append_unsafe_str(std::string &out, std::string const& str) {
	if (str.find(',') == str.npos)
		out += str;
	else
		out += boost::replace_all_copy(str, ",", ";");
	out += ',';
}
}

std::string AssDialogue::GetData(bool ssa) const {
	std::string str = Comment ? "Comment: " : "Dialogue: ";
	str.reserve(51 + Style.get().size() + Actor.get().size() + Effect.get().size() + Text.get().size());

	if (ssa)
		append_str(str, "Marked=0");
	else
		append_int(str, Layer);
	append_str(str, Start.GetAssFormated());
	append_str(str, End.GetAssFormated());
	append_unsafe_str(str, Style);
	append_unsafe_str(str, Actor);
	for (int i = 0; i < 3; ++i)
		append_int(str, Margin[i]);
	append_unsafe_str(str, Effect);
	str += Text.get();

	if (str.find('\n') != str.npos || str.find('\r') != str.npos) {
		boost::replace_all(str, "\n", "");
		boost::replace_all(str, "\r", "");
	}

	return str;
}

const std::string AssDialogue::GetEntryData() const {
	return GetData(false);
}

std::string AssDialogue::GetSSAText() const {
	return GetData(true);
}

void AssDialogue::SetMarginString(std::string const& origvalue, int which) {
	Margin[which] = mid<int>(0, atoi(origvalue.c_str()), 9999);
}

std::string AssDialogue::GetMarginString(int which) const {
	return std::to_string(Margin[which]);
}

bool AssDialogue::CollidesWith(const AssDialogue *target) const {
	if (!target) return false;
	return ((Start < target->Start) ? (target->Start < End) : (Start < target->End));
}

AssEntry *AssDialogue::Clone() const {
	AssDialogue *clone = new AssDialogue(*this);
	*const_cast<int *>(&clone->Id) = Id;
	return clone;
}
