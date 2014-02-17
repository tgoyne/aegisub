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
#include "libaegisub/util_osx.h"

#include "libaegisub/exception.h"

#include <boost/format.hpp>
#include <boost/locale/boundary.hpp>
#include <boost/locale/conversion.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <ctime>

namespace {
const size_t bad_pos = (size_t)-1;
const std::pair<size_t, size_t> bad_match(bad_pos, bad_pos);

template<typename Iterator>
size_t advance_both(Iterator& folded, Iterator& raw) {
	size_t len;
	if (*folded == *raw) {
		len = folded->length();
		++folded;
	}
	else {
		// This character was changed by case folding, so refold it and eat the
		// appropriate number of characters from folded
		len = boost::locale::fold_case(raw->str()).size();
		for (size_t folded_consumed = 0; folded_consumed < len; ++folded)
			folded_consumed += folded->length();
	}

	++raw;
	return len;
}

std::pair<size_t, size_t> find_range(std::string const& haystack, std::string const& needle, size_t start = 0) {
	const size_t match_start = haystack.find(needle, start);
	if (match_start == std::string::npos)
		return bad_match;
	return std::make_pair(match_start, match_start + needle.size());
}

}

namespace agi { namespace util {

std::string to_string(double value) {
	std::string s = str(boost::format("%.3f") % value);
	size_t pos = s.find_last_not_of("0");
	if (pos != s.find(".")) ++pos;
	s.erase(begin(s) + pos, end(s));
	return s;
}

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
	auto match = find_range(folded_hs, folded_n);
	if (match == bad_match || folded_hs == haystack)
		return match;

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
		return match;

	const auto map_folded_to_raw = [&]() -> std::pair<size_t, size_t> {
		size_t start = -1;

		// Iterate over each pair of characters and refold each character which was
		// changed by folding, so that we can find the corresponding positions in
		// the unfolded string
		auto folded_it = begin(folded_characters);
		auto haystack_it = begin(haystack_characters);
		size_t folded_pos = 0;

		while (folded_pos < match.first)
			folded_pos += advance_both(folded_it, haystack_it);
		// If we overshot the start then the match started in the middle of a
		// character which was folded to multiple characters
		if (folded_pos > match.first)
			return bad_match;

		start = distance(begin(haystack), begin(*haystack_it));

		while (folded_pos < match.second)
			folded_pos += advance_both(folded_it, haystack_it);
		if (folded_pos > match.second)
			return bad_match;

		return std::make_pair(start, distance(begin(haystack), begin(*haystack_it)));
	};

	auto ret = map_folded_to_raw();
	while (ret == bad_match) {
		// Found something, but it was an invalid match so retry from the next character
		match = find_range(folded_hs, folded_n, match.first + 1);
		if (match == bad_match) return match;
		ret = map_folded_to_raw();
	}

	return ret;
}

}

#ifndef __APPLE__
namespace osx {
AppNapDisabler::AppNapDisabler(std::string const& reason) { }
AppNapDisabler::~AppNapDisabler() { }
}
#endif
}
