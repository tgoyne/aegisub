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


///////////////
// Blend modes
enum PRSBlendMode {
	BLEND_NORMAL = 0,
	BLEND_ADD,
	BLEND_SUBTRACT,
	BLEND_INVERSE_SUBTRACT,
	BLEND_MULTIPLY
};


///////////
// Headers
#include "prs_entry.h"
#include <vector>


/////////////////
// Display class
class PRSDisplay : public PRSEntry {
public:
	unsigned int start;		// First time to show this on (INCLUSIVE) (possible first frame?)
	unsigned int end;		// Last time to show this on (EXCLUSIVE) (possible last frame?)
	unsigned int id;		// ID of picture to be shown
	unsigned int layer;		// Number of layer to draw this on
	short x,y;				// X and Y coordinates to draw picture on
	unsigned char alpha;	// Alpha blend of picture
	unsigned char blend;	// Blend mode to use

	PRSDisplay();
	~PRSDisplay();

	PRSEntryType GetType() { return DISPLAY_ENTRY; }
	void WriteData(std::vector<char> &vec);
	void ReadData(std::vector<char> &vec);
};
