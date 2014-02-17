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

#include "../config.h"

#include "libaegisub/ass/resample.h"

#include "libaegisub/split.h"
#include "libaegisub/util.h"

namespace agi {
std::string ResampleDrawing(std::string const& drawing, ResampleState const& config) {
	bool is_x = true;
	std::string final;
	final.reserve(drawing.size());

	for (auto const& cur : Split(drawing, ' ')) {
		double val;
		if (util::try_parse(str(cur), &val)) {
			if (is_x)
				val = (val + config.margin[ResampleState::LEFT]) * config.scale_x;
			else
				val = (val + config.margin[ResampleState::TOP]) * config.scale_y;
			val = int(val * 8 + .5) / 8.0; // round to eighth-pixels
			final += util::to_string(val);
			final += ' ';
			is_x = !is_x;
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

	if (final.size())
		final.pop_back();
	return final;
}

}
