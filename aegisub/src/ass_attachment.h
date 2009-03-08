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


#pragma once


///////////
// Headers
#include "ass_entry.h"
#include "boost/shared_ptr.hpp"
#include <vector>


///////////
// Typedef
typedef std::vector<unsigned char> DataVec;


///////////////////
// Attachment data
class AttachData {
private:
	DataVec data;
	wxString buffer;

public:
	AttachData();
	~AttachData();

	DataVec &GetData();
	void AddData(wxString data);
	void Finish();
};


//////////////////////////////
// Class to store attachments
class AssAttachment : public AssEntry {
private:
	boost::shared_ptr<AttachData> data;
	wxString filename;

public:
	const DataVec &GetData();

	void AddData(wxString data);
	void Finish();

	void Extract(wxString filename);
	void Import(wxString filename);
	wxString GetFileName(bool raw=false);

	const wxString GetEntryData();
	ASS_EntryType GetType() { return ENTRY_ATTACHMENT; }
	AssEntry *Clone();

	AssAttachment(wxString name);
	~AssAttachment();
};
