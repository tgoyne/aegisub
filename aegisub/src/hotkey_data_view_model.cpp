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
//
// $Id$

/// @file hotkey_data_view_model.cpp
/// @see hotkey_data_view_model.h
/// @ingroup hotkey configuration_ui
///

#include "config.h"

#include "hotkey_data_view_model.h"

#include <libaegisub/exception.h>
#include <libaegisub/hotkey.h>

#include "command/command.h"
#include "command/icon.h"
#include "compat.h"
#include "include/aegisub/hotkey.h"
#include "preferences.h"

#ifndef AGI_PRE
#include <wx/dataview.h>
#include <wx/regex.h>

#include <set>
#endif

using namespace agi::hotkey;

/// @class HotkeyModelItem
/// @brief A base class for things exposed by HotkeyDataViewModel
class HotkeyModelItem {
public:
	virtual unsigned int GetChildren(wxDataViewItemArray &children) const=0;
	virtual wxDataViewItem GetParent() const=0;
	virtual void GetValue(wxVariant &variant, unsigned int col) const=0;
	virtual bool IsContainer() const=0;
	virtual bool SetValue(wxVariant const& variant, unsigned int col)=0;
};

class HotkeyModelRoot;
class HotkeyModelCategory;

/// @class HotkeyModelCombo
/// @brief A single hotkey exposed in the data view
///
/// All actual mutation of hotkeys happens through this class
class HotkeyModelCombo : public HotkeyModelItem {
	HotkeyModelCategory *parent; ///< The containing category
	Combo combo;                 ///< The actual hotkey
	wxString cmd_name;
	wxString cmd_str;
public:
	HotkeyModelCombo(HotkeyModelCategory *parent, Combo const& combo)
	: parent(parent)
	, combo(combo)
	, cmd_name(lagi_wxString(combo.CmdName()))
	, cmd_str(lagi_wxString(combo.Str()))
	{
	}

	bool IsVisible(wxRegEx const& filter) const {
		return filter.Matches(cmd_name) || filter.Matches(cmd_str);
	}

	void Apply(Hotkey::HotkeyMap *hk_map) {
		hk_map->insert(make_pair(combo.CmdName(), combo));
	}

	unsigned int GetChildren(wxDataViewItemArray &children) const { return 0; }
	wxDataViewItem GetParent() const { return wxDataViewItem(parent); }
	bool IsContainer() const { return false; }

	void GetValue(wxVariant &variant, unsigned int col) const {
		if (col == 0)
			variant = combo.Str();
		else if (col == 1) {
			wxIcon icon;
			icon.CopyFromBitmap(icon::get(combo.CmdName(), 16));
			variant << wxDataViewIconText(combo.CmdName(), icon);
		}
		else if (col == 2) {
			try {
				variant = cmd::get(combo.CmdName())->StrHelp();
			}
			catch (agi::Exception const& e) {
				variant = lagi_wxString(e.GetChainedMessage());
			}
		}
		else
			throw agi::InternalError("HotkeyDataViewModel asked for an invalid column number", 0);
	}

	bool SetValue(wxVariant const& variant, unsigned int col) {
		if (col == 0) {
			wxArrayString toks = wxSplit(variant.GetString(), '-');
			std::vector<std::string> keys;
			keys.resize(toks.size());
			for (size_t i = 0; i < toks.size(); ++i)
				keys[i] = STD_STR(toks[i]);
			combo = Combo(combo.Context(), combo.CmdName(), keys);
			cmd_str = combo.Str();
			return true;
		}
		else if (col == 1) {
			wxDataViewIconText text;
			text << variant;
			combo = Combo(combo.Context(), STD_STR(text.GetText()), combo.Get());
			cmd_name = text.GetText();
			return true;
		}
		return false;
	}
};

