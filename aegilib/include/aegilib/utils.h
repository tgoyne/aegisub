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
// AEGISUB/GORGONSUB
//
// Website: http://www.aegisub.net
// Contact: mailto:amz@aegisub.net
//


#pragma once
#include "Gorgonsub.h"


/////////////
// Templates

// Returns the largest of two values
template <typename T>
T Max(T a,T b)
{
	if (b < a) return a;
	return b;
}

// Returns the smallest of two values
template <typename T>
T Min(T a,T b)
{
	if (a < b) return a;
	return b;
}

// Returns b, but limiting it to the interval [a,c]
template <typename T>
T Mid(T a,T b,T c)
{
	return Min(Max(a,b),c);
}


////////////////////
// Helper functions
namespace Gorgonsub {

	// Convert a string to an integer
	int StringToInt(const String &str);
	int SubStringToInteger(const String &str,size_t start,size_t end);

	// Number to string functions
	String PrettyFloat(String src);
	String PrettyFloatF(float src);
	String PrettyFloatD(double src);
	String FloatToString(double value);
	String IntegerToString(int value);
	String PrettySize(int bytes);

	// Fast string functions
	inline void WriteText(wxChar *&dst,const wxChar *src,size_t len,size_t &pos) {
		memcpy(dst,src,len*sizeof(wxChar));
		dst += len;
		pos += len;
	}
	inline void WriteChar(wxChar *&dst,const wxChar &src,size_t &pos) {
		*dst = src;
		dst++;
		pos++;
	}
	void WriteNumber(wxChar *&dst,wxChar *temp,int number,int pad,size_t &pos);
	const wxChar *StringTrim(wxString &str,size_t start);
};
