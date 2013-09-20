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

#pragma once

#include <string>

namespace agi {
namespace vfr { class Framerate; }
namespace ass { namespace time {

class Time {
	int time; ///< Time in milliseconds

public:
	Time(int ms = 0);
	Time(std::string const& text);

	/// Get time in millisecond, truncated to centisecond precision
	operator int() const { return time / 10 * 10; }

	int Hours()        const { return time / 3600000;           } ///< Get the hours portion of this time
	int Minutes()      const { return (time % 3600000) / 60000; } ///< Get the minutes portion of this time
	int Seconds()      const { return (time % 60000) / 1000;    } ///< Get the seconds portion of this time
	int Miliseconds()  const { return (time % 1000);            } ///< Get the miliseconds portion of this time
	int Centiseconds() const { return (time % 1000) / 10;       } ///< Get the centiseconds portion of this time

	/// Return the time as a string
	/// @param ms Use milliseconds precision, for non-ASS formats
	std::string GetAssFormated(bool ms=false) const;
};

/// Convert a time to a string using ASS formatting (i.e. centisecond precision)
std::string AssFormat(Time time);

/// Convert a time to a string with millisecond precision
std::string MillisecondFormat(Time time);

/// Convert a time to a string using SMPTE formatting
/// @param time Time to convert
/// @param fps Framerate to use (since SMPTE timecodes are actually a funny way to write frame numbers)
/// @param sep Component separator character
std::string SmpteFormat(Time time, agi::vfr::Framerate const& fps, char sep=':');

/// Parse a string containing a SMPTE timecode
/// @param str String to parse
/// @param fps Framerate for converting the timecode to an actual time
///
/// @return The parsed and converted time, or zero on failure.
Time ParseSmpte(std::string const& str, agi::vfr::Framerate const& fps);

}

using time::Time;
using time::ParseSmpte;

} }

