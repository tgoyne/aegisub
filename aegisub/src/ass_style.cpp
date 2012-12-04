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

/// @file ass_style.cpp
/// @brief Class for style definitions in subtitles
/// @ingroup subs_storage
///

#include "config.h"

#include "ass_style.h"
#include "compat.h"
#include "subtitle_format.h"
#include "utils.h"

#include <algorithm>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <cctype>

#include <wx/intl.h>

AssStyle::AssStyle()
: AssEntry("")
, name("Default")
, font("Arial")
, fontsize(20.)
, primary(255, 255, 255)
, secondary(255, 0, 0)
, bold(false)
, italic(false)
, underline(false)
, strikeout(false)
, scalex(100.)
, scaley(100.)
, spacing(0.)
, angle(0.)
, borderstyle(1)
, outline_w(2.)
, shadow_w(2.)
, alignment(2)
, encoding(1)
{
	std::fill(Margin.begin(), Margin.end(), 10);

	UpdateData();
}

namespace {
class parser {
	typedef boost::iterator_range<std::string::const_iterator> string_range;
	string_range str;
	std::vector<string_range> tkns;
	size_t tkn_idx;

	string_range& next_tok() {
		if (tkn_idx >= tkns.size())
			throw SubtitleFormatParseError("Malformed style: not enough fields", 0);
		return tkns[tkn_idx++];
	}

public:
	parser(std::string const& str)
	: tkn_idx(0)
	{
		auto pos = find(str.begin(), str.end(), ':');
		if (pos != str.end()) {
			this->str = string_range(pos + 1, str.end());
			split(tkns, this->str, [](char c) { return c == ','; });
		}
	}

	~parser() {
		if (tkn_idx != tkns.size())
			throw SubtitleFormatParseError("Malformed style: too many fields", 0);
	}

	std::string next_str() {
		auto tkn = trim_copy(next_tok());
		return std::string(begin(tkn), end(tkn));
	}

	agi::Color next_color() {
		auto &tkn = next_tok();
		return std::string(begin(tkn), end(tkn));
	}

	int next_int() {
		try {
			return boost::lexical_cast<int>(next_tok());
		}
		catch (boost::bad_lexical_cast const&) {
			throw SubtitleFormatParseError("Malformed style: bad int field", 0);
		}
	}

	double next_double() {
		try {
			return boost::lexical_cast<double>(next_tok());
		}
		catch (boost::bad_lexical_cast const&) {
			throw SubtitleFormatParseError("Malformed style: bad double field", 0);
		}
	}

	void skip_token() {
		++tkn_idx;
	}
};
}

AssStyle::AssStyle(std::string const& rawData, int version)
: AssEntry("")
{
	parser p(rawData);

	name = p.next_str();
	font = p.next_str();
	fontsize = p.next_double();

	if (version != 0) {
		primary = p.next_color();
		secondary = p.next_color();
		outline = p.next_color();
		shadow = p.next_color();
	}
	else {
		primary = p.next_color();
		secondary = p.next_color();

		// Skip tertiary color
		p.skip_token();

		// Read shadow/outline color
		outline = p.next_color();
		shadow = outline;
	}

	bold = !!p.next_int();
	italic = !!p.next_int();

	if (version != 0) {
		underline = !!p.next_int();
		strikeout = !!p.next_int();

		scalex = p.next_double();
		scaley = p.next_double();
		spacing = p.next_double();
		angle = p.next_double();
	}
	else {
		// SSA defaults
		underline = false;
		strikeout = false;

		scalex = 100;
		scaley = 100;
		spacing = 0;
		angle = 0.0;
	}

	borderstyle = p.next_int();
	outline_w = p.next_double();
	shadow_w = p.next_double();
	alignment = p.next_int();

	if (version == 0)
		alignment = SsaToAss(alignment);

	Margin[0] = mid(0, p.next_int(), 9999);
	Margin[1] = mid(0, p.next_int(), 9999);
	Margin[2] = mid(0, p.next_int(), 9999);

	// Skip alpha level
	if (version == 0)
		p.skip_token();

	encoding = p.next_int();

	UpdateData();
}
}

