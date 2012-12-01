// Copyright (c) 2005, Rodrigo Braz Monteiro
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

/// @file ass_style_storage.cpp
/// @brief Manage stores of styles
/// @ingroup style_editor
///

#include "config.h"

#include "ass_style_storage.h"

#include <boost/algorithm/string/predicate.hpp>
#include <functional>

#include "ass_style.h"
#include "compat.h"
#include "standard_paths.h"
#include "utils.h"

#include <libaegisub/io.h>
#include <libaegisub/line_iterator.h>

AssStyleStorage::~AssStyleStorage() {
	delete_clear(style);
}

void AssStyleStorage::Save() const {
	if (storage_name.empty()) return;

	std::string dirname(StandardPaths::DecodePath("?user/catalog/"));
	if (!wxDirExists(to_wx(dirname)) && !wxMkdir(to_wx(dirname)))
		throw "Failed creating directory for style catalogs";

	agi::io::Save file(dirname + storage_name + ".sty");
	transform(style.begin(), style.end(),
		std::ostream_iterator<std::string>(file.Get(), "\n"),
		std::mem_fn(&AssStyle::GetEntryData));
}

void AssStyleStorage::Load(std::string const& name) {
	storage_name = name;
	Clear();

	try {
		agi::scoped_ptr<std::istream> file(agi::io::Open(StandardPaths::DecodePath("?user/catalog/" + name + ".sty")));
		for (std::string const& line : boost::make_iterator_range(agi::line_iterator<std::string>(*file, "UTF-8"), agi::line_iterator<std::string>())) {
			if (boost::starts_with(line, "Style:")) {
				try {
					style.push_back(new AssStyle(line));
				} catch(...) {
					/* just ignore invalid lines for now */
				}
			}
		}
	}
	catch (agi::FileNotAccessibleError const&) {
		// Just treat a missing file as an empty file
	}
}

void AssStyleStorage::Clear() {
	delete_clear(style);
}

void AssStyleStorage::Delete(int idx) {
	delete style[idx];
	style.erase(style.begin() + idx);
}

std::vector<std::string> AssStyleStorage::GetNames() {
	std::vector<std::string> names;
	for (const AssStyle *cur : style)
		names.push_back(cur->name);
	return names;
}

AssStyle *AssStyleStorage::GetStyle(std::string const& name) {
	for (AssStyle *cur : style) {
		if (boost::iequals(name, cur->name))
			return cur;
	}
	return 0;
}
