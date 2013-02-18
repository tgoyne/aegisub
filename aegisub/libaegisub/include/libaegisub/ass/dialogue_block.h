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

#include <libaegisub/color.h>
#include <libaegisub/util.h>

#include <boost/format.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_int.hpp>
#include <boost/spirit/include/karma_real.hpp>
#include <boost/variant.hpp>
#include <string>
#include <vector>
#include <tuple>

namespace agi { namespace ass {

namespace detail {

namespace tags {
	struct alpha;
	struct an;
	struct b;
	struct be;
	struct blur;
	struct bord;
	struct clip;
	struct fad;
	struct fade;
	struct fax;
	struct fay;
	struct fe;
	struct fn;
	struct frx;
	struct fry;
	struct frz;
	struct fs;
	struct fs_minus;
	struct fs_plus;
	struct fscx;
	struct fscy;
	struct fsp;
	struct i;
	struct iclip;
	struct k;
	struct kf;
	struct ko;
	struct move;
	struct org;
	struct outline_alpha;
	struct outline_color;
	struct p;
	struct pbo;
	struct pos;
	struct primary_alpha;
	struct primary_color;
	struct q;
	struct r;
	struct s;
	struct secondary_alpha;
	struct secondary_color;
	struct shad;
	struct shadow_alpha;
	struct shadow_color;
	struct t;
	struct u;
	struct xbord;
	struct xshad;
	struct ybord;
	struct yshad;

	struct none;
}

#define DIALOGUE_BLOCK_H_PARAMETER_TYPE(name, backing_type) \
	struct name { \
		backing_type value; \
		name() { } \
		name(backing_type value) : value(value) { } \
		operator backing_type() const { return value; } \
	}

namespace parameter_type {
	DIALOGUE_BLOCK_H_PARAMETER_TYPE(absolute_size, double);
	DIALOGUE_BLOCK_H_PARAMETER_TYPE(alpha, int);
	DIALOGUE_BLOCK_H_PARAMETER_TYPE(drawing, std::string);
	DIALOGUE_BLOCK_H_PARAMETER_TYPE(karaoke, int);
	DIALOGUE_BLOCK_H_PARAMETER_TYPE(pos_x, double);
	DIALOGUE_BLOCK_H_PARAMETER_TYPE(pos_y, double);
	DIALOGUE_BLOCK_H_PARAMETER_TYPE(size_x, double);
	DIALOGUE_BLOCK_H_PARAMETER_TYPE(size_y, double);
	DIALOGUE_BLOCK_H_PARAMETER_TYPE(string, double);
	DIALOGUE_BLOCK_H_PARAMETER_TYPE(time_end, double);
	DIALOGUE_BLOCK_H_PARAMETER_TYPE(time_start, double);

