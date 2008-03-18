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

#pragma once
#include "athenastring.h"
#include "tr1.h"
#include "section_entry.h"

namespace Athenasub {
	// Prototypes
	class FormatHandler;
	class Model;

	// Format interface
	class Format {
	public:
		virtual ~Format() {}

		virtual String GetName() const = 0;
		virtual StringArray GetReadExtensions() const = 0;
		virtual StringArray GetWriteExtensions() const = 0;
		virtual shared_ptr<FormatHandler> GetHandler(Model &model) const = 0;

		virtual bool CanStoreText() const { return false; }
		virtual bool CanStoreImages() const { return false; }
		virtual bool CanUseTime() const { return false; }
		virtual bool CanUseFrames() const { return false; }

		virtual bool HasStyles() const { return false; }
		virtual bool HasMargins() const { return false; }
		virtual bool HasActors() const { return false; }
		virtual bool HasUserField() const { return false; }
		virtual String GetUserFieldName() const { return L""; }

		virtual int GetTimingPrecision() const { return 10; }	// In milliseconds
		virtual int GetMaxTime() const { return 36000000-10; }	// In milliseconds, default 9h 59min 59.99s

		virtual DialoguePtr CreateDialogue() const = 0;
		virtual StylePtr CreateStyle() const = 0;
	};
	typedef shared_ptr<Format> FormatPtr;

}
