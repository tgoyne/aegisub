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
#include "subtitle_format_writer.h"
#include "subtitle_format_ass.h"
#include "subtitle_format_srt.h"
#include "subtitle_format_txt.h"
#include "ass_file.h"


///////////////
// Constructor
SubtitleFormatWriter::SubtitleFormatWriter() {
	Line = NULL;
	Register();
}


//////////////
// Destructor
SubtitleFormatWriter::~SubtitleFormatWriter() {
	Remove();
}


//////////////
// Set target
void SubtitleFormatWriter::SetTarget(AssFile *file) {
	if (!file) Line = NULL;
	else Line = &file->Line;
	assFile = file;
}


////////
// List
std::list<SubtitleFormatWriter*> SubtitleFormatWriter::writers;
bool SubtitleFormatWriter::loaded = false;


/////////////////////////////
// Get an appropriate writer
SubtitleFormatWriter *SubtitleFormatWriter::GetWriter(wxString filename) {
	LoadWriters();
	std::list<SubtitleFormatWriter*>::iterator cur;
	SubtitleFormatWriter *writer;
	for (cur=writers.begin();cur!=writers.end();cur++) {
		writer = *cur;
		if (writer->CanWriteFile(filename)) return writer;
	}
	return NULL;
}


////////////
// Register
void SubtitleFormatWriter::Register() {
	std::list<SubtitleFormatWriter*>::iterator cur;
	for (cur=writers.begin();cur!=writers.end();cur++) {
		if (*cur == this) return;
	}
	writers.push_back(this);
}


//////////
// Remove
void SubtitleFormatWriter::Remove() {
	std::list<SubtitleFormatWriter*>::iterator cur;
	for (cur=writers.begin();cur!=writers.end();cur++) {
		if (*cur == this) {
			writers.erase(cur);
			return;
		}
	}
}


///////////////
// Add loaders
void SubtitleFormatWriter::LoadWriters () {
	if (!loaded) {
		//new ASSSubtitleFormatWriter();
		//new SRTSubtitleFormatWriter();
	}
	loaded = true;
}


///////////////////
// Destroy loaders
void SubtitleFormatWriter::DestroyWriters () {
	SubtitleFormatWriter *writer;
	std::list<SubtitleFormatWriter*>::iterator cur,next;
	for (cur=writers.begin();cur!=writers.end();cur = next) {
		next = cur;
		next++;
		writer = *cur;
		writers.erase(cur);
		delete writer;
	}
	writers.clear();
}
