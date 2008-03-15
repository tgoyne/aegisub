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

#include "gorgonsub.h"
using namespace Gorgonsub;


////////////////////////////////
// Get a section from the model
SectionPtr Action::GetSection(const Model &model,const String &name) const
{
	return model.GetSection(name);
}


/////////////////// Insert line /////////////////////////////

///////////////
// Constructor
ActionInsert::ActionInsert(shared_ptr<SectionEntry> data,int line,const String &sName)
: entry(data), lineNumber(line), section(sName) {}


/////////////////////////////////
// Create anti-action for insert
ActionPtr ActionInsert::GetAntiAction(const Model &model) const
{
	(void) model;
	String sect = section;
	if (section.IsEmpty()) sect = entry->GetDefaultGroup();
	return ActionPtr(new ActionRemove(lineNumber,sect));
}


/////////////////////
// Execute insertion
void ActionInsert::Execute(Model &model)
{
	// Find the section to insert it on
	String sectionName = section;
	if (sectionName.IsEmpty()) sectionName = entry->GetDefaultGroup();
	SectionPtr sect = GetSection(model,sectionName);

	// Insert the line
	sect->AddEntry(entry,lineNumber);
}



/////////////////// Remove line /////////////////////////////


///////////////
// Constructor
ActionRemove::ActionRemove(int line,const String &sName)
: lineNumber(line), section(sName) {}


/////////////////////////////////
// Create anti-action for remove
ActionPtr ActionRemove::GetAntiAction(const Model &model) const
{
	SectionPtr sect = GetSection(model,section);
	SectionEntryPtr entry = sect->GetEntry(lineNumber);
	return ActionPtr(new ActionInsert(entry,lineNumber,section));
}


///////////////////
// Execute removal
void ActionRemove::Execute(Model &model)
{
	// Find the section to remote it from
	String sect = section;
	if (sect.IsEmpty()) throw Exception(Exception::TODO); // TODO
	SectionPtr section = GetSection(model,sect);

	// Remove the line
	section->RemoveEntryByIndex(lineNumber);
}
