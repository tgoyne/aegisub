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

/// @file path.cpp
/// @brief Windows-specific path code
/// @ingroup libaegisub windows

#include <libaegisub/path.h>

#include <libaegisub/charset_conv_win.h>
#include <libaegisub/util_win.h>

#include <boost/algorithm/string/predicate.hpp>

namespace {
#include <Shlobj.h>
#include <Shellapi.h>

std::string WinGetFolderPath(int folder) {
	wchar_t path[MAX_PATH+1] = {0};
	if (FAILED(SHGetFolderPathW(0, folder, 0, 0, path)))
		throw agi::EnvironmentError("SHGetFolderPath failed. This should not happen.");

	return agi::charset::ConvertW(path);
}

}

namespace agi {

char Path::dir_sep = '\\';
char Path::non_dir_sep = '/';

void Path::FillPlatformSpecificPaths() {
	//tokens["?temp"] = "";

	SetToken("?user", WinGetFolderPath(CSIDL_APPDATA) + "\\Aegisub");
	SetToken("?local", WinGetFolderPath(CSIDL_LOCAL_APPDATA) + "\\Aegisub");

	/// @todo error checking
	int argc;
	LPWSTR *argv = CommandLineToArgvW(L"", &argc);
	SetTokenFile("?data", charset::ConvertW(argv[0]));
	LocalFree(argv);

	SetToken("?dictionary", Decode("?data/dictionaries"));
	SetToken("?docs", Decode("?data/docs"));
}

std::string Path::Normalize(std::string const& path) {
	std::string normalized;
	normalized.reserve(path.size());

	size_t first_slash = 0;
	size_t out = 0;
	for (char c : path) {
		// Convert forward slashes to backslashes
		if (c == '/') c = '\\';

		// Drop duplicated slashes
		if (c == '\\' && normalized.back() == '\\') continue;

		// Convert \.\ to \ 
		if (c == '\\' && boost::ends_with(normalized, "\\.")) {
			normalized.pop_back();
			continue;
		}

		// Convert prefix\a\.. to prefix
		if (c == '.' && boost::ends_with(normalized, "\\.")) {
			// \.. is \, so don't remove the slash if it's at the beginning
			// (or just after a drive letter)
			if (normalized.size() == 2 || normalized[normalized.size() - 3] == ':')
				normalized.pop_back();
			else
				normalized.erase(normalized.size() - 2, 2);
			continue;
		}

		// Otherwise append this character
		normalized.push_back(c);
	}

	// Convert prefix\. to prefix\ 
	if (boost::ends_with(normalized, "\\."))
		normalized.pop_back();

	// Ensure drive letters have a trailing slash
	if (normalized.size() == 2 && normalized[1] == ':')
		normalized.push_back('\\');

	return normalized;
}

bool Path::IsAbsolute(std::string const& path) {
	if (path.size() < 3) return false;

	return
		// Network path must be at least two slashes and something
		(path[0] == '\\' && path[1] == '\\') ||
		// Local path must be at least drive letter, colon, slash
		((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z') && path[1] == ':' && (path[2] == '\\' || path[2] == '/'));
}

}
