// Copyright (c) 2005, Rodrigo Braz Monteiro
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

/// @file ass_override.h
/// @see ass_override.cpp
/// @ingroup subs_storage
///

#include <vector>

#include "variable_data.h"

/// Type of parameter; probably only used by the resample tool
enum AssParameterClass {
	PARCLASS_NORMAL,
	PARCLASS_ABSOLUTE_SIZE,
	PARCLASS_ABSOLUTE_POS_X,
	PARCLASS_ABSOLUTE_POS_Y,
	PARCLASS_RELATIVE_SIZE_X,
	PARCLASS_RELATIVE_SIZE_Y,
	PARCLASS_RELATIVE_TIME_START,
	PARCLASS_RELATIVE_TIME_END,
	PARCLASS_KARAOKE,
	PARCLASS_DRAWING
};

/// The parameter is absent unless the total number of parameters is the
/// indicated number. Note that only arguments not at the end need to be marked
/// as optional; this is just to know which parameters to skip when there are
/// earlier optional arguments
enum AssParameterOptional {
	NOT_OPTIONAL = 0xFF,
	OPTIONAL_1 = 0x01,
	OPTIONAL_2 = 0x02,
	OPTIONAL_3 = 0x04,
	OPTIONAL_4 = 0x08,
	OPTIONAL_5 = 0x10,
	OPTIONAL_6 = 0x20,
	OPTIONAL_7 = 0x40
};

/// A single parameter to an override tag
class AssOverrideParameter : public VariableData {
public:
	/// Type of parameter
	AssParameterClass classification;

	/// Is the parameter's value actually given?
	bool omitted;

	AssOverrideParameter();
	AssOverrideParameter(const AssOverrideParameter&);
	void operator=(const AssOverrideParameter &param);
};

/// Prototype of a single override parameter
struct AssOverrideParamProto {
	/// ASS_ParameterOptional
	int optional;

	/// Type of this parameter
	VariableDataType type;

	/// Semantic type of this parameter
	AssParameterClass classification;

	AssOverrideParamProto(VariableDataType type, int opt=NOT_OPTIONAL, AssParameterClass classi=PARCLASS_NORMAL);
};

struct AssOverrideTagProto {
	/// Name of the tag, with slash
	std::string name;
	/// Parameters to this tag
	std::vector<AssOverrideParamProto> params;
	typedef std::vector<AssOverrideTagProto>::iterator iterator;

	/// @brief Add a parameter to this tag prototype
	/// @param type Data type of the parameter
	/// @param classi Semantic type of the parameter
	/// @param opt Situations in which this parameter is present
	void AddParam(VariableDataType type, AssParameterClass classi = PARCLASS_NORMAL, int opt = NOT_OPTIONAL);
	/// @brief Convenience function for single-argument tags
	/// @param name Name of the tag, with slash
	/// @param type Data type of the parameter
	/// @param classi Semantic type of the parameter
	/// @param opt Situations in which this parameter is present
	void Set(std::string const& name, VariableDataType type, AssParameterClass classi = PARCLASS_NORMAL, int opt = NOT_OPTIONAL);
};

class AssOverrideTag {
	bool valid;

public:
	std::string Name;
	std::vector <AssOverrideParameter*> Params;

	AssOverrideTag();
	AssOverrideTag(std::string const& text);
	~AssOverrideTag();

	bool IsValid() const { return valid; }
	/// @brief Parses the parameters for the ass override tag
	/// @param text All text between the name and the next \ or the end of the override block
	void ParseParameters(std::string const& text, AssOverrideTagProto::iterator proto);
	void Clear();
	void SetText(std::string const& text);
	operator std::string() const;
};
