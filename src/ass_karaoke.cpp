// Copyright (c) 2011, Thomas Goyne <plorkyeran@aegisub.org>
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

#include "ass_karaoke.h"

#include "ass_dialogue.h"

#include <libaegisub/format.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>

std::string AssKaraoke::Syllable::GetText(bool k_tag) const {
	std::string ret;

	if (k_tag)
		ret = agi::format("{%s%d}", tag_type, ((duration + 5) / 10));

	size_t idx = 0;
	for (auto const& ovr : ovr_tags) {
		ret += text.substr(idx, ovr.first - idx);
		ret += ovr.second;
		idx = ovr.first;
	}
	ret += text.substr(idx);
	return ret;
}

AssKaraoke::AssKaraoke(const AssDialogue *line, bool auto_split, bool normalize) {
	if (line) SetLine(line, auto_split, normalize);
}

void AssKaraoke::SetLine(const AssDialogue *line, bool auto_split, bool normalize) {
	syls.clear();
	Syllable syl;
	syl.start_time = line->Start;
	syl.duration = 0;
	syl.tag_type = "\\k";

	ParseSyllables(line, syl);

	if (normalize) {
		// Normalize the syllables so that the total duration is equal to the line length
		int end_time = line->End;
		int last_end = syl.start_time + syl.duration;

		// Total duration is shorter than the line length so just extend the last
		// syllable; this has no effect on rendering but is easier to work with
		if (last_end < end_time)
			syls.back().duration += end_time - last_end;
		else if (last_end > end_time) {
			// Truncate any syllables that extend past the end of the line
			for (auto& syl : syls) {
				if (syl.start_time > end_time) {
					syl.start_time = end_time;
					syl.duration = 0;
				}
				else {
					syl.duration = std::min(syl.duration, end_time - syl.start_time);
				}
			}
		}
	}

	// Add karaoke splits at each space
	if (auto_split && syls.size() == 1) {
		size_t pos;
		no_announce = true;
		while ((pos = syls.back().text.find(' ')) != std::string::npos)
			AddSplit(syls.size() - 1, pos + 1);
		no_announce = false;
	}

	AnnounceSyllablesChanged();
}

void AssKaraoke::ParseSyllables(const AssDialogue *line, Syllable &syl) {
	for (auto& block : line->ParseTags()) {
		std::string text = block->GetText();

		switch (block->GetType()) {
		case AssBlockType::PLAIN:
			syl.text += text;
			break;
		case AssBlockType::COMMENT:
		// drawings aren't override tags but they shouldn't show up in the
		// stripped text so pretend they are
		case AssBlockType::DRAWING:
			syl.ovr_tags[syl.text.size()] += text;
			break;
		case AssBlockType::OVERRIDE:
			auto ovr = static_cast<AssDialogueBlockOverride*>(block.get());
			bool in_tag = false;
			for (auto& tag : ovr->Tags) {
				if (tag.IsValid() && boost::istarts_with(tag.Name, "\\k")) {
					if (in_tag) {
						syl.ovr_tags[syl.text.size()] += "}";
						in_tag = false;
					}

					// Dealing with both \K and \kf is mildly annoying so just
					// convert them both to \kf
					if (tag.Name == "\\K") tag.Name = "\\kf";

					// Don't bother including zero duration zero length syls
					if (syl.duration > 0 || !syl.text.empty()) {
						syls.push_back(syl);
						syl.text.clear();
						syl.ovr_tags.clear();
					}

					syl.tag_type = tag.Name;
					syl.start_time += syl.duration;
					syl.duration = tag.Params[0].Get(0) * 10;
				}
				else {
					std::string& otext = syl.ovr_tags[syl.text.size()];
					// Merge adjacent override tags
					boost::trim_right_if(text, [](char c) { return c == '}'; });
					if (!in_tag)
						otext += "{";

					in_tag = true;
					otext += tag;
				}
			}

			if (in_tag)
				syl.ovr_tags[syl.text.size()] += "}";
			break;
		}
	}

	syls.push_back(syl);
}

