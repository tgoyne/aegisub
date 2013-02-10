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

#include "ass_entry.h"
#include "ass_time.h"

#include <libaegisub/exception.h>

#include <boost/flyweight.hpp>
#include <vector>

class AssDialogue : public AssEntry {
	std::string GetData(bool ssa) const;

public:
	/// Unique ID of this line. Copies of the line for Undo/Redo purposes
	/// preserve the unique ID, so that the equivalent lines can be found in
	/// the different versions of the file.
	const int Id;

	/// Is this a comment line?
	bool Comment;
	/// Layer number
	int Layer;
	/// Margins: 0 = Left, 1 = Right, 2 = Top (Vertical)
	int Margin[3];
	/// Starting time
	AssTime Start;
	/// Ending time
	AssTime End;
	/// Style name
	boost::flyweight<std::string> Style;
	/// Actor name
	boost::flyweight<std::string> Actor;
	/// Effect name
	boost::flyweight<std::string> Effect;
	/// Raw text data
	boost::flyweight<std::string> Text;

	AssEntryGroup Group() const override { return ENTRY_DIALOGUE; }

	const std::string GetEntryData() const override;

	template<int which>
	void SetMarginString(std::string const& value) { SetMarginString(value, which);}
	/// @brief Set a margin
	/// @param value New value of the margin
	/// @param which 0 = left, 1 = right, 2 = vertical
	void SetMarginString(std::string const& value, int which);
	/// @brief Get a margin
	/// @param which 0 = left, 1 = right, 2 = vertical
	std::string GetMarginString(int which) const;
	/// Get the line as SSA rather than ASS
	std::string GetSSAText() const override;
	/// Does this line collide with the passed line?
	bool CollidesWith(const AssDialogue *target) const;

	AssEntry *Clone() const;

	AssDialogue();
	AssDialogue(AssDialogue const&);
	AssDialogue(std::string const& data);
};
