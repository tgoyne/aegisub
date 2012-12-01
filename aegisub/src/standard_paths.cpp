// Copyright (c) 2007, Rodrigo Braz Monteiro
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of the Aegisub Group nor the names of its contributors
//     may be used to endorse or promote products derived from this software
//     without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Aegisub Project http://www.aegisub.org/

/// @file standard_paths.cpp
/// @brief Encode and decode paths relative to various special locations
/// @ingroup utility
///

#include "config.h"

#include "standard_paths.h"

#include "compat.h"

#include <boost/algorithm/string/replace.hpp>

#include <wx/filename.h>
#include <wx/stdpaths.h>

StandardPaths &StandardPaths::GetInstance() {
	static StandardPaths instance;
	return instance;
}

StandardPaths::StandardPaths() {
   wxStandardPathsBase &paths = wxStandardPaths::Get();

#if defined(__UNIX__) && !defined(__APPLE__)
   // Relocation support, this is required to set the prefix to all
   // wx StandardPaths.
   static_cast<wxStandardPaths&>(paths).SetInstallPrefix(wxT(INSTALL_PREFIX));
#endif

	DoSetPathValue("?data", from_wx(paths.GetDataDir()));
	DoSetPathValue("?user", from_wx(paths.GetUserDataDir()));
	DoSetPathValue("?local", from_wx(paths.GetUserLocalDataDir()));
	DoSetPathValue("?temp", from_wx(paths.GetTempDir()));

	// Create paths if they don't exist
	if (!wxDirExists(paths.GetUserDataDir()))
		wxMkDir(paths.GetUserDataDir(), 0777);
	if (!wxDirExists(paths.GetUserLocalDataDir()))
		wxMkDir(paths.GetUserLocalDataDir(), 0777);
}

std::string StandardPaths::DoDecodePath(std::string path) {
	if (path.empty() || path[0] != '?')
		return path;

	// Split ?part from rest
	replace(begin(path), end(path), '\\', '/');
	auto pos = find(begin(path), end(path), '/');

	// Replace ?part if valid
	auto iter = paths.find(std::string(begin(path), pos));
	if (iter == paths.end()) return path;
	std::string final = iter->second + "/" + std::string(pos + 1, end(path));
	boost::replace_all(final, "//", "/");
#ifdef WIN32
	replace(begin(final), end(final), '/', '\\');
#endif
	return final;
}

void StandardPaths::DoSetPathValue(const std::string &path, const std::string &value) {
	paths[path] = value;
}
