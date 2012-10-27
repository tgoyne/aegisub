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

#include <libaegisub/ass/dialogue_parser.h>

#include "main.h"
#include "util.h"

class lagi_dialogue_lexer : public libagi {
};

using namespace agi::ass;

TEST(lagi_dialogue_lexer, empty) {
	ASSERT_TRUE(TokenizeDialogueBody("").empty());
}

#define tok_str(arg1, ...) do { \
	std::string str = arg1; \
	std::vector<DialogueToken> tok = TokenizeDialogueBody(str); \
	size_t token_index = 0; \
	__VA_ARGS__ \
	EXPECT_EQ(token_index, tok.size()); \
} while(false)

#define expect_tok(expected_type, expected_start) do { \
	EXPECT_LT(token_index, tok.size()); \
	if (token_index < tok.size()) { \
		EXPECT_EQ(DialogueTokenType::expected_type, tok[token_index].type); \
		EXPECT_EQ(expected_start, tok[token_index].start - str.c_str()); \
		++token_index; \
	} \
} while(false)

TEST(lagi_dialogue_lexer, plain_text) {
	tok_str("hello there",
		expect_tok(TEXT, 0);
	);

	tok_str("hello\\Nthere",
		expect_tok(TEXT, 0);
		expect_tok(LINE_BREAK, 5);
		expect_tok(TEXT, 7);
	);

	tok_str("hello\\n\\h\\kthere",
		expect_tok(TEXT, 0);
		expect_tok(LINE_BREAK, 5);
		expect_tok(TEXT, 9);
	);
}

TEST(lagi_dialogue_lexer, basic_override_tags) {
	tok_str("{\\b1}bold text{\\b0}",
		expect_tok(OVR_BEGIN, 0);
		expect_tok(TAG_NAME, 1);
		expect_tok(ARG, 3);
		expect_tok(OVR_END, 4);
		expect_tok(TEXT, 5);
		expect_tok(OVR_BEGIN, 14);
		expect_tok(TAG_NAME, 15);
		expect_tok(ARG, 17);
		expect_tok(OVR_END, 18);
	);

	tok_str("{\\fnComic Sans MS}text",
		expect_tok(OVR_BEGIN, 0);
		expect_tok(TAG_NAME, 1);
		expect_tok(ARG, 4);
		expect_tok(OVR_END, 17);
		expect_tok(TEXT, 18);
	);

	tok_str("{\\pos(0,0)}a",
		expect_tok(OVR_BEGIN, 0);
		expect_tok(TAG_NAME, 1);
		expect_tok(OPEN_PAREN, 5);
		expect_tok(ARG, 6);
		expect_tok(ARG_SEP, 7);
		expect_tok(ARG, 8);
		expect_tok(CLOSE_PAREN, 9);
		expect_tok(OVR_END, 10);
		expect_tok(TEXT, 11);
	);

	tok_str("{\\pos( 0 , 0 )}a",
		expect_tok(OVR_BEGIN, 0);
		expect_tok(TAG_NAME, 1);
		expect_tok(OPEN_PAREN, 5);
		expect_tok(ARG, 7);
		expect_tok(ARG_SEP, 8);
		expect_tok(ARG, 11);
		expect_tok(CLOSE_PAREN, 12);
		expect_tok(OVR_END, 14);
		expect_tok(TEXT, 15);
	);

	tok_str("{\\c&HFFFFFF&\\2c&H0000FF&\\3c&H000000&}a",
		expect_tok(OVR_BEGIN, 0);
		expect_tok(TAG_NAME, 1);
		expect_tok(ARG, 3);
		expect_tok(TAG_NAME, 12);
		expect_tok(ARG, 15);
		expect_tok(TAG_NAME, 24);
		expect_tok(ARG, 27);
		expect_tok(OVR_END, 36);
		expect_tok(TEXT, 37);
	);

	tok_str("{\\t(0,100,\\clip(1, m 0 0 l 10 10 10 20))}a",
		expect_tok(OVR_BEGIN, 0);
		expect_tok(TAG_NAME, 1);
		expect_tok(OPEN_PAREN, 3);
		expect_tok(ARG, 4);
		expect_tok(ARG_SEP, 5);
		expect_tok(ARG, 6);
		expect_tok(ARG_SEP, 9);
		expect_tok(TAG_NAME, 10);
		expect_tok(OPEN_PAREN, 15);
		expect_tok(ARG, 16);
		expect_tok(ARG_SEP, 17);
		expect_tok(ARG, 19);
		expect_tok(CLOSE_PAREN, 38);
		expect_tok(OVR_END, 40);
		expect_tok(TEXT, 41);
	);
}

TEST(lagi_dialogue_lexer, malformed) {
	tok_str("}",
		expect_tok(TEXT, 0);
	);

	tok_str("{{",
		expect_tok(OVR_BEGIN, 0);
		expect_tok(ERROR, 1);
	);

	tok_str("{\\pos(0,0}a",
		expect_tok(OVR_BEGIN, 0);
		expect_tok(TAG_NAME, 1);
		expect_tok(OPEN_PAREN, 5);
		expect_tok(ARG, 6);
		expect_tok(ARG_SEP, 7);
		expect_tok(ARG, 8);
		expect_tok(OVR_END, 9);
		expect_tok(TEXT, 10);
	);
}
