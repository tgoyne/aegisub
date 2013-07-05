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

#include "style_manager.h"

#include "ass_dialogue.h"
#include "ass_file.h"
#include "ass_style.h"
#include "compat.h"
#include "subtitle_format.h"

#include <libaegisub/fs.h>
#include <libaegisub/io.h>
#include <libaegisub/line_iterator.h>
#include <libaegisub/of_type_adaptor.h>
#include <libaegisub/util.h>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/format.hpp>
#include <boost/range/algorithm.hpp>
#include <list>

namespace {

template<typename Container>
std::string unique_copy_name(Container const& existing, std::string const& source_name) {
	if (!existing.count(boost::to_lower_copy(source_name)))
		return source_name;

	std::string name = from_wx(wxString::Format(_("%s - Copy"), to_wx(source_name)));
	for (int i = 2; existing.count(boost::to_lower_copy(name)); ++i)
		name = from_wx(wxString::Format(_("%s - Copy (%d)"), to_wx(source_name), i));
	return name;
}

template<typename Container>
std::string unique_new_name(Container const& existing) {
	if (!existing.count("default"))
		return "Default";

	int i = 2;
	while (existing.count(str(boost::format("default %d") % i))) ++i;
	return str(boost::format("Default %d") % i);
}

struct cmp_name {
	template<typename T>
	bool operator()(T const& lft, T const& rgt) const { return lft->name < rgt->name; }
};

template<typename Container>
void do_move(Container& styles, int type, int &first, int &last) {
	auto begin = styles.begin();

	// Move up
	if (type == 0) {
		if (first == 0) return;
		rotate(begin + first - 1, begin + first, begin + last + 1);
		first--;
		last--;
	}
	// Move to top
	else if (type == 1) {
		rotate(begin, begin + first, begin + last + 1);
		last = last - first;
		first = 0;
	}
	// Move down
	else if (type == 2) {
		if (last + 1 == (int)styles.size()) return;
		rotate(begin + first, begin + last + 1, begin + last + 2);
		first++;
		last++;
	}
	// Move to bottom
	else if (type == 3) {
		rotate(begin + first, begin + last + 1, styles.end());
		first = styles.size() - (last - first + 1);
		last = styles.size() - 1;
	}
	// Sort
	else if (type == 4) {
		sort(styles.begin(), styles.end(), cmp_name());
		first = 0;
		last = 0;
	}
}

struct style_references {
	AssStyle *style;
	std::vector<AssDialogue*> styles;
	std::vector<AssDialogue*> reset_tag;
	style_references() : style(nullptr) { }
};

typedef std::map<std::string, style_references> style_reference_map;

struct AssFileManager : public StyleManager {
	AssFile file;
	AssFile *ass;
	std::vector<AssStyle *> styles;
	style_reference_map style_map;
	bool has_renamed;

	AssFileManager(AssFile *ass)
	: ass(ass)
	, has_renamed(false)
	{
		Init();
	}

	AssFileManager(agi::fs::path const& filename)
	: ass(&file)
	, has_renamed(false)
	{
		auto reader = SubtitleFormat::GetReader(filename);
		reader->ReadFile(&file, filename, "");
		Init();
	}

	void Init() {
		for (auto style : ass->Line | agi::of_type<AssStyle>()) {
			styles.push_back(style);
			style_map[boost::to_lower_copy(style->name)].style = style;
		}

		auto process_tag = [](std::string const& tag, AssOverrideParameter *param, void *ud) {
			if (tag == "\\r" && param->GetType() == VariableDataType::TEXT)
				static_cast<std::vector<std::string>*>(ud)->emplace_back(param->Get<std::string>());
		};

		for (auto diag : ass->Line | agi::of_type<AssDialogue>()) {
			style_map[boost::to_lower_copy(diag->Style.get())].styles.push_back(diag);

			boost::ptr_vector<AssDialogueBlock> blocks(diag->ParseTags());
			for (auto block : blocks | agi::of_type<AssDialogueBlockOverride>()) {
				std::vector<std::string> styles;
				block->ProcessParameters(process_tag, &styles);
				for (auto& style : styles) {
					boost::to_lower(style);
					auto& lines = style_map[style].reset_tag;
					if (boost::find(lines, diag) == end(lines))
						lines.push_back(diag);
				}
			}
		}
	}

