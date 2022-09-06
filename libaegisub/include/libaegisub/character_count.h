// Copyright (c) 2014, Thomas Goyne <plorkyeran@aegisub.org>
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

#include <string_view>

namespace agi {
	enum {
		IGNORE_NONE = 0,
		IGNORE_WHITESPACE = 1,
		IGNORE_PUNCTUATION = 2,
		IGNORE_BLOCKS = 4
	};

	/// Get the length in characters of the longest line in the given text
	size_t MaxLineLength(std::string_view text, int ignore_mask);
	/// Get the total number of characters in the string
	size_t CharacterCount(std::string_view str, int ignore_mask);
	/// Get index in bytes of the nth character in str, or str.size() if str
	/// has less than n characters
	size_t IndexOfCharacter(std::string_view str, size_t n);
}
