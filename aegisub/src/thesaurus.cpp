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
// Aegisub Project http://www.aegisub.org/

/// @file thesaurus.cpp
/// @brief Thesaurus implementation
/// @ingroup thesaurus
///

#include "config.h"

#include "thesaurus.h"

#include "options.h"

#include <boost/format.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/range/algorithm.hpp>

#include <libaegisub/dispatch.h>
#include <libaegisub/fs.h>
#include <libaegisub/log.h>
#include <libaegisub/lzma_filter.h>
#include <libaegisub/path.h>
#include <libaegisub/thesaurus.h>
#include <libaegisub/util.h>

Thesaurus::Thesaurus()
: lang_listener(OPT_SUB("Tool/Thesaurus/Language", &Thesaurus::OnLanguageChanged, this))
, dict_path_listener(OPT_SUB("Path/Dictionary", &Thesaurus::OnPathChanged, this))
{
	OnLanguageChanged();
}

Thesaurus::~Thesaurus() {
	// Explicit empty destructor needed for scoped_ptr with incomplete types
}

std::vector<Thesaurus::Entry> Thesaurus::Lookup(std::string word) {
	if (!impl.get()) return {};
	boost::to_lower(word);
	return impl->Lookup(word);
}

static std::vector<std::string> langs(const char *ext) {
	std::vector<std::string> paths;
	auto data_path = config::path->Decode("?data/dictionaries/");
	auto user_path = config::path->Decode(OPT_GET("Path/Dictionary")->GetString());

	auto filter = std::string("th_*.") + ext;
	agi::fs::DirectoryIterator(data_path, filter).GetAll(paths);
	agi::fs::DirectoryIterator(user_path, filter).GetAll(paths);

	// Drop extensions and the th_ prefix
	for (auto& fn : paths) fn = fn.substr(3, fn.size() - filter.size() + 1);

#ifdef WITH_XZ
	size_t uncompressed_count = paths.size();
	filter += ".xz";
	agi::fs::DirectoryIterator(data_path, filter).GetAll(paths);
	agi::fs::DirectoryIterator(user_path, filter).GetAll(paths);
	for (size_t i = uncompressed_count; i < paths.size(); ++i)
		paths[i] = paths[i].substr(3, paths[i].size() - filter.size() + 1);
#endif

	boost::sort(paths);
	paths.erase(unique(begin(paths), end(paths)), end(paths));

	return paths;
}

std::vector<std::string> Thesaurus::GetLanguageList() const {
	if (languages.empty())
		boost::set_intersection(langs("idx"), langs("dat"), back_inserter(languages));
	return languages;
}

static bool check_path(agi::fs::path const& path, std::string const& language, agi::fs::path& idx, agi::fs::path& dat) {
	idx = path/str(boost::format("th_%s.idx") % language);
	dat = path/str(boost::format("th_%s.dat") % language);

#ifdef WITH_XZ
	if (agi::fs::FileExists(idx) && agi::fs::FileExists(dat))
		return true;
	idx = path/str(boost::format("th_%s.idx.xz") % language);
	dat = path/str(boost::format("th_%s.dat.xz") % language);
#endif

	return agi::fs::FileExists(idx) && agi::fs::FileExists(dat);
}

void Thesaurus::OnLanguageChanged() {
	impl.reset();

	auto language = OPT_GET("Tool/Thesaurus/Language")->GetString();
	if (language.empty()) return;

	agi::fs::path idx, dat;

	auto path = config::path->Decode(OPT_GET("Path/Dictionary")->GetString() + "/");
	if (!check_path(path, language, idx, dat)) {
		path = config::path->Decode("?data/dictionaries/");
		if (!check_path(path, language, idx, dat))
			return;
	}

	LOG_I("thesaurus/file") << "Using thesaurus: " << dat;

	agi::dispatch::Background().Async([=]{
		try {
			auto thes = agi::util::make_unique<agi::Thesaurus>(dat, idx);
			agi::dispatch::Main().Sync([&]{
				impl = std::move(thes);
			});
		}
		catch (agi::Exception const& e) {
			LOG_E("thesaurus") << e.GetChainedMessage();
		}
	});
}

void Thesaurus::OnPathChanged() {
	languages.clear();
}
