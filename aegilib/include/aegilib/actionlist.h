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
#include <list>
#include "action.h"
#include "gorgonstring.h"
#include "section_entry.h"

namespace Gorgonsub {

	// Prototypes
	class Controller;

	// ActionList class
	class ActionList {
		friend class Model;
		friend class Controller;

	private:
		String actionName;
		String owner;
		Model &model;
		std::list<ActionPtr> actions;
		bool valid;
		bool undoAble;

		ActionList(Model &model,const String actionName,const String owner,bool undoAble);
		void Start(const String actionName);
		void AddActionStart(const ActionPtr action);

	public:
		~ActionList();

		String GetName() const { return actionName; }
		String GetOwner() const { return owner; }

		void AddAction(const ActionPtr action);
		void Finish();

		void InsertLine(SectionEntryPtr line,int position=-1,const String section=L"");
		void RemoveLine(int position,const String section);
		SectionEntryPtr ModifyLine(int position,const String section);
	};
	typedef shared_ptr<ActionList> ActionListPtr;

}
