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

#include "subtitle_format_sty.h"

#include "ass_file.h"
#include "ass_style.h"

#include <libaegisub/fs.h>
#include <libaegisub/io.h>
#include <libaegisub/line_iterator.h>
#include <libaegisub/of_type_adaptor.h>

STYSubtitleFormat::STYSubtitleFormat()
: SubtitleFormat("Style Catalog")
{
}

std::vector<std::string> STYSubtitleFormat::GetReadWildcards() const {
	std::vector<std::string> formats;
	formats.push_back("sty");
	return formats;
}

std::vector<std::string> STYSubtitleFormat::GetWriteWildcards() const {
	return GetReadWildcards();
}

void STYSubtitleFormat::ReadFile(AssFile *target, agi::fs::path const& filename, std::string const&) const {
	try {
		std::unique_ptr<std::ifstream> in(agi::io::Open(filename));
		for (auto const& line : agi::line_iterator<std::string>(*in)) {
			try {
				target->InsertLine(new AssStyle(line));
			} catch(...) {
				/* just ignore invalid lines for now */
			}
		}
	}
	catch (agi::fs::FileNotAccessible const&) {
		// Just treat a missing file as an empty file
	}
}

void STYSubtitleFormat::WriteFile(const AssFile *src, agi::fs::path const& filename, std::string const&) const {
	agi::fs::CreateDirectory(filename.parent_path());

	agi::io::Save out(filename);
	out.Get() << "\xEF\xBB\xBF";

	for (auto style : src->Line | agi::of_type<AssStyle>())
		out.Get() << style->GetEntryData() << std::endl;
}