	typedef boost::variant<std::tuple<pos_x, pos_y, pos_x, pos_y>, std::tuple<int, drawing>> clip;
}

#undef DIALOGUE_BLOCK_H_PARAMETER_TYPE

// The preferred name of this tag. No default since all tags must have a name.
template<typename Tag> struct tag_name;

// The name of a tag which sets this tag's value along with other properties.
// Get will read the value of this tag, but Set won't replace it.
template<typename Tag> struct tag_parent { typedef tags::none type; };

// The alternate name for this tage, which is identical to the primary name
template<typename Tag> struct tag_alias { typedef tags::none type; };

// The data type of this tag
template<typename Tag> struct tag_type;


// Tag names
template<> struct tag_name<tags::alpha>           { static const char *name() { return "alpha"; } };
template<> struct tag_name<tags::an>              { static const char *name() { return "an";    } };
template<> struct tag_name<tags::b>               { static const char *name() { return "b";     } };
template<> struct tag_name<tags::be>              { static const char *name() { return "be";    } };
template<> struct tag_name<tags::blur>            { static const char *name() { return "blur";  } };
template<> struct tag_name<tags::bord>            { static const char *name() { return "bord";  } };
template<> struct tag_name<tags::clip>            { static const char *name() { return "clip";  } };
template<> struct tag_name<tags::fad>             { static const char *name() { return "fad";   } };
template<> struct tag_name<tags::fade>            { static const char *name() { return "fade";  } };
template<> struct tag_name<tags::fax>             { static const char *name() { return "fax";   } };
template<> struct tag_name<tags::fay>             { static const char *name() { return "fay";   } };
template<> struct tag_name<tags::fe>              { static const char *name() { return "fe";    } };
template<> struct tag_name<tags::fn>              { static const char *name() { return "fn";    } };
template<> struct tag_name<tags::frx>             { static const char *name() { return "frx";   } };
template<> struct tag_name<tags::fry>             { static const char *name() { return "fry";   } };
template<> struct tag_name<tags::frz>             { static const char *name() { return "frz";   } };
template<> struct tag_name<tags::fs>              { static const char *name() { return "fs";    } };
template<> struct tag_name<tags::fs_minus>        { static const char *name() { return "fs-";   } };
template<> struct tag_name<tags::fs_plus>         { static const char *name() { return "fs+";   } };
template<> struct tag_name<tags::fscx>            { static const char *name() { return "fscx";  } };
template<> struct tag_name<tags::fscy>            { static const char *name() { return "fscy";  } };
template<> struct tag_name<tags::fsp>             { static const char *name() { return "fsp";   } };
template<> struct tag_name<tags::i>               { static const char *name() { return "i";     } };
template<> struct tag_name<tags::iclip>           { static const char *name() { return "iclip"; } };
template<> struct tag_name<tags::k>               { static const char *name() { return "k";     } };
template<> struct tag_name<tags::kf>              { static const char *name() { return "kf";    } };
template<> struct tag_name<tags::ko>              { static const char *name() { return "ko";    } };
template<> struct tag_name<tags::move>            { static const char *name() { return "move";  } };
template<> struct tag_name<tags::org>             { static const char *name() { return "org";   } };
template<> struct tag_name<tags::outline_alpha>   { static const char *name() { return "3a";    } };
template<> struct tag_name<tags::outline_color>   { static const char *name() { return "3c";    } };
template<> struct tag_name<tags::p>               { static const char *name() { return "p";     } };
template<> struct tag_name<tags::pbo>             { static const char *name() { return "pbo";   } };
template<> struct tag_name<tags::pos>             { static const char *name() { return "pos";   } };
template<> struct tag_name<tags::primary_alpha>   { static const char *name() { return "1a";    } };
template<> struct tag_name<tags::primary_color>   { static const char *name() { return "c";     } };
template<> struct tag_name<tags::q>               { static const char *name() { return "q";     } };
template<> struct tag_name<tags::r>               { static const char *name() { return "r";     } };
template<> struct tag_name<tags::s>               { static const char *name() { return "s";     } };
template<> struct tag_name<tags::secondary_alpha> { static const char *name() { return "2a";    } };
template<> struct tag_name<tags::secondary_color> { static const char *name() { return "2c";    } };
template<> struct tag_name<tags::shad>            { static const char *name() { return "shad";  } };
template<> struct tag_name<tags::shadow_alpha>    { static const char *name() { return "4a";    } };
template<> struct tag_name<tags::shadow_color>    { static const char *name() { return "4c";    } };
template<> struct tag_name<tags::t>               { static const char *name() { return "t";     } };
template<> struct tag_name<tags::u>               { static const char *name() { return "u";     } };
template<> struct tag_name<tags::xbord>           { static const char *name() { return "xbord"; } };
template<> struct tag_name<tags::xshad>           { static const char *name() { return "xshad"; } };
template<> struct tag_name<tags::ybord>           { static const char *name() { return "ybord"; } };
template<> struct tag_name<tags::yshad>           { static const char *name() { return "yshad"; } };

// Tag parents
template<> struct tag_parent<tags::outline_alpha>   { typedef tags::alpha type; };
template<> struct tag_parent<tags::primary_alpha>   { typedef tags::alpha type; };
template<> struct tag_parent<tags::secondary_alpha> { typedef tags::alpha type; };
template<> struct tag_parent<tags::shadow_alpha>    { typedef tags::alpha type; };
template<> struct tag_parent<tags::xbord>           { typedef tags::bord type; };
template<> struct tag_parent<tags::xshad>           { typedef tags::shad type; };
template<> struct tag_parent<tags::ybord>           { typedef tags::bord type; };
template<> struct tag_parent<tags::yshad>           { typedef tags::shad type; };

// Tag aliases
template<> struct tag_alias<tags::an>   { static const char *name() { return "a";    } };
template<> struct tag_alias<tags::frz>  { static const char *name() { return "fr";   } };
template<> struct tag_alias<tags::kf>   { static const char *name() { return "K";    } };
template<> struct tag_alias<tags::pos>  { static const char *name() { return "move"; } };
template<> struct tag_alias<tags::move> { static const char *name() { return "pos";  } };

// Tag data types
template<> struct tag_type<tags::alpha>           { typedef parameter_type::alpha type; };
template<> struct tag_type<tags::an>              { typedef int type; };
template<> struct tag_type<tags::b>               { typedef int type; };
template<> struct tag_type<tags::be>              { typedef int type; };
template<> struct tag_type<tags::blur>            { typedef double type; };
template<> struct tag_type<tags::bord>            { typedef parameter_type::absolute_size type; };
template<> struct tag_type<tags::clip>            { typedef parameter_type::clip type; };
template<> struct tag_type<tags::fad>             { typedef std::tuple<parameter_type::time_start, parameter_type::time_end> type; };
template<> struct tag_type<tags::fade>            { typedef std::tuple<parameter_type::alpha, parameter_type::alpha, parameter_type::alpha, parameter_type::time_start, parameter_type::time_start, parameter_type::time_start, parameter_type::time_start> type; };
template<> struct tag_type<tags::fax>             { typedef double type; };
template<> struct tag_type<tags::fay>             { typedef double type; };
template<> struct tag_type<tags::fe>              { typedef int type; };
template<> struct tag_type<tags::fn>              { typedef std::string type; };
template<> struct tag_type<tags::frx>             { typedef double type; };
template<> struct tag_type<tags::fry>             { typedef double type; };
template<> struct tag_type<tags::frz>             { typedef double type; };
template<> struct tag_type<tags::fs>              { typedef parameter_type::absolute_size type; };
template<> struct tag_type<tags::fs_minus>        { typedef int type; };
template<> struct tag_type<tags::fs_plus>         { typedef int type; };
template<> struct tag_type<tags::fscx>            { typedef parameter_type::size_x type; };
template<> struct tag_type<tags::fscy>            { typedef parameter_type::size_y type; };
template<> struct tag_type<tags::fsp>             { typedef parameter_type::absolute_size type; };
template<> struct tag_type<tags::i>               { typedef int type; };
template<> struct tag_type<tags::iclip>           { typedef parameter_type::clip type; };
template<> struct tag_type<tags::k>               { typedef parameter_type::karaoke type; };
template<> struct tag_type<tags::kf>              { typedef parameter_type::karaoke type; };
template<> struct tag_type<tags::ko>              { typedef parameter_type::karaoke type; };
template<> struct tag_type<tags::move>            { typedef std::tuple<parameter_type::pos_x, parameter_type::pos_y, parameter_type::pos_x, parameter_type::pos_y, parameter_type::time_start, parameter_type::time_start> type; };
template<> struct tag_type<tags::org>             { typedef std::tuple<parameter_type::pos_x, parameter_type::pos_y> type; };
template<> struct tag_type<tags::outline_alpha>   { typedef parameter_type::alpha type; };
template<> struct tag_type<tags::outline_color>   { typedef agi::Color type; };
template<> struct tag_type<tags::p>               { typedef int type; };
template<> struct tag_type<tags::pbo>             { typedef parameter_type::pos_y type; };
template<> struct tag_type<tags::pos>             { typedef std::tuple<parameter_type::pos_x, parameter_type::pos_y> type; };
template<> struct tag_type<tags::primary_alpha>   { typedef parameter_type::alpha type; };
template<> struct tag_type<tags::primary_color>   { typedef agi::Color type; };
template<> struct tag_type<tags::q>               { typedef int type; };
template<> struct tag_type<tags::r>               { typedef std::string type; };
template<> struct tag_type<tags::s>               { typedef int type; };
template<> struct tag_type<tags::secondary_alpha> { typedef parameter_type::alpha type; };
template<> struct tag_type<tags::secondary_color> { typedef agi::Color type; };
template<> struct tag_type<tags::shad>            { typedef parameter_type::absolute_size type; };
template<> struct tag_type<tags::shadow_alpha>    { typedef parameter_type::alpha type; };
template<> struct tag_type<tags::shadow_color>    { typedef agi::Color type; };
template<> struct tag_type<tags::t>               { typedef std::tuple<parameter_type::time_start, parameter_type::time_end, double, std::string> type; };
template<> struct tag_type<tags::u>               { typedef int type; };
template<> struct tag_type<tags::xbord>           { typedef parameter_type::size_x type; };
template<> struct tag_type<tags::xshad>           { typedef parameter_type::size_x type; };
template<> struct tag_type<tags::ybord>           { typedef parameter_type::size_y type; };
template<> struct tag_type<tags::yshad>           { typedef parameter_type::size_y type; };

// Parameter parsers
#define DIALOGUE_BLOCK_H_PARSER(type, expr) \
	template<typename Tag> \
	void parse_param(type& ret, std::vector<std::string> const& toks, size_t N) { \
		auto const& p = toks[N]; \
		ret = expr; \
	}

DIALOGUE_BLOCK_H_PARSER(int, atoi(p.c_str()))
DIALOGUE_BLOCK_H_PARSER(parameter_type::karaoke, atoi(p.c_str()))

DIALOGUE_BLOCK_H_PARSER(double, atof(p.c_str()))
DIALOGUE_BLOCK_H_PARSER(parameter_type::absolute_size, atof(p.c_str()));
DIALOGUE_BLOCK_H_PARSER(parameter_type::pos_x, atof(p.c_str()));
DIALOGUE_BLOCK_H_PARSER(parameter_type::pos_y, atof(p.c_str()));
DIALOGUE_BLOCK_H_PARSER(parameter_type::size_x, atof(p.c_str()));
DIALOGUE_BLOCK_H_PARSER(parameter_type::size_y, atof(p.c_str()));
DIALOGUE_BLOCK_H_PARSER(parameter_type::string, atof(p.c_str()));
DIALOGUE_BLOCK_H_PARSER(parameter_type::time_end, atof(p.c_str()));
DIALOGUE_BLOCK_H_PARSER(parameter_type::time_start, atof(p.c_str()));

DIALOGUE_BLOCK_H_PARSER(std::string, p)
DIALOGUE_BLOCK_H_PARSER(agi::Color, p)
DIALOGUE_BLOCK_H_PARSER(parameter_type::drawing, p)

DIALOGUE_BLOCK_H_PARSER(parameter_type::alpha, util::mid<int>(0, strtol(std::find_if(p.c_str(), p.c_str() + p.size(), isxdigit), nullptr, 16), 255))

#undef DIALOGUE_BLOCK_H_PARSER

namespace iterate_tuple_detail {
	template <size_t N, typename Tuple, typename Func>
	typename std::enable_if<N == 0>::type
	iterate_tuple(Tuple const&, Func&) { }

