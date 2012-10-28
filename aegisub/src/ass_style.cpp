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

#ifndef AGI_PRE
#include <ctype.h>

#include <wx/intl.h>
#include <wx/tokenzr.h>
#endif

#include "ass_style.h"
#include "compat.h"
#include "subtitle_format.h"
#include "utils.h"

AssStyle::AssStyle()
: AssEntry(wxString(), wxS("[V4+ Styles]"))
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

static wxString get_next_string(wxStringTokenizer &tok) {
	if (!tok.HasMoreTokens()) throw SubtitleFormatParseError("Malformed style: not enough fields", 0);
	return tok.GetNextToken();
}

static int get_next_int(wxStringTokenizer &tok) {
	long temp;
	if (!get_next_string(tok).ToLong(&temp))
		throw SubtitleFormatParseError("Malformed style: could not parse int field", 0);
	return temp;
}

static double get_next_double(wxStringTokenizer &tok) {
	double temp;
	if (!get_next_string(tok).ToDouble(&temp))
		throw SubtitleFormatParseError("Malformed style: could not parse double field", 0);
	return temp;
}

#if 0

using namespace boost::spirit;

template<typename Iterator>
struct ass_style_grammar : qi::grammar<Iterator, AssStyle(), ascii::space_type> {
	qi::rule<Iterator, AssStyle(), ascii::space_type> ass_style;

	ass_style_grammar() : ass_style_grammar::base_type(ass_style) {
		using qi::lit;
		using qi::lexeme;
		using ascii::char_;
		using ascii::string;
		using namespace qi::labels;

		ass_style =
			lexeme[+(char_ - ',')] > ',' > // name
			lexeme[+(char_ - ',')] > ',' > // font
			double_ > ',' // fontsize
		;
	}
};

#endif

AssStyle::AssStyle(wxString rawData, int version)
: AssEntry(wxString(), wxS("[V4+ Styles]"))
{
	wxStringTokenizer tkn(rawData.Trim(false).Mid(6), ",", wxTOKEN_RET_EMPTY_ALL);

	name = get_next_string(tkn).Trim(true).Trim(false);
	font = get_next_string(tkn).Trim(true).Trim(false);
	fontsize = get_next_double(tkn);

	if (version != 0) {
		primary = from_wx(get_next_string(tkn));
		secondary = from_wx(get_next_string(tkn));
		outline = from_wx(get_next_string(tkn));
		shadow = from_wx(get_next_string(tkn));
	}
	else {
		primary = from_wx(get_next_string(tkn));
		secondary = from_wx(get_next_string(tkn));

		// Read and discard tertiary color
		get_next_string(tkn);

		// Read shadow/outline color
		outline = from_wx(get_next_string(tkn));
		shadow = outline;
	}

	bold = !!get_next_int(tkn);
	italic = !!get_next_int(tkn);

	if (version != 0) {
		underline = !!get_next_int(tkn);
		strikeout = !!get_next_int(tkn);

		scalex = get_next_double(tkn);
		scaley = get_next_double(tkn);
		spacing = get_next_double(tkn);
		angle = get_next_double(tkn);
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

	borderstyle = get_next_int(tkn);
	outline_w = get_next_double(tkn);
	shadow_w = get_next_double(tkn);
	alignment = get_next_int(tkn);

	if (version == 0)
		alignment = SsaToAss(alignment);

	// Read left margin
	Margin[0] = mid(0, get_next_int(tkn), 9999);

	// Read right margin
	Margin[1] = mid(0, get_next_int(tkn), 9999);

	// Read vertical margin
	Margin[2] = mid(0, get_next_int(tkn), 9999);

	// Skip alpha level
	if (version == 0)
		get_next_string(tkn);

	// Read encoding
	encoding = get_next_int(tkn);

	if (tkn.HasMoreTokens())
		throw SubtitleFormatParseError("Malformed style: too many fields", 0);

	UpdateData();
}

void AssStyle::UpdateData() {
	wxString final;

	//name.Replace(",",";");
	//font.Replace(",",";");


	final = wxString::Format("Style: %s,%s,%g,%s,%s,%s,%s,%d,%d,%d,%d,%g,%g,%g,%g,%d,%g,%g,%i,%i,%i,%i,%i",
					  name, font, fontsize,
					  primary.GetAssStyleFormatted(),
					  secondary.GetAssStyleFormatted(),
					  outline.GetAssStyleFormatted(),
					  shadow.GetAssStyleFormatted(),
					  (bold? -1 : 0), (italic ? -1 : 0),
					  (underline?-1:0),(strikeout?-1:0),
					  scalex,scaley,spacing,angle,
					  borderstyle,outline_w,shadow_w,alignment,
					  Margin[0],Margin[1],Margin[2],encoding);

	SetEntryData(final);
}

wxString AssStyle::GetSSAText() const {
	wxString output;
	int align = AssToSsa(alignment);
	wxString n = name;
	n.Replace(",", ";");
	wxString f = font;
	f.Replace(",", ";");

	output = wxString::Format("Style: %s,%s,%g,%s,%s,0,%s,%d,%d,%d,%g,%g,%d,%d,%d,%d,0,%i",
				  n, f, fontsize,
				  primary.GetSsaFormatted(),
				  secondary.GetSsaFormatted(),
				  shadow.GetSsaFormatted(),
				  (bold? -1 : 0), (italic ? -1 : 0),
				  borderstyle,outline_w,shadow_w,align,
				  Margin[0],Margin[1],Margin[2],encoding);

	return output;
}

AssEntry *AssStyle::Clone() const {
	return new AssStyle(*this);
}

void AssStyle::GetEncodings(wxArrayString &encodingStrings) {
	encodingStrings.Clear();
	encodingStrings.Add(wxString("0 - ") + _("ANSI"));
	encodingStrings.Add(wxString("1 - ") + _("Default"));
	encodingStrings.Add(wxString("2 - ") + _("Symbol"));
	encodingStrings.Add(wxString("77 - ") + _("Mac"));
	encodingStrings.Add(wxString("128 - ") + _("Shift_JIS"));
	encodingStrings.Add(wxString("129 - ") + _("Hangeul"));
	encodingStrings.Add(wxString("130 - ") + _("Johab"));
	encodingStrings.Add(wxString("134 - ") + _("GB2312"));
	encodingStrings.Add(wxString("136 - ") + _("Chinese BIG5"));
	encodingStrings.Add(wxString("161 - ") + _("Greek"));
	encodingStrings.Add(wxString("162 - ") + _("Turkish"));
	encodingStrings.Add(wxString("163 - ") + _("Vietnamese"));
	encodingStrings.Add(wxString("177 - ") + _("Hebrew"));
	encodingStrings.Add(wxString("178 - ") + _("Arabic"));
	encodingStrings.Add(wxString("186 - ") + _("Baltic"));
	encodingStrings.Add(wxString("204 - ") + _("Russian"));
	encodingStrings.Add(wxString("222 - ") + _("Thai"));
	encodingStrings.Add(wxString("238 - ") + _("East European"));
	encodingStrings.Add(wxString("255 - ") + _("OEM"));
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
