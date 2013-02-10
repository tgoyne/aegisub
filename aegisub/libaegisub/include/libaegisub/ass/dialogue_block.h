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

#pragma once

#include <boost/variant.hpp>
#include <string>
#include <vector>

namespace agi { namespace ass {
enum class ParameterClass {
	NORMAL,
	ABSOLUTE_SIZE,
	POS_X,
	POS_Y,
	SIZE_X,
	SIZE_Y,
	TIME_START,
	TIME_END,
	KARAOKE,
	DRAWING,
	ALPHA
};

struct OverrideParameter {
	std::string value;
	ParameterClass classification;
};

struct OverrideTag {
	size_t begin;
	size_t end;
	std::string name;
	std::vector<OverrideParameter> params;
};

struct BlockBase {
	size_t begin;
	size_t end;
};

struct TextBlock : public BlockBase {
	std::string text;
};

struct PlainBlock : public TextBlock { };
struct CommentBlock : public TextBlock { };
struct DrawingBlock : public TextBlock { };
struct OverrideBlock : public BlockBase { std::vector<OverrideTag> Tags; };

typedef boost::variant<PlainBlock, CommentBlock, DrawingBlock, OverrideBlock> DialogueBlock;

std::string GetText(DialogueBlock const& block);
std::string GetText(OverrideTag const& tag);

template<typename T> T Get(OverrideTag const& tag, size_t idx) { return Get<T>(tag.params[idx]); }
template<typename T> T Get(OverrideTag const& tag, size_t idx, T const& def) {
	return idx >= tag.params.size() ? def : Get<T>(tag.params[idx]);
}
template<typename T> void Set(OverrideTag& tag, size_t idx, T const& value) { Set(tag.params[idx], value); }

template<typename T> T Get(OverrideParameter const& parameter);
template<typename T> void Set(OverrideParameter& parameter, T const& value);

std::string TransformDrawing(std::string const& str, int shift_x, int shift_y, double scale_x, double scale_y);
std::string SetTag(std::string const& text, size_t position, std::string const& tag_name, std::string const& value, std::pair<size_t, size_t> *tag_pos = nullptr);

template<typename Range, typename Func>
void VisitTags(Range& r, Func f) {
	bool stop = false;
	for (auto& b : r) {
		if (auto ovr = boost::get<OverrideBlock>(&b)) {
			for (auto& t : ovr->Tags) {
				f(t, &stop);
				if (stop) break;
			}
		}
	}
}

template<typename Range, typename Func>
void VisitParameters(Range& r, Func f) {
	VisitTags(r, [&](OverrideTag& tag, bool *stop) {
		for (auto& p : tag.params) {
			f(p, stop);
			if (*stop) break;
		}
	});
}

template<typename ResultType, typename Impl>
struct Visitor : public boost::static_visitor<ResultType>, public Impl {
	Visitor(Impl impl) : Impl(impl) { }
};

template<typename ResultType, typename Impl>
Visitor<ResultType, Impl> AddResultType(Impl impl) {
	return impl;
}

namespace tags {
	struct move;
	struct pos {
		typedef move alternate;
		typedef std::tuple<double, double> value_type;
		static const char *name() { return "pos"; }
	};
	struct move {
		typedef pos alternate;
		typedef std::tuple<double, double, double, double, int, int> value_type;
		static const char *name() { return "move"; }
	};
}

template<typename Tag>
auto GetValue(std::string const& text, AssStyle *style, size_t position, Tag) -> typename Tag::value_type {
	auto blocks = Parse(text);
	Tag::value_type ret;
	bool got_value = false;
	VisitTags(blocks, [&](OverrideTag& tag, bool *stop) {
		if (tag.name == tag_name<Tag>() || tag.name == alternate_name<Tag>())
			parse_tag<Tag>(ret, tag);
			*stop = true;
			got_value = true;
		}
	});

	if (!got_value)
		set_default<Tag>(ret, style);

	return ret;
}

namespace param_types {
	struct normal;
	struct absolute_size;
	struct pos_x;
	struct pos_y;
	struct size_x;
	struct size_y;
	struct time_start;
	struct time_end;
	struct karaoke;
	struct drawing;
	struct alpha;
}


template<typename Tag, int N> struct param_type { typedef param_types::normal type; };
template<> struct param_type<tags::pos, 0> { typedef param_types::pos_x type; };
template<> struct param_type<tags::pos, 1> { typedef param_types::pos_y type; };

template<typename Tag> std::string tag_name();
template<> std::string tag_name<tags::move>() { return "move"; }
template<> std::string tag_name<tags::pos>() { return "pos"; }

template<typename Tag> std::string alternate_name() { return ""; }
template<> std::string alternate_name<tags::move>() { return "pos"; }
template<> std::string alternate_name<tags::pos>() { return "move"; }

template<typename Tag> struct value_type;
template<> struct value_type<tags::move> { typedef std::tuple<int, int> type; };
template<> struct value_type<tags::pos> { typedef std::tuple<int, int> type; };

template<typename Tag, int N, typename Value> void set_default(Value& value);

template<typename Tag, int N>
std::enable_if<!std::is_same<param_type<Tag, N>::type, param_types::alpha>::type
parse_value(int& value, OverrideParameter const& param) {
	value = atoi(param.value.c_str());
}

template<typename Tag, int N>
std::enable_if<std::is_same<param_type<Tag, N>::type, param_types::alpha>::type
parse_value(int& value, OverrideParameter const& p) {
	// &Hxx&, but vsfilter lets you leave everything out
	value = util::mid<int>(0, strtol(std::find_if(p.value.c_str(), p.value.c_str() + p.value.size(), isxdigit), nullptr, 16), 255);
}

template<typename Tag, int N>
void parse_value(double& value, OverrideParameter const& param) {
	value = atof(param.value.c_str());
}

template<typename Tag, int N>
void parse_value(bool& value, OverrideParameter const& param) {
	value = atoi(param.value.c_str()) != 0;
}

template<typename Tag, int N>
void parse_value(agi::Color& value, OverrideParameter const& param) {
	value = param.value;
}


template<> int Get<int>(OverrideParameter const& p) {
	if (p.classification == ParameterClass::ALPHA)
	return atoi(Get<std::string>(p).c_str());
}

// Template types can be partially specialized, but functions can't
template<typename Tag, int N>
struct parse_value {
	parse_value(typename value_type<Tag>::type& ret, OverrideTag const& tag) {
		if (N < tag.params.size())
			parse_value<Tag, N>(get<N>(ret), tag.params[N]);
		else
			set_default<Tag, N>(get<N>(ret));
		parse_value<Tag, N + 1>(ret, tag);
	}
};

// Recursion base case
template<typename Tag>
struct parse_value<std::tuple_size<value_type<Tag>::type>::value> {
	parse_value(typename value_type<Tag>::type& ret, OverrideTag const& tag) { }
};

template<typename Tag> void parse_value(typename value_type<Tag>::type& ret, OverrideTag const& tag) {
	parse_value<Tag, 0>(ret, tag);
}


template<typename Tag>
std::string SetValue(std::string const& text, size_t position, typename Tag::value_type const& value, Tag) {

}

void usage() {
	auto c1 = GetValue("{\\cdongs}dongs", nullptr, 0, tags::primary_color);
}

} }
