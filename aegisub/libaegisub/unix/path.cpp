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

#include <libaegisub/path.h>

#include <cstdlib>
#include <cstdio>
#include <boost/algorithm/string/predicate.hpp>
#include <pwd.h>

namespace {
std::string home_dir() {
	const char *env = getenv("HOME");
	if (env) return env;

	if ((env = getenv("USER")) || (env = getenv("LOGNAME"))) {
		if (passwd *user_info = getpwnam(env))
			return user_info->pw_dir;
	}

	throw agi::EnvironmentError("Could not get home directory. Make sure HOME is set.");
}

std::string temp_dir() {
	const char *env = getenv("TMPDIR");
	return env ? env : "/tmp";
}
}

namespace agi {

char Path::dir_sep = '/';
char Path::non_dir_sep = '\\';

void Path::FillPlatformSpecificPaths() {
	std::string home = home_dir();
	SetToken("?user", Combine(home, ".aegisub"));
	SetToken("?local", Combine(home, ".aegisub"));
	SetToken("?temp", temp_dir());
	SetToken("?dictionaries", "/usr/share/hunspell");
	SetToken("?docs", P_DOC);
}

std::string Path::Normalize(std::string const& path) {
	std::string normalized;
	normalized.reserve(path.size());

	for (char c : path) {
		// Convert backslashes to slashes
		if (c == '\\') c = '/';

		// Drop duplicated slashes
		if (c == '/' && normalized.back() == '/') continue;

		// Convert /./ to / 
		if (c == '/' && boost::ends_with(normalized, "/.")) {
			normalized.pop_back();
			continue;
		}

		// Convert prefix/.. to prefix
		if (c == '.' && boost::ends_with(normalized, "/.")) {
			// /.. is /, so don't remove the slash if it's at the beginning
			if (normalized.size() == 2)
				normalized.pop_back();
			else
				normalized.erase(normalized.size() - 2, 2);
			continue;
		}

		// Otherwise append this character
		normalized.push_back(c);
	}

	// Convert prefix/. to prefix/
	if (boost::ends_with(normalized, "/."))
		normalized.pop_back();

	return normalized;
}

bool Path::IsAbsolute(std::string const& path) {
	return !path.empty() && (path[0] == '/' || path[0] == '?');
}

}
