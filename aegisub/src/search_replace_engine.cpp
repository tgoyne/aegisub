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

#include "search_replace_engine.h"

#include "ass_dialogue.h"
#include "ass_file.h"
#include "include/aegisub/context.h"
#include "selection_controller.h"
#include "text_selection_controller.h"

#include <libaegisub/of_type_adaptor.h>

#include <boost/algorithm/string/case_conv.hpp>

#include <wx/msgdlg.h>

static const size_t bad_pos = -1;

namespace {
auto get_dialogue_field(SearchReplaceSettings::Field field) -> decltype(&AssDialogue::Text) {
	switch (field) {
		case SearchReplaceSettings::Field::TEXT: return &AssDialogue::Text;
		case SearchReplaceSettings::Field::STYLE: return &AssDialogue::Style;
		case SearchReplaceSettings::Field::ACTOR: return &AssDialogue::Actor;
		case SearchReplaceSettings::Field::EFFECT: return &AssDialogue::Effect;
	}
	throw agi::InternalError("Bad field for search", 0);
}

typedef std::function<MatchState (const AssDialogue*, size_t)> matcher;

class noop_accessor {
	boost::flyweight<std::string> AssDialogue::*field;
	size_t start;

public:
	noop_accessor(SearchReplaceSettings::Field f) : field(get_dialogue_field(f)), start(0) { }

	std::string get(const AssDialogue *d, size_t s) {
		start = s;
		return (d->*field).get().substr(s);
	}

	MatchState make_match_state(size_t s, size_t e, boost::u32regex *r = nullptr) {
		return MatchState(s + start, e + start, r);
	}
};

class skip_tags_accessor {
	boost::flyweight<std::string> AssDialogue::*field;
	std::vector<std::pair<size_t, size_t>> blocks;
	size_t start;

	void parse_str(std::string const& str) {
		blocks.clear();

		size_t ovr_start = bad_pos;
		size_t i = 0;
		for (auto const& c : str) {
			if (c == '{' && ovr_start == bad_pos)
				ovr_start = i;
			else if (c == '}' && ovr_start != bad_pos) {
				blocks.emplace_back(ovr_start, i);
				ovr_start = bad_pos;
			}
			++i;
		}
	}

public:
	skip_tags_accessor(SearchReplaceSettings::Field f) : field(get_dialogue_field(f)), start(0) { }

	std::string get(const AssDialogue *d, size_t s) {
		auto const& str = (d->*field).get();
		parse_str(str);

		std::string out;

		size_t last = s;
		for (auto const& block : blocks) {
			if (block.second < s) continue;
			if (block.first > last)
				out.append(str.begin() + last, str.begin() + block.first);
			last = block.second + 1;
		}

		if (last < str.size())
			out.append(str.begin() + last, str.end());

		start = s;
		return out;
	}

	MatchState make_match_state(size_t s, size_t e, boost::u32regex *r = nullptr) {
		s += start;
		e += start;

		// Shift the start and end of the match to be relative to the unstripped
		// match
		for (auto const& block : blocks) {
			// Any blocks before start are irrelevant as they're included in `start`
			if (block.second < s) continue;
			// Skip over blocks at the very beginning of the match
			// < should only happen if the cursor was within an override block
			// when the user started a search
			if (block.first <= s) {
				size_t len = block.second - std::max(block.first, s) + 1;
				s += len;
				e += len;
				continue;
			}

			assert(block.first > s);
			// Blocks after the match are irrelevant
			if (block.first >= e) break;

			// Extend the match to include blocks within the match
			// Note that blocks cannot be partially within the match
			e += block.second - block.first + 1;
		}
		return MatchState(s, e, r);
	}
};

template<typename Accessor>
matcher get_matcher(SearchReplaceSettings const& settings, Accessor&& a) {
	if (settings.use_regex) {
		int flags = boost::u32regex::perl;
		if (!settings.match_case)
			flags |= boost::u32regex::icase;

		auto regex = boost::make_u32regex(settings.find, flags);

		return [=](const AssDialogue *diag, size_t start) mutable -> MatchState {
			boost::smatch result;
			auto const& str = a.get(diag, start);
			if (!u32regex_search(str, result, regex, start > 0 ? boost::match_not_bol : boost::match_default))
				return MatchState();
			return a.make_match_state(result.position(), result.position() + result.length(), &regex);
		};
	}

	bool full_match_only = settings.exact_match;
	bool match_case = settings.match_case;
	std::string look_for = settings.find;
	if (!settings.match_case)
		boost::to_lower(look_for);

	return [=](const AssDialogue *diag, size_t start) mutable -> MatchState {
		auto str = a.get(diag, start);
		if (full_match_only && str.size() != look_for.size())
			return MatchState();

		if (!match_case)
			boost::to_lower(str);

		size_t pos = str.find(look_for);
		if (pos == std::string::npos)
			return MatchState();

		return a.make_match_state(pos, pos + look_for.size());
	};
}

template<typename Iterator, typename Container>
Iterator circular_next(Iterator it, Container& c) {
	++it;
	if (it == c.end())
		it = c.begin();
	return it;
}

}

std::function<MatchState (const AssDialogue*, size_t)> SearchReplaceEngine::GetMatcher(SearchReplaceSettings const& settings) {
	if (settings.skip_tags)
		return get_matcher(settings, skip_tags_accessor(settings.field));
	return get_matcher(settings, noop_accessor(settings.field));
}