/// A hotkey context exposed in the data view
class HotkeyModelCategory : public HotkeyModelItem {
	std::list<HotkeyModelCombo> children;
	wxDataViewModel *model;
	wxString name;
	wxDataViewItemArray visible_items;
public:
	HotkeyModelCategory(wxDataViewModel *model, wxString const& name)
	: model(model)
	, name(name)
	{
	}

	void AddChild(Combo const& combo) {
		children.push_back(HotkeyModelCombo(this, combo));
		visible_items.push_back(wxDataViewItem(&children.back()));
		model->ItemAdded(wxDataViewItem(this), wxDataViewItem(&children.back()));
	}

	void Delete(wxDataViewItem const& item) {
		for (std::list<HotkeyModelCombo>::iterator it = children.begin(); it != children.end(); ++it) {
			if (&*it == item.GetID()) {
				model->ItemDeleted(wxDataViewItem(this), wxDataViewItem((void*)&*it));
				children.erase(it);
				return;
			}
		}
	}

	void Apply(Hotkey::HotkeyMap *hk_map) {
		for_each(children.begin(), children.end(),
			bind(&HotkeyModelCombo::Apply, std::tr1::placeholders::_1, hk_map));
	}

	void SetFilter(wxRegEx const& new_filter) {
		std::set<HotkeyModelCombo*> old_visible;
		for (size_t i = 0; i < visible_items.size(); ++i)
			old_visible.insert(static_cast<HotkeyModelCombo*>(visible_items[i].GetID()));

		visible_items.clear();

		wxDataViewItemArray added;
		wxDataViewItemArray removed;

		for (std::list<HotkeyModelCombo>::iterator it = children.begin(); it != children.end(); ++it) {
			bool was_visible = old_visible.count(&*it) > 0;
			bool is_visible = it->IsVisible(new_filter);

			if (is_visible)
				visible_items.push_back(wxDataViewItem(&*it));
			if (was_visible && !is_visible)
				removed.push_back(wxDataViewItem(&*it));
			if (is_visible && !was_visible)
				added.push_back(wxDataViewItem(&*it));
		}

		if (!added.empty())
			model->ItemsAdded(wxDataViewItem(this), added);
		if (!removed.empty())
			model->ItemsDeleted(wxDataViewItem(this), removed);
	}


	wxDataViewItem GetParent() const { return wxDataViewItem(0); }
	bool IsContainer() const { return true; }
	bool SetValue(wxVariant const& variant, unsigned int col) { return false; }
	void GetValue(wxVariant &variant, unsigned int col) const {
		if (col == 1)
			variant << wxDataViewIconText(name);
		else
			variant = name;
	}

	unsigned int GetChildren(wxDataViewItemArray &out) const {
		out = visible_items;
		return out.size();
	}
};

/// The root containing the hotkey contexts
class HotkeyModelRoot : public HotkeyModelItem {
	std::list<HotkeyModelCategory> categories;
public:
	HotkeyModelRoot(wxDataViewModel *model) {
		Hotkey::HotkeyMap const& hk_map = hotkey::inst->GetHotkeyMap();
		std::map<std::string, HotkeyModelCategory*> cat_map;

		for (Hotkey::HotkeyMap::const_iterator it = hk_map.begin(); it != hk_map.end(); ++it) {
			std::string cat_name = it->second.Context();
			HotkeyModelCategory *cat;
			std::map<std::string, HotkeyModelCategory*>::iterator cat_it = cat_map.find(cat_name);
			if (cat_it != cat_map.end())
				cat = cat_it->second;
			else {
				categories.push_back(HotkeyModelCategory(model, cat_name));
				cat = cat_map[cat_name] = &categories.back();
			}

			cat->AddChild(it->second);
		}
	}

	void Apply(Hotkey::HotkeyMap *hk_map) {
		for_each(categories.begin(), categories.end(),
			bind(&HotkeyModelCategory::Apply, std::tr1::placeholders::_1, hk_map));
	}

