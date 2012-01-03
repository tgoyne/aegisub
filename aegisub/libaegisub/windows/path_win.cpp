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
/// @brief Windows-specific path code
/// @ingroup libaegisub windows

#ifndef LAGI_PRE
#include <string>
#endif

#include <libaegisub/path.h>

#include <libaegisub/charset_conv_win.h>
#include <libaegisub/util_win.h>

namespace {
#include <Shlobj.h>
#include <Shellapi.h>

std::string WinGetFolderPath(int folder) {
	wchar_t path[MAX_PATH+1] = {0};
	HRESULT res = SHGetFolderPathW(
		0,      // hwndOwner
		folder, // nFolder
		0,      // hToken
		0,      // dwFlags
		path    // pszPath
		);
	if (FAILED(res))
		throw ("SHGetFolderPath() failed"); //< @fixme error message?

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
}

std::string Path::Normalize(std::string const& path) {
	wchar_t fullpath[32768]; // Max UNC path length plus one
	DWORD used = GetFullPathNameW(charset::ConvertW(path).c_str(), sizeof(fullpath) / sizeof(wchar_t), fullpath, 0);
	std::string normalized(charset::ConvertW(std::wstring(fullpath, used)));

	// Special cases where we want to force the path to end with /:
	// Drive letter + colon
	if (normalized.size() == 2 && normalized[1] == ':')
		normalized.push_back('\\');
	// Trailing . or ..
	else if (path.size() > 2 && path[path.size() - 1] == '.' && normalized[normalized.size() - 1] != '\\') {
		char c = path[path.size() - 2];
		if (c == '.')
			c = path[path.size() - 3];
		if (c == '\\' || c == '/')
			normalized.push_back('\\');
	}
	// Original path is just a drive letter, so the normalized path is the
	// current working directory
	else if (path.size() == 2 && path[1] == ':')
		normalized.push_back('\\');

	// De-absolute things starting with a token
	if (path.size() && path[0] == '?')
		normalized.erase(0, normalized.find('?'));

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
