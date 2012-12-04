// Copyright (c) 2005, Rodrigo Braz Monteiro
// Copyright (c) 2010, Thomas Goyne <plorkyeran@aegisub.org>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of the Aegisub Group nor the names of its contributors
//     may be used to endorse or promote products derived from this software
//     without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Aegisub Project http://www.aegisub.org/

/// @file ass_override.cpp
/// @brief Parse and modify ASSA style overrides
/// @ingroup subs_storage
///

#include "config.h"

#include <wx/log.h>
#include <wx/tokenzr.h>

#include <libaegisub/log.h>

#include "ass_dialogue.h"
#include "ass_override.h"
#include "utils.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>

AssOverrideParameter::AssOverrideParameter()
: classification(PARCLASS_NORMAL)
, omitted(false)
{
}

AssOverrideParameter::AssOverrideParameter(const AssOverrideParameter &param)
: VariableData(param)
, classification(param.classification)
, omitted(param.omitted)
{
}

void AssOverrideParameter::operator=(const AssOverrideParameter &param) {
	DeleteValue();
	new(this) AssOverrideParameter(param);
}

// From ass_dialogue.h
AssDialogueBlockOverride::~AssDialogueBlockOverride() {
	delete_clear(Tags);
}

void AssDialogueBlockOverride::ParseTags() {
	delete_clear(Tags);

	int depth = 0;
	size_t start = 0;
	for (size_t i = 1; i < text.size(); ++i) {
		if (depth > 0) {
			if (text[i] == ')')
				--depth;
		}
		else if (text[i] == '\\') {
			AddTag(text.substr(start, i - start));
			start  = 1;
		}
		else if (text[i] == '(')
			++depth;
	}

	if (!text.empty())
		AddTag(text.substr(start));
}

void AssDialogueBlockOverride::AddTag(std::string const& tag) {
	Tags.push_back(new AssOverrideTag(tag));
}

using namespace boost::adaptors;

void AssDialogueBlockOverride::ProcessParameters(ProcessParametersCallback callback, void *userData) {
	for (auto tag : Tags) {
		for (auto par : tag->Params) {
			if (par->GetType() == VARDATA_NONE || par->omitted) continue;

			callback(tag->Name, par, userData);

			// Go recursive if it's a block parameter
			if (par->GetType() == VARDATA_BLOCK)
				par->Get<AssDialogueBlockOverride*>()->ProcessParameters(callback, userData);
		}
	}
}

std::string const& AssDialogueBlockOverride::GetText() {
	return text = boost::accumulate(Tags | indirected);
}

AssOverrideParamProto::AssOverrideParamProto(VariableDataType type,int opt,AssParameterClass classi)
: optional(opt)
, type(type)
, classification(classi)
{
}

void AssOverrideTagProto::AddParam(VariableDataType type, AssParameterClass classi, int opt) {
	params.emplace_back(type, opt, classi);
}

void AssOverrideTagProto::Set(std::string const& newName, VariableDataType type, AssParameterClass classi, int opt) {
	name = newName;
	params.emplace_back(type, opt, classi);
}

