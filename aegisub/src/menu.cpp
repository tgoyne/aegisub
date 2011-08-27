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
// $Id$

/// @file menu.cpp
/// @brief Dynamic menu and toolbar generator.
/// @ingroup menu

#include "config.h"

#include "include/aegisub/menu.h"

#include "include/aegisub/hotkey.h"

#include "command/command.h"
#include "compat.h"
#include "libresrc/libresrc.h"
#include "main.h"
#include "standard_paths.h"

#include <libaegisub/json.h>
#include <libaegisub/log.h>

#ifndef AGI_PRE
#include <deque>
#include <sstream>
#include <vector>

#include <wx/app.h>
#include <wx/menuitem.h>
#endif

namespace {
/// Window ID of first menu item
static const int MENU_ID_BASE = 10000;

using std::tr1::placeholders::_1;
using std::tr1::bind;

/// Get the menu text for a command along with hotkey
inline wxString get_menu_text(cmd::Command *command, agi::Context *c) {
	return command->StrMenu(c) + "\t" + hotkey::get_hotkey_str_first("Default", command->name());
}

class MruMenu : public wxMenu {
	std::string type;
	std::vector<wxMenuItem *> items;
	std::vector<cmd::Command*> *cmds;

	void Resize(size_t new_size) {
		for (size_t i = GetMenuItemCount(); i > new_size; --i) {
			Remove(FindItemByPosition(i - 1));
		}

		for (size_t i = GetMenuItemCount(); i < new_size; ++i) {
			if (i >= items.size()) {
				items.push_back(new wxMenuItem(this, MENU_ID_BASE + cmds->size(), "_"));
				cmds->push_back(cmd::get(STD_STR(wxString::Format("recent/%s/%d", lagi_wxString(type).Lower(), i))));
			}
			Append(items[i]);
		}
	}

public:
	MruMenu(std::string const& type, std::vector<cmd::Command*> *cmds)
	: type(type)
	, cmds(cmds)
	{
	}

	~MruMenu() {
		// Append all items to ensure that they're all cleaned up
		Resize(items.size());
	}

	void Update() {
		const agi::MRUManager::MRUListMap *mru = config::mru->Get(type);

		Resize(mru->size());

		if (mru->empty()) {
			Resize(1);
			items[0]->Enable(false);
			items[0]->SetItemLabel(_("Empty"));
			return;
		}

		int i = 0;
		for (agi::MRUManager::MRUListMap::const_iterator it = mru->begin();
			it != mru->end();
			++it, ++i)
		{
			items[i]->SetItemLabel(wxString::Format("%s%d %s",
				i <= 9 ? "&" : "", i + 1,
				wxFileName(lagi_wxString(*it)).GetFullName()));
			items[i]->Enable(true);
		}
	}
};

/// @class CommandManager
/// @brief Event dispatcher to update menus on open and handle click events
///
/// Some of what the class does could be dumped off on wx, but wxEVT_MENU_OPEN
/// is super buggy (GetMenu() often returns NULL and it outright doesn't trigger
/// on submenus in many cases, and registering large numbers of wxEVT_UPDATE_UI
/// handlers makes everything involves events unusably slow.
class CommandManager {
	/// Menu items which need to do something on menu open
	std::deque<std::pair<cmd::Command*, wxMenuItem*> > dynamic_items;
	/// window id -> command map
	std::vector<cmd::Command*> items;
	/// MRU menus which need to be updated on menu open
	std::deque<MruMenu*> mru;

	/// Project context
	agi::Context *context;

	/// Update a single dynamic menu item
	void UpdateItem(std::pair<cmd::Command*, wxMenuItem*> const& item) {
		int flags = item.first->Type();
		if (flags & cmd::COMMAND_DYNAMIC_NAME)
			item.second->SetItemLabel(get_menu_text(item.first, context));
		if (flags & cmd::COMMAND_VALIDATE)
			item.second->Enable(item.first->Validate(context));
		if (flags & cmd::COMMAND_RADIO || flags & cmd::COMMAND_TOGGLE)
			item.second->Check(item.first->IsActive(context));
	}

public:
	CommandManager(agi::Context *context) : context(context) { }

