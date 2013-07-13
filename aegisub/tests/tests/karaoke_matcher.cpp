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

#include <libaegisub/karaoke_matcher.h>

#include "main.h"
#include "util.h"

class lagi_karaoke_matcher : public libagi { };

namespace agi {
bool operator==(karaoke_match_result const& a, karaoke_match_result const& b) {
	return a.source_length == b.source_length && a.destination_length == b.destination_length;
}
::std::ostream& operator<<(::std::ostream& os, karaoke_match_result const& r) {
	return os << "karaoke_match_result{" << r.source_length << ", " << r.destination_length << "}";
}
}

using agi::auto_match_karaoke;
using agi::karaoke_match_result;

TEST(lagi_karaoke_matcher, empty_src_gives_zero_src_length) {
	EXPECT_EQ(0, auto_match_karaoke(std::vector<std::string>(), "").source_length);
	EXPECT_EQ(0, auto_match_karaoke(std::vector<std::string>(), "a").source_length);
}

TEST(lagi_karaoke_matcher, empty_dest_gives_zero_dest_length) {
	EXPECT_EQ(0, auto_match_karaoke(std::vector<std::string>(), "").destination_length);
}

TEST(lagi_karaoke_matcher, empty_dest_with_source_selects_all_source) {
	EXPECT_EQ(2, auto_match_karaoke(std::vector<std::string>{"a", "b"}, "").source_length);
}

TEST(lagi_karaoke_matcher, empty_but_present_src_syllable_matches_no_dest) {
	EXPECT_EQ((karaoke_match_result{1, 0}),
	          auto_match_karaoke(std::vector<std::string>{"", "b"}, "cc"));
}

TEST(lagi_karaoke_matcher, dest_with_non_match_selects_first_character) {
	EXPECT_EQ((karaoke_match_result{1, 1}),
	          auto_match_karaoke(std::vector<std::string>{"a", "b"}, "cc"));
}

TEST(lagi_karaoke_matcher, dest_with_identical_match_selects_match) {
	EXPECT_EQ((karaoke_match_result{1, 3}),
	          auto_match_karaoke(std::vector<std::string>{"abc", "de"}, "abcde"));
}

TEST(lagi_karaoke_matcher, match_is_case_insensitive) {
	EXPECT_EQ((karaoke_match_result{1, 3}),
	          auto_match_karaoke(std::vector<std::string>{"abc", "de"}, "ABCDE"));
	EXPECT_EQ((karaoke_match_result{1, 3}),
	          auto_match_karaoke(std::vector<std::string>{"ABC", "DE"}, "abcde"));
}

TEST(lagi_karaoke_matcher, leading_whitespace_in_source_is_ignored) {
	EXPECT_EQ((karaoke_match_result{1, 3}),
	          auto_match_karaoke(std::vector<std::string>{" abc", "de"}, "abcde"));
}

TEST(lagi_karaoke_matcher, trailing_whitespace_in_source_is_ignored) {
	EXPECT_EQ((karaoke_match_result{1, 3}),
	          auto_match_karaoke(std::vector<std::string>{"abc ", "de"}, "abcde"));
}

TEST(lagi_karaoke_matcher, whitespace_in_dest_is_consumed) {
	EXPECT_EQ((karaoke_match_result{1, 4}),
	          auto_match_karaoke(std::vector<std::string>{"abc ", "de"}, " abcde"));
	EXPECT_EQ((karaoke_match_result{1, 4}),
	          auto_match_karaoke(std::vector<std::string>{"abc ", "de"}, "abc de"));
	EXPECT_EQ((karaoke_match_result{1, 5}),
	          auto_match_karaoke(std::vector<std::string>{"abc ", "de"}, "ab c de"));
}

TEST(lagi_karaoke_matcher, dest_match_is_in_characters) {
	EXPECT_EQ((karaoke_match_result{1, 2}),
	          auto_match_karaoke(std::vector<std::string>{"∫∫", "de"}, "∫∫a"));
}

TEST(lagi_karaoke_matcher, decomposed_characters_are_handled_atomically) {
	// YODO
	EXPECT_EQ((karaoke_match_result{1, 2}),
	          auto_match_karaoke(std::vector<std::string>{"∫∫", "de"}, "∫∫a"));
}

TEST(lagi_karaoke_matcher, single_hiragana_is_matched) {
	EXPECT_EQ((karaoke_match_result{1, 1}),
	          auto_match_karaoke(std::vector<std::string>{"ro" "de"}, "ろ"));
}