static std::vector<AssOverrideTagProto> proto;
static void load_protos() {
	static bool loaded = false;
	if (loaded) return;
	loaded = true;

	proto.resize(56);
	int i = 0;

	// Longer tag names must appear before shorter tag names

	proto[0].Set("\\alpha", VARDATA_TEXT); // \alpha
	proto[++i].Set("\\bord", VARDATA_FLOAT,PARCLASS_ABSOLUTE_SIZE); // \bord<depth>
	proto[++i].Set("\\xbord", VARDATA_FLOAT,PARCLASS_ABSOLUTE_SIZE); // \xbord<depth>
	proto[++i].Set("\\ybord", VARDATA_FLOAT,PARCLASS_ABSOLUTE_SIZE); // \ybord<depth>
	proto[++i].Set("\\shad", VARDATA_FLOAT,PARCLASS_ABSOLUTE_SIZE); // \shad<depth>
	proto[++i].Set("\\xshad", VARDATA_FLOAT,PARCLASS_ABSOLUTE_SIZE); // \xshad<depth>
	proto[++i].Set("\\yshad", VARDATA_FLOAT,PARCLASS_ABSOLUTE_SIZE); // \yshad<depth>

	// \fade(<a1>,<a2>,<a3>,<t1>,<t2>,<t3>,<t4>)
	i++;
	proto[i].name = "\\fade";
	proto[i].AddParam(VARDATA_INT);
	proto[i].AddParam(VARDATA_INT);
	proto[i].AddParam(VARDATA_INT);
	proto[i].AddParam(VARDATA_INT,PARCLASS_RELATIVE_TIME_START);
	proto[i].AddParam(VARDATA_INT,PARCLASS_RELATIVE_TIME_START);
	proto[i].AddParam(VARDATA_INT,PARCLASS_RELATIVE_TIME_START);
	proto[i].AddParam(VARDATA_INT,PARCLASS_RELATIVE_TIME_START);

	// \move(<x1>,<y1>,<x2>,<y2>[,<t1>,<t2>])
	i++;
	proto[i].name = "\\move";
	proto[i].AddParam(VARDATA_FLOAT,PARCLASS_ABSOLUTE_POS_X);
	proto[i].AddParam(VARDATA_FLOAT,PARCLASS_ABSOLUTE_POS_Y);
	proto[i].AddParam(VARDATA_FLOAT,PARCLASS_ABSOLUTE_POS_X);
	proto[i].AddParam(VARDATA_FLOAT,PARCLASS_ABSOLUTE_POS_Y);
	proto[i].AddParam(VARDATA_INT,PARCLASS_RELATIVE_TIME_START);
	proto[i].AddParam(VARDATA_INT,PARCLASS_RELATIVE_TIME_START);

	// If these are rearranged, keep rect clip and vector clip adjacent in this order
	// \clip(<x1>,<y1>,<x2>,<y2>)
	i++;
	proto[i].name = "\\clip";
	proto[i].AddParam(VARDATA_INT,PARCLASS_ABSOLUTE_POS_X);
	proto[i].AddParam(VARDATA_INT,PARCLASS_ABSOLUTE_POS_Y);
	proto[i].AddParam(VARDATA_INT,PARCLASS_ABSOLUTE_POS_X);
	proto[i].AddParam(VARDATA_INT,PARCLASS_ABSOLUTE_POS_Y);

	// \clip([<scale>,]<some drawings>)
	i++;
	proto[i].name = "\\clip";
	proto[i].AddParam(VARDATA_INT,PARCLASS_NORMAL,OPTIONAL_2);
	proto[i].AddParam(VARDATA_TEXT,PARCLASS_DRAWING);

	// \iclip(<x1>,<y1>,<x2>,<y2>)
	i++;
	proto[i].name = "\\iclip";
	proto[i].AddParam(VARDATA_INT,PARCLASS_ABSOLUTE_POS_X);
	proto[i].AddParam(VARDATA_INT,PARCLASS_ABSOLUTE_POS_Y);
	proto[i].AddParam(VARDATA_INT,PARCLASS_ABSOLUTE_POS_X);
	proto[i].AddParam(VARDATA_INT,PARCLASS_ABSOLUTE_POS_Y);

	// \iclip([<scale>,]<some drawings>)
	i++;
	proto[i].name = "\\iclip";
	proto[i].AddParam(VARDATA_INT,PARCLASS_NORMAL,OPTIONAL_2);
	proto[i].AddParam(VARDATA_TEXT,PARCLASS_DRAWING);

	proto[++i].Set("\\fscx", VARDATA_FLOAT,PARCLASS_RELATIVE_SIZE_X); // \fscx<percent>
	proto[++i].Set("\\fscy", VARDATA_FLOAT,PARCLASS_RELATIVE_SIZE_Y); // \fscy<percent>
	// \pos(<x>,<y>)
	i++;
	proto[i].name = "\\pos";
	proto[i].AddParam(VARDATA_FLOAT,PARCLASS_ABSOLUTE_POS_X);
	proto[i].AddParam(VARDATA_FLOAT,PARCLASS_ABSOLUTE_POS_Y);

	// \org(<x>,<y>)
	i++;
	proto[i].name = "\\org";
	proto[i].AddParam(VARDATA_INT,PARCLASS_ABSOLUTE_POS_X);
	proto[i].AddParam(VARDATA_INT,PARCLASS_ABSOLUTE_POS_Y);

	proto[++i].Set("\\pbo", VARDATA_INT,PARCLASS_ABSOLUTE_POS_Y); // \pbo<y>
	// \fad(<t1>,<t2>)
	i++;
	proto[i].name = "\\fad";
	proto[i].AddParam(VARDATA_INT,PARCLASS_RELATIVE_TIME_START);
	proto[i].AddParam(VARDATA_INT,PARCLASS_RELATIVE_TIME_END);

	proto[++i].Set("\\fsp", VARDATA_FLOAT,PARCLASS_ABSOLUTE_SIZE); // \fsp<pixels>
	proto[++i].Set("\\frx", VARDATA_FLOAT); // \frx<degrees>
	proto[++i].Set("\\fry", VARDATA_FLOAT); // \fry<degrees>
	proto[++i].Set("\\frz", VARDATA_FLOAT); // \frz<degrees>
	proto[++i].Set("\\fr", VARDATA_FLOAT); // \fr<degrees>
	proto[++i].Set("\\fax", VARDATA_FLOAT); // \fax<factor>
	proto[++i].Set("\\fay", VARDATA_FLOAT); // \fay<factor>
	proto[++i].Set("\\1c", VARDATA_TEXT); // \1c&H<bbggrr>&
	proto[++i].Set("\\2c", VARDATA_TEXT); // \2c&H<bbggrr>&
	proto[++i].Set("\\3c", VARDATA_TEXT); // \3c&H<bbggrr>&
	proto[++i].Set("\\4c", VARDATA_TEXT); // \4c&H<bbggrr>&
	proto[++i].Set("\\1a", VARDATA_TEXT); // \1a&H<aa>&
	proto[++i].Set("\\2a", VARDATA_TEXT); // \2a&H<aa>&
	proto[++i].Set("\\3a", VARDATA_TEXT); // \3a&H<aa>&
	proto[++i].Set("\\4a", VARDATA_TEXT); // \4a&H<aa>&
	proto[++i].Set("\\fe", VARDATA_TEXT); // \fe<charset>
	proto[++i].Set("\\ko", VARDATA_INT,PARCLASS_KARAOKE); // \ko<duration>
	proto[++i].Set("\\kf", VARDATA_INT,PARCLASS_KARAOKE); // \kf<duration>
	proto[++i].Set("\\be", VARDATA_INT); // \be<strength>
	proto[++i].Set("\\blur", VARDATA_FLOAT); // \blur<strength>
	proto[++i].Set("\\fn", VARDATA_TEXT); // \fn<name>
	proto[++i].Set("\\fs+", VARDATA_FLOAT); // \fs+<size>
	proto[++i].Set("\\fs-", VARDATA_FLOAT); // \fs-<size>
	proto[++i].Set("\\fs", VARDATA_FLOAT,PARCLASS_ABSOLUTE_SIZE); // \fs<size>
	proto[++i].Set("\\an", VARDATA_INT); // \an<alignment>
	proto[++i].Set("\\c", VARDATA_TEXT); // \c&H<bbggrr>&
	proto[++i].Set("\\b", VARDATA_INT); // \b<0/1/weight>
	proto[++i].Set("\\i", VARDATA_BOOL); // \i<0/1>
	proto[++i].Set("\\u", VARDATA_BOOL); // \u<0/1>
	proto[++i].Set("\\s", VARDATA_BOOL); // \s<0/1>
	proto[++i].Set("\\a", VARDATA_INT); // \a<alignment>
	proto[++i].Set("\\k", VARDATA_INT,PARCLASS_KARAOKE); // \k<duration>
	proto[++i].Set("\\K", VARDATA_INT,PARCLASS_KARAOKE); // \K<duration>
	proto[++i].Set("\\q", VARDATA_INT); // \q<0-3>
	proto[++i].Set("\\p", VARDATA_INT); // \p<n>
	proto[++i].Set("\\r", VARDATA_TEXT); // \r[<name>]

	// \t([<t1>,<t2>,][<accel>,]<style modifiers>)
	i++;
	proto[i].name = "\\t";
	proto[i].AddParam(VARDATA_INT,PARCLASS_RELATIVE_TIME_START,OPTIONAL_3 | OPTIONAL_4);
	proto[i].AddParam(VARDATA_INT,PARCLASS_RELATIVE_TIME_START,OPTIONAL_3 | OPTIONAL_4);
	proto[i].AddParam(VARDATA_FLOAT,PARCLASS_NORMAL,OPTIONAL_2 | OPTIONAL_4);
	proto[i].AddParam(VARDATA_BLOCK);
}

