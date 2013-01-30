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

#include "libaegisub/util.h"

#include "libaegisub/exception.h"

#include <boost/locale/boundary.hpp>
#include <boost/locale/conversion.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <ctime>

static const size_t bad_pos = (size_t)-1;
static const std::pair<size_t, size_t> bad_match(bad_pos, bad_pos);

namespace agi { namespace util {

std::string strftime(const char *fmt, const tm *tmptr) {
	if (!tmptr) {
		time_t t = time(nullptr);
		tmptr = localtime(&t);
	}

	char buff[65536];
	::strftime(buff, sizeof buff, fmt, tmptr);
	return buff;
}

std::pair<size_t, size_t> ifind(std::string const& haystack, std::string const& needle) {
	const auto folded_hs = boost::locale::fold_case(haystack);
	const auto folded_n = boost::locale::fold_case(needle);
	const size_t pos = folded_hs.find(folded_n);
	if (pos == std::string::npos)
		return bad_match;
	if (folded_hs == haystack)
		return std::make_pair(pos, pos + folded_n.size());

	// We have a match, but the position is an index into the folded string
	// and we want an index into the unfolded string.

	using namespace boost::locale::boundary;
	const ssegment_index haystack_characters(character, begin(haystack), end(haystack));
	const ssegment_index folded_characters(character, begin(folded_hs), end(folded_hs));
	const size_t haystack_char_count = boost::distance(haystack_characters);
	const size_t folded_char_count = boost::distance(folded_characters);

	// As of Unicode 6.2, case folding can never reduce the number of
	// characters, and can only reduce the number of bytes with UTF-8 when
	// increasing the number of characters. As a result, iff the bytes and
	// characters are unchanged, no folds changed the size of any characters
	// and our indices are correct.
	if (haystack.size() == folded_hs.size() && haystack_char_count == folded_char_count)
		return std::make_pair(pos, pos + needle.size());

	size_t start = -1;

	// Iterate over each pair of characters and refold each character which was
	// changed by folding, so that we can find the corresponding positions in
	// the unfolded string
	auto f_it = begin(folded_characters);
	for (auto const& hs_char : haystack_characters) {
		size_t f_pos = distance(begin(folded_hs), begin(*f_it));
		if (f_pos == pos)
			start = distance(begin(haystack), begin(hs_char));
		else if (f_pos > pos && start == bad_pos)
			return bad_match;
		if (f_pos == pos + needle.size())
			return std::make_pair(start, distance(begin(haystack), begin(hs_char)));
		else if (f_pos > pos + needle.size())
			return bad_match;

		// If this character is changed by case folding, fold just it and eat
		// the appropriate amount from the folded string
		if (*f_it != hs_char) {
			auto folded_len = boost::locale::fold_case(hs_char.str()).size();
			while (folded_len != f_it->length()) {
				folded_len -= f_it->length();
				++f_it;
			}
		}

		++f_it;
	}

	return bad_match;
}

} }
