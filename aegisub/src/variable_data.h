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
//
// $Id$

/// @file variable_data.h
/// @see variable_data.cpp
/// @ingroup utility subs_storage
///

#pragma once

#include <libaegisub/exception.h>

class AssDialogueBlockOverride;
class wxColour;

/// Type of data which can be stored in VariableData
enum VariableDataType {
	VARDATA_NONE,
	VARDATA_INT,
	VARDATA_FLOAT,
	VARDATA_TEXT,
	VARDATA_BOOL,
	VARDATA_COLOUR,
	VARDATA_BLOCK
};

/// @class VariableData
/// @brief A variant class with automatic coercion used to store tag parameters
class VariableData {
	union {
		void *value;
		int *value_int;
		double *value_float;
		bool *value_bool;
		wxString *value_text;
		wxColour *value_colour;
		AssDialogueBlockOverride **value_block;
	};

	/// The current type of this variant
	VariableDataType type;

	void operator=(VariableData const& param);
	VariableData(VariableData const&);

protected:
	/// Delete the stored value
	void DeleteValue();

public:
	/// @class VariableData::BadCast
	/// @brief An invalid conversion was requested
	DEFINE_SIMPLE_EXCEPTION(BadCast, agi::InternalError, "variable_data/bad_cast")
		/// @class VariableData::NoData
		/// @brief Get() was called with no default on a variant with no data
	DEFINE_SIMPLE_EXCEPTION(NoData, agi::InternalError, "variable_data/no_data")

	/// Constructor
	VariableData();
	/// Destructor
	virtual ~VariableData();

	/// Set the variant to a value
	/// @param new_value New value of the variant
	///
	/// new_value must be of type int, double, bool, wxString, wxColour or
	/// AssDialogueBlockOverride*
	template<class T> void Set(T new_value);

	/// Get the value, coercing it if needed and possible
	/// @return The value coerced to the requested type
	/// @throw VariableData::BadCast if the value could not be coerced to the requested type
	/// @throw VariableData::NoData if there is no data currently stored
	template<class T> T Get() const;

	/// Get the value, coercing it if needed and possible, or def if no value
	/// @param def Default value to use if this variant has none
	/// @return The value coerced to the requested type
	/// @throw VariableData::BadCast if the value could not be coerced to the requested type
	template<class T> T Get(T def) const {
		return value ? Get<T>() : def;
	}

	/// Does this variant currently have a value?
	bool HasValue() const { return !!value; }

	/// Get the current type of this variant
	VariableDataType GetType() const { return type; }
};
