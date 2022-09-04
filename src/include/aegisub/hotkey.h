// Copyright (c) 2010, Amar Takhar <verm@aegisub.org>
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

/// @file hotkey.h
/// @brief Hotkey handler
/// @ingroup hotkey menu event window

#include <string_view>
#include <vector>

#include <wx/event.h>

namespace agi {
	struct Context;
	namespace hotkey { class Hotkey; }
}

namespace hotkey {

extern agi::hotkey::Hotkey *inst;

void init();
void clear();

bool check(std::string_view context, agi::Context *c, wxKeyEvent &evt);
std::string keypress_to_str(int key_code, int modifier);
std::string_view get_hotkey_str_first(std::string_view context, std::string_view command);
std::vector<std::string> get_hotkey_strs(std::string_view context, std::string_view command);

} // namespace hotkey
