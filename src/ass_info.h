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
//
// Aegisub Project http://www.aegisub.org/

#include "ass_entry.h"

class AssInfo final : public AssEntry {
	std::string key;
	std::string value;

public:
	AssInfo(AssInfo const& o) = default;
	AssInfo(std::string_view key, std::string_view value) : key(key), value(value) { }

	AssEntryGroup Group() const override { return AssEntryGroup::INFO; }
	std::string GetEntryData() const { return key + ": " + value; }

	std::string_view Key() const { return key; }
	std::string_view Value() const { return value; }
	void SetValue(std::string_view new_value) { value = new_value; }
};
