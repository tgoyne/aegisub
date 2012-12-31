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

/// @file standard_paths.cpp
/// @brief Encode and decode paths relative to various special locations
/// @ingroup utility
///

#include "config.h"

#include "standard_paths.h"

#include "compat.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <map>

#include <wx/stdpaths.h>

std::map<std::string, std::string>& paths() {
	static std::map<std::string, std::string> paths;
	if (paths.empty()) {
		wxStandardPathsBase &wxpaths = wxStandardPaths::Get();

#if defined(__UNIX__) && !defined(__APPLE__)
		static_cast<wxStandardPaths&>(wxpaths).SetInstallPrefix(wxT(INSTALL_PREFIX));
#endif

		paths["?data"] = from_wx(wxpaths.GetDataDir());
		paths["?user"] = from_wx(wxpaths.GetUserDataDir());
		paths["?local"] = from_wx(wxpaths.GetUserLocalDataDir());
		paths["?temp"] = from_wx(wxpaths.GetTempDir());

		if (!wxDirExists(wxpaths.GetUserDataDir()))
			wxMkDir(wxpaths.GetUserDataDir(), 0777);
		if (!wxDirExists(wxpaths.GetUserLocalDataDir()))
			wxMkDir(wxpaths.GetUserLocalDataDir(), 0777);
	}
	return paths;
}

std::string StandardPaths::DecodePath(std::string path) {
	if (path.empty() || path[0] != '?')
		return path;

	auto it = paths().lower_bound(path);
	if (it == paths().begin()) return path;
	if (!boost::starts_with(path, (--it)->second)) return path;

	path = it->second + "/" + path.substr(it->second.size());
	boost::replace_all(path, "//", "/");
#ifdef _WIN32
	boost::replace_all(path, "/", "\\");
#endif
	return path;
}

void StandardPaths::SetPathValue(const std::string &path, const std::string &value) {
	paths()[path] = value;
}
