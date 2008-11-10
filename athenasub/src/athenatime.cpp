// Copyright (c) 2008, Rodrigo Braz Monteiro
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
// AEGISUB/ATHENASUB
//
// Website: http://www.aegisub.net
// Contact: mailto:amz@aegisub.net
//

#include "athenatime.h"
#include "utils.h"
using namespace Athenasub;


////////////////
// Constructors
Time::Time()
{
	ms = 0;
}

Time::Time(int milliseconds)
{
	SetMS(milliseconds);
}


///////////////////
// Getters/setters
void Time::SetMS(int milliseconds)
{
	ms = Max(milliseconds,0);
}

int Time::GetMS() const
{
	return ms;
}


//////////////////////////////////
// Generates a string from a time
String Time::GetString(int ms_precision,int h_precision) const
{
	// Enforce sanity
	ms_precision = Mid(0,ms_precision,3);
	h_precision = Mid(0,h_precision,2);

	// Generate values
	int _ms = GetMS();
	int h = _ms / 3600000;
	_ms -= h*3600000;
	int min = _ms / 60000;
	_ms -= min*60000;
	int s = _ms / 1000;
	_ms -= s*1000;

	// Cap hour value
	if (h > 9 && h_precision == 1) {
		h = 9;
		min = 59;
		s = 59;
		_ms = 999;
	}

	// Modify ms to account for precision
	if (ms_precision == 2) _ms /= 10;
	else if (ms_precision == 1) _ms /= 100;
	else if (ms_precision == 0) _ms = 0;

	// Asserts
	assert(h   >= 0 && h   <= 9);
	assert(min >= 0 && min <= 59);
	assert(s   >= 0 && s   <= 59);
	assert(_ms >= 0 && _ms <= 999);

	// Get write buffer
	String final;
	size_t size = 7+h_precision+ms_precision;
	size_t pos = 0;
	//wxChar *buffer = final.GetWriteBuf(size);
	Character temp[16];
	final.resize(size);

	// Write time
	final.WriteNumber(temp,h,h_precision,pos);
	final.WriteChar(':',pos);
	final.WriteNumber(temp,min,2,pos);
	final.WriteChar(':',pos);
	final.WriteNumber(temp,s,2,pos);
	final.WriteChar('.',pos);
	final.WriteNumber(temp,_ms,ms_precision,pos);

	// Write terminator
	//final.WriteText("\0",1,pos);

	// Restore string's state and return
	final.SetSize(pos-1);
	return final;
}


///////////////////////////////
// Parses a string into a time
void Time::ParseString(const String &data)
{
	// Break into an array of values
	array<size_t,4> values;
	size_t last = 0;
	size_t len = data.Length();
	size_t curIndex = 0;
	char cur = 0;
	for (size_t i=0;i<len;i++) {
		cur = data[i];
		if (cur == ':' || cur == '.' || cur == ',' || cur == ';') {
			values.at(curIndex++) = data.SubToInteger(last,i);
			last = i+1;
		}
		if (i == len-1) {
			int value = data.SubToInteger(last,len);
			size_t digits = len - last;
			if (digits == 2) value *= 10;
			if (digits == 1) value *= 100;
			values.at(curIndex++) = value;
		}
	}

	// Turn into milliseconds
	size_t mult[] = { 0, 1, 1000, 60000, 3600000 };
	size_t accum = 0;
	for (int i=(int)curIndex;--i>=0;) {
		accum += values[i] * mult[curIndex-i];
	}
	
	// Set
	SetMS((int)accum);
}