	template <size_t N, typename Tuple, typename Func>
	typename std::enable_if<N >= 1>::type
	iterate_tuple(Tuple& tuple, Func& func) {
		func(std::get<std::tuple_size<Tuple>::value - N>(tuple));
		iterate_tuple<N - 1, Tuple, Func>(tuple, func);
	}
}

template <typename Tuple, typename Func>
void iterate_tuple(Tuple& tuple, Func& func) {
    iterate_tuple_detail::iterate_tuple<std::tuple_size<Tuple>::value>(tuple, func);
}

template<typename Tag>
struct parse_tuple {
	std::vector<std::string> const& toks;
	size_t i;
	parse_tuple(std::vector<std::string> const& toks) : toks(toks), i(0) { }

	template<typename ValueType>
	void operator()(ValueType& value) {
		if (i < toks.size())
			parse_param<Tag>(value, toks, i++);
	}
};

template<typename Tag, typename T1, typename T2>
void parse_param(boost::variant<T1, T2>& ret, std::vector<std::string> toks, size_t N) {
	parse_tuple<Tag> parser(toks);
	if (toks.size() == 4) {
		T1 v;
		iterate_tuple(v, parser);
		ret = v;
	}
	else {
		if (toks.size() == 1)
			toks.emplace(toks.begin(), "1");
		T2 v;
		iterate_tuple(v, parser);
		ret = v;
	}
}

template<typename Tag, typename Tuple>
void parse_param(Tuple& ret, std::vector<std::string> const& toks, size_t N) {
	parse_tuple<Tag> parser(toks);
	iterate_tuple(ret, parser);
}

// Parameter unparsers
#define DIALOGUE_BLOCK_H_UNPARSER(type, expr) \
	void unparse_param(std::string& out, type const& v) { \
		expr; \
	}

#define DIALOGUE_BLOCK_H_UNPARSER_ALIAS(type, real) \
	DIALOGUE_BLOCK_H_UNPARSER(type, unparse_param(out, (real)v))

void unparse_param(std::string& out, int v) {
	boost::spirit::karma::generate(back_inserter(out), boost::spirit::karma::int_, v);
}

DIALOGUE_BLOCK_H_UNPARSER_ALIAS(parameter_type::karaoke, int)

void unparse_param(std::string& out, double v) {
	boost::spirit::karma::generate(back_inserter(out), boost::spirit::karma::double_, v);
	while (out.back() == '0')
		out.pop_back();
	if (out.back() == '.')
		out.pop_back();
}

DIALOGUE_BLOCK_H_UNPARSER_ALIAS(parameter_type::absolute_size, double)
DIALOGUE_BLOCK_H_UNPARSER_ALIAS(parameter_type::pos_x, double)
DIALOGUE_BLOCK_H_UNPARSER_ALIAS(parameter_type::pos_y, double)
DIALOGUE_BLOCK_H_UNPARSER_ALIAS(parameter_type::size_x, double)
DIALOGUE_BLOCK_H_UNPARSER_ALIAS(parameter_type::size_y, double)
DIALOGUE_BLOCK_H_UNPARSER_ALIAS(parameter_type::string, double)
DIALOGUE_BLOCK_H_UNPARSER_ALIAS(parameter_type::time_end, double)
DIALOGUE_BLOCK_H_UNPARSER_ALIAS(parameter_type::time_start, double)


DIALOGUE_BLOCK_H_UNPARSER(std::string, out += v)
DIALOGUE_BLOCK_H_UNPARSER(parameter_type::drawing, out += v)
DIALOGUE_BLOCK_H_UNPARSER(agi::Color, out += v.GetAssOverrideFormatted())

DIALOGUE_BLOCK_H_UNPARSER(parameter_type::alpha, out += str(boost::format("&H%02X&") % v))

#undef DIALOGUE_BLOCK_H_UNPARSER
#undef DIALOGUE_BLOCK_H_UNPARSER_ALIAS

struct tuple_unparser {
	std::string& out;
	tuple_unparser(std::string& out) : out(out) { out += '('; }
	~tuple_unparser() { out.back() = ')'; }

