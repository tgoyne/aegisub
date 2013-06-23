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

#include "line_utils.h"

#include "ass_dialogue.h"
#include "ass_file.h"
#include "ass_style.h"

#include <libaegisub/of_type_adaptor.h>

const std::vector<AssOverrideParameter> *FindTag(boost::ptr_vector<AssDialogueBlock>& blocks, std::string const& tag_name) {
	for (auto ovr : blocks | agi::of_type<AssDialogueBlockOverride>()) {
		for (auto const& tag : ovr->Tags) {
			if (tag.Name == tag_name)
				return &tag.Params;
		}
	}

	return nullptr;
}

int GetAlign(AssDialogue const* diag, AssFile const& file) {
	boost::ptr_vector<AssDialogueBlock> blocks(diag->ParseTags());

	int ovr_align = 0;
	if (auto align_tag = FindTag(blocks, "\\an"))
		ovr_align = (*align_tag)[0].Get<int>(ovr_align);
	else if ((align_tag = FindTag(blocks, "\\a")))
		ovr_align = AssStyle::SsaToAss((*align_tag)[0].Get<int>(2));

	if (ovr_align > 0 && ovr_align <= 9)
		return ovr_align;

	if (auto style = file.GetStyle(diag->Style))
		return style->alignment;

	return 2;
}
