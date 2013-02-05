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
// Aegisub Project http://www.aegisub.org/

/// @file ass_attachment.h
/// @see ass_attachment.cpp
/// @ingroup subs_storage
///

#include "ass_entry.h"

#include <libaegisub/fs_fwd.h>

#include <memory>
#include <vector>

/// @class AssAttachment
class AssAttachment : public AssEntry {
	/// Decoded file data
	std::shared_ptr<std::vector<char>> data;

	/// Encoded data which has been read from the script but not yet decoded
	std::vector<char> buffer;

	/// Name of the attached file, with SSA font mangling if it is a ttf
	std::string filename;

	AssEntryGroup group;

public:
	/// Get the size of the attached file in bytes
	size_t GetSize() const { return data->size(); }

	/// Add a line of data (without newline) read from a subtitle file to the
	/// buffer waiting to be decoded
	void AddData(std::string const& data) { buffer.insert(buffer.end(), data.begin(), data.end()); }
	/// Decode all data passed with AddData
	void Finish();

	/// Extract the contents of this attachment to a file
	/// @param filename Path to save the attachment to
	void Extract(agi::fs::path const& filename) const;

	/// Get the name of the attached file
	/// @param raw If false, remove the SSA filename mangling
	std::string GetFileName(bool raw=false) const;

	const std::string GetEntryData() const override;
	AssEntryGroup Group() const override { return group; }
	AssEntry *Clone() const override;

	AssAttachment(std::string const& name, AssEntryGroup group);
	AssAttachment(agi::fs::path const& name, AssEntryGroup group);
};
