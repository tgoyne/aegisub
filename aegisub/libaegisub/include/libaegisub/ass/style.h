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

#ifndef LAGI_PRE
#include <tr1/array>
#endif

#include <libaegisub/color.h>

namespace agi { namespace ass {

class Style {
public:
	//std::string name;   ///< Name of the style; must be case-insensitively unique within a file despite being case-sensitive
	//std::string font;   ///< Font face name
	double fontsize; ///< Font size

	Color primary;   ///< Default text color
	Color secondary; ///< Text color for not-yet-reached karaoke syllables
	Color outline;   ///< Outline color
	Color shadow;    ///< Shadow color

	bool bold;
	bool italic;
	bool underline;
	bool strikeout;

	double scalex;    ///< Font x scale with 100 = 100%
	double scaley;    ///< Font y scale with 100 = 100%
	double spacing;   ///< Additional spacing between characters in pixels
	double angle;     ///< Counterclockwise z rotation in degrees
	int borderstyle;  ///< 1: Normal; 3: Opaque box; others are unused in Aegisub
	double outline_w; ///< Outline width in pixels
	double shadow_w;  ///< Shadow distance in pixels
	int alignment;    ///< \an-style line alignment
	std::tr1::array<int, 3> Margin; ///< Left/Right/Vertical
	int encoding;     ///< ASS font encoding needed for some non-unicode fonts

	Style();
	Style(std::string const& data, int version=1);

	std::string AssText() const;
	std::string SsaText() const;

	/// Convert an ASS alignment to the equivalent SSA alignment
	static int AssToSsa(int ass_align);
	/// Convert a SSA  alignment to the equivalent ASS alignment
	static int SsaToAss(int ssa_align);
};

}
}
