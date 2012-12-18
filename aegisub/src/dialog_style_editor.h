// Copyright(c) 2005, Rodrigo Braz Monteiro
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
// CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Aegisub Project http://www.aegisub.org/

/// @file dialog_style_editor.h
/// @see dialog_style_editor.cpp
/// @ingroup style_editor
///

#include <wx/dialog.h>

#include <libaegisub/scoped_ptr.h>

namespace { class StyleRenamer; }
namespace agi { struct Context; }
class AssStyle;
class AssStyleStorage;
class PersistLocation;
class SubtitlesPreview;
class wxSizer;

class DialogStyleEditor : public wxDialog {
	agi::Context *c;
	agi::scoped_ptr<PersistLocation> persist;

	/// If true, the style was just created and so the user should not be
	/// asked if they want to change any existing lines should they rename
	/// the style
	bool is_new;

	/// The style currently being edited
	AssStyle *style;

	/// Copy of style passed to the subtitles preview to avoid making changes
	/// before Apply is clicked
	agi::scoped_ptr<AssStyle> work;

	/// The style storage style is in, if applicable
	AssStyleStorage *store;

	wxString preview_text;
	SubtitlesPreview *subs_preview;

	void UpdateWorkStyle();
	bool NameIsDuplicate() const;
	bool StyleRefsNeedRename(StyleRenamer& renamer) const;

	void OnCommandPreviewUpdate(wxCommandEvent &event);

	void MakePreviewBox(wxSizer *PreviewBox);

	wxSizer *MakeColors();
	wxSizer *MakeMargins();
	wxSizer *MakeMiscBox();
	wxSizer *MakeFontBox();
	wxSizer *MakeStyleNameBox();

	/// Save the current changes to the file or storage
	void Apply();

	/// Close the dialog
	/// @param apply Save the current changes first
	void Close(bool apply);

public:
	DialogStyleEditor(wxWindow *parent, AssStyle *style, agi::Context *c, AssStyleStorage *store = 0, wxString const& new_name = "");
	~DialogStyleEditor();

	wxString GetStyleName() const;
};
