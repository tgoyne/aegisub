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

#include "resolution_resampler.h"

#include "ass_dialogue.h"
#include "ass_file.h"
#include "ass_style.h"
#include "utils.h"

#include <libaegisub/ass/resample.h>
#include <libaegisub/of_type_adaptor.h>
#include <libaegisub/split.h>
#include <libaegisub/util.h>

#include <algorithm>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <functional>

using namespace agi;

namespace {
	void resample_tags(std::string const& name, AssOverrideParameter *cur, void *ud) {
		ResampleState *state = static_cast<ResampleState *>(ud);

		double resizer = 1.0;
		int shift = 0;

		switch (cur->classification) {
			case AssParameterClass::ABSOLUTE_SIZE:
				resizer = state->scale_x;
				break;

			case AssParameterClass::ABSOLUTE_POS_X:
				resizer = state->scale_x;
				shift = state->margin[ResampleState::LEFT];
				break;

			case AssParameterClass::ABSOLUTE_POS_Y:
				resizer = state->scale_y;
				shift = state->margin[ResampleState::TOP];
				break;

			case AssParameterClass::RELATIVE_SIZE_X:
				resizer = state->ar;
				break;

			case AssParameterClass::RELATIVE_SIZE_Y:
				break;

			case AssParameterClass::DRAWING:
				 cur->Set(ResampleDrawing(cur->Get<std::string>(), *state));
				return;

			default:
				return;
		}

		VariableDataType curType = cur->GetType();
		if (curType == VariableDataType::FLOAT)
			cur->Set((cur->Get<double>() + shift) * resizer);
		else if (curType == VariableDataType::INT)
			cur->Set<int>((cur->Get<int>() + shift) * resizer + 0.5);
	}

	template<typename Array>
	void resample_margin(Array& margin, ResampleState const& state) {
		for (size_t i = 0; i < 3; ++i)
			margin[i] = int((margin[i] + state.margin[i]) * (i < 2 ? state.scale_x : state.scale_y) + 0.5);
	}

	void resample_line(ResampleState *state, AssEntry &line) {
		AssDialogue *diag = dynamic_cast<AssDialogue*>(&line);
		if (diag && !(diag->Comment && (boost::starts_with(diag->Effect.get(), "template") || boost::starts_with(diag->Effect.get(), "code")))) {
			boost::ptr_vector<AssDialogueBlock> blocks(diag->ParseTags());

			for (auto block : blocks | of_type<AssDialogueBlockOverride>())
				block->ProcessParameters(resample_tags, state);

			for (auto drawing : blocks | of_type<AssDialogueBlockDrawing>())
				drawing->text = ResampleDrawing(drawing->text, *state);

			resample_margin(diag->Margin, *state);

			diag->UpdateText(blocks);
		}
		else if (AssStyle *style = dynamic_cast<AssStyle*>(&line)) {
			style->fontsize = int(style->fontsize * state->scale_y + 0.5);
			style->outline_w *= state->scale_y;
			style->shadow_w *= state->scale_y;
			style->spacing *= state->scale_x;
			style->scalex *= state->ar;
			resample_margin(style->Margin, *state);
			style->UpdateData();
		}
	}
}

void ResampleResolution(AssFile *ass, ResampleSettings settings) {
	// Add margins to original resolution
	settings.source_x += settings.margin[ResampleState::LEFT] + settings.margin[ResampleState::RIGHT];
	settings.source_y += settings.margin[ResampleState::TOP] + settings.margin[ResampleState::BOTTOM];

	ResampleState state = {
		settings.margin,
		double(settings.dest_x) / double(settings.source_x),
		double(settings.dest_y) / double(settings.source_y),
		1.0
	};

	if (settings.change_ar)
		state.ar = state.scale_x / state.scale_y;

	for (auto& line : ass->Line)
		resample_line(&state, line);

	ass->SetScriptInfo("PlayResX", std::to_string(settings.dest_x));
	ass->SetScriptInfo("PlayResY", std::to_string(settings.dest_y));

	ass->Commit(_("resolution resampling"), AssFile::COMMIT_SCRIPTINFO | AssFile::COMMIT_DIAG_FULL);
}
