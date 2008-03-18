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

#include "Athenasub.h"
using namespace Athenasub;


///////////////
// Constructor
ActionList::ActionList(Model &_model,String _actionName,const String _owner,bool _undoAble)
: model(_model), owner(_owner), undoAble(_undoAble)
{
	valid = false;
	Start(_actionName);
}


//////////////
// Destructor
ActionList::~ActionList()
{
}


//////////////////////////////
// Add an action to the queue
void ActionList::AddAction(const ActionPtr action)
{
	if (!valid) THROW_ATHENA_EXCEPTION(Exception::Invalid_ActionList);
	actions.push_back(action);
	if (actions.size() > 2) {
		int a = 0;
		a++;
	}
}


///////////////////////////////////////////
// Add an action to the start of the queue
void ActionList::AddActionStart(const ActionPtr action)
{
	if (!valid) THROW_ATHENA_EXCEPTION(Exception::Invalid_ActionList);
	actions.push_front(action);
	if (actions.size() > 2) {
		int a = 0;
		a++;
	}
}


/////////////////////////////
// Starts performing actions
void ActionList::Start(const String name)
{
	if (valid) Finish();
	actionName = name;
	valid = true;
}


////////////////////////
// Ends the action list
void ActionList::Finish()
{
	if (valid) {
		model.ProcessActionList(*this);
		actions.clear();
		valid = false;
	}
}


//////////////////////////////////
// Create an "insert line" action
void ActionList::InsertLine(EntryPtr line,int position,const String section)
{
	ActionPtr action = ActionPtr (new ActionInsert(line,position,section));
	AddAction(action);
}


/////////////////////////////////
// Create a "remove line" action
void ActionList::RemoveLine(int position,const String section)
{
	ActionPtr action = ActionPtr (new ActionRemove(position,section));
	AddAction(action);
}


/////////////////////////////////
// Insert a "modify line" action
EntryPtr ActionList::ModifyLine(int position,const String section)
{
	SectionPtr sect = model.GetSection(section);
	EntryPtr entry = sect->GetEntry(position)->Clone();
	ActionPtr action = ActionPtr (new ActionModify(entry,position,section,false));
	AddAction(action);
	return entry;
}


////////////////////////////////////////
// Insert a "modify lines" batch action
std::vector<EntryPtr> ActionList::ModifyLines(Selection selection,const String section)
{
	// Get section
	SectionPtr sect = model.GetSection(section);

	// Generate entries
	std::vector<EntryPtr> entries(selection.GetCount());
	size_t len = selection.GetRanges();
	size_t n = 0;
	for (size_t i=0;i<len;i++) {
		size_t rLen = selection.GetLinesInRange(i);
		for (size_t j=0;j<rLen;j++) {
			entries[n++] = sect->GetEntry(selection.GetLineInRange(j,i))->Clone();
		}
	}

	// Generate the action
	ActionPtr action = ActionPtr (new ActionModifyBatch(entries,std::vector<VoidPtr>(),selection,section,false));
	AddAction(action);
	return entries;
}