	template<typename ValueType>
	void operator()(ValueType& value) {
		unparse_param(out, value);
		out += ',';
	}
};

template<typename Tuple>
void unparse_param(std::string& out, Tuple const& v) {
	tuple_unparser unparser(out);
	iterate_tuple(v, unparser);
}

struct variant_unparser : boost::static_visitor<void> {
	std::string& out;
	variant_unparser(std::string& out) : out(out) { }
	template<typename Tuple>
	void operator()(Tuple const& t) {
		unparse_param(out, t);
	}
};

template<typename T1, typename T2>
void unparse_param(std::string& out, boost::variant<T1, T2> const& v) {
	variant_unparser unparser(out);
	boost::apply_visitor(unparser, v);
}

template<typename Tag>
typename tag_type<Tag>::type parse(std::vector<std::string> const& toks) {
	typename tag_type<Tag>::type ret;
	parse_param<Tag>(ret, toks, 0);
	return ret;
}

template<typename Tag>
std::string unparse(typename tag_type<Tag>::type const& value) {
	std::string ret = "\\";
	ret += tag_name<Tag>::name();
	unparse_param(ret, value);
	return ret;
}

} // namespace detail

namespace tags = detail::tags;
using detail::parse;
using detail::unparse;

struct OverrideTag {
	size_t begin;
	size_t end;
	std::string name;
	std::vector<std::string> params;
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

#if 0
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

#endif

} }
