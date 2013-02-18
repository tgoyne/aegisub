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

#include "../config.h"

#include <libaegisub/ass/dialogue_parser.h>

#include <libaegisub/color.h>
#include <libaegisub/exception.h>
#include <libaegisub/split.h>
#include <libaegisub/util.h>

#include <boost/algorithm/string/join.hpp>
#include <boost/format.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <functional>

namespace agi { namespace ass { 

namespace {
	struct get_text_visitor : public boost::static_visitor<std::string> {
		std::string operator()(PlainBlock const& b) const { return b.text; }
		std::string operator()(CommentBlock const& b) const { return b.text; }
		std::string operator()(DrawingBlock const& b) const { return b.text; }
		std::string operator()(OverrideBlock const& b) const { return GetText(b); }
	};
}

using namespace boost::adaptors;

std::string GetText(DialogueBlock const& block) {
	return apply_visitor(get_text_visitor(), block);
}

std::string TransformDrawing(std::string const& text, int mx, int my, double x, double y) {
	bool is_x = true;
	std::string final;

	for (auto const& cur : agi::Split(text, ' ')) {
		if (std::all_of(begin(cur), end(cur), isdigit)) {
			int val = atoi(&*begin(cur));
			if (is_x)
				val = (int)((val + mx) * x +.5);
			else
				val = (int)((val + my) * y +.5);
			final += std::to_string(val);
			final += ' ';
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

	if (!final.empty())
		final.pop_back();
	return final;
}

struct block_end : public boost::static_visitor<size_t> {
	template<typename T>
	size_t operator()(T const& b) const { return b.end; }

	size_t operator()(OverrideBlock const& b) const { return b.end + 1; }
};

std::string SetTag(std::string const& text, size_t position, std::string const& tag_name, std::string const& value, std::pair<size_t, size_t> *tag_pos) {
	auto blocks = Parse(text);

	auto block = begin(blocks);
	while (block != end(blocks) && apply_visitor(block_end(), *block) < position)
		++block;
	if (block == end(blocks))
		block = end(blocks) - 1;

	std::string insert = "\\" + tag_name + value;

	while (true) {
		bool plain_insert = false;
		if (boost::get<PlainBlock>(&*block))
			plain_insert = true;
		else if (boost::get<DrawingBlock>(&*block))
			--block; // Drawing blocks always have an override block somewhere before them
		else if (boost::get<CommentBlock>(&*block)) {
			if (block != begin(blocks))
				--block;
			else {
				position = 0;
				plain_insert = true;
			}
		}
		else {
			OverrideBlock& ovr = boost::get<OverrideBlock>(*block);

			std::string alt = get_alternate(tag_name);

			bool found = false;
			for (auto const& tag : ovr.Tags | reversed) {
				if (tag_name == tag.name || alt == tag.name) {
					if (tag_pos)
						*tag_pos = std::make_pair(tag.begin, tag.begin + insert.size());
					return text.substr(0, tag.begin) + insert + text.substr(tag.end);
				}
			}
			if (tag_pos)
				*tag_pos = std::make_pair(ovr.end, ovr.end + insert.size());
			return text.substr(0, ovr.end) + insert + text.substr(ovr.end);
		}

		if (plain_insert) {
			if (tag_pos)
				*tag_pos = std::make_pair(position, position + insert.size() + 2);
			return text.substr(0, position) + "{" + insert + "}" + text.substr(position);
		}
	}
}

} }