std::string AssKaraoke::GetText() const {
	std::string text;
	text.reserve(size() * 10);

	for (auto const& syl : syls)
		text += syl.GetText(true);

	return text;
}

std::string AssKaraoke::GetTagType() const {
	return begin()->tag_type;
}

void AssKaraoke::SetTagType(std::string const& new_type) {
	for (auto& syl : syls)
		syl.tag_type = new_type;
}

void AssKaraoke::AddSplit(size_t syl_idx, size_t pos) {
	syls.insert(syls.begin() + syl_idx + 1, Syllable());
	Syllable &syl = syls[syl_idx];
	Syllable &new_syl = syls[syl_idx + 1];

	// If the syl is empty or the user is adding a syllable past the last
	// character then pos will be out of bounds. Doing this is a bit goofy,
	// but it's sometimes required for complex karaoke scripts
	if (pos < syl.text.size()) {
		new_syl.text = syl.text.substr(pos);
		syl.text = syl.text.substr(0, pos);
	}

	if (new_syl.text.empty())
		new_syl.duration = 0;
	else if (syl.text.empty()) {
		new_syl.duration = syl.duration;
		syl.duration = 0;
	}
	else {
		new_syl.duration = (syl.duration * new_syl.text.size() / (syl.text.size() + new_syl.text.size()) + 5) / 10 * 10;
		syl.duration -= new_syl.duration;
	}

	assert(syl.duration >= 0);

	new_syl.start_time = syl.start_time + syl.duration;
	new_syl.tag_type = syl.tag_type;

	// Move all override tags after the split to the new syllable and fix the indices
	size_t text_len = syl.text.size();
	for (auto it = syl.ovr_tags.begin(); it != syl.ovr_tags.end(); ) {
		if (it->first < text_len)
			++it;
		else {
			new_syl.ovr_tags[it->first - text_len] = it->second;
			syl.ovr_tags.erase(it++);
		}
	}

	if (!no_announce) AnnounceSyllablesChanged();
}

void AssKaraoke::RemoveSplit(size_t syl_idx) {
	// Don't allow removing the first syllable
	if (syl_idx == 0) return;

	Syllable &syl = syls[syl_idx];
	Syllable &prev = syls[syl_idx - 1];

	prev.duration += syl.duration;
	for (auto const& tag : syl.ovr_tags)
		prev.ovr_tags[tag.first + prev.text.size()] = tag.second;
	prev.text += syl.text;

	syls.erase(syls.begin() + syl_idx);

	if (!no_announce) AnnounceSyllablesChanged();
}

void AssKaraoke::SetStartTime(size_t syl_idx, int time) {
	// Don't allow moving the first syllable
	if (syl_idx == 0) return;

	Syllable &syl = syls[syl_idx];
	Syllable &prev = syls[syl_idx - 1];

	assert(time >= prev.start_time);
	assert(time <= syl.start_time + syl.duration);

	int delta = time - syl.start_time;
	syl.start_time = time;
	syl.duration -= delta;
	prev.duration += delta;
}

void AssKaraoke::SetLineTimes(int start_time, int end_time) {
	assert(end_time >= start_time);

	size_t idx = 0;
	// Chop off any portion of syllables starting before the new start_time
	do {
		int delta = start_time - syls[idx].start_time;
		syls[idx].start_time = start_time;
		syls[idx].duration = std::max(0, syls[idx].duration - delta);
	} while (++idx < syls.size() && syls[idx].start_time < start_time);

	// And truncate any syllables ending after the new end_time
	idx = syls.size() - 1;
	while (syls[idx].start_time > end_time) {
		syls[idx].start_time = end_time;
		syls[idx].duration = 0;
		--idx;
	}
	syls[idx].duration = end_time - syls[idx].start_time;
}
