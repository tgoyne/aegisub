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
#include "exception.h"
#include "colour.h"
#include "section_entry.h"


namespace Gorgonsub {

	// Style class
	class Style : public Entry {
	private:
		static const bool dodgeWarning = true;
		void ThrowUnsupported() const { if (dodgeWarning) THROW_GORGON_EXCEPTION(Exception::Unsupported_Format_Feature); }

	public:
		// Destructor
		virtual ~Style() {}

		// Type
		SectionEntryType GetType() const { return SECTION_ENTRY_STYLE; }
		Style *GetAsStyle() { return this; }

		// Read accessors
		virtual String GetName() const=0;
		virtual String GetFontName() const { ThrowUnsupported(); return L""; }
		virtual float GetFontSize() const { ThrowUnsupported(); return 0.0f; }
		virtual Colour GetColour(int n) const { (void) n; ThrowUnsupported(); return Colour(); }
		virtual int GetMargin(int n) const { (void) n; ThrowUnsupported(); return 0; }
	};

}
