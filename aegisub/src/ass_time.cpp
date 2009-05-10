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
// -----------------------------------------------------------------------------
//
// AEGISUB
//
// Website: http://aegisub.cellosoft.com
// Contact: mailto:zeratul@cellosoft.com
//


////////////
// Includes
#include "config.h"

#include <wx/regex.h>
#include <fstream>
#include <algorithm>
#include "ass_time.h"
#include "vfr.h"
#include "utils.h"



////////////////////// AssTime //////////////////////
// AssTime constructors
AssTime::AssTime () {
	time = 0;
}


////////////////////
// Parses from ASS
// ---------------
// Note that this function is atomic, it won't touch the values if it's invalid.
void AssTime::ParseASS (const wxString text) {
	// Prepare
	size_t pos = 0;
	size_t end = 0;
	long th=0,tm=0,tms=0;

	// Count the number of colons
	size_t len = text.Length();
	int colons = 0;
	for (pos=0;pos<len;pos++) if (text[pos] == _T(':')) colons++;
	pos = 0;
	
	// Set start so that there are only two colons at most
	if (colons > 2) {
		for (pos=0;pos<len;pos++) {
			if (text[pos] == _T(':')) {
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
			while (text[end++] != _T(':'));
			th = AegiStringToInt(text,pos,end);
			pos = end;
		}

		// Minutes
		if (colons >= 1) {
			while (text[end++] != _T(':'));
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
	time = tms + tm*60000 + th*3600000;
}


///////////////////
// Parses from SRT
void AssTime::ParseSRT (const wxString _text) {
	// Prepare
	wxString text = _text;
	text.Trim(false);
	text.Trim(true);
	long tempv;
	wxString temp;
	int ms,s,m,h;

	// Parse
	temp = text.Mid(0,2);
	temp.ToLong(&tempv);
	h = tempv;
	temp = text.Mid(3,2);
	temp.ToLong(&tempv);
	m = tempv;
	temp = text.Mid(6,2);
	temp.ToLong(&tempv);
	s = tempv;
	temp = text.Mid(9,3);
	temp.ToLong(&tempv);
	ms = tempv;

	// Set value
	time = ms + s*1000 + m*60000 + h*3600000;
}


//////////////////////////////////////////
// AssTime conversion to/from miliseconds
int AssTime::GetMS () {
	if (!UseMSPrecision) return time/10*10;
	else return time;
}

void AssTime::SetMS (int _ms) {
	time = _ms;
}


////////////////
// ASS Formated
wxString AssTime::GetASSFormated (bool msPrecision) {
	int h,m,s,ms;
	int _ms = time;

	// Centisecond precision
	msPrecision = msPrecision || UseMSPrecision;
	if (!msPrecision) _ms = _ms/10*10;

	// Reset
	h = m = s = ms = 0;
	if (_ms < 0) _ms = 0;

	// Hours
	while (_ms >= 3600000) {
		_ms -= 3600000;
		h++;
	}

	// Ass overflow
	if (h > 9) {
		h = 9;
		m = 59;
		s = 59;
		ms = 999;
	}

	// Minutes
	while (_ms >= 60000) {
		_ms -= 60000;
		m++;
	}

	// Seconds
	while (_ms >= 1000) {
		_ms -= 1000;
		s++;
	}
	ms = _ms;

	if (msPrecision) return wxString::Format(_T("%01i:%02i:%02i.%03i"),h,m,s,ms);
	else return wxString::Format(_T("%01i:%02i:%02i.%02i"),h,m,s,ms/10);
}


////////////////
// SRT Formated
wxString AssTime::GetSRTFormated () {
	int h,m,s,ms;
	int _ms = time;

	// Centisecond precision
	if (!UseMSPrecision) _ms = _ms/10*10;

	// Reset
	h = m = s = ms = 0;
	if (_ms < 0) _ms = 0;

	// Hours
	while (_ms >= 3600000) {
		_ms -= 3600000;
		h++;
	}

	// Ass overflow
	if (h > 9) {
		h = 9;
		m = 59;
		s = 59;
		ms = 999;
	}

	// Minutes
	while (_ms >= 60000) {
		_ms -= 60000;
		m++;
	}

	// Seconds
	while (_ms >= 1000) {
		_ms -= 1000;
		s++;
	}
	ms = _ms;

	wxString result = wxString::Format(_T("%02i:%02i:%02i,%03i"),h,m,s,ms);
	return result;
}


//////////////////////
// AssTime comparison
bool operator < (AssTime &t1, AssTime &t2) {
	return (t1.GetMS() < t2.GetMS());
}

bool operator > (AssTime &t1, AssTime &t2) {
	return (t1.GetMS() > t2.GetMS());
}

bool operator <= (AssTime &t1, AssTime &t2) {
	return (t1.GetMS() <= t2.GetMS());
}

bool operator >= (AssTime &t1, AssTime &t2) {
	return (t1.GetMS() >= t2.GetMS());
}

bool operator == (AssTime &t1, AssTime &t2) {
	return (t1.GetMS() == t2.GetMS());
}

bool operator != (AssTime &t1, AssTime &t2) {
	return (t1.GetMS() != t2.GetMS());
}


/////////////////
// Static option
bool AssTime::UseMSPrecision = false;


///////
// Get
int AssTime::GetTimeHours() { return time / 3600000; }
int AssTime::GetTimeMinutes() { return (time % 3600000)/60000; }
int AssTime::GetTimeSeconds() { return (time % 60000)/1000; }
int AssTime::GetTimeMiliseconds() { return (time % 1000); }
int AssTime::GetTimeCentiseconds() { return (time % 1000)/10; }




FractionalTime::FractionalTime (wxString separator, double numerator, double denominator) {
	num = numerator;
	den = denominator;
	sep = separator;

	// fractions < 1 are not welcome here
	if ((num <= 0 || den <= 0) || (num < den))
		throw _T("FractionalTime: nonsensical enumerator or denominator");
	if (sep.IsEmpty())
		throw _T("FractionalTime: no separator specified");
}

FractionalTime::~FractionalTime () {
	sep.Clear();
}

int64_t FractionalTime::ToMillisecs (wxString _text) {
	wxString text = _text;
	wxString re_str = _T("");
	wxString sep_e = _T("\\") + sep; // escape this just in case it may be a reserved regex character
	text.Trim(false);
	text.Trim(true);
	long h=0,m=0,s=0,ms=0,f=0;

	//           hour                    minute                  second                  fraction
	re_str << _T("(\\d+)") << sep_e << _T("(\\d+)") << sep_e << _T("(\\d+)") << sep_e << _T("(\\d+)");

	wxRegEx re(re_str, wxRE_ADVANCED);
	if (!re.IsValid())
		throw _T("FractionalTime: regex failure");
	if (!re.Matches(text))
		return 0; // FIXME: throw here too?
	
	re.GetMatch(text, 1).ToLong(&h);
	re.GetMatch(text, 2).ToLong(&m);
	re.GetMatch(text, 3).ToLong(&s);
	re.GetMatch(text, 4).ToLong(&f);
	// FIXME: find out how to do this in a sane way
	//if ((double)f >= ((double)num/(double)den) // overflow?
	//	f = (num/den - 1);
	ms = long((1000.0 / (num/den)) * (double)f);

	return (int64_t)((h * 3600000) + (m * 60000) + (s * 1000) + ms);
}

AssTime FractionalTime::ToAssTime (wxString _text) {
	AssTime time;
	time.SetMS((int)ToMillisecs(_text));
	return time;
}

wxString FractionalTime::FromAssTime(AssTime time) {
	return FromMillisecs((int64_t)time.GetMS());
}

wxString FractionalTime::FromMillisecs(int64_t msec) {
	int h = msec / 3600000;
	int m = (msec % 3600000)/60000;
	int s = (msec % 60000)/1000;
	int f = int((msec % 1000) * ((num/den) / 1000.0));
	return wxString::Format(_T("%02i") + sep + _T("%02i") + sep + _T("%02i") + sep + _T("%02i"),h,m,s,f);
}
