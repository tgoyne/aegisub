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
//
// $Id$

/// @file util.cpp
/// @brief Windows utility methods.
/// @ingroup libaegisub windows

#ifndef LAGI_PRE
#include <stdarg.h>
#include <stdio.h>

#include <string>
#include <fstream>

#include <time.h>
#include <windows.h>

#endif

//#include <string.h>
#include "libaegisub/types.h"
#include "libaegisub/util.h"
#include "libaegisub/util_win.h"

namespace agi {
	namespace util {


const std::string DirName(const std::string& path) {
	if (path.find('/') == std::string::npos) {
		const std::string cwd(".");
		return cwd;
	}

	const std::string stripped = path.substr(0, path.rfind("/")+1);
	return stripped;
}

void Rename(const std::string& from, const std::string& to) {
	acs::CheckFileWrite(from);

	try {
		acs::CheckFileWrite(to);
	} catch (acs::AcsNotFound&) {
		acs::CheckDirWrite(DirName(to));
	}

	MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING);
}

std::string ErrorString(DWORD error) {
	LPSTR lpstr = NULL;

	if(FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, (LPSTR)&lpstr, 0, NULL) == 0) {
		/// @todo Return the actual 'unknown error' string from windows.
		std::string str("Unknown Error");		
		return str;
	}

	std::string str(lpstr);
	LocalFree(lpstr);
	return str;
}

/// @brief Get seconds and microseconds.
/// @param tv[out] timeval struct
/// This code is from http://www.suacommunity.com/dictionary/gettimeofday-entry.php
void time_log(timeval &tv) {
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

	// Define a structure to receive the current Windows filetime
	FILETIME ft;
 
	// Initialize the present time to 0 and the timezone to UTC
	unsigned __int64 tmpres = 0;
	static int tzflag = 0;

	GetSystemTimeAsFileTime(&ft);
 
	// The GetSystemTimeAsFileTime returns the number of 100 nanosecond 
	// intervals since Jan 1, 1601 in a structure. Copy the high bits to 
	// the 64 bit tmpres, shift it left by 32 then or in the low 32 bits.
	tmpres |= ft.dwHighDateTime;
	tmpres <<= 32;
	tmpres |= ft.dwLowDateTime;
 
	// Convert to microseconds by dividing by 10
	tmpres /= 10;
 
	// The Unix epoch starts on Jan 1 1970.  Need to subtract the difference 
	// in seconds from Jan 1 1601.
	tmpres -= DELTA_EPOCH_IN_MICROSECS;
 
	// Finally change microseconds to seconds and place in the seconds value. 
	// The modulus picks up the microseconds.
	tv.tv_sec = (long)(tmpres / 1000000UL);
	tv.tv_usec = (long)(tmpres % 1000000UL);
}

	} // namespace io
} // namespace agi
