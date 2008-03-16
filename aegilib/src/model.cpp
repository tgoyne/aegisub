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

#include "Gorgonsub.h"
using namespace Gorgonsub;


/////////////////////////////////////////////////////////
// Adds a listener to be notified whenever things change
void Model::AddListener(ViewPtr listener)
{
	wxASSERT(listener);
	listeners.push_back(listener);
}


////////////////////////
// Notify all listeners
void Model::DispatchNotifications(const Notification &notification) const
{
	for (ViewList::const_iterator cur=listeners.begin();cur!=listeners.end();cur++) {
		(*cur)->Notify(notification);
	}
}


////////////////////////////
// Processes an action list
void Model::ProcessActionList(const ActionList &_actionList,int type)
{
	// Copy the list
	ActionListPtr actions = ActionListPtr(new ActionList(_actionList));

	// Setup undo
	ActionListPtr undo = ActionListPtr(new ActionList(actions->model,actions->actionName,actions->owner,actions->undoAble));
	ActionStack *stack;
	if (type == 1) stack = &redoStack;
	else stack = &undoStack;

	// Execute actions
	std::list<ActionPtr>::const_iterator cur;
	for (cur=actions->actions.begin();cur!=actions->actions.end();cur++) {
		// Inserts the opposite into the undo action first
		if (actions->undoAble) undo->AddActionStart((*cur)->GetAntiAction(*this));
		
		// Execute the action itself
		(*cur)->Execute(*this);
	}

	// Insert into undo stack
	if (actions->undoAble) {
		stack->push_back(undo);
		if (type == 0) redoStack.clear();
	}

	// Notify listeners
	DispatchNotifications(Notification());
}


//////////////////
// Load subtitles
void Model::Load(wxInputStream &input,const FormatPtr _format,const String encoding)
{
	// Autodetect format
	if (!_format) {
		// TODO

		// No format found
		throw Exception(Exception::No_Format_Handler);
	}

	// Get handler
	FormatHandlerPtr handler = _format->GetHandler(*this);
	if (!handler) throw Exception(Exception::No_Format_Handler);

	// Clear the model first
	Clear();

	// Load
	handler->Load(input,encoding);

	// Set the format
	format = _format;
}


//////////////////
// Save subtitles
void Model::Save(wxOutputStream &output,const FormatPtr _format,const String encoding)
{
	// Use another format
	if (_format && _format != format) {
		// TODO
		throw Exception(Exception::TODO);
	}

	// Get handler
	FormatHandlerPtr handler = format->GetHandler(*this);
	if (!handler) throw Exception(Exception::No_Format_Handler);

	// Load
	handler->Save(output,encoding);
}


/////////////////////////
// Inserts a new section
void Model::AddSection(String name)
{
	SectionPtr prev = GetSection(name);
	if (prev) throw Exception(Exception::Section_Already_Exists);
	sections.push_back(SectionPtr(new Section(name)));
}


//////////////////
// Gets a section
SectionPtr Model::GetSection(String name) const
{
	size_t len = sections.size();
	for (size_t i=0;i<len;i++) {
		if (sections[i]->GetName() == name) return sections[i];
	}
	return SectionPtr();
}


////////////////////////
// Get section by index
SectionPtr Model::GetSectionByIndex(size_t index) const
{
	return sections.at(index);
}


/////////////////////
// Get section count
size_t Model::GetSectionCount() const
{
	return sections.size();
}


//////////
// Clear
void Model::Clear()
{
	sections.clear();
	undoStack.clear();
	redoStack.clear();
}


//////////////////
// Can undo/redo?
bool Model::CanUndo(const String owner) const
{
	(void) owner;
	return undoStack.size() > 0;
}
bool Model::CanRedo(const String owner) const
{
	(void) owner;
	return redoStack.size() > 0;
}


///////////////////
// Perform an undo
void Model::Undo(const String owner)
{
	ActivateStack(undoStack,true,owner);
}


//////////////////
// Perform a redo
void Model::Redo(const String owner)
{
	ActivateStack(redoStack,false,owner);
}


/////////////////////
// Perform undo/redo
void Model::ActivateStack(ActionStack &stack,bool isUndo,const String &owner)
{
	// TODO: do something with this
	(void) owner;

	// Process list
	ProcessActionList(*stack.back(),isUndo?1:2);

	// Pop original
	stack.pop_back();
}


//////////////////////////
// Get undo/redo messages
String Model::GetUndoMessage(const String owner) const
{
	(void) owner;
	if (CanUndo()) return undoStack.back()->GetName();
	return L"";
}
String Model::GetRedoMessage(const String owner) const
{
	(void) owner;
	if (CanRedo()) return redoStack.back()->GetName();
	return L"";
}
