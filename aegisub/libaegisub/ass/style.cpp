// Copyright (c) 2012, Thomas Goyne <plorkyeran@aegisub.org>
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

#include "../config.h"

#include "libaegisub/ass/style.h"

#include "../common/parser.h"

#include <boost/format.hpp>

namespace agi { namespace ass {

Style::Style()
: name("Default")
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
}

Style::Style(std::string const& data, int version) {
	parser::parse(*this, data);
}

std::string Style::AssText() const {
	//name.replace(',', ';');
	//font.replace(',', ';');

	return str(boost::format("Style: %s,%s,%g,%s,%s,%s,%s,%d,%d,%d,%d,%g,%g,%g,%g,%d,%g,%g,%d,%d,%d,%d,%d")
		% name % font % fontsize
		% primary.GetAssStyleFormatted()
		% secondary.GetAssStyleFormatted()
		% outline.GetAssStyleFormatted()
		% shadow.GetAssStyleFormatted()
		% (bold? -1 : 0) % (italic ? -1 : 0)
		% (underline ? -1 : 0) % (strikeout ? -1 : 0)
		% scalex % scaley % spacing % angle
		% borderstyle % outline_w % shadow_w % alignment
		% Margin[0] % Margin[1] % Margin[2] % encoding);
}

std::string Style::SsaText() const {
	//name.replace(',', ';');
	//font.replace(',', ';');

	return str(boost::format("Style: %s,%s,%g,%s,%s,0,%s,%d,%d,%d,%g,%g,%d,%d,%d,%d,0,%d")
		% name % font % fontsize
		% primary.GetSsaFormatted()
		% secondary.GetSsaFormatted()
		% outline.GetSsaFormatted()
		% shadow.GetSsaFormatted()
		% (bold? -1 : 0) % (italic ? -1 : 0)
		% borderstyle % outline_w % shadow_w % AssToSsa(alignment)
		% Margin[0] % Margin[1] % Margin[2] % encoding);
}

int Style::AssToSsa(int ass_align) {
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

int Style::SsaToAss(int ssa_align) {
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

}
}
