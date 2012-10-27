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

#include "../config.h"

#include "parser.h"

#include "libaegisub/color.h"
#include "libaegisub/ass/style.h"

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

BOOST_FUSION_ADAPT_STRUCT(
	agi::Color,
	(unsigned char, r)
	(unsigned char, g)
	(unsigned char, b)
	(unsigned char, a)
)

typedef std::tr1::array<int, 3> MarginArray;
BOOST_FUSION_ADAPT_STRUCT(
	agi::ass::Style,
	//(std::string, name)
	//(std::string, font)
	(double, fontsize)
	//(agi::Color, primary)
	//(agi::Color, secondary)
	//(agi::Color, outline)
	//(agi::Color, shadow)
	(bool, bold)
	(bool, italic)
	(bool, underline)
	(bool, strikeout)
	(double, scalex)
	(double, scaley)
	(double, spacing)
	(double, angle)
	(int, borderstyle)
	(double, outline_w)
	(double, shadow_w)
	(int, alignment)
	//(MarginArray, Margin)
	(int, encoding)
)

namespace {
using namespace boost::spirit;

struct unpack_colors : public boost::static_visitor<agi::Color> {
	template<typename T> struct result { typedef agi::Color type; };

	template<class T> agi::Color operator()(T arg) const {
		return boost::apply_visitor(*this, arg);
	}

	agi::Color operator()(int abgr) const { return (*this)((unsigned)abgr); }
	agi::Color operator()(unsigned int abgr) const {
		return agi::Color(abgr & 0xFF, (abgr >> 8) & 0xFF, (abgr >> 16) & 0xFF, (abgr >> 24) & 0xFF);
	}
};

template<typename Iterator>
struct color_grammar : qi::grammar<Iterator, agi::Color()> {
	qi::rule<Iterator, agi::Color()> color;

	qi::rule<Iterator, agi::Color()> css_color;
	qi::rule<Iterator, agi::Color()> ass_color;

	qi::rule<Iterator, unsigned char()> rgb_component;
	qi::rule<Iterator, unsigned char()> rgb_percent;
	qi::rule<Iterator, unsigned char()> hex_byte;
	qi::rule<Iterator, unsigned char()> hex_char;

	qi::rule<Iterator> comma;
	qi::rule<Iterator> blank;

#define HEX_PARSER(type, len) qi::uint_parser<unsigned type, 16, len, len>()

	color_grammar() : color_grammar::base_type(color, "color") {
		color = css_color | ass_color;

		boost::phoenix::function<unpack_colors> unpack;
		ass_color = (
			int_
			| -lit('&') >> -(lit('H') | lit('h')) >> (HEX_PARSER(int, 8) | HEX_PARSER(int, 6)) >> -lit('&')
		)[_val = unpack(_1)] >> blank >> qi::eoi;

		css_color
			= "rgb(" >> blank >> rgb_component >> comma >> rgb_component >> comma >> rgb_component >> blank >> ')'
			| '#' >> hex_byte >> hex_byte >> hex_byte
			| '#' >> hex_char >> hex_char >> hex_char
			;

		hex_char = HEX_PARSER(int, 1)[_val = _1 * 16 + _1];
		hex_byte = HEX_PARSER(int, 2);
		rgb_component = qi::uint_parser<unsigned char, 10, 1, 3>();

		comma = *qi::blank >> "," >> *qi::blank;
		blank = *qi::blank;
	}
};

}

// Register color_grammar with the auto parser generation stuff
namespace boost { namespace spirit { namespace traits {
	template<> struct create_parser<agi::Color> {
		typedef color_grammar<const char *>& type;
		static color_grammar<const char *> grammar;
		static type call() { return grammar; }
	};

	color_grammar<const char *> create_parser<agi::Color>::grammar;
}}}

namespace agi { namespace parser {
	void parse(Color &dst, const char *str, size_t len) {
		qi::phrase_parse(str, str + len, auto_, '!', dst);
	}

	void parse(ass::Style &dst, const char *str, size_t len) {
		boost::fusion::vector<int, double, int> foo;
		qi::phrase_parse(str, str + len, auto_, lit(','), foo);
		//qi::phrase_parse(str, str + len, auto_, lit(','), dst);
	}
}
}
