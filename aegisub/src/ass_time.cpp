// Copyright (c) 2005, Rodrigo Braz Monteiro
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

/// @file ass_time.cpp
/// @brief Class for managing timestamps in subtitles
/// @ingroup subs_storage
///


////////////
// Includes
#include "config.h"

#ifndef AGI_PRE
#include <math.h>

#include <algorithm>
#include <fstream>

#include <wx/regex.h>
#endif

#include "ass_time.h"
#include "utils.h"


/// @brief AssTime constructors
AssTime::AssTime() : time(0) { }
AssTime::AssTime(int time) { SetMS(time); }



/// @brief Note that this function is atomic, it won't touch the values if it's invalid. --------------- Parses from ASS 
/// @param text 
/// @return 
///
void AssTime::ParseASS (const wxString text) {
	// Prepare
	size_t pos = 0;
	size_t end = 0;
	long th=0,tm=0,tms=0;

	// Count the number of colons
	size_t len = text.Length();
	int colons = 0;
	for (pos=0;pos<len;pos++) if (text[pos] == ':') colons++;
	pos = 0;
	
	// Set start so that there are only two colons at most
	if (colons > 2) {
		for (pos=0;pos<len;pos++) {
			if (text[pos] == ':') {
				colons--;
				if (colons == 2) break;
			}
		}
		pos++;
		end = pos;
	}

	try {
		// Hours
		if (colons == 2) {
			while (text[end++] != ':') {};
			th = AegiStringToInt(text,pos,end);
			pos = end;
		}

		// Minutes
		if (colons >= 1) {
			while (text[end++] != ':') {};
			tm = AegiStringToInt(text,pos,end);
			pos = end;
		}

		// Miliseconds (includes seconds)
		end = text.Length();
		tms = AegiStringToFix(text,3,pos,end);
	}

	// Something went wrong, don't change anything
	catch (...) {
		return;
	}

	// OK, set values
	SetMS(tms + tm*60000 + th*3600000);
}

/// @brief AssTime conversion to/from miliseconds 
/// @return 
///
int AssTime::GetMS () const {
	return time / 10 * 10;
}


/// @brief DOCME
/// @param _ms 
///
void AssTime::SetMS (int ms) {
	time = mid(0, ms, 10 * 60 * 60 * 1000 - 1);
}



/// @brief ASS Formated 
/// @param msPrecision 
/// @return 
///
wxString AssTime::GetASSFormated (bool msPrecision) const {
	int ms = msPrecision ? time : GetMS();

	int h = ms / (1000 * 60 * 60);
	int m = (ms / (1000 * 60)) % 60;
	int s = (ms / 1000) % 60;
	ms = ms % 1000;

	if (msPrecision)
		return wxString::Format("%01i:%02i:%02i.%03i",h,m,s,ms);
	else
		return wxString::Format("%01i:%02i:%02i.%02i",h,m,s,ms/10);
}

/// @brief AssTime comparison 
/// @param t1 
/// @param t2 
/// @return 
///
bool operator < (const AssTime &t1, const AssTime &t2) {
	return (t1.GetMS() < t2.GetMS());
}


/// @brief DOCME
/// @param t1 
/// @param t2 
/// @return 
///
bool operator > (const AssTime &t1, const AssTime &t2) {
	return (t1.GetMS() > t2.GetMS());
}


/// @brief DOCME
/// @param t1 
/// @param t2 
/// @return 
///
bool operator <= (const AssTime &t1, const AssTime &t2) {
	return (t1.GetMS() <= t2.GetMS());
}


/// @brief DOCME
/// @param t1 
/// @param t2 
/// @return 
///
bool operator >= (const AssTime &t1, const AssTime &t2) {
	return (t1.GetMS() >= t2.GetMS());
}


/// @brief DOCME
/// @param t1 
/// @param t2 
/// @return 
///
bool operator == (const AssTime &t1, const AssTime &t2) {
	return (t1.GetMS() == t2.GetMS());
}


/// @brief DOCME
/// @param t1 
/// @param t2 
/// @return 
///
bool operator != (const AssTime &t1, const AssTime &t2) {
	return (t1.GetMS() != t2.GetMS());
}

AssTime operator + (const AssTime &t1, const AssTime &t2) {
	return AssTime(t1.GetMS() + t2.GetMS());
}

AssTime operator - (const AssTime &t1, const AssTime &t2) {
	return AssTime(t1.GetMS() - t2.GetMS());
}

/// @brief Get 
/// @return 
///
int AssTime::GetTimeHours() { return time / 3600000; }

/// @brief DOCME
/// @return 
///
int AssTime::GetTimeMinutes() { return (time % 3600000)/60000; }

/// @brief DOCME
/// @return 
///
int AssTime::GetTimeSeconds() { return (time % 60000)/1000; }

/// @brief DOCME
/// @return 
///
int AssTime::GetTimeMiliseconds() { return (time % 1000); }

/// @brief DOCME
/// @return 
///
int AssTime::GetTimeCentiseconds() { return (time % 1000)/10; }

FractionalTime::FractionalTime(agi::vfr::Framerate fps, bool dropframe)
: fps(fps)
, drop(dropframe)
{
}

wxString FractionalTime::FromAssTime(AssTime time, char sep) {
	return FromMillisecs(time.GetMS(), sep);
}

wxString FractionalTime::FromMillisecs(int64_t msec, char sep) {
	int h=0, m=0, s=0, f=0; // hours, minutes, seconds, fractions
	int fn = fps.FrameAtTime(msec);

	// return 00:00:00:00
	if (msec <= 0) {
	}
	// dropframe?
	else if (drop) {
		fn += 2 * (fn / (30 * 60)) - 2 * (fn / (30 * 60 * 10));
		h = fn / (30 * 60 * 60);
		m = (fn / (30 * 60)) % 60;
		s = (fn / 30) % 60;
		f = fn % 30;
	}
	// no dropframe; h/m/s may or may not sync to wallclock time
	else {
		/*
		This is truly the dumbest shit. What we're trying to ensure here
		is that non-integer framerates are desynced from the wallclock
		time by a correct amount of time. For example, in the
		NTSC-without-dropframe case, 3600*num/den would be 107892
		(when truncated to int), which is quite a good approximation of
		how a long an hour is when counted in 30000/1001 frames per second.
		Unfortunately, that's not what we want, since frame numbers will
		still range from 00 to 29, meaning that we're really getting _30_
		frames per second and not 29.97 and the full hour will be off by
		almost 4 seconds (108000 frames versus 107892).

		DEATH TO SMPTE
		*/ 
		int fps_approx = floor(fps.FPS() + 0.5);
		int frames_per_h = 3600*fps_approx;
		int frames_per_m = 60*fps_approx;
		int frames_per_s = fps_approx;

		h = fn / frames_per_h;
		fn = fn % frames_per_h;

		m = fn / frames_per_m;
		fn = fn % frames_per_m;

		s = fn / frames_per_s;
		fn = fn % frames_per_s;

		f = fn;
	}

	return wxString::Format("%02i%c%02%c%02i%c%02i", h, sep, m, sep, s, sep, f);
}