#ifdef _WIN32
#define snprintf(buf, _, fmt, ...) _snprintf_s(buf, _TRUNCATE, fmt, __VA_ARGS__)
#endif

void AssStyle::UpdateData() {
	replace(begin(name), end(name), ',', ';');
	replace(begin(font), end(font), ',', ';');

	char buff[8192];
	snprintf(buff, sizeof(buff), "Style: %s,%s,%g,%s,%s,%s,%s,%d,%d,%d,%d,%g,%g,%g,%g,%d,%g,%g,%i,%i,%i,%i,%i",
		name.c_str(), font.c_str(), fontsize,
		primary.GetAssStyleFormatted().c_str(),
		secondary.GetAssStyleFormatted().c_str(),
		outline.GetAssStyleFormatted().c_str(),
		shadow.GetAssStyleFormatted().c_str(),
		(bold? -1 : 0),  (italic ? -1 : 0),
		(underline ? -1 : 0), (strikeout ? -1 : 0),
		scalex, scaley, spacing, angle,
		borderstyle, outline_w, shadow_w, alignment,
		Margin[0], Margin[1], Margin[2], encoding);

	SetEntryData(buff);
}

std::string AssStyle::GetSSAText() const {
	char buff[8192];
	snprintf(buff, sizeof(buff), "Style: %s,%s,%g,%s,%s,0,%s,%d,%d,%d,%g,%g,%d,%d,%d,%d,0,%i",
		name.c_str(), font.c_str(), fontsize,
		primary.GetSsaFormatted(),
		secondary.GetSsaFormatted(),
		shadow.GetSsaFormatted(),
		(bold? -1 : 0), (italic ? -1 : 0),
		borderstyle, outline_w, shadow_w, AssToSsa(alignment),
		Margin[0], Margin[1], Margin[2], encoding);
	return buff;
}

AssEntry *AssStyle::Clone() const {
	return new AssStyle(*this);
}

void AssStyle::GetEncodings(wxArrayString &encodingStrings) {
	encodingStrings.Clear();
	encodingStrings.Add("0 - " + _("ANSI"));
	encodingStrings.Add("1 - " + _("Default"));
	encodingStrings.Add("2 - " + _("Symbol"));
	encodingStrings.Add("77 - " + _("Mac"));
	encodingStrings.Add("128 - " + _("Shift_JIS"));
	encodingStrings.Add("129 - " + _("Hangeul"));
	encodingStrings.Add("130 - " + _("Johab"));
	encodingStrings.Add("134 - " + _("GB2312"));
	encodingStrings.Add("136 - " + _("Chinese BIG5"));
	encodingStrings.Add("161 - " + _("Greek"));
	encodingStrings.Add("162 - " + _("Turkish"));
	encodingStrings.Add("163 - " + _("Vietnamese"));
	encodingStrings.Add("177 - " + _("Hebrew"));
	encodingStrings.Add("178 - " + _("Arabic"));
	encodingStrings.Add("186 - " + _("Baltic"));
	encodingStrings.Add("204 - " + _("Russian"));
	encodingStrings.Add("222 - " + _("Thai"));
	encodingStrings.Add("238 - " + _("East European"));
	encodingStrings.Add("255 - " + _("OEM"));
}

int AssStyle::AssToSsa(int ass_align) {
	switch (ass_align) {
		case 1:  return 1;
		case 2:  return 2;
		case 3:  return 3;
		case 4:  return 9;
		case 5:  return 10;
		case 6:  return 11;
		case 7:  return 5;
		case 8:  return 6;
		case 9:  return 7;
		default: return 2;
	}
}

int AssStyle::SsaToAss(int ssa_align) {
	switch(ssa_align) {
		case 1:  return 1;
		case 2:  return 2;
		case 3:  return 3;
		case 5:  return 7;
		case 6:  return 8;
		case 7:  return 9;
		case 9:  return 4;
		case 10: return 5;
		case 11: return 6;
		default: return 2;
	}
}
