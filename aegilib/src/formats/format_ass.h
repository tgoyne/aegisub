// Copyright (c) 2008, Rodrigo Braz Monteiro
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
// -----------------------------------------------------------------------------
//
// AEGISUB/AEGILIB
//
// Website: http://www.aegisub.net
// Contact: mailto:amz@aegisub.net
//

#pragma once
#include "format.h"
#include "format_handler.h"
#include "section.h"
#include "section_entry_dialogue.h"
#include "section_entry_style.h"

namespace Aegilib {

	// Prototypes
	class Model;

	// Advanced Substation Alpha format handler
	class FormatHandlerASS : public FormatHandler {
	private:
		SectionEntryPtr MakeEntry(const String &data,SectionPtr section,int version);
		void ProcessGroup(String cur,String &curGroup,int &version);
		Model &model;

	public:
		FormatHandlerASS(Model &model);
		~FormatHandlerASS();

		void Load(wxInputStream &file,const String encoding);
	};

	// Advanced Substation Alpha format
	class FormatASS : public Format {
	public:
		String GetName() const { return L"Advanced Substation Alpha"; }
		StringArray GetReadExtensions() const;
		StringArray GetWriteExtensions() const;
		FormatHandlerPtr GetHandler(Model &model) const { return FormatHandlerPtr(new FormatHandlerASS(model)); }

		bool CanStoreText() const { return true; }
		bool CanUseTime() const { return true; }

		bool HasStyles() const { return true; }
		bool HasMargins() const { return true; }
		bool HasActors() const { return true; }
	};

	// Dialogue
	class DialogueASS : public SectionEntryDialogue {
	private:
		String text;
		String style;
		String effect;
		String actor;
		Time start,end;
		int margin[4];
		int layer;
		bool isComment;

		bool Parse(String data,int version);

	public:
		// Constructors
		DialogueASS();
		DialogueASS(String data,int version);

		// Capabilities
		bool HasText() const { return true; }
		bool HasTime() const { return true; }
		bool HasStyle() const { return true; }
		bool HasMargins() const { return true; }

		// Read accessors
		const String& GetText() const { return text; }
		Time GetStartTime() const { return start; }
		Time GetEndTime() const { return end; }
		bool IsComment() const { return isComment; }
		int GetLayer() const { return layer; }
		int GetMargin(int n) const { return margin[n]; }
		const String& GetStyle() const { return style; }
		const String& GetActor() const { return actor; }
		const String& GetUserField() const { return effect; }

		// Write acessors
		void SetText(const String &setText) { text = setText; }
		void SetStartTime(Time setStart) { start = setStart; }
		void SetEndTime(Time setEnd) { end = setEnd; }
		void SetComment(bool _isComment) { isComment = _isComment; }
		void SetLayer(int _layer) { layer = _layer; }
		void SetMargin(int _margin,int value) { margin[_margin] = value; }
		void SetStyle(const String &_style) { style = _style; }
		void SetUserField(const String &userField) { effect = userField; }
	};

	// Style
	class StyleASS : public SectionEntryStyle {
	private:
		String name;
		String font;
		float fontSize;

		Colour colour[5];	// 0 = Primary, 1 = Secondary, 2 = Tertiary, 3 = Outline, 4 = Shadow
		int margin[4];

		bool bold;
		bool italic;
		bool underline;
		bool strikeout;

		int borderStyle;
		int alignment;
		int encoding;
		int relativeTo;

		float scalex;
		float scaley;
		float spacing;
		float angle;
		float outline_w;
		float shadow_w;

		bool Parse(String data,int version);
		int AlignSSAtoASS(int ssaAlignment);
		int AlignASStoSSA(int assAlignment);

	public:
		// Constructors
		StyleASS();
		StyleASS(String data,int version);

		// Read accessors
		String GetName() const { return name; }
		String GetFontName() const { return font; }
		float GetFontSize() const { return fontSize; }
		Colour GetColour(int n) const { return colour[n]; }
		int GetMargin(int n) const { return margin[n]; }
	};

};
