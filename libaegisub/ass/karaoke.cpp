// Copyright (c) 2022, Thomas Goyne <plorkyeran@aegisub.org>
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

#include <libaegisub/ass/karaoke.h>

#include <libaegisub/karaoke_matcher.h>
#include <libaegisub/format.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>

namespace agi::ass {
std::string KaraokeSyllable::GetText(bool k_tag) const {
	std::string ret;

	if (k_tag)
		ret = agi::format("{%s%d}", tag_type, ((duration + 5) / 10));

	std::string_view sv = text;
	size_t idx = 0;
	for (auto const& ovr : ovr_tags) {
		ret += sv.substr(idx, ovr.first - idx);
		ret += ovr.second;
		idx = ovr.first;
	}
	ret += sv.substr(idx);
	return ret;
}

void Karaoke::SetLine(std::vector<KaraokeSyllable>&& syls, bool auto_split, std::optional<int> end_time) {
	this->syls = std::move(syls);

	if (end_time) {
		Normalize(*end_time);
	}

	// Add karaoke splits at each space
	if (auto_split && size() == 1) {
		AutoSplit();
	}

	AnnounceSyllablesChanged();
}

void Karaoke::Normalize(int end_time) {
	auto& last_syl = syls.back();
	int last_end = last_syl.start_time + last_syl.duration;

	// Total duration is shorter than the line length so just extend the last
	// syllable; this has no effect on rendering but is easier to work with
	if (last_end < end_time)
		last_syl.duration += end_time - last_end;
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

void Karaoke::AutoSplit() {
	size_t pos;
	while ((pos = syls.back().text.find(' ')) != std::string::npos)
		DoAddSplit(syls.size() - 1, pos + 1);
}

std::string Karaoke::GetText() const {
	std::string text;
	text.reserve(size() * 10);

	for (auto const& syl : syls)
		text += syl.GetText(true);

	return text;
}

std::string_view Karaoke::GetTagType() const {
	return begin()->tag_type;
}

void Karaoke::SetTagType(std::string_view new_type) {
	for (auto& syl : syls)
		syl.tag_type = new_type;
}

void Karaoke::DoAddSplit(size_t syl_idx, size_t pos) {
	syls.insert(syls.begin() + syl_idx + 1, KaraokeSyllable());
	KaraokeSyllable &syl = syls[syl_idx];
	KaraokeSyllable &new_syl = syls[syl_idx + 1];

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
}

void Karaoke::AddSplit(size_t syl_idx, size_t pos) {
	DoAddSplit(syl_idx, pos);
	AnnounceSyllablesChanged();
}

void Karaoke::RemoveSplit(size_t syl_idx) {
	// Don't allow removing the first syllable
	if (syl_idx == 0) return;

	KaraokeSyllable &syl = syls[syl_idx];
	KaraokeSyllable &prev = syls[syl_idx - 1];

	prev.duration += syl.duration;
	for (auto const& tag : syl.ovr_tags)
		prev.ovr_tags[tag.first + prev.text.size()] = tag.second;
	prev.text += syl.text;

	syls.erase(syls.begin() + syl_idx);

	AnnounceSyllablesChanged();
}

void Karaoke::SetStartTime(size_t syl_idx, int time) {
	// Don't allow moving the first syllable
	if (syl_idx == 0) return;

	KaraokeSyllable &syl = syls[syl_idx];
	KaraokeSyllable &prev = syls[syl_idx - 1];

	assert(time >= prev.start_time);
	assert(time <= syl.start_time + syl.duration);

	int delta = time - syl.start_time;
	syl.start_time = time;
	syl.duration -= delta;
	prev.duration += delta;
}

void Karaoke::SetLineTimes(int start_time, int end_time) {
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

} // namespace agi::ass