	/// Append a command to a menu and register the needed handlers
	int AddCommand(cmd::Command *co, wxMenu *parent, std::string const& text) {
		int flags = co->Type();
		wxItemKind kind =
			flags & cmd::COMMAND_RADIO ? wxITEM_RADIO :
			flags & cmd::COMMAND_TOGGLE ? wxITEM_CHECK :
			wxITEM_NORMAL;

		wxString menu_text = text.empty() ?
			get_menu_text(co, context) :
			lagi_wxString(text) + "\t" + hotkey::get_hotkey_str_first("Default", co->name());

		wxMenuItem *item = new wxMenuItem(parent, MENU_ID_BASE + items.size(), menu_text, co->StrHelp(), kind);
#ifndef __WXMAC__
		/// @todo Maybe make this a configuration option instead?
		item->SetBitmap(co->Icon(16));
#endif
		parent->Append(item);
		items.push_back(co);

		if (flags != cmd::COMMAND_NORMAL)
			dynamic_items.push_back(std::make_pair(co, item));

		return item->GetId();
	}

	/// Create a MRU menu and register the needed handlers
	/// @param name MRU type
	/// @param parent Menu to append the new MRU menu to
	void AddRecent(std::string const& name, wxMenu *parent) {
		mru.push_back(new MruMenu(name, &items));
		parent->AppendSubMenu(mru.back(), _("Recent"));
	}

	void OnMenuOpen(wxMenuEvent &) {
		for_each(dynamic_items.begin(), dynamic_items.end(), bind(&CommandManager::UpdateItem, this, _1));
		for_each(mru.begin(), mru.end(), std::mem_fun(&MruMenu::Update));
	}