SearchReplaceEngine::SearchReplaceEngine(agi::Context *c)
: context(c)
, initialized(false)
{
}

void SearchReplaceEngine::Replace(AssDialogue *diag, MatchState &ms) {
	auto& diag_field = diag->*get_dialogue_field(settings.field);
	auto text = diag_field.get();

	std::string replacement = settings.replace_with;
	if (ms.re) {
		auto to_replace = text.substr(ms.start, ms.end - ms.start);
		replacement = u32regex_replace(to_replace, *ms.re, replacement, boost::format_first_only);
	}

	diag_field = text.substr(0, ms.start) + replacement + text.substr(ms.end);
	ms.end = ms.start + replacement.size();
}

bool SearchReplaceEngine::FindReplace(bool replace) {
	if (!initialized)
		return false;

	auto matches = GetMatcher(settings);

	AssDialogue *line = context->selectionController->GetActiveLine();
	auto it = context->ass->Line.iterator_to(*line);
	size_t pos = 0;

	MatchState replace_ms;
	if (replace) {
		if (settings.field == SearchReplaceSettings::Field::TEXT)
			pos = context->textSelectionController->GetSelectionStart();

		if ((replace_ms = matches(line, pos))) {
			size_t end = bad_pos;
			if (settings.field == SearchReplaceSettings::Field::TEXT)
				end = context->textSelectionController->GetSelectionEnd();

			if (end == bad_pos || (pos == replace_ms.start && end == replace_ms.end)) {
				Replace(line, replace_ms);
				pos = replace_ms.end;
				context->ass->Commit(_("replace"), AssFile::COMMIT_DIAG_TEXT);
			}
			else {
				// The current line matches, but it wasn't already selected,
				// so the match hasn't been "found" and displayed to the user
				// yet, so do that rather than replacing
				context->textSelectionController->SetSelection(replace_ms.start, replace_ms.end);
				return true;
			}
		}
	}
	// Search from the end of the selection to avoid endless matching the same thing
	else if (settings.field == SearchReplaceSettings::Field::TEXT)
		pos = context->textSelectionController->GetSelectionEnd();
	// For non-text fields we just look for matching lines rather than each
	// match within the line, so move to the next line
	else if (settings.field != SearchReplaceSettings::Field::TEXT)
		it = circular_next(it, context->ass->Line);

	auto const& sel = context->selectionController->GetSelectedSet();
	bool selection_only = settings.limit_to == SearchReplaceSettings::Limit::SELECTED;

	do {
		AssDialogue *diag = dynamic_cast<AssDialogue*>(&*it);
		if (!diag) continue;
		if (selection_only && !sel.count(diag)) continue;
		if (settings.ignore_comments && diag->Comment) continue;

		if (MatchState ms = matches(diag, pos)) {
			if (selection_only)
				// We're cycling through the selection, so don't muck with it
				context->selectionController->SetActiveLine(diag);
			else {
				SubtitleSelection new_sel;
				new_sel.insert(diag);
				context->selectionController->SetSelectionAndActive(new_sel, diag);
			}

			if (settings.field == SearchReplaceSettings::Field::TEXT)
				context->textSelectionController->SetSelection(ms.start, ms.end);

			return true;
		}
	} while (pos = 0, &*(it = circular_next(it, context->ass->Line)) != line);

	// Replaced something and didn't find another match, so select the newly
	// inserted text
	if (replace_ms && settings.field == SearchReplaceSettings::Field::TEXT)
		context->textSelectionController->SetSelection(replace_ms.start, replace_ms.end);

	return true;
}

bool SearchReplaceEngine::ReplaceAll() {
	if (!initialized)
		return false;

	size_t count = 0;

	auto matches = GetMatcher(settings);

	SubtitleSelection const& sel = context->selectionController->GetSelectedSet();
	bool selection_only = settings.limit_to == SearchReplaceSettings::Limit::SELECTED;

	for (auto diag : context->ass->Line | agi::of_type<AssDialogue>()) {
		if (selection_only && !sel.count(diag)) continue;
		if (settings.ignore_comments && diag->Comment) continue;

		if (settings.use_regex) {
			if (MatchState ms = matches(diag, 0)) {
				auto& diag_field = diag->*get_dialogue_field(settings.field);
				std::string const& text = diag_field.get();
				count += distance(
					boost::u32regex_iterator<std::string::const_iterator>(begin(text), end(text), *ms.re),
					boost::u32regex_iterator<std::string::const_iterator>());
				diag_field = u32regex_replace(text, *ms.re, settings.replace_with);
			}
			continue;
		}

		size_t pos = 0;
		while (MatchState ms = matches(diag, pos)) {
			++count;
			Replace(diag, ms);
			pos = ms.end;
		}
	}

	if (count > 0) {
		context->ass->Commit(_("replace"), AssFile::COMMIT_DIAG_TEXT);
		wxMessageBox(wxString::Format(_("%i matches were replaced."), (int)count));
	}
	else {
		wxMessageBox(_("No matches found."));
	}

	return true;
}

void SearchReplaceEngine::Configure(SearchReplaceSettings const& new_settings) {
	settings = new_settings;
	initialized = true;
}
