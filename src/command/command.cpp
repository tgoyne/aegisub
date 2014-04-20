// Copyright (c) 2010, Amar Takhar <verm@aegisub.org>
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

#include "command.h"

#include "../compat.h"
#include "../options.h"

#include <libaegisub/io.h>
#include <libaegisub/log.h>
#include <libaegisub/path.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <wx/intl.h>

namespace cmd {
	static std::map<std::string, std::unique_ptr<Command>> cmd_map;
	typedef std::map<std::string, std::unique_ptr<Command>>::iterator iterator;

	static iterator find_command(std::string const& name) {
		auto it = cmd_map.find(name);
		if (it == cmd_map.end())
			throw CommandNotFound(from_wx(wxString::Format(_("'%s' is not a valid command name"), to_wx(name))));
		return it;
	}

	void reg(std::unique_ptr<Command> cmd) {
		cmd_map[cmd->name()] = std::move(cmd);
	}

	void unreg(std::string const& name) {
		cmd_map.erase(find_command(name));
	}

	Command *get(std::string const& name) {
		return find_command(name)->second.get();
	}

	void call(std::string const& name, agi::Context*c) {
		Command &cmd = *find_command(name)->second;
		if (cmd.Validate(c))
			cmd(c);
	}

	std::vector<std::string> get_registered_commands() {
		std::vector<std::string> ret;
		ret.reserve(cmd_map.size());
		for (auto const& it : cmd_map)
			ret.push_back(it.first);
		return ret;
	}

#ifndef AEGISUB_CATALOG
#define AEGISUB_CATALOG "aegisub"
#endif

	std::string fix_str(wxString const& wxs) {
		auto str = from_wx(wxs);
		boost::replace_all(str, "&", "");
		if (boost::contains(str, "'"))
			return '"' + str + '"';
		return "'" + str + "'";
	}

	void log_commands(agi::Context *c) {
		agi::io::Save file("/Users/tgoyne/aegisite/data/commands_31.yml");
		auto& out = file.Get();

		wxTranslations *translations = wxTranslations::Get();
		wxArrayString langs = translations->GetAvailableTranslations(AEGISUB_CATALOG);
		langs.insert(langs.begin(), "en_US");

		for (auto const& lang : langs) {
			// 
			wxTranslations::Set(translations = new wxTranslations);
			wxFileTranslationsLoader::AddCatalogLookupPathPrefix(config::path->Decode("?data/locale/").wstring());

			translations->SetLanguage(lang);
			translations->AddCatalog(AEGISUB_CATALOG);
			translations->AddStdCatalog();

			auto info = wxLocale::FindLanguageInfo(lang);
			auto name = info ? wxLocale::GetLanguageName(info->Language) : lang;

			out << " - locale: " << lang << "\n"
				<< "   name: " << name << "\n"
				<< "   commands:\n";

			for (auto const& it : cmd_map) {
				out << "    - name: " << it.first << "\n"
					<< "      display: " << fix_str(it.second->StrDisplay(c)) << "\n"
					<< "      help: " << fix_str(it.second->StrHelp()) << "\n\n";
			}
		}
	}

	// These forward declarations exist here since we don't want to expose
	// them in a header, they're strictly internal-use.
	void init_app();
	void init_audio();
	void init_automation();
	void init_command();
	void init_edit();
	void init_grid();
	void init_help();
	void init_keyframe();
	void init_recent();
	void init_subtitle();
	void init_time();
	void init_timecode();
	void init_tool();
	void init_video();
	void init_visual_tools();

	void init_builtin_commands() {
		LOG_D("command/init") << "Populating command map";
		init_app();
		init_audio();
		init_automation();
		init_edit();
		init_grid();
		init_help();
		init_keyframe();
		init_recent();
		init_subtitle();
		init_time();
		init_timecode();
		init_tool();
		init_video();
		init_visual_tools();
	}

	void clear() {
		cmd_map.clear();
	}
}
