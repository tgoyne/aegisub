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

#pragma once

#ifndef LAGI_PRE
#include <string>
#endif

namespace agi {
	struct Color {
		unsigned char r;	///< Red component
		unsigned char g;	///< Green component
		unsigned char b;	///< Blue component
		unsigned char a;	///< Alpha component

		Color();
		Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 0);
		Color(std::string const& str);
		Color(const char *str);

		bool operator==(Color const& col) const;
		bool operator!=(Color const& col) const;

		std::string GetAssStyleFormatted() const;
		std::string GetAssOverrideFormatted() const;
		std::string GetSsaFormatted() const;
		std::string GetHexFormatted() const;
		std::string GetRgbFormatted() const;

		operator std::string() const { return GetRgbFormatted(); }
	};
}
