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


#pragma once


///////////
// Headers
#include <wx/wxprec.h>


/////////////////////////////
// Class for Ass format time
class AssTime {
private:
	int time;		// Miliseconds

public:
	static bool UseMSPrecision;

	AssTime();

	int GetMS();						// Returns miliseconds
	void SetMS(int ms);					// Sets values to miliseconds
	void ParseASS(const wxString text);	// Sets value to text-form time, in ASS format
	void ParseSRT(const wxString text);	// Sets value to text-form time, in SRT format
	wxString GetASSFormated();			// Returns the ASS representation of time
	wxString GetSRTFormated();			// Returns the SRT representation of time
	void UpdateFromTextCtrl(wxTextCtrl *ctrl);	// Reads value from text control and updates both
};

// Comparison operators
bool operator == (AssTime &t1, AssTime &t2);
bool operator != (AssTime &t1, AssTime &t2);
bool operator < (AssTime &t1, AssTime &t2);
bool operator > (AssTime &t1, AssTime &t2);
bool operator <= (AssTime &t1, AssTime &t2);
bool operator >= (AssTime &t1, AssTime &t2);
