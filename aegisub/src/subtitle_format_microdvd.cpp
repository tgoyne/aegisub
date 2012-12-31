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

/// @file subtitle_format_microdvd.cpp
/// @brief Reading/writing MicroDVD subtitle format (.SUB)
/// @ingroup subtitle_io
///

#include "config.h"

#include "subtitle_format_microdvd.h"

#include "ass_dialogue.h"
#include "ass_file.h"
#include "ass_time.h"
#include "compat.h"
#include "text_file_reader.h"
#include "text_file_writer.h"
#include "video_context.h"

#include <libaegisub/of_type_adaptor.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/format.hpp>

#include <wx/regex.h>

MicroDVDSubtitleFormat::MicroDVDSubtitleFormat()
: SubtitleFormat("MicroDVD")
{
}

std::vector<std::string> MicroDVDSubtitleFormat::GetReadWildcards() const {
	std::vector<std::string> formats;
	formats.push_back("sub");
	return formats;
}

std::vector<std::string> MicroDVDSubtitleFormat::GetWriteWildcards() const {
	return GetReadWildcards();
}

bool MicroDVDSubtitleFormat::CanReadFile(std::string const& filename) const {
	// Return false immediately if extension is wrong
	if (!boost::iends_with(filename, ".sub")) return false;

	// Since there is an infinity of .sub formats, load first line and check if it's valid
	TextFileReader file(filename);
	if (file.HasMoreLines()) {
		wxRegEx exp("^[\\{\\[]([0-9]+)[\\}\\]][\\{\\[]([0-9]+)[\\}\\]](.*)$", wxRE_ADVANCED);
		return exp.Matches(to_wx(file.ReadLineFromFile()));
	}

	return false;
}

void MicroDVDSubtitleFormat::ReadFile(AssFile *target, std::string const& filename, std::string const& encoding) const {
	TextFileReader file(filename, encoding);
	wxRegEx exp("^[\\{\\[]([0-9]+)[\\}\\]][\\{\\[]([0-9]+)[\\}\\]](.*)$", wxRE_ADVANCED);

	target->LoadDefault(false);

	agi::vfr::Framerate fps;

	bool isFirst = true;
	while (file.HasMoreLines()) {
		wxString line = to_wx(file.ReadLineFromFile());
		if (exp.Matches(line)) {
			long f1, f2;
			exp.GetMatch(line, 1).ToLong(&f1);
			exp.GetMatch(line, 2).ToLong(&f2);
			wxString text = exp.GetMatch(line, 3);

			// If it's the first, check if it contains fps information
			if (isFirst) {
				isFirst = false;

				if (f1 == 1 && f2 == 1) {
					// Convert fps
					double cfr;
					if (text.ToDouble(&cfr)) {
						fps = cfr;
						continue;
					}
				}

				// If it wasn't an fps line, ask the user for it
				fps = AskForFPS(true, false);
				if (!fps.IsLoaded()) return;
			}

			text.Replace("|", "\\N");

			AssDialogue *diag = new AssDialogue;
			diag->Start = fps.TimeAtFrame(f1, agi::vfr::START);
			diag->End = fps.TimeAtFrame(f2, agi::vfr::END);
			diag->Text = from_wx(text);
			target->Line.push_back(*diag);
		}
	}
}

void MicroDVDSubtitleFormat::WriteFile(const AssFile *src, std::string const& filename, std::string const& encoding) const {
	agi::vfr::Framerate fps = AskForFPS(true, false);
	if (!fps.IsLoaded()) return;

	AssFile copy(*src);
	copy.Sort();
	StripComments(copy);
	RecombineOverlaps(copy);
	MergeIdentical(copy);
	StripTags(copy);
	ConvertNewlines(copy, "|");

	TextFileWriter file(filename, encoding);

	// Write FPS line
	if (!fps.IsVFR())
		file.WriteLineToFile(str(boost::format("{1}{1}%.6f") % fps.FPS()));

	// Write lines
	for (auto current : copy.Line | agi::of_type<AssDialogue>()) {
		int start = fps.FrameAtTime(current->Start, agi::vfr::START);
		int end = fps.FrameAtTime(current->End, agi::vfr::END);

		file.WriteLineToFile(str(boost::format("{%i}{%i}%s") % start % end % current->Text.get()));
	}
}