	void OnMenuClick(wxCommandEvent &evt) {
		// This also gets clicks on unrelated things such as the toolbar, so
		// the window ID ranges really need to be unique
		size_t id = static_cast<size_t>(evt.GetId() - MENU_ID_BASE);
		if (id < items.size())
			(*items[id])(context);
	}
};

/// Wrapper for wxMenu to add a command manager
struct CommandMenu : public wxMenu {
	CommandManager cm;
	CommandMenu(agi::Context *c) : cm(c) { }
};

/// Wrapper for wxMenuBar to add a command manager
struct CommandMenuBar : public wxMenuBar {
	CommandManager cm;
	CommandMenuBar(agi::Context *c) : cm(c) { }
};

/// Read a string from a json object
/// @param obj Object to read from
/// @param name Index to read from
/// @param[out] value Output value to write to
/// @return Was the requested index found
bool read_entry(json::Object const& obj, const char *name, std::string *value) {
	json::Object::const_iterator it = obj.Find(name);
	if (it == obj.End()) return false;
	*value = static_cast<json::String const&>(it->element);
	return true;
}

typedef json::Array menu_items;
typedef json::Object menu_map;

/// Get the root object of the menu configuration
menu_map const& get_menus_root() {
	static menu_map root;
	if (!root.Empty()) return root;

	try {
		root = agi::json_util::file(StandardPaths::DecodePath("?user/menu.json").utf8_str().data(), GET_DEFAULT_CONFIG(default_menu));
		return root;
	}
	catch (json::Reader::ParseException const& e) {
		LOG_E("menu/parse") << "json::ParseException: " << e.what() << ", Line/offset: " << e.m_locTokenBegin.m_nLine + 1 << '/' << e.m_locTokenBegin.m_nLineOffset + 1;
		throw;
	}
	catch (std::exception const& e) {
		LOG_E("menu/parse") << e.what();
		throw;
	}
}

/// Get the menu with the specified name
/// @param name Name of menu to get
/// @return Array of menu items
menu_items const& get_menu(std::string const& name) {
	menu_map const& root = get_menus_root();

	menu_map::const_iterator it = root.Find(name);
	if (it == root.End()) throw menu::UnknownMenu("Menu named " + name + " not found");
	return it->element;
}

wxMenu *build_menu(std::string const& name, agi::Context *c, CommandManager *cm, wxMenu *menu = 0);

/// Recursively process a single entry in the menu json
/// @param parent Menu to add the item(s) from this entry to
/// @param c Project context to bind the menu to
/// @param ele json object to process
/// @param cm Command manager for this menu
void process_menu_item(wxMenu *parent, agi::Context *c, json::Object const& ele, CommandManager *cm) {
	if (ele.Empty()) {
		parent->AppendSeparator();
		return;
	}

	std::string submenu, recent, command, text, special;
	read_entry(ele, "special", &special);

	if (read_entry(ele, "submenu", &submenu) && read_entry(ele, "text", &text)) {
		parent->AppendSubMenu(build_menu(submenu, c, cm), lagi_wxString(text));
#ifdef __WXMAC__
		if (special == "help")
			wxApp::s_macHelpMenuTitleName = lagi_wxString(text);
#endif
		return;
	}

	if (read_entry(ele, "recent", &recent)) {
		cm->AddRecent(recent, parent);
		return;
	}

		if (special == "automation") {
#ifdef WITH_AUTOMATION
			/// @todo Actually implement this
			parent->Append(-1, _("No Automation macros loaded"))->Enable(false);
#endif
			return;
		}

	if (!read_entry(ele, "command", &command))
		return;

	read_entry(ele, "text", &text);

	try {
		int id = cm->AddCommand(cmd::get(command), parent, text);
#ifdef __WXMAC__
		if (!special.empty()) {
			if (special == "about")
				wxApp::s_macAboutMenuItemId = id;
			else if (special == "exit")
				wxApp::s_macExitMenuItemId = id;
			else if (special == "options")
				wxApp::s_macPreferencesMenuItemId = id;
		}
#endif
	}
	catch (agi::Exception const& e) {
#ifdef _DEBUG
		parent->Append(-1, lagi_wxString(e.GetMessage()))->Enable(false);
#endif
		LOG_W("menu/command/not_found") << "Skipping command " << command << ": " << e.GetMessage();
	}
}

/// Build the menu with the given name
/// @param name Name of the menu
/// @param c Project context to bind the menu to
wxMenu *build_menu(std::string const& name, agi::Context *c, CommandManager *cm, wxMenu *menu) {
	menu_items const& items = get_menu(name);

	if (!menu) menu = new wxMenu;
	for_each(items.Begin(), items.End(), bind(process_menu_item, menu, c, _1, cm));
	return menu;
}

}

namespace menu {
	void GetMenuBar(std::string const& name, wxFrame *window, agi::Context *c) {
		menu_items const& items = get_menu(name);

		CommandMenuBar *menu = new CommandMenuBar(c);
		for (menu_items::const_iterator it = items.Begin(); it != items.End(); ++it) {
			std::string submenu, disp;
			read_entry(*it, "submenu", &submenu);
			read_entry(*it, "text", &disp);
			menu->Append(build_menu(submenu, c, &menu->cm), lagi_wxString(disp));
		}

		window->SetMenuBar(menu);
		window->Bind(wxEVT_MENU_OPEN, &CommandManager::OnMenuOpen, &menu->cm);
		window->Bind(wxEVT_COMMAND_MENU_SELECTED, &CommandManager::OnMenuClick, &menu->cm);
	}

	wxMenu *GetMenu(std::string const& name, agi::Context *c) {
		CommandMenu *menu = new CommandMenu(c);
		build_menu(name, c, &menu->cm, menu);
		menu->Bind(wxEVT_MENU_OPEN, &CommandManager::OnMenuOpen, &menu->cm);
		menu->Bind(wxEVT_COMMAND_MENU_SELECTED, &CommandManager::OnMenuClick, &menu->cm);
		return menu;
	}

	void OpenPopupMenu(wxMenu *menu, wxWindow *parent_window) {
		wxMenuEvent evt;
		evt.SetEventType(wxEVT_MENU_OPEN);
		menu->ProcessEvent(evt);
		parent_window->PopupMenu(menu);
	}
}
