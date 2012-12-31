// Copyright (c) 2005-2006, Rodrigo Braz Monteiro
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

/// @file utils.cpp
/// @brief Misc. utility functions
/// @ingroup utility
///

#include "config.h"

#include "utils.h"

#include "compat.h"
#include "options.h"

#ifdef __UNIX__
#include <unistd.h>
#endif
#include <map>

#include <wx/clipbrd.h>
#include <wx/stdpaths.h>
#include <wx/window.h>

#include <libaegisub/fs.h>
#include <libaegisub/log.h>
#include <libaegisub/path.h>

#ifdef __APPLE__
#include <libaegisub/util_osx.h>
#endif

wxDEFINE_EVENT(EVT_CALL_THUNK, wxThreadEvent);

std::string MakeRelativePath(std::string const& path, std::string const& reference) {
	if (path.empty() || path[0] == '?') return path;
	wxFileName wx_path(to_wx(path));
	wx_path.MakeRelativeTo(wxFileName(to_wx(reference)).GetPath());
	return from_wx(wx_path.GetFullPath());
}

std::string DecodeRelativePath(std::string const& path, std::string const& reference) {
	if (path.empty() || path[0] == '?') return path;
	return agi::Path::Combine(reference, path);
}

wxString AegiFloatToString(double value) {
	return wxString::Format("%g",value);
}

wxString AegiIntegerToString(int value) {
	return wxString::Format("%i",value);
}