	void Save(wxString const& message) override {
		ass->Commit(message, AssFile::COMMIT_STYLES | (has_renamed ? AssFile::COMMIT_DIAG_FULL : 0));
		has_renamed = false;
	}

	std::vector<AssStyle *> GetStyles() override {
		return styles;
	}

	AssStyle *GetStyle(std::string name) override {
		boost::to_lower(name);
		auto it = style_map.find(name);
		return it != end(style_map) ? it->second.style : nullptr;
	}

	std::vector<std::string> GetStyleNames() override {
		std::vector<std::string> style_names;
		for (auto const& style : styles)
			style_names.push_back(style->name);
		return style_names;
	}

	void Delete(std::string const& name) override {
		auto it = style_map.find(name);
		if (it == end(style_map))
			return;

		styles.erase(boost::remove_if(styles, [&](AssStyle *style) { return style->name == name; }));
		delete it->second.style;
		style_map.erase(it);
	}

	AssStyle *Create(AssStyle *original) override {
		auto style = original ? new AssStyle(*original) : new AssStyle;
		ass->InsertLine(style);
		styles.push_back(style);
		style->name = original ? unique_copy_name(style_map, original->name) : unique_new_name(style_map);
		style_map[boost::to_lower_copy(style->name)].style = style;
		return style;
	}

	bool NeedsRenameProcessing(std::string const& old_name, std::string const& new_name) override {
		return false;
	}

	void RenameStyle(AssStyle *style, std::string const& new_name) override {
		std::string old_name = style->name;
		boost::to_lower(old_name);
		auto& to_rename = style_map[old_name];

		for (auto line : to_rename.styles)
			line->Style = new_name;

		auto update_r_tag = [](std::string const& tag, AssOverrideParameter *param, void *ud) {
			auto& names = *static_cast<std::pair<std::string, std::string>*>(ud);
			if (tag == "\\r" && param->GetType() == VariableDataType::TEXT && param->Get<std::string>() == names.first)
				param->Set(names.second);
		};

		auto names = std::make_pair(old_name, new_name);
		for (auto diag : to_rename.reset_tag) {
			boost::ptr_vector<AssDialogueBlock> blocks(diag->ParseTags());
			for (auto block : blocks | agi::of_type<AssDialogueBlockOverride>())
				block->ProcessParameters(update_r_tag, &names);
		}

		auto new_name_lower = boost::to_lower_copy(new_name);
		style_map[new_name_lower].style = style;
		style_map.erase(old_name);

		style->name = new_name;
		has_renamed = true;
		SetSelection(std::vector<std::string>(1, new_name));
	}

	void Move(int type, int first, int last) override {
		do_move(styles, type, first, last);
		Save(_("style move"));

		// Apply the new order to the actual subtitles file by swapping each
		// style into the correct position
		size_t curn = 0;
		for (auto it = ass->Line.begin(); it != ass->Line.end(); ++it) {
			if (!dynamic_cast<AssStyle*>(&*it)) continue;

			auto new_style_at_pos = ass->Line.iterator_to(*styles[curn]);
			EntryList::node_algorithms::swap_nodes(it.pointed_node(), new_style_at_pos.pointed_node());
			if (++curn == styles.size()) break;
			it = new_style_at_pos;
		}
	}

	void AddStyles(std::vector<AssStyle*> const& to_copy) override {

	}
};

}

std::unique_ptr<StyleManager> StyleManager::Create(agi::fs::path const& storage_name) {
	return agi::util::make_unique<AssFileManager>(storage_name);
}

std::unique_ptr<StyleManager> StyleManager::Create(AssFile *file) {
	return agi::util::make_unique<AssFileManager>(file);
}
