// Copyright (c) 2012, Thomas Goyne <plorkyeran@aegisub.org>
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

/// @file path.cpp
/// @brief Platform-independent path code
/// @ingroup libaegisub

#include "../config.h"

#include "libaegisub/path.h"

#include "libaegisub/option.h"
#include "libaegisub/option_value.h"
#include "libaegisub/util.h"

namespace {
	char last(std::string const& str) {
		return str.empty() ? 0 : str[str.size() - 1];
	}

	template<class T, class U>
	typename T::const_iterator last_less_than(T const& cont, U const& value) {
		T::const_iterator it = cont.upper_bound(value);
		if (it != cont.begin())
			--it;
		return it;
	}
}

namespace agi {

Path::Path(Options *opt) : opt(opt) {
	tokens["?user"] = "";
	tokens["?local"] = "";
	tokens["?data"] = "";
	tokens["?temp"] = "";

	FillPlatformSpecificPaths();

	tokens["?audio"] = "";
	tokens["?script"] = "";
	tokens["?video"] = "";
}

std::string Path::Decode(std::string const& path) const {
	if (path[0] != '?') return Normalize(path);

	str_map::const_iterator it = last_less_than(tokens, path);

	std::string suffix;
	if (it->second.size() && util::begins_with(path, it->first, &suffix))
		return Combine(it->second, suffix);
	return Normalize(path);
}

std::string Path::Encode(std::string path) const {
	std::string search_path;
	if (last(path) != dir_sep && last(path) != non_dir_sep)
		search_path = GetDir(path);
	else
		search_path = Normalize(path);

	str_map::const_iterator it = last_less_than(paths, search_path);

	std::string suffix;
	if (!util::begins_with(path, it->first, &suffix))
		return path;

	if (suffix.empty())
		return it->second;
	else
		return it->second + dir_sep + suffix;
}

std::string Path::GetPath(std::string const& path_opt_name) const {
	if (!opt) throw OptionErrorNotFound("Option value not found: " + path_opt_name);
	return Decode(opt->Get(path_opt_name)->GetString());
}

void Path::SetPath(std::string const& path_opt_name, std::string const& value) {
	if (!opt) throw OptionErrorNotFound("Option value not found: " + path_opt_name);
	opt->Get(path_opt_name)->SetString(Encode(value));
}

void Path::SetToken(std::string const& token_name, std::string const& token_value) {
	str_map::iterator it = tokens.find(token_name);
	if (it == tokens.end()) throw agi::InternalError("Bad token: " + token_name, 0);
	it->second = token_value.empty() ? "" : Normalize(token_value + dir_sep);

	paths.clear();
	for (str_map::const_iterator t_it = tokens.begin(); t_it != tokens.end(); ++t_it) {
		if (t_it->second.size())
			paths[t_it->second] = t_it->first;
	}
}

void Path::SetTokenFile(std::string const& token_name, std::string const& token_value) {
	SetToken(token_name, GetDir(token_value));
}

std::string Path::Combine(std::string const& prefix, std::string const& suffix) {
	if (IsAbsolute(suffix) || prefix.empty())
		return Normalize(suffix);
	return Normalize(prefix + dir_sep + suffix);
}

std::string Path::GetDir(std::string const& path) {
	std::string norm(Normalize(path));
	if (norm.size() < 2) return norm;
	size_t pos = norm.rfind(dir_sep, norm.size() - 2);
	if (pos == std::string.npos) return norm;

	norm.resize(pos + 1);
	return norm;
}

std::string Path::GetFileName(std::string const& path) {
	std::string norm(Normalize(path));
	size_t pos = norm.rfind(dir_sep);
	if (pos == std::string.npos) return norm;

	norm.erase(0, pos + 1);
	return norm;
}

}