/// @brief There shall be no kiB, MiB stuff here Pretty reading of size
wxString PrettySize(int bytes) {
	const char *suffix[] = { "", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB" };

	// Set size
	size_t i = 0;
	double size = bytes;
	while (size > 1024 && i + 1 < sizeof(suffix) / sizeof(suffix[0])) {
		size /= 1024.0;
		i++;
	}

	// Set number of decimal places
	const char *fmt = "%.0f";
	if (size < 10)
		fmt = "%.2f";
	else if (size < 100)
		fmt = "%1.f";
	return wxString::Format(fmt, size) + " " + suffix[i];
}

int SmallestPowerOf2(int x) {
	x--;
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	x++;
	return x;
}

bool IsWhitespace(wchar_t c)
{
	const wchar_t whitespaces[] = {
		// http://en.wikipedia.org/wiki/Space_(punctuation)#Table_of_spaces
		0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x0020, 0x0085, 0x00A0,
		0x1680, 0x180E, 0x2000, 0x2001, 0x2002, 0x2003, 0x2004, 0x2005,
		0x2006, 0x2007, 0x2008, 0x2009, 0x200A, 0x2028, 0x2029, 0x202F,
		0x025F, 0x3000, 0xFEFF
	};
	const size_t num_chars = sizeof(whitespaces) / sizeof(whitespaces[0]);
	return std::binary_search(whitespaces, whitespaces + num_chars, c);
}

bool StringEmptyOrWhitespace(const wxString &str)
{
	for (size_t i = 0; i < str.size(); ++i)
		if (!IsWhitespace(str[i]))
			return false;

	return true;
}

void RestartAegisub() {
	config::opt->Flush();

#if defined(__WXMSW__)
	wxStandardPaths stand;
	wxExecute("\"" + stand.GetExecutablePath() + "\"");
#elif defined(__WXMAC__)
	std::string bundle_path = agi::util::OSX_GetBundlePath();
	std::string helper_path = agi::util::OSX_GetBundleAuxillaryExecutablePath("restart-helper");
	if (bundle_path.empty() || helper_path.empty()) return;

	wxString exec = wxString::Format("\"%s\" /usr/bin/open -n \"%s\"'", to_wx(helper_path), to_wx(bundle_path));
	LOG_I("util/restart/exec") << exec;
	wxExecute(exec);
#else
	wxStandardPaths stand;
	wxExecute(stand.GetExecutablePath());
#endif
}

bool ForwardMouseWheelEvent(wxWindow *source, wxMouseEvent &evt) {
	wxWindow *target = wxFindWindowAtPoint(wxGetMousePosition());
	if (!target || target == source) return true;

	// If the mouse is over a parent of the source window just pretend it's
	// over the source window, so that the mouse wheel works on borders and such
	wxWindow *parent = source->GetParent();
	while (parent && parent != target) parent = parent->GetParent();
	if (parent == target) return true;

	// Otherwise send it to the new target
	target->GetEventHandler()->ProcessEvent(evt);
	evt.Skip(false);
	return false;
}

std::string GetClipboard() {
	wxString data;
	wxClipboard *cb = wxClipboard::Get();
	if (cb->Open()) {
		if (cb->IsSupported(wxDF_TEXT)) {
			wxTextDataObject raw_data;
			cb->GetData(raw_data);
			data = raw_data.GetText();
		}
		cb->Close();
	}
	return from_wx(data);
}

void SetClipboard(std::string const& new_data) {
	wxClipboard *cb = wxClipboard::Get();
	if (cb->Open()) {
		cb->SetData(new wxTextDataObject(to_wx(new_data)));
		cb->Close();
		cb->Flush();
	}
}

void SetClipboard(wxBitmap const& new_data) {
	wxClipboard *cb = wxClipboard::Get();
	if (cb->Open()) {
		cb->SetData(new wxBitmapDataObject(new_data));
		cb->Close();
		cb->Flush();
	}
}

namespace {
class cache_cleaner : public wxThread {
	std::string directory;
	std::string file_type;
	uint64_t max_size;
	uint64_t max_files;

	ExitCode Entry() {
		static wxMutex cleaning_mutex;
		wxMutexLocker lock(cleaning_mutex);

		if (!lock.IsOk()) {
			LOG_D("utils/clean_cache") << "cleaning already in progress, thread exiting";
			return (ExitCode)1;
		}

		// sleep for a bit so we (hopefully) don't thrash the disk too much while indexing is in progress
		wxThread::This()->Sleep(2000);

		uint64_t total_size = 0;
		std::multimap<int64_t, std::string> cachefiles;
		for (auto const& file : agi::fs::FilesInDirectory(directory, file_type)) {
			cachefiles.insert(make_pair(agi::fs::ModifiedTime(file), file));
			total_size += agi::fs::Size(file);
			wxThread::This()->Sleep(250);
		}

		if (cachefiles.size() <= max_files && total_size <= max_size) {
			LOG_D("utils/clean_cache")
				<< "cache does not need cleaning (maxsize=" << max_size
				<< ", cursize=" << total_size
				<< "; maxfiles=" << max_files
				<< ", numfiles=" << cachefiles.size()
				<< "), exiting";
			return (wxThread::ExitCode)0;
		}

		int deleted = 0;
		for (auto const& i : cachefiles) {
			// stop cleaning?
			if ((total_size <= max_size && cachefiles.size() - deleted <= max_files) || cachefiles.size() - deleted < 2)
				break;

			uint64_t size = agi::fs::Size(i.second);
			try {
				agi::fs::Remove(i.second);
			}
			catch  (agi::Exception const& e) {
				LOG_D("utils/clean_cache") << "failed to remove file " << i.second << ": " << e.GetChainedMessage();
				continue;
			}

			total_size -= size;
			++deleted;

			wxThread::This()->Sleep(250);
		}

		LOG_D("utils/clean_cache") << "deleted " << deleted << " files, exiting";
		return (wxThread::ExitCode)0;
	}

public:
	cache_cleaner(std::string const& directory, std::string const& file_type, uint64_t max_size, uint64_t max_files)
	: wxThread(wxTHREAD_DETACHED)
	, directory(directory)
	, file_type(file_type)
	, max_size(max_size << 20)
	{
		if (max_files < 1)
			this->max_files = (size_t)-1;
		else
			this->max_files = (size_t)max_files;
	}
};
}

void CleanCache(std::string const& directory, std::string const& file_type, uint64_t max_size, uint64_t max_files) {
	LOG_D("utils/clean_cache") << "attempting to start cleaner thread";
	wxThread *CleaningThread = new cache_cleaner(directory, file_type, max_size, max_files);

	if (CleaningThread->Create() != wxTHREAD_NO_ERROR) {
		LOG_D("utils/clean_cache") << "thread creation failed";
		delete CleaningThread;
	}
	else if (CleaningThread->Run() != wxTHREAD_NO_ERROR) {
		LOG_D("utils/clean_cache") << "failed to start thread";
		delete CleaningThread;
	}

	LOG_D("utils/clean_cache") << "thread started successfully";
}

size_t MaxLineLength(std::string const& text) {
	size_t max_line_length = 0;
	size_t current_line_length = 0;
	bool last_was_slash = false;
	bool in_ovr = false;

	for (auto const& c : text) {
		if (in_ovr) {
			in_ovr = c != '}';
			continue;
		}

		if (c == '\\') {
			current_line_length += last_was_slash; // for the slash before this one
			last_was_slash = true;
			continue;
		}

		if (last_was_slash) {
			last_was_slash = false;
			if (c == 'h') {
				++current_line_length;
				continue;
			}

			if (c == 'n' || c == 'N') {
				max_line_length = std::max(max_line_length, current_line_length);
				current_line_length = 0;
				continue;
			}

			// Not actually an escape so add the character for the slash and fall through
			++current_line_length;
		}

		if (c == '{')
			in_ovr = true;
		else
			++current_line_length;
	}

	current_line_length += last_was_slash;
	return std::max(max_line_length, current_line_length);
}

// OS X implementation in osx_utils.mm
#ifndef __WXOSX_COCOA__
void AddFullScreenButton(wxWindow *) { }
void SetFloatOnParent(wxWindow *) { }
#endif