	void SetFilter(wxString filter) {
		// Escape any regular-expression special characters
		static wxRegEx escape_meta("[-[\\]{}()*+?.,\\\\^$|#]", wxRE_ADVANCED);
		escape_meta.Replace(&filter, "\\\\&");

		// Using wxRegEx for case-insensitive contains
		wxRegEx re(filter, wxRE_ADVANCED | wxRE_ICASE | wxRE_NOSUB);
		for_each(categories.begin(), categories.end(),
			bind(&HotkeyModelCategory::SetFilter, std::tr1::placeholders::_1, std::tr1::ref(re)));
	}

	wxDataViewItem GetParent() const { return wxDataViewItem(0); }
	bool IsContainer() const { return true; }
	bool SetValue(wxVariant const& variant, unsigned int col) { return false; }
	void GetValue(wxVariant &variant, unsigned int col) const { }

	unsigned int GetChildren(wxDataViewItemArray &out) const {
		out.reserve(categories.size());
		for (std::list<HotkeyModelCategory>::const_iterator it = categories.begin(); it != categories.end(); ++it)
			out.push_back(wxDataViewItem((void*)&*it));
		return out.size();
	}
};


HotkeyDataViewModel::HotkeyDataViewModel(Preferences *parent)
: root(new HotkeyModelRoot(this))
, parent(parent)
, has_pending_changes(false)
{
}

const HotkeyModelItem * HotkeyDataViewModel::get(wxDataViewItem const& item) const {
	if (item.IsOk())
		return static_cast<HotkeyModelItem*>(item.GetID());
	return root.get();
}

HotkeyModelItem * HotkeyDataViewModel::get(wxDataViewItem const& item) {
	if (item.IsOk())
		return static_cast<HotkeyModelItem*>(item.GetID());
	return root.get();
}

unsigned int HotkeyDataViewModel::GetChildren(wxDataViewItem const& item, wxDataViewItemArray &children) const {
	return get(item)->GetChildren(children);
}

wxDataViewItem HotkeyDataViewModel::GetParent(wxDataViewItem const& item) const {
	return get(item)->GetParent();
}

void HotkeyDataViewModel::GetValue(wxVariant &variant, wxDataViewItem const& item, unsigned int col) const {
	get(item)->GetValue(variant, col);
}

bool HotkeyDataViewModel::IsContainer(wxDataViewItem const& item) const {
	return get(item)->IsContainer();
}

bool HotkeyDataViewModel::SetValue(wxVariant const& variant, wxDataViewItem const& item, unsigned int col) {
	if (!has_pending_changes) {
		has_pending_changes = true;
		parent->AddPendingChange(std::tr1::bind(&HotkeyDataViewModel::Apply, this));
	}
	return get(item)->SetValue(variant, col);
}

void HotkeyDataViewModel::New(wxDataViewItem item) {
	if (!item.IsOk()) return;

	if (!IsContainer(item))
		item = GetParent(item);

	HotkeyModelCategory *ctx = static_cast<HotkeyModelCategory*>(item.GetID());
	wxVariant name;
	ctx->GetValue(name, 0);
	ctx->AddChild(Combo(STD_STR(name.GetString()), "", std::vector<std::string>()));
}

void HotkeyDataViewModel::Delete(wxDataViewItem const& item) {
	if (!item.IsOk() || IsContainer(item)) return;

	static_cast<HotkeyModelCategory*>(GetParent(item).GetID())->Delete(item);

	if (!has_pending_changes) {
		has_pending_changes = true;
		parent->AddPendingChange(std::tr1::bind(&HotkeyDataViewModel::Apply, this));
	}
}

void HotkeyDataViewModel::Apply() {
	Hotkey::HotkeyMap hk_map;
	root->Apply(&hk_map);
	hotkey::inst->SetHotkeyMap(hk_map);
	has_pending_changes = false;
}

void HotkeyDataViewModel::SetFilter(wxString const& filter) {
	root->SetFilter(filter);
}
