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

#include <libaegisub/util_win.h>

#include <boost/filesystem.hpp>

namespace {
#include <Shlobj.h>
#include <Shellapi.h>

agi::fs::path WinGetFolderPath(int folder) {
	wchar_t path[MAX_PATH+1] = {0};
	if (FAILED(SHGetFolderPathW(0, folder, 0, 0, path)))
		throw agi::EnvironmentError("SHGetFolderPath failed. This should not happen.");
	return path;
}
}

namespace agi {

void Path::FillPlatformSpecificPaths() {
	tokens["?temp"] = boost::filesystem::temp_directory_path();

	SetToken("?user", WinGetFolderPath(CSIDL_APPDATA)/"Aegisub");
	SetToken("?local", WinGetFolderPath(CSIDL_LOCAL_APPDATA)/"Aegisub");

	/// @todo error checking
	int argc;
	LPWSTR *argv = CommandLineToArgvW(L"", &argc);
	SetToken("?data", argv[0]);
	LocalFree(argv);

	SetToken("?dictionary", Decode("?data/dictionaries"));
	//SetToken("?docs", Decode("?data/docs"));
}

}
