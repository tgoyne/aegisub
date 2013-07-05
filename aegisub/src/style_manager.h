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

#include <libaegisub/fs_fwd.h>
#include <libaegisub/signal.h>

#include <memory>
#include <string>
#include <vector>

class AssFile;
class AssStyleStorage;
class AssStyle;

class StyleManager {
protected:
	agi::signal::Signal<> StylesChanged;
	agi::signal::Signal<std::vector<std::string>> SetSelection;
public:
	/// Create a new StyleManager for the given subtitles file
	static std::unique_ptr<StyleManager> Create(AssFile *ass);

	/// Create a new StyleManager for the given storage file
	static std::unique_ptr<StyleManager> Create(agi::fs::path const& storage_name);

	virtual AssStyle *GetStyle(std::string name) = 0;
	virtual bool NeedsRenameProcessing(std::string const& old_name, std::string const& new_name) = 0;
	virtual void RenameStyle(AssStyle *style, std::string const& new_name) = 0;
	virtual std::vector<AssStyle *> GetStyles() = 0;
	virtual std::vector<std::string> GetStyleNames() = 0;
	virtual void Delete(std::string const& name) = 0;
	virtual AssStyle *Create(AssStyle *original = nullptr) = 0;
	virtual void Save(wxString const& message) = 0;
	virtual void Move(int type, int first, int last) = 0;
	virtual void AddStyles(std::vector<AssStyle*> const& to_copy) = 0;

	DEFINE_SIGNAL_ADDERS(StylesChanged, AddStylesChangedListener)
	DEFINE_SIGNAL_ADDERS(SetSelection, AddSelectionListener)
};