AssOverrideTag::AssOverrideTag() : valid(false) { }
AssOverrideTag::AssOverrideTag(std::string const& text) {
	SetText(text);
}

AssOverrideTag::~AssOverrideTag () {
	delete_clear(Params);
}

void AssOverrideTag::Clear() {
	delete_clear(Params);
	Params.reserve(6);
	valid = false;
}

void AssOverrideTag::SetText(std::string const& text) {
	load_protos();
	auto p = find_if(proto.begin(), proto.end(), [&](AssOverrideTagProto const& p) {
		return boost::starts_with(text, p.name);
	});

	if (p != proto.end()) {
		Name = p->name;
		ParseParameters(text.Mid(Name.size()), p);
		valid = true;
	}
	else {
		Name = text;
		valid = false;
	}
}

std::vector<std::string> tokenize(std::string const& text) {
	std::vector<std::string> params;
	if (text.empty()) return params;

	params.reserve(6);

	if (text[0] != '(') {
		// There's just one parameter (because there's no parentheses)
		// This means text is all our parameters
		params.push_back(boost::trim_copy(text));
		return params;
	}

	auto start = text.begin() + 1;
	int depth = 0;
	for (size_t i = 0; i < text.size(); ++i) {
		if (text[i] == '(')
			++depth;
		else if (text[i] == ')') {
			--depth;
			if (depth == 0) {
				params.emplace_back(start, text.begin() + i);
				start = text.begin() + i + 1;
			}
		}
		else if (depth == 1 && text[i] == ',') {
			params.emplace_back(start, text.begin() + i);
			start = text.begin() + i + 1;
		}
	}

	if (start != text.end())
		params.emplace_back(start, text.end());

	return params;
}

