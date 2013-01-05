// Copyright (c) 2011, Amar Takhar <verm@aegisub.org>
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

/// @file util.cpp
/// @brief Unix utility methods.
/// @ingroup libaegisub

#include <errno.h>

#include <climits>
#include <cstdio>
#include <ctime>

#include "libaegisub/util.h"

#include <boost/algorithm/string/case_conv.hpp>

namespace agi {
	namespace util {

void str_lower(std::string &str) {
	boost::to_lower(str);
}

int strtoi(std::string const& str) {
	errno = 0;
	long l = strtol(str.c_str(), nullptr, 10);

	if ((errno == ERANGE) || (l < INT_MIN) || (l > INT_MAX))
		return 0;

	return (int)l;
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

	} // namespace util
} // namespace agi
