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

/// @file ass_export_filter.h
/// @see ass_export_filter.cpp
/// @ingroup export
///

#pragma once

#ifndef AGI_PRE
#include <list>
#include <memory>

#include <wx/string.h>
#include <wx/window.h>
#endif

class AssFile;
class AssExportFilter;
class DialogExport;
class AssExporter;

/// DOCME
typedef std::list<AssExportFilter*> FilterList;

/// DOCME
/// @class AssExportFilterChain
/// @brief DOCME
///
/// DOCME
class AssExportFilterChain {
	static FilterList *filters();
public:
	/// Register an export filter
	static void Register(AssExportFilter *filter);
	/// Unregister an export filter; must have been registered
	static void Unregister(AssExportFilter *filter);
	/// Unregister and delete all export filters
	static void Clear();
	/// Get a filter by name or NULL if it doesn't exist
	static AssExportFilter *GetFilter(wxString const& name);

	/// Get the list of registered filters
	static const FilterList *GetFilterList();
};

/// DOCME
/// @class AssExportFilter
/// @brief DOCME
///
/// DOCME
class AssExportFilter {
	/// The filter chain needs to be able to muck around with filter names when
	/// they're registered to avoid duplicates
	friend class AssExportFilterChain;

	/// This filter's name
	wxString name;

	/// Higher priority = run earlier
	int priority;

	/// User-visible description of this filter
	wxString description;

	/// Should this filter be automatically applied when sending subtitles to
	/// the renderer and exporting to non-ASS formats
	bool auto_apply;

public:
	AssExportFilter(wxString const& name, wxString const& description, int priority = 0, bool auto_apply = false);
	virtual ~AssExportFilter() { };

	wxString const& GetName() const { return name; }
	wxString const& GetDescription() const { return description; }
	bool GetAutoApply() const { return auto_apply; }

	/// Process subtitles
	/// @param subs Subtitles to process
	/// @param parent_window Window to use as the parent if the filter wishes
	///                      to open a progress dialog
	virtual void ProcessSubs(AssFile *subs, wxWindow *parent_window=0)=0;

	/// Draw setup controls
	virtual wxWindow *GetConfigDialogWindow(wxWindow *parent) { return 0; }

	/// Load settings to use from the configuration dialog
	/// @param is_default If true use default settings instead
	virtual void LoadSettings(bool is_default) { }
};
