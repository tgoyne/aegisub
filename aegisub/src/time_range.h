// Copyright (c) 2010, Niels Martin Hansen
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of the Aegisub Group nor the names of its contributors
//     may be used to endorse or promote products derived from this software
//     without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Aegisub Project http://www.aegisub.org/
//
// $Id$

/// @file time_range.h
/// @ingroup audio_ui

/// @class TimeRange
/// @brief Represents an immutable range of time
class TimeRange {
	int _begin;
	int _end;

public:
	/// @brief Constructor
	/// @param begin Index of the first millisecond to include in the range
	/// @param end   Index of one past the last millisecond to include in the range
	TimeRange(int begin, int end) : _begin(begin), _end(end)
	{
		assert(end >= begin);
	}

	/// @brief Copy constructor, optionally adjusting the range
	/// @param src          The range to duplicate
	/// @param begin_adjust Number of milliseconds to add to the start of the range
	/// @param end_adjust   Number of milliseconds to add to the end of the range
	TimeRange(const TimeRange &src, int begin_adjust = 0, int end_adjust = 0)
	{
		_begin = src._begin + begin_adjust;
		_end = src._end + end_adjust;
		assert(_end >= _begin);
	}

	/// Get the length of the range in milliseconds
	int length() const { return _end - _begin; }
	/// Get the start time of the range in milliseconds
	int begin() const { return _begin; }
	/// Get the exclusive end time of the range in milliseconds
	int end() const { return _end; }

	/// Determine whether the range contains a given time in milliseconds
	bool contains(int ms) const { return ms >= begin() && ms < end(); }

	/// Determine whether there is an overlap between two ranges
	bool overlaps(const TimeRange &other) const
	{
		return other.contains(_begin) || contains(other._begin);
	}
};
