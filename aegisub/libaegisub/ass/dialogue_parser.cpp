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

#include "../config.h"

#include "libaegisub/ass/dialogue_parser.h"

#include "libaegisub/spellchecker.h"

#include "iconv.h"

#include <boost/locale/boundary.hpp>

namespace {

typedef std::vector<agi::ass::DialogueToken> TokenVec;
using namespace agi::ass;
namespace dt = DialogueTokenType;
namespace ss = SyntaxStyle;

class SyntaxHighlighter {
	TokenVec ranges;
	std::string const& text;
	agi::SpellChecker *spellchecker;

	void SetStyling(int len, int type) {
		if (ranges.size() && ranges.back().type == type)
			ranges.back().length += len;
		else
			ranges.push_back(DialogueToken(type, len));
	}

public:
	SyntaxHighlighter(std::string const& text, agi::SpellChecker *spellchecker)
	: text(text)
	, spellchecker(spellchecker)
	{ }

	TokenVec Highlight(TokenVec const& tokens) {
		if (tokens.empty()) return ranges;

		size_t pos = 0;

		for (auto tok : tokens) {
			switch (tok.type) {
				case dt::KARAOKE_TEMPLATE: SetStyling(tok.length, ss::KARAOKE_TEMPLATE); break;
				case dt::KARAOKE_VARIABLE: SetStyling(tok.length, ss::KARAOKE_VARIABLE); break;
				case dt::LINE_BREAK: SetStyling(tok.length, ss::LINE_BREAK); break;
				case dt::ERROR:      SetStyling(tok.length, ss::ERROR);      break;
				case dt::ARG:        SetStyling(tok.length, ss::PARAMETER);  break;
				case dt::COMMENT:    SetStyling(tok.length, ss::COMMENT);    break;
				case dt::DRAWING:    SetStyling(tok.length, ss::DRAWING);    break;
				case dt::TEXT:       SetStyling(tok.length, ss::NORMAL);     break;
				case dt::TAG_NAME:   SetStyling(tok.length, ss::TAG);        break;
				case dt::OPEN_PAREN: case dt::CLOSE_PAREN: case dt::ARG_SEP: case dt::TAG_START:
					SetStyling(tok.length, ss::PUNCTUATION);
					break;
				case dt::OVR_BEGIN: case dt::OVR_END:
					SetStyling(tok.length, ss::OVERRIDE);
					break;
				case dt::WHITESPACE:
					if (ranges.size() && ranges.back().type == ss::PARAMETER)
						SetStyling(tok.length, ss::PARAMETER);
					else
						SetStyling(tok.length, ss::NORMAL);
					break;
				case dt::WORD:
					if (spellchecker && !spellchecker->CheckWord(text.substr(pos, tok.length)))
						SetStyling(tok.length, ss::SPELLING);
					else
						SetStyling(tok.length, ss::NORMAL);
					break;
			}

			pos += tok.length;
		}

		return ranges;
	}
};

class WordSplitter {
	std::string const& text;
	std::vector<DialogueToken> &tokens;
	size_t pos;

	void SwitchTo(size_t &i, int type, int len) {
		auto old = tokens[i];
		tokens[i].type = type;
		tokens[i].length = len;

		if (old.length != (size_t)len) {
			tokens.insert(tokens.begin() + i + 1, DialogueToken(old.type, old.length - len));
			++i;
		}
	}

	void SplitText(size_t &i) {
		using namespace boost::locale::boundary;
		ssegment_index map(word, text.begin() + pos, text.begin() + pos + tokens[i].length);
		for (auto const& segment : map) {
			int len = distance(begin(segment), end(segment));
			if (segment.rule() & word_letters)
				SwitchTo(i, dt::WORD, len);
			else
				SwitchTo(i, dt::TEXT, len);
		}
	}

public:
	WordSplitter(std::string const& text, std::vector<DialogueToken> &tokens)
	: text(text)
	, tokens(tokens)
	, pos(0)
	{ }

