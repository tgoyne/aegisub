// Copyright (c) 2006, Rodrigo Braz Monteiro
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


///////////
// Headers
#include "subtitle_format.h"
#include "subtitle_format_ass.h"
#include "subtitle_format_srt.h"
#include "subtitle_format_txt.h"
#include "subtitle_format_prs.h"
#include "ass_file.h"


///////////////
// Constructor
SubtitleFormat::SubtitleFormat() {
	Line = NULL;
	Register();
	isCopy = false;
}


//////////////
// Destructor
SubtitleFormat::~SubtitleFormat () {
	Remove();
}


////////
// List
std::list<SubtitleFormat*> SubtitleFormat::formats;
bool SubtitleFormat::loaded = false;


//////////////
// Set target
void SubtitleFormat::SetTarget(AssFile *file) {
	ClearCopy();
	if (!file) Line = NULL;
	else Line = &file->Line;
	assFile = file;
}


///////////////
// Create copy
void SubtitleFormat::CreateCopy() {
	SetTarget(new AssFile(*assFile));
	isCopy = true;
}


//////////////
// Clear copy
void SubtitleFormat::ClearCopy() {
	if (isCopy) {
		delete assFile;
		assFile = NULL;
		isCopy = false;
	}
}


///////////////////
// Clear subtitles
void SubtitleFormat::Clear() {
	assFile->Clear();
}


////////////////
// Load default
void SubtitleFormat::LoadDefault() {
	assFile->LoadDefault();
}


///////////////////
// Set if it's ASS
void SubtitleFormat::SetIsASS(bool isASS) {
	assFile->IsASS = isASS;
}


////////////
// Add line
int SubtitleFormat::AddLine(wxString data,wxString group,int lasttime,bool &IsSSA) {
	return assFile->AddLine(data,group,lasttime,IsSSA);
}


///////////////
// Add formats
void SubtitleFormat::LoadFormats () {
	if (!loaded) {
		new ASSSubtitleFormat();
		new SRTSubtitleFormat();
		new TXTSubtitleFormat();
		new PRSSubtitleFormat();
	}
	loaded = true;
}


///////////////////
// Destroy formats
void SubtitleFormat::DestroyFormats () {
	std::list<SubtitleFormat*>::iterator cur;
	for (cur=formats.begin();cur!=formats.end();cur = formats.begin()) {
		delete *cur;
	}
	formats.clear();
}


/////////////////////////////
// Get an appropriate reader
SubtitleFormat *SubtitleFormat::GetReader(wxString filename) {
	LoadFormats();
	std::list<SubtitleFormat*>::iterator cur;
	SubtitleFormat *reader;
	for (cur=formats.begin();cur!=formats.end();cur++) {
		reader = *cur;
		if (reader->CanReadFile(filename)) return reader;
	}
	return NULL;
}


/////////////////////////////
// Get an appropriate writer
SubtitleFormat *SubtitleFormat::GetWriter(wxString filename) {
	LoadFormats();
	std::list<SubtitleFormat*>::iterator cur;
	SubtitleFormat *writer;
	for (cur=formats.begin();cur!=formats.end();cur++) {
		writer = *cur;
		if (writer->CanWriteFile(filename)) return writer;
	}
	return NULL;
}


////////////
// Register
void SubtitleFormat::Register() {
	std::list<SubtitleFormat*>::iterator cur;
	for (cur=formats.begin();cur!=formats.end();cur++) {
		if (*cur == this) return;
	}
	formats.push_back(this);
}


//////////
// Remove
void SubtitleFormat::Remove() {
	std::list<SubtitleFormat*>::iterator cur;
	for (cur=formats.begin();cur!=formats.end();cur++) {
		if (*cur == this) {
			formats.erase(cur);
			return;
		}
	}
}
