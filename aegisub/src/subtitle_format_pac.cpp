// Copyright (c) 2013, Thomas Goyne <plorkyeran@aegisub.org>
//
// Permission to use, copy, modify, and distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//
// Aegisub Project http://www.aegisub.org/

#include "config.h"

#include "subtitle_format_pac.h"

#include "ass_dialogue.h"
#include "ass_file.h"
#include "line_utils.h"

#include <libaegisub/io.h>
#include <libaegisub/of_type_adaptor.h>
#include <libaegisub/vfr.h>

#include <boost/range/irange.hpp>

namespace {
	enum class pac_encoding {
		none
	};

	// Detect which encoding can be used for the file
	pac_encoding detect_encoding(EntryList const& lines) {
		return pac_encoding::none;
	}

	int get_align(AssDialogue const* diag, AssFile const& file) {
	}

	int get_line_count(AssDialogue const* diag) {
		return 0;
	}

	void write_line(std::ostream& out, AssDialogue const* diag) {
		write_timecode(out, diag->Start);
		write_timecode(out, diag->End);
	}

	void write_timecode(std::ostream& out, AssTime time, agi::vfr::Framerate const& fps) {
		int smpte[4];
		fps.SmpteAtTime(time, smpte, smpte+1, smpte+2, smpte+3);
		for (auto ts : smpte) {
			out.put((ts & 0xFF00) >> 8);
			out.put(ts & 0xFF);
		}
	}

	struct pac_line {
		bool large_font;
		int horz_align;
		int vert_position;
		std::string text;
	};

	pac_line convert_line(AssDialogue const* diag, AssFile const& file) {
		pac_line line;
		int align = GetAlign(diag, file);
		line.horz_align = align % 3; // right: 0, left: 1, center: 2
		line.vert_position = 0;
		if (align < 7) {
			int lines = get_line_count(diag);
			if (align < 4)
				line.vert_position = 12 - lines;
			else
				line.vert_position = 6 - (lines + 1) / 2;
		}

		bool italic = false;
		boost::ptr_vector<AssDialogueBlock> blocks(diag->ParseTags());
		for (auto const& block : blocks) {
			if (auto plain = dynamic_cast<const AssDialogueBlockPlain*>(&block))
				line.text += plain->GetText();
			else if (auto ovr = dynamic_cast<const AssDialogueBlockOverride*>(&block)) {

			}
		}
	}
}

PACSubtitleFormat::PACSubtitleFormat()
: SubtitleFormat("PAC (Screen Electrons Polistream)")
{
}

std::vector<std::string> PACSubtitleFormat::GetReadWildcards() const {
	std::vector<std::string> formats;
	formats.push_back("pac");
	return formats;
}

std::vector<std::string> PACSubtitleFormat::GetWriteWildcards() const {
	return GetReadWildcards();
}

void PACSubtitleFormat::ReadFile(AssFile *target, agi::fs::path const& filename, std::string const& encoding) const {
}

void PACSubtitleFormat::WriteFile(const AssFile *src, agi::fs::path const& filename, std::string const& encoding) const {
	pac_encoding encoding = detect_encoding(src->Line);
	agi::io::Save save(filename);
	auto& out = save.Get();

	AssFile copy(*src);
	copy.Sort();
	StripComments(copy);
	RecombineOverlaps(copy);
	MergeIdentical(copy);
#ifdef _WIN32
	ConvertNewlines(copy, "\r\n", false);
#else
	ConvertNewlines(copy, "\n", false);
#endif

	// Write header
	out.put(1);
	for (auto i : boost::irange(0, 23))
		out.put(0);
	out.put(60);

	for (auto current : copy.Line | agi::of_type<AssDialogue>())
		write_line(out, current);

	// Write footer
	out.put(0xFF);
	for (auto i : boost::irange(0, 11))
		out.put(0);
	out.put(0x11);
	out.put(0);
	write_text(out, "dummy end of file");
}

bool PACSubtitleFormat::CanSave(const AssFile *file) const {
	return true;
}
