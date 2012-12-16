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

#include <cassert>
#include <memory>
#include <sstream>

#include "libaegisub/cajun/reader.h"
#include "libaegisub/cajun/writer.h"
#include "libaegisub/cajun/elements.h"

#include "libaegisub/io.h"
#include "libaegisub/log.h"
#include "libaegisub/option_value.h"

#include "option_visit.h"

#include <boost/range/adaptor/map.hpp>

namespace {
	/// @brief Write an option to a json object
	/// @param[out] obj  Parent object
	/// @param[in] path  Path option should be stored in.
	/// @param[in] value Value to write.
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

Options::Options(const std::string &file, const std::string& default_config, const OptionSetting setting)
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

	for (auto option_value : values | boost::adaptors::map_values)
		delete option_value;
}

void Options::ConfigNext(std::istream& stream) {
	LoadConfig(stream);
}

void Options::ConfigUser() {
	std::unique_ptr<std::istream> stream;

	try {
		stream.reset(agi::io::Open(config_file));
	} catch (const FileNotFoundError&) {
		return;
	}

	/// @todo Handle other errors such as parsing and notifying the user.
	LoadConfig(*stream, true);
}

void Options::LoadConfig(std::istream& stream, bool ignore_errors) {
	/// @todo Store all previously loaded configs in an array for bug report purposes,
	///       this is just a temp stub.
	json::UnknownElement config_root;

	try {
		json::Reader::Read(config_root, stream);
	} catch (json::Reader::ParseException& e) {
		LOG_E("option/load") << "json::ParseException: " << e.what() << ", Line/offset: " << e.m_locTokenBegin.m_nLine + 1 << '/' << e.m_locTokenBegin.m_nLineOffset + 1;
	} catch (json::Exception& e) {
		/// @todo Do something better here, maybe print the exact error
		LOG_E("option/load") << "json::Exception: " << e.what();
	}

	ConfigVisitor config_visitor(values, "", ignore_errors, !ignore_errors);
	config_root.Accept(config_visitor);
}

OptionValue* Options::Get(const std::string &name) {
	auto index = values.find(name);
	if (index != values.end())
		return index->second;

	LOG_E("option/get") << "agi::Options::Get Option not found: (" << name << ")";
	throw OptionErrorNotFound("Option value not found: " + name);
}

void Options::Flush() const {
	json::Object obj_out;

	for (auto const& ov : values) {
		switch (ov.second->GetType()) {
			case OptionValue::Type_String:
				put_option(obj_out, ov.first, ov.second->GetString());
				break;

			case OptionValue::Type_Int:
				put_option(obj_out, ov.first, ov.second->GetInt());
				break;

			case OptionValue::Type_Double:
				put_option(obj_out, ov.first, ov.second->GetDouble());
				break;

			case OptionValue::Type_Color:
				put_option(obj_out, ov.first, ov.second->GetColor().GetRgbFormatted());
				break;

			case OptionValue::Type_Bool:
				put_option(obj_out, ov.first, ov.second->GetBool());
				break;

			case OptionValue::Type_List_String:
				put_array(obj_out, ov.first, "string", ov.second->GetListString());
				break;

			case OptionValue::Type_List_Int:
				put_array(obj_out, ov.first, "int", ov.second->GetListInt());
				break;

			case OptionValue::Type_List_Double:
				put_array(obj_out, ov.first, "double", ov.second->GetListDouble());
				break;

			case OptionValue::Type_List_Color:
				put_array(obj_out, ov.first, "color", ov.second->GetListColor());
				break;

			case OptionValue::Type_List_Bool:
				put_array(obj_out, ov.first, "bool", ov.second->GetListBool());
				break;
		}
	}

	json::Writer::Write(obj_out, io::Save(config_file).Get());
}

} // namespace agi
