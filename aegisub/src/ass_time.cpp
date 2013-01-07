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

/// @file ass_time.cpp
/// @brief Class for managing timestamps in subtitles
/// @ingroup subs_storage
///

#include "config.h"

#include "ass_time.h"

#include "utils.h"

#include <libaegisub/util.h>

#include <algorithm>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/format.hpp>
#include <boost/range/adaptor/filtered.hpp>

AssTime::AssTime(int time) : time(mid(0, time, 10 * 60 * 60 * 1000 - 1)) { }

AssTime::AssTime(std::string const& text)
: time(0)
{
	int after_decimal = -1;
	for (char c : text | boost::adaptors::filtered(boost::is_any_of(":0123456789."))) {
		if (c == ':')
			time *= 60;
		else if (c == '.') {
			after_decimal = 100;
			time *= 1000;
		}
		else if (after_decimal < 0)
			time = time * 10 + c - '0';
		else {
			time += (c - '0') * after_decimal;
			after_decimal /= 10;
		}
	}

	// Limit to the valid range
	time = mid(0, time, 10 * 60 * 60 * 1000 - 1);
}

std::string AssTime::GetAssFormated(bool msPrecision) const {
	std::string ret(10 + msPrecision, ':');
	ret[0] = '0' + GetTimeHours();
	ret[2] = '0' + (time % (60 * 60 * 1000)) / (60 * 1000 * 10);
	ret[3] = '0' + (time % (10 * 60 * 1000)) / (60 * 1000);
	ret[5] = '0' + (time % (60 * 1000)) / (1000 * 10);
	ret[6] = '0' + (time % (10 * 1000)) / 1000;
	ret[7] = '.';
	ret[8] = '0' + (time % 1000) / 100;
	ret[9] = '0' + (time % 100) / 10;
	if (msPrecision)
		ret[10] = '0' + time % 10;
	return ret;
}

int AssTime::GetTimeHours() const { return time / 3600000; }
int AssTime::GetTimeMinutes() const { return (time % 3600000) / 60000; }
int AssTime::GetTimeSeconds() const { return (time % 60000) / 1000; }
int AssTime::GetTimeMiliseconds() const { return (time % 1000); }
int AssTime::GetTimeCentiseconds() const { return (time % 1000) / 10; }

SmpteFormatter::SmpteFormatter(agi::vfr::Framerate fps, std::string const& sep)
: fps(fps)
, sep(sep)
{
}

std::string SmpteFormatter::ToSMPTE(AssTime time) const {
	int h=0, m=0, s=0, f=0;
	fps.SmpteAtTime(time, &h, &m, &s, &f);
	return str(boost::format("%02d%s%02d%s%02d%c%02d") % h % sep % m % sep % s % sep % f);
}

AssTime SmpteFormatter::FromSMPTE(std::string const& str) const {
	std::vector<std::string> toks;
	boost::split(toks, str, boost::is_any_of(sep));
	if (toks.size() != 4) return 0;

	int h, m, s, f;
	agi::util::try_parse(toks[0], &h);
	agi::util::try_parse(toks[1], &m);
	agi::util::try_parse(toks[2], &s);
	agi::util::try_parse(toks[3], &f);
	return fps.TimeAtSmpte(h, m, s, f);
}
