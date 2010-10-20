// Copyright (c) 2010, Thomas Goyne <plorkyeran@aegisub.org>
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
//
// $Id$

/// @file font_file_lister.cpp
/// @brief Base-class for font collector implementations
/// @ingroup font_collector
///

#include "config.h"

#include "ass_dialogue.h"
#include "ass_override.h"
#include "ass_style.h"

#ifdef WITH_FREETYPE2
#include "font_file_lister_freetype.h"
#define FontListerClass FreetypeFontFileLister
#else
#include "font_file_lister.h"
#endif

namespace std { using namespace std::tr1; }
using namespace std::placeholders;

std::vector<wxString> FontFileLister::GetFontPaths(std::list<AssEntry*> const& file, std::tr1::function<void (wxString, int)> status) {
	/// @todo make this choosable at runtime
#ifdef FontListerClass
	FontFileLister *inst = new FontListerClass(status);
	return inst->Run(file);
#else
	return std::vector<wxString>();
#endif
}

void FontFileLister::ProcessDialogueLine(AssDialogue *line) {
	if (line->Comment) return;

	line->ParseASSTags();
	bool text = false;
	StyleInfo style = styles[line->Style];
	StyleInfo initial = style;

	for (size_t i = 0; i < line->Blocks.size(); ++i) {
		if (AssDialogueBlockOverride *ovr = dynamic_cast<AssDialogueBlockOverride *>(line->Blocks[i])) {
			for (size_t j = 0; j < ovr->Tags.size(); ++j) {
				AssOverrideTag *tag = ovr->Tags[j];
				StyleInfo newStyle = style;
				wxString name = tag->Name;

				if (name == L"\\r") {
					newStyle = styles[tag->Params[0]->Get(line->Style)];
				}
				if (name == L"\\b") {
					newStyle.bold = tag->Params[0]->Get(initial.bold);
				}
				else if (name == L"\\i") {
					newStyle.italic = tag->Params[0]->Get(initial.italic);
				}
				else if (name == L"\\fn") {
					newStyle.facename = tag->Params[0]->Get(initial.facename);
				}
				else {
					continue;
				}
				if (text) {
					dialogueChunks.push_back(style);
					text = false;
				}
				style = newStyle;
			}
		}
		else if (AssDialogueBlockPlain *txt = dynamic_cast<AssDialogueBlockPlain *>(line->Blocks[i])) {
			text = text || !txt->GetText().empty();
		}
		// Do nothing with drawing blocks
	}
	if (text) {
		dialogueChunks.push_back(style);
	}
	line->ClearBlocks();
}

void FontFileLister::ProcessChunk(StyleInfo const& style) {
	if (results.find(style) != results.end()) return;

	wxString path = GetFontPath(style.facename, style.bold, style.italic);
	results[style] = path;

	if (path.empty()) {
		statusCallback(wxString::Format("Could not find font '%s'\n", style.facename), 2);
	}
	else {
		statusCallback(wxString::Format("Found '%s' at '%s'\n", style.facename, path), 0);
	}
}


std::vector<wxString> FontFileLister::Run(std::list<AssEntry*> const& file) {
	statusCallback(_("Parsing file\n"), 0);
	for (std::list<AssEntry*>::const_iterator cur = file.begin(); cur != file.end(); ++cur) {
		switch ((*cur)->GetType()) {
			case ENTRY_STYLE: {
				AssStyle *style = static_cast<AssStyle*>(*cur);
				StyleInfo info;
				info.facename = style->font;
				info.bold     = style->bold;
				info.italic   = style->italic;
				styles[style->name] = info;
				break;
			}
			case ENTRY_DIALOGUE:
				ProcessDialogueLine(static_cast<AssDialogue*>(*cur));
				break;
			default: break;
		}
	}

	if (dialogueChunks.empty()) {
		statusCallback(_("No non-empty dialogue lines found"), 2);
		return std::vector<wxString>();
	}

	statusCallback(_("Searching for font files\n"), 0);
	std::for_each(dialogueChunks.begin(), dialogueChunks.end(), std::bind(&FontFileLister::ProcessChunk, this, _1));
	statusCallback(_("Done\n\n"), 0);

	std::vector<wxString> paths;
	paths.reserve(results.size());
	int missing = 0;
	for (std::map<StyleInfo, wxString>::const_iterator cur = results.begin(); cur != results.end(); ++cur) {
		if (cur->second.empty()) {
			++missing;
		}
		else {
			paths.push_back(cur->second);
		}
	}

	if (missing == 0) {
		statusCallback(_("All fonts found.\n"), 1);
	}
	else {
		statusCallback(wxString::Format(_("%d fonts could not be found.\n"), missing), 2);
	}

	return paths;
}

bool FontFileLister::StyleInfo::operator<(StyleInfo const& rgt) const {
#define CMP(field) \
	if (field < rgt.field) return true; \
	if (field > rgt.field) return false

	CMP(facename);
	CMP(bold);
	CMP(italic);

#undef CMP

	return false;
}