TEST(lagi_karaoke_matcher, single_katakana_is_matched) {
	EXPECT_EQ((karaoke_match_result{1, 1}),
	          auto_match_karaoke(std::vector<std::string>{"ro" "de"}, "ロ"));
}

TEST(lagi_karaoke_matcher, multiple_characters_matched) {
	EXPECT_EQ((karaoke_match_result{1, 3}),
	          auto_match_karaoke(std::vector<std::string>{"romaji" "de"}, "ろまじ"));
}
TEST(lagi_karaoke_matcher, multiple_character_kana) {
	EXPECT_EQ((karaoke_match_result{1, 2}),
	          auto_match_karaoke(std::vector<std::string>{"kya", "e"}, "きゃe"));
	EXPECT_EQ((karaoke_match_result{1, 2}),
	          auto_match_karaoke(std::vector<std::string>{"kya"}, "きゃ"));
}

TEST(lagi_karaoke_matcher, whitespace_between_characters_in_source_ignored) {
	EXPECT_EQ((karaoke_match_result{1, 3}),
	          auto_match_karaoke(std::vector<std::string>{"ro ma ji" "de"}, "ろまじ"));
}

TEST(lagi_karaoke_matcher, whitespace_inside_characters_in_source_breaks_match) {
	EXPECT_EQ((karaoke_match_result{1, 1}),
	          auto_match_karaoke(std::vector<std::string>{"r om aj i" "de"}, "ろまじ"));
}

TEST(lagi_karaoke_matcher, single_dest_character_consumes_all_source) {
	EXPECT_EQ((karaoke_match_result{3, 1}),
	          auto_match_karaoke(std::vector<std::string>{"a", "b", "c"}, "ろ"));
}

TEST(lagi_karaoke_matcher, fullwidth_letters_are_matched_to_ascii) {
	EXPECT_EQ((karaoke_match_result{1, 2}),
	          auto_match_karaoke(std::vector<std::string>{"ab", "cd"}, "ａｂc"));
}

TEST(lagi_karaoke_matcher, simple_lookahead) {
	EXPECT_EQ((karaoke_match_result{1, 2}),
	          auto_match_karaoke(std::vector<std::string>{"ab", "ro"}, "eeろ"));
}

TEST(lagi_karaoke_matcher, lookahead_ignores_empty_syllables) {
	EXPECT_EQ((karaoke_match_result{1, 2}),
	          auto_match_karaoke(std::vector<std::string>{"ab", "", "ro"}, "eeろ"));
	EXPECT_EQ((karaoke_match_result{1, 2}),
	          auto_match_karaoke(std::vector<std::string>{"ab", "", "", "ro"}, "eeろ"));
}

TEST(lagi_karaoke_matcher, lookahead_only_looks_at_three_characters_of_dst) {
	EXPECT_EQ((karaoke_match_result{1, 3}),
	          auto_match_karaoke(std::vector<std::string>{"abc", "", "ro"}, "eeeろ"));
	EXPECT_EQ((karaoke_match_result{1, 1}),
	          auto_match_karaoke(std::vector<std::string>{"abcd", "", "ro"}, "eeeeろ"));
}

TEST(lagi_karaoke_matcher, lookahead_two_syllables) {
	EXPECT_EQ((karaoke_match_result{1, 1}),
	          auto_match_karaoke(std::vector<std::string>{"a", "b", "ro"}, "eeろ"));
	EXPECT_EQ((karaoke_match_result{2, 1}),
	          auto_match_karaoke(std::vector<std::string>{"a", "b", "c", "ro"}, "eeろ"));
	EXPECT_EQ((karaoke_match_result{2, 1}),
	          auto_match_karaoke(std::vector<std::string>{"a", "b", "c", "d", "ro"}, "eeろ"));
	EXPECT_EQ((karaoke_match_result{3, 1}),
	          auto_match_karaoke(std::vector<std::string>{"a", "b", "c", "d", "f", "ro"}, "eeろ"));
	EXPECT_EQ((karaoke_match_result{3, 2}),
	          auto_match_karaoke(std::vector<std::string>{"a", "b", "c", "d", "f", "ro"}, " eeろ"));
}

TEST(lagi_karaoke_matcher, lookahead_multicharacter_kana) {
	EXPECT_EQ((karaoke_match_result{1, 2}),
	          auto_match_karaoke(std::vector<std::string>{"aa", "kya"}, "eeきゃ"));
}
