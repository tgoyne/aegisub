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
#include <list>
#include <vector>
#include <wx/wfstream.h>
#include "actionlist.h"
#include "section.h"
#include "api.h"

namespace Athenasub {

	// Prototypes
	class View;
	typedef shared_ptr<View> ViewPtr;
	class Notification;
	class Format;
	
	// Model class
	// Stores the subtitle data
	class Model {
		friend class FormatHandler;
		friend class ActionList;
		friend class Controller;
		friend class Action;

		typedef std::list<ViewPtr> ViewList;
		typedef std::list<ActionListPtr> ActionStack;
		typedef shared_ptr<Format> FormatPtr;
		typedef shared_ptr<Controller> ControllerPtr;

	private:
		std::vector<SectionPtr> sections;
		ActionStack undoStack;
		ActionStack redoStack;
		ViewList listeners;
		bool readOnly;
		FormatPtr format;

		void ProcessActionList(const ActionList &actionList,int type=0);

		String GetUndoMessage(const String owner=L"") const;
		String GetRedoMessage(const String owner=L"") const;
		bool CanUndo(const String owner=L"") const;
		bool CanRedo(const String owner=L"") const;
		void Undo(const String owner=L"");
		void Redo(const String owner=L"");
		void ActivateStack(ActionStack &stack,bool isUndo,const String &owner);

		void DispatchNotifications(const Notification &notification) const;

		void AddSection(String name);
		SectionPtr GetSection(String name) const;
		SectionPtr GetSectionByIndex(size_t index) const;
		size_t GetSectionCount() const;

		void Clear();
		void Load(wxInputStream &input,const FormatPtr format=FormatPtr(),const String encoding=L"");
		void Save(wxOutputStream &output,const FormatPtr format=FormatPtr(),const String encoding=L"UTF-8");

	public:
		ControllerPtr CreateController();

		const FormatPtr GetFormat() const { return format; }
		void AddListener(ViewPtr listener);
	};

	typedef shared_ptr<Model> ModelPtr;

}
