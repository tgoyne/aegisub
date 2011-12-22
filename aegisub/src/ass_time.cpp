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
	if (!UseMSPrecision) return time/10*10;
	else return time;
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
	int ms = time;

	// Centisecond precision
	msPrecision = msPrecision || UseMSPrecision;
	if (!msPrecision) ms = ms/10*10;

	int h = ms / (1000 * 60 * 60);
	int m = (ms / (1000 * 60)) % 60;
	int s = (ms / 1000) % 60;
	ms = ms % 1000;

	if (msPrecision) return wxString::Format("%01i:%02i:%02i.%03i",h,m,s,ms);
	else return wxString::Format("%01i:%02i:%02i.%02i",h,m,s,ms/10);
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


/// DOCME
bool AssTime::UseMSPrecision = false;



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

FractionalTime::FractionalTime(int numerator, int denominator, bool dropframe)
: num(numerator)
, den(denominator)
, drop(dropframe)
{
	if (drop) {
		// no dropframe for any other framerates
		num = 30000;
		den = 1001;
	}

	// fractions < 1 are not welcome here
	if ((num <= 0 || den <= 0) || (num < den))
		throw "FractionalTime: nonsensical enumerator or denominator";
}

int FractionalTime::ToMillisecs(wxString text, char sep) {
	text.Trim(false);
	text.Trim(true);

	wxRegEx re(wxString::Format("(\\d+)%c(\\d+)%c(\\d+)%c(\\d+)", sep, sep, sep, sep), wxRE_ADVANCED);
	if (!re.Matches(text))
		return 0; // FIXME: throw here too?

	long h=0, m=0, s=0, f=0;
	re.GetMatch(text,1).ToLong(&h);
	re.GetMatch(text,2).ToLong(&m);
	re.GetMatch(text,3).ToLong(&s);
	re.GetMatch(text,4).ToLong(&f);

	int msecs_f = 0;
	int fn = 0;
	// dropframe? do silly things
	if (drop) {
		fn += h * frames_per_period * 6;
		fn += (m % 10) * frames_per_period;
		
		if (m > 0) {
			fn += 1800;
			m--;

			fn += m * 1798; // two timestamps dropped per minute after the first
			fn += s * 30 + f - 2;
		}
		else { // minute is evenly divisible by 10, keep first two timestamps
			fn += s * 30;
			fn += f;
		}

		msecs_f = (fn * num) / den;
	}
	// no dropframe, may or may not sync with wallclock time
	// (see comment in FromMillisecs for an explanation of why it's done like this)
	else {
		int fps_approx = floor((double(num)/double(den))+0.5);
		fn += h * 3600 * fps_approx;
		fn += m * 60 * fps_approx;
		fn += s * fps_approx;
		fn += f;

		msecs_f = (fn * num) / den;
	}

	return msecs_f;
}

AssTime FractionalTime::ToAssTime(wxString text, char sep) {
	return AssTime(ToMillisecs(text, sep));
}

wxString FractionalTime::FromAssTime(AssTime time, char sep) {
	return FromMillisecs(time.GetMS(), sep);
}

wxString FractionalTime::FromMillisecs(int64_t msec, char sep) {
	int h=0, m=0, s=0, f=0; // hours, minutes, seconds, fractions
	int fn = (msec*(int64_t)num) / (1000*den); // frame number

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
		int fps_approx = floor((double(num)/double(den))+0.5);
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
