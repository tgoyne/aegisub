// Copyright (c) 2005-2009, Rodrigo Braz Monteiro, Niels Martin Hansen
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


#pragma once


///////////
// Headers
#include <wx/menuitem.h>
#include <vector>
#include <utility>
#include <stdint.h>


////////////
// Typedefs
typedef std::vector<std::pair<int,int> > IntPairVector;


///////////////////////
// Function prototypes
#ifndef __LINUX__
int64_t abs64(int64_t input);
#endif
int CountMatches(wxString parent,wxString child);
wxString MakeRelativePath(wxString path,wxString reference);
wxString DecodeRelativePath(wxString path,wxString reference);
wxString PrettyFloat(wxString src);
wxString PrettyFloatF(float src);
wxString PrettyFloatD(double src);
wxString AegiFloatToString(double value);
wxString AegiIntegerToString(int value);
wxString PrettySize(int bytes);
wxMenuItem *AppendBitmapMenuItem (wxMenu* parentMenu,int id,wxString text,wxString help,wxBitmap bmp,int pos=-1);
int SmallestPowerOf2(int x);
void GetWordBoundaries(const wxString text,IntPairVector &results,int start=0,int end=-1);
bool IsWhitespace(wchar_t c);
bool StringEmptyOrWhitespace(const wxString &str);
int AegiStringToInt(const wxString &str,int start=0,int end=-1);
int AegiStringToFix(const wxString &str,size_t decimalPlaces,int start=0,int end=-1);
wxIcon BitmapToIcon(wxBitmap bmp);
void RestartAegisub();


//////////
// Macros
#ifndef MIN
#define MIN(a,b) ((a)<(b))?(a):(b)
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b))?(a):(b)
#endif

#ifndef MID
#define MID(a,b,c) MAX((a),MIN((b),(c)))
#endif

#ifndef FORCEINLINE
#ifdef __VISUALC__
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE inline
// __attribute__((always_inline)) gives me errors on g++ ~amz
#endif
#endif


///////////
// Inlines
static inline void IntSwap(int &a,int &b) {
	int c = a;
	a = b;
	b = c;
}


//////////////////////////
// Clamp integer to range
// Code taken from http://bob.allegronetwork.com/prog/tricks.html#clamp
static FORCEINLINE int ClampSignedInteger32(int x,int min,int max) {
	x -= min;
	x &= (~x) >> 31;
	x += min;
	x -= max;
	x &= x >> 31;
	x += max;
	return x;
}
