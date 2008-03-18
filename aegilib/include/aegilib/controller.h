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
#include "format.h"

namespace Athenasub {
	
	// Prototypes
	class Model;
	class ActionList;
	typedef shared_ptr<ActionList> ActionListPtr;

	// Controller class
	class Controller {
	private:
		Model &model;

	public:
		Controller (Model &model);
		ActionListPtr CreateActionList(const String title,const String owner=L"",bool undoAble=true);

		void LoadFile(const String filename,const String encoding=L"");
		void SaveFile(const String filename,const String encoding=L"UTF-8");

		bool CanUndo(const String owner=L"") const;
		bool CanRedo(const String owner=L"") const;
		void Undo(const String owner=L"");
		void Redo(const String owner=L"");

		DialoguePtr CreateDialogue() const;
		StylePtr CreateStyle() const;

		DialogueConstPtr GetDialogue(size_t n) const;
		DialogueConstPtr GetStyle(size_t n) const;
		StyleConstPtr GetStyle(String name) const;
		EntryConstPtr GetEntry(size_t n,String section) const;

		const FormatPtr GetFormat() const;
	};

}
