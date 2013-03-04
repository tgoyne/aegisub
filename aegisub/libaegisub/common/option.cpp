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

/// @file option.cpp
/// @brief Option interface.
/// @ingroup libaegisub

#include "../config.h"

#include "libaegisub/option.h"

#include "libaegisub/cajun/reader.h"
#include "libaegisub/cajun/writer.h"
#include "libaegisub/cajun/elements.h"

#include "libaegisub/fs.h"
#include "libaegisub/io.h"
#include "libaegisub/log.h"
#include "libaegisub/option_value.h"

#include "option_visit.h"

#include <cassert>
#include <memory>
#include <sstream>

namespace {
	void put_option(json::Object &obj, const std::string &path, const json::UnknownElement &value) {
		std::string::size_type pos = path.find('/');
		// Not having a '/' denotes it is a leaf.
		if (pos == std::string::npos) {
			assert(obj.find(path) == obj.end());
			obj[path] = value;
		}
		else
			put_option(obj[path.substr(0, pos)], path.substr(pos + 1), value);
	}

	template<class T>
	void put_array(json::Object &obj, const std::string &path, const char *element_key, std::vector<T> const& value) {
		json::Array array;
		for (T const& elem : value) {
			array.push_back(json::Object());
			static_cast<json::Object&>(array.back())[element_key] = (json::UnknownElement)elem;
		}

		put_option(obj, path, array);
	}
}

namespace agi {

Options::Options(agi::fs::path const& file, const std::string& default_config, const OptionSetting setting)
: config_file(file)
, setting(setting)
{
	LOG_D("agi/options") << "New Options object";
	std::istringstream stream(default_config);
	LoadConfig(stream);
}

Options::~Options() {
	if ((setting & FLUSH_SKIP) != FLUSH_SKIP)
		Flush();
}

void Options::ConfigNext(std::istream& stream) {
	LoadConfig(stream);
}

void Options::ConfigUser() {
	try {
		std::unique_ptr<std::istream> stream(io::Open(config_file));
		LoadConfig(*stream, true);
	}
	catch (fs::FileNotFound const&) {
		return;
	}
}

void Options::LoadConfig(std::istream& stream, bool ignore_errors) {
	json::UnknownElement config_root;

	try {
		json::Reader::Read(config_root, stream);
	}
	catch (json::Reader::ParseException& e) {
		LOG_E("option/load") << "json::ParseException: " << e.what() << ", Line/offset: " << e.m_locTokenBegin.m_nLine + 1 << '/' << e.m_locTokenBegin.m_nLineOffset + 1;
	}
	catch (json::Exception& e) {
		LOG_E("option/load") << "json::Exception: " << e.what();
	}

	ConfigVisitor config_visitor(values, "", ignore_errors, !ignore_errors);
	config_root.Accept(config_visitor);
}

OptionValue* Options::Get(const std::string &name) {
	auto index = values.find(name);
	if (index != values.end())
		return &index->second;

	LOG_E("option/get") << "agi::Options::Get Option not found: (" << name << ")";
	throw OptionErrorNotFound("Option value not found: " + name);
}

struct flush_visitor : boost::static_visitor<void> {
	json::Object obj_out;
	const std::string *name;

	template<typename T>
	void operator()(T const& value) {
		put_option(obj_out, *name, value);
	}

	void operator()(agi::Color const& value) {
		put_option(obj_out, *name, value.GetRgbFormatted());
	}

	template<typename T>
	void operator()(std::vector<T> const& value) {
		put_array(obj_out, *name, "dongs", value);
	}
};

void Options::Flush() const {
	flush_visitor visitor;

	for (auto const& ov : values) {
		visitor.name = &ov.first;
		boost::apply_visitor(visitor, ov.second);
	}

	json::Writer::Write(visitor.obj_out, io::Save(config_file).Get());
}

} // namespace agi