void AssOverrideTag::ParseParameters(std::string const& text, AssOverrideTagProto::iterator proto_it) {
	Clear();

	// Tokenize text, attempting to find all parameters
	std::vector<std::string> paramList(tokenize(text));
	size_t totalPars = paramList.size();

	int parsFlag = 1 << (totalPars - 1); // Get optional parameters flag
	// vector (i)clip is the second clip proto_ittype in the list
	if ((Name == "\\clip" || Name == "\\iclip") && totalPars != 4)
		++proto_it;

	unsigned curPar = 0;
	for (auto& curproto : proto_it->params) {
		// Create parameter
		AssOverrideParameter *newparam = new AssOverrideParameter;
		newparam->classification = curproto.classification;
		Params.push_back(newparam);

		// Check if it's optional and not present
		if (!(curproto.optional & parsFlag) || curPar >= totalPars) {
			newparam->omitted = true;
			continue;
		}

		std::string const& curtok = paramList[curPar++];

		if (curtok.empty()) {
			curPar++;
			continue;
		}

		char firstChar = curtok[0];
		bool auto4 = (firstChar == '!' || firstChar == '$' || firstChar == '%') && curproto.type != VARDATA_BLOCK;
		if (auto4) {
			newparam->Set(curtok);
			continue;
		}

		switch (curproto.type) {
			case VARDATA_INT:
				newparam->Set(boost::lexical_cast<int>(curtok));
				break;
			case VARDATA_FLOAT:
				newparam->Set(boost::lexical_cast<double>(curtok));
				break;
			case VARDATA_TEXT:
				newparam->Set(curtok);
				break;
			case VARDATA_BOOL:
				newparam->Set(boost::lexical_cast<int>(curtok) != 0);
				break;
			case VARDATA_BLOCK: {
				AssDialogueBlockOverride *temp = new AssDialogueBlockOverride(curtok);
				temp->ParseTags();
				newparam->Set(temp);
				break;
			}
			default:
				break;
		}
	}
}

AssOverrideTag::operator std::string() const {
	std::string result(Name);

	if (Params.size() > 1) result += '(';

	result += boost::join(Params
		| filtered([](const AssOverrideParameter *p) { return p->GetType() != VARDATA_NONE && !p->omitted; })
		| transformed(std::mem_fn(&AssOverrideParameter::Get<std::string>)),
		std::string(","));

	if (Params.size() > 1) result += ')';
	return result;
}