	void SplitWords() {
		if (tokens.empty()) return;

		for (size_t i = 0; i < tokens.size(); ++i) {
			size_t len = tokens[i].length;
			if (tokens[i].type == dt::TEXT)
				SplitText(i);
			pos += len;
		}
	}
};

static std::multimap<std::string, std::vector<ParameterClass>> tag_protos;

struct param_adder {
	std::vector<ParameterClass> *p;
	param_adder(std::vector<ParameterClass> *p) : p(p) { }
	param_adder Param(ParameterClass classi) {
		p->emplace_back(classi);
		return *this;
	}
};

param_adder Tag(const char *name) {
	return param_adder(&tag_protos.insert(std::pair<std::string, std::vector<ParameterClass>>(name, std::vector<ParameterClass>()))->second);
}

void load_protos() {
	if (!tag_protos.empty()) return;

	typedef ParameterClass pc;

	Tag("fade") // \fade(<a1>,<a2>,<a3>,<t1>,<t2>,<t3>,<t4>)
		.Param(pc::NORMAL)
		.Param(pc::NORMAL)
		.Param(pc::NORMAL)
		.Param(pc::TIME_START)
		.Param(pc::TIME_START)
		.Param(pc::TIME_START)
		.Param(pc::TIME_START);

	Tag("move") // \move(<x1>,<y1>,<x2>,<y2>[,<t1>,<t2>])
		.Param(pc::POS_X)
		.Param(pc::POS_Y)
		.Param(pc::POS_X)
		.Param(pc::POS_Y)
		.Param(pc::TIME_START)
		.Param(pc::TIME_START);

	Tag("clip") // \clip(<x1>,<y1>,<x2>,<y2>)
		.Param(pc::POS_X)
		.Param(pc::POS_Y)
		.Param(pc::POS_X)
		.Param(pc::POS_Y);

	Tag("iclip") // \iclip(<x1>,<y1>,<x2>,<y2>)
		.Param(pc::POS_X)
		.Param(pc::POS_Y)
		.Param(pc::POS_X)
		.Param(pc::POS_Y);

	Tag("t").Param(pc::NORMAL); // \t(<style modifiers>)
	Tag("t").Param(pc::NORMAL).Param(pc::NORMAL); // \t(<accel>,<style modifiers>)
	Tag("t") // \t(<t1>,<t2>,<style modifiers>)
		.Param(pc::TIME_START)
		.Param(pc::TIME_START)
		.Param(pc::NORMAL);
	Tag("t") // \t(<t1>,<t2>,<accel>,<style modifiers>)
		.Param(pc::TIME_START)
		.Param(pc::TIME_START)
		.Param(pc::NORMAL)
		.Param(pc::NORMAL);

	Tag("1a").Param(pc::ALPHA); // \1a&H<aa>&
	Tag("1c").Param(pc::NORMAL); // \1c&H<bbggrr>&
	Tag("2a").Param(pc::ALPHA); // \2a&H<aa>&
	Tag("2c").Param(pc::NORMAL); // \2c&H<bbggrr>&
	Tag("3a").Param(pc::ALPHA); // \3a&H<aa>&
	Tag("3c").Param(pc::NORMAL); // \3c&H<bbggrr>&
	Tag("4a").Param(pc::ALPHA); // \4a&H<aa>&
	Tag("4c").Param(pc::NORMAL); // \4c&H<bbggrr>&
	Tag("K").Param(pc::KARAOKE); // \K<duration>
	Tag("a").Param(pc::NORMAL); // \a<alignment>
	Tag("alpha").Param(pc::ALPHA); // \alpha&H<aa>&
	Tag("an").Param(pc::NORMAL); // \an<alignment>
	Tag("b").Param(pc::NORMAL); // \b<0/1/weight>
	Tag("be").Param(pc::NORMAL); // \be<strength>
	Tag("blur").Param(pc::NORMAL); // \blur<strength>
	Tag("bord").Param(pc::ABSOLUTE_SIZE); // \bord<depth>
	Tag("c").Param(pc::NORMAL); // \c&H<bbggrr>&
	Tag("clip").Param(pc::DRAWING); // \clip(<some drawings>)
	Tag("clip").Param(pc::NORMAL).Param(pc::DRAWING); // \clip(<scale>,<some drawings>)
	Tag("fad").Param(pc::TIME_START).Param(pc::TIME_END); // \fad(<t1>,<t2>)
	Tag("fax").Param(pc::NORMAL); // \fax<factor>
	Tag("fay").Param(pc::NORMAL); // \fay<factor>
	Tag("fe").Param(pc::NORMAL); // \fe<charset>
	Tag("fn").Param(pc::NORMAL); // \fn<name>
	Tag("fr").Param(pc::NORMAL); // \fr<degrees>
	Tag("frx").Param(pc::NORMAL); // \frx<degrees>
	Tag("fry").Param(pc::NORMAL); // \fry<degrees>
	Tag("frz").Param(pc::NORMAL); // \frz<degrees>
	Tag("fs").Param(pc::ABSOLUTE_SIZE); // \fs<size>
	Tag("fs+").Param(pc::NORMAL); // \fs+<size>
	Tag("fs-").Param(pc::NORMAL); // \fs-<size>
	Tag("fscx").Param(pc::SIZE_X); // \fscx<percent>
	Tag("fscy").Param(pc::SIZE_Y); // \fscy<percent>
	Tag("fsp").Param(pc::ABSOLUTE_SIZE); // \fsp<pixels>
	Tag("i").Param(pc::NORMAL); // \i<0/1>
	Tag("iclip").Param(pc::DRAWING); // \iclip(<some drawings>)
	Tag("iclip").Param(pc::NORMAL).Param(pc::DRAWING); // \iclip(<scale>,<some drawings>)
	Tag("k").Param(pc::KARAOKE); // \k<duration>
	Tag("kf").Param(pc::KARAOKE); // \kf<duration>
	Tag("ko").Param(pc::KARAOKE); // \ko<duration>
	Tag("org").Param(pc::POS_X).Param(pc::POS_Y); // \org(<x>,<y>)
	Tag("p").Param(pc::NORMAL); // \p<n>
	Tag("pbo").Param(pc::POS_Y); // \pbo<y>
	Tag("pos").Param(pc::POS_X).Param(pc::POS_Y); // \pos(<x>,<y>)
	Tag("q").Param(pc::NORMAL); // \q<0-3>
	Tag("r").Param(pc::NORMAL); // \r[<name>]
	Tag("s").Param(pc::NORMAL); // \s<0/1>
	Tag("shad").Param(pc::ABSOLUTE_SIZE); // \shad<depth>
	Tag("u").Param(pc::NORMAL); // \u<0/1>
	Tag("xbord").Param(pc::ABSOLUTE_SIZE); // \xbord<depth>
	Tag("xshad").Param(pc::ABSOLUTE_SIZE); // \xshad<depth>
	Tag("ybord").Param(pc::ABSOLUTE_SIZE); // \ybord<depth>
	Tag("yshad").Param(pc::ABSOLUTE_SIZE); // \yshad<depth>
}

bool IsTagEnd(int type, int depth) {
	return
		type == DialogueTokenType::OVR_END ||
		(depth == 0 && type == DialogueTokenType::TAG_START) ||
		(depth == 1 && type == DialogueTokenType::CLOSE_PAREN);
}

template<typename StringIterator, typename TokenIterator>
void ParseParameters(StringIterator& str, TokenIterator& token, OverrideTag& tag) {
	int depth = 0;
	for (; !IsTagEnd(token->type, depth); ++token) {
		switch (token->type) {
			case DialogueTokenType::OPEN_PAREN:
				++depth;
				break;
			case DialogueTokenType::CLOSE_PAREN:
				--depth;
				break;
			case DialogueTokenType::ARG:
				tag.params.emplace_back();
				tag.params.back().classification = ParameterClass::NORMAL;
				tag.params.back().value.assign(str, str + token->length);
				break;
			case DialogueTokenType::TAG_START:
				auto start = str;
				for (; !IsTagEnd(token->type, 1); ++token)
					str += token->length;
				tag.params.emplace_back();
				tag.params.back().classification = ParameterClass::NORMAL;
				tag.params.back().value.assign(start, str);
				--depth;
				break;
		}

		str += token->length;
	}
}

template<typename StringIterator, typename TokenIterator>
void ParseTag(StringIterator str_begin, StringIterator& str, TokenIterator& token, OverrideBlock *out) {
	assert(token->type == DialogueTokenType::TAG_NAME);

	out->Tags.emplace_back();
	auto& tag = out->Tags.back();
	tag.name.assign(str, str + token->length);
	tag.begin = distance(str_begin, str);

	++token;
	ParseParameters(str, token, tag);
	tag.end = distance(str_begin, str);

	const auto protos = tag_protos.equal_range(tag.name);
	if (protos.first == protos.second) return;

	const auto arg_count = tag.params.size();
	auto best = protos.first;
	auto best_count = best->second.size();
	for (auto it = next(protos.first); it != protos.second; ++it) {
		const auto it_count = it->second.size();
		if ((best_count > arg_count && it_count > arg_count && it_count < best_count) || (best_count < arg_count && it_count > best_count)) {
			best = it;
			best_count = it_count;
		}
	}

	for (size_t i = 0; i < best->second.size() && i < tag.params.size(); ++i)
		tag.params[i].classification = best->second[i];
}
}

