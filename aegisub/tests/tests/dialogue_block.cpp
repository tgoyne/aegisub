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

#include "../config.h"

#include <libaegisub/ass/dialogue_block.h>

#include "main.h"
#include "util.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace agi::ass;

#define TEST_ROUNDTRIP_TN(tag_name, args, tag) \
	TEST(lagi_dialogue_block, tag##_should_roundtrip_through_parse_unparse) { \
		std::vector<std::string> toks; \
		if (args[0] == '(') { \
			std::string argstr(args + 1, args + sizeof(args) - 1); \
			boost::split(toks, argstr, boost::is_any_of(",")); \
		} \
		else { \
			toks.emplace_back(args); \
		} \
		auto ret = parse<tags::tag>(toks); \
		EXPECT_STREQ("\\" #tag_name args, unparse<tags::tag>(ret).c_str()); \
	}

#define TEST_ROUNDTRIP(tag_name, args) TEST_ROUNDTRIP_TN(tag_name, args, tag_name)

TEST_ROUNDTRIP(alpha, "&HFF&")
TEST_ROUNDTRIP(an, "7")
TEST_ROUNDTRIP(b, "1")
TEST_ROUNDTRIP(be, "5")
TEST_ROUNDTRIP(blur, "7.3")
TEST_ROUNDTRIP(bord, "1.3")
TEST_ROUNDTRIP(clip, "(0,0,1280,720)")
TEST_ROUNDTRIP(fax, "0.5")
TEST_ROUNDTRIP(fay, "0.5")
TEST_ROUNDTRIP(fe, "0")
TEST_ROUNDTRIP(fn, "Comic Sans")
TEST_ROUNDTRIP(frx, "90")
TEST_ROUNDTRIP(fry, "85.1")
TEST_ROUNDTRIP(frz, "0")
TEST_ROUNDTRIP(fs, "20")
TEST_ROUNDTRIP_TN(fs-, "2", fs_minus)
TEST_ROUNDTRIP_TN(fs+, "2", fs_plus)
TEST_ROUNDTRIP(fscx, "100")
TEST_ROUNDTRIP(fscy, "90.5")
TEST_ROUNDTRIP(fsp, "2.1")
TEST_ROUNDTRIP(i, "0")
TEST_ROUNDTRIP(iclip, "(m 0 0 l 0 10 10 10 10 0)")
TEST_ROUNDTRIP(k, "5")
TEST_ROUNDTRIP(kf, "0")
TEST_ROUNDTRIP(ko, "3")
// TEST_ROUNDTRIP(move, "(0,0,20,20)")
TEST_ROUNDTRIP(org, "(5,5)")
TEST_ROUNDTRIP_TN(3a, "&H00&", outline_alpha)
TEST_ROUNDTRIP_TN(3c, "&HFF00FF&", outline_color)
TEST_ROUNDTRIP(p, "1")
TEST_ROUNDTRIP(pbo, "2.2")
TEST_ROUNDTRIP(pos, "(20,10.5)")
TEST_ROUNDTRIP_TN(1a, "&HA3&", primary_alpha)
TEST_ROUNDTRIP_TN(c, "&H00FF00&", primary_color)
TEST_ROUNDTRIP(q, "2")
TEST_ROUNDTRIP(r, "Foo")
TEST_ROUNDTRIP(s, "1")
TEST_ROUNDTRIP_TN(2a, "&H12&", secondary_alpha)
TEST_ROUNDTRIP_TN(2c, "&H123456&", secondary_color)
TEST_ROUNDTRIP(shad, "2.2")
TEST_ROUNDTRIP_TN(4a, "&H00&", shadow_alpha)
TEST_ROUNDTRIP_TN(4c, "&H000000&", shadow_color)
// TEST_ROUNDTRIP(t, "")
TEST_ROUNDTRIP(u, "0")
TEST_ROUNDTRIP(xbord, "1.1")
TEST_ROUNDTRIP(xshad, "1.1")
TEST_ROUNDTRIP(ybord, "1.1")
TEST_ROUNDTRIP(yshad, "1.1")

TEST(lagi_dialogue_block, t_optional_params) {
	using detail::tag_type;
	std::vector<std::string> toks;

	toks = { "a" };
	EXPECT_EQ(tag_type<tags::t>::type(0, 0, 1.0, "a"), parse<tags::t>(toks));

	toks = { "1.5", "a" };
	EXPECT_EQ(tag_type<tags::t>::type(0, 0, 1.5, "a"), parse<tags::t>(toks));

	toks = { "3", "4", "a" };
	EXPECT_EQ(tag_type<tags::t>::type(3, 4, 1.0, "a"), parse<tags::t>(toks));

	toks = { "2", "3", "4", "a" };
	EXPECT_EQ(tag_type<tags::t>::type(2, 3, 4.0, "a"), parse<tags::t>(toks));
}

TEST(lagi_dialogue_block, move_optional_params) {
	using detail::tag_type;
	std::vector<std::string> toks;

	toks = { "0", "0", "10", "10" };
	EXPECT_EQ(tag_type<tags::move>::type(0, 0, 10, 10, 0, 0), parse<tags::move>(toks));

	toks = { "0", "0", "10", "10", "1", "2" };
	EXPECT_EQ(tag_type<tags::move>::type(0, 0, 10, 10, 1, 2), parse<tags::move>(toks));
}

TEST(lagi_dialogue_block, move_should_swap_unordered_times) {
	std::vector<std::string> toks = { "0", "0", "0", "0", "2000", "1000" };
	auto ret = parse<tags::move>(toks);
	EXPECT_EQ(1000, std::get<4>(ret));
	EXPECT_EQ(2000, std::get<5>(ret));
}

#define DEFAULT_PARAM_VALUES(tag, value) \
	TEST(lagi_dialogue_block, tag ## _should_set_to_default_if_param_is_missing) { \
		EXPECT_EQ(value, parse<tags::tag>(std::vector<std::string>())); \
	}

DEFAULT_PARAM_VALUES(be, 0)
DEFAULT_PARAM_VALUES(blur, 0)
DEFAULT_PARAM_VALUES(fax, 0)
DEFAULT_PARAM_VALUES(fay, 0)
DEFAULT_PARAM_VALUES(frx, 0)
DEFAULT_PARAM_VALUES(fry, 0)
DEFAULT_PARAM_VALUES(fn, std::string(""))
DEFAULT_PARAM_VALUES(fs_minus, 0)
DEFAULT_PARAM_VALUES(fs_plus, 0)
DEFAULT_PARAM_VALUES(k, 0)
DEFAULT_PARAM_VALUES(kf, 0)
DEFAULT_PARAM_VALUES(ko, 0)
DEFAULT_PARAM_VALUES(p, 0)
DEFAULT_PARAM_VALUES(pbo, 0)
DEFAULT_PARAM_VALUES(r, std::string(""))

#define MISSING_PARAM_VALUES(tag) \
	TEST(lagi_dialogue_block, tag ## _should_return_none_if_param_is_missing) { \
		EXPECT_FALSE(parse<tags::tag>(std::vector<std::string>())); \
	}

MISSING_PARAM_VALUES(alpha)
MISSING_PARAM_VALUES(an)
MISSING_PARAM_VALUES(bord)
MISSING_PARAM_VALUES(fe)
MISSING_PARAM_VALUES(frz)
MISSING_PARAM_VALUES(fs)
MISSING_PARAM_VALUES(fscx)
MISSING_PARAM_VALUES(fscy)
MISSING_PARAM_VALUES(fsp)
MISSING_PARAM_VALUES(i)
MISSING_PARAM_VALUES(outline_alpha)
MISSING_PARAM_VALUES(outline_color)
MISSING_PARAM_VALUES(primary_alpha)
MISSING_PARAM_VALUES(primary_color)
MISSING_PARAM_VALUES(q)
MISSING_PARAM_VALUES(s)
MISSING_PARAM_VALUES(secondary_alpha)
MISSING_PARAM_VALUES(secondary_color)
MISSING_PARAM_VALUES(u)
MISSING_PARAM_VALUES(xbord)
MISSING_PARAM_VALUES(xshad)
MISSING_PARAM_VALUES(ybord)
MISSING_PARAM_VALUES(yshad)