namespace agi {
namespace ass {

std::vector<DialogueToken> SyntaxHighlight(std::string const& text, std::vector<DialogueToken> const& tokens, SpellChecker *spellchecker) {
	return SyntaxHighlighter(text, spellchecker).Highlight(tokens);
}

void MarkDrawings(std::string const& str, std::vector<DialogueToken> &tokens) {
	if (tokens.empty()) return;

	size_t last_ovr_end = 0;
	for (size_t i = tokens.size(); i > 0; --i) {
		if (tokens[i - 1].type == dt::OVR_END) {
			last_ovr_end = i;
			break;
		}
	}

	size_t pos = 0;
	bool in_drawing = false;

	for (size_t i = 0; i < last_ovr_end; ++i) {
		size_t len = tokens[i].length;
		switch (tokens[i].type) {
			case dt::TEXT:
				if (in_drawing)
					tokens[i].type = dt::DRAWING;
				break;
			case dt::TAG_NAME:
				if (len != 1 || i + 1 >= tokens.size() || str[pos] != 'p')
					break;

				in_drawing = false;

				if (i + 1 == last_ovr_end || tokens[i + 1].type != dt::ARG)
					break;

				for (size_t j = pos + len; j < pos + len + tokens[i + 1].length; ++j) {
					char c = str[j];
					// I have no idea why one would use leading zeros for
					// the scale, but vsfilter allows it
					if (c >= '1' && c <= '9')
						in_drawing = true;
					else if (c != '0')
						break;
				}
				break;
			default: break;
		}

		pos += len;
	}

	// VSFilter treats unclosed override blocks as plain text, so merge all
	// the tokens after the last override block into a single TEXT (or DRAWING)
	// token
	for (size_t i = last_ovr_end; i < tokens.size(); ++i) {
		switch (tokens[i].type) {
			case dt::KARAOKE_TEMPLATE: break;
			case dt::KARAOKE_VARIABLE: break;
			case dt::LINE_BREAK: break;
			default:
				tokens[i].type = in_drawing ? dt::DRAWING : dt::TEXT;
				if (i > 0 && tokens[i - 1].type == tokens[i].type) {
					tokens[i - 1].length += tokens[i].length;
					tokens.erase(tokens.begin() + i);
					--i;
				}
		}
	}
}

void SplitWords(std::string const& str, std::vector<DialogueToken> &tokens) {
	MarkDrawings(str, tokens);
	WordSplitter(str, tokens).SplitWords();
}

template<typename T>
T make(std::string::const_iterator str_start, std::string::const_iterator begin, std::string::const_iterator end) {
	T obj;
	obj.begin = distance(str_start, begin);
	obj.end = distance(str_start, end);
	obj.text.assign(begin, end);
	return obj;
}

std::vector<DialogueBlock> Parse(std::string const& str) {
	std::vector<DialogueBlock> ret;

	if (str.empty()) {
		ret.emplace_back(PlainBlock());
		return ret;
	}

	auto tokens = TokenizeDialogueBody(str);
	MarkDrawings(str, tokens);

	auto pos = begin(str);
	auto start = begin(str);
	for (auto it = begin(tokens); it != end(tokens); ++it) {
		const auto tok = *it;
		switch (tok.type) {
			case DialogueTokenType::OVR_BEGIN:
				if (start < pos)
					ret.emplace_back(make<PlainBlock>(begin(str), start, pos - 1));
				break;

			case DialogueTokenType::OVR_END:
				start = pos + tok.length;
				break;

			case DialogueTokenType::COMMENT:
				ret.emplace_back(make<CommentBlock>(begin(str), pos, pos + tok.length));
				break;

			case DialogueTokenType::TAG_NAME: {
				OverrideBlock *ovr = nullptr;
				if (!ret.empty())
					ovr = boost::get<OverrideBlock>(&ret.back());
				if (!ovr) {
					ret.emplace_back(OverrideBlock());
					ovr = boost::get<OverrideBlock>(&ret.back());
					ovr->begin = distance(begin(str), pos);
				}
				ParseTag(begin(str), pos, it, ovr);
				ovr->end = distance(begin(str), pos);
				break;
			}

			case DialogueTokenType::DRAWING:
				ret.emplace_back(make<DrawingBlock>(begin(str), pos, pos + tok.length));
				break;

			default: break;
		}

		pos += tok.length;
	}

	if (start < pos)
		ret.emplace_back(make<PlainBlock>(begin(str), start, end(str)));
	return ret;
}

std::string StripTags(std::string const& str) {
	auto tokens = TokenizeDialogueBody(str);
	MarkDrawings(str, tokens);

	std::string ret;
	auto pos = begin(str);
	for (const auto tok : tokens) {
		switch (tok.type) {
			case DialogueTokenType::TEXT:
			case DialogueTokenType::LINE_BREAK:
				ret.append(pos, pos + tok.length);
			default:;
		}
		pos += tok.length;
	}

	return ret;
}

}
}
