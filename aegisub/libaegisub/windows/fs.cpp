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

#include "config.h"

#include "libaegisub/fs.h"

#include "libaegisub/access.h"
#include "libaegisub/charset_conv_win.h"
#include "libaegisub/exception.h"
#include "libaegisub/path.h"
#include "libaegisub/scoped_ptr.h"
#include "libaegisub/util_win.h"

using agi::charset::ConvertW;
using agi::charset::ConvertLocal;

namespace {
	using namespace agi;

void get_attributes(WIN32_FILE_ATTRIBUTE_DATA &info, std::string const& path) {
	if (GetFileAttributesEx(ConvertW(path).c_str(), GetFileExInfoStandard, &info)) {
		if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			throw acs::NotAFile(path + " is not a file");
		return;
	}

	switch (DWORD err = GetLastError()) {
		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			throw FileNotFoundError(path);
		case ERROR_ACCESS_DENIED:
			throw acs::Read("Access denied to file or path component");
		default:
			throw acs::Read("Error occurred reading file size: " + util::ErrorString(err));
	}
}
}

namespace agi { namespace fs {
bool Exists(std::string const& path) {
	return GetFileAttributes(ConvertW(path).c_str()) != INVALID_FILE_ATTRIBUTES;
}

bool FileExists(std::string const& path) {
	DWORD attrib = GetFileAttributes(ConvertW(path).c_str());
	return attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY);
}

bool DirectoryExists(std::string const& path) {
	DWORD attrib = GetFileAttributes(ConvertW(path).c_str());
	return attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY);
}

std::string ShortName(std::string const& path) {
	std::wstring out(MAX_PATH + 1, 0);
	DWORD len = GetShortPathName(ConvertW(path).c_str(), &out[0], out.size());
	if (!len)
		return path;
	out.resize(len);
	return ConvertLocal(out);
}

uint64_t FreeSpace(std::string const& path) {
	if (FileExists(path))
		return FreeSpace(Path::DirName(path));

	ULARGE_INTEGER bytes_available;
	if (GetDiskFreeSpaceEx(ConvertW(path).c_str(), &bytes_available, 0, 0))
		return bytes_available.QuadPart;

	acs::CheckDirRead(path);

	/// @todo GetLastError -> Exception mapping
	throw "Unknown error getting free space";
}

uint64_t Size(std::string const& path) {
	WIN32_FILE_ATTRIBUTE_DATA info;
	get_attributes(info, path);
	return (uint64_t(info.nFileSizeHigh) << 32) + info.nFileSizeLow;
}

int64_t ModifiedTime(std::string const& path) {
	WIN32_FILE_ATTRIBUTE_DATA info;
	get_attributes(info, path);
	return (int64_t(info.ftLastWriteTime.dwHighDateTime) << 32) + info.ftLastWriteTime.dwLowDateTime;
}

#undef CreateDirectory
void CreateDirectory(std::string const& path) {
	if (DirectoryExists(path)) return;

	std::string parent = Path::DirName(path);
	if (!DirectoryExists(parent))
		CreateDirectory(parent);

	if (!CreateDirectoryW(ConvertW(path).c_str(), nullptr))
		throw acs::Write("Could not create directory: " + path);
}

void Touch(std::string const& path) {
	CreateDirectory(Path::DirName(path));

	SYSTEMTIME st;
	FILETIME ft;
	GetSystemTime(&st);
	if(!SystemTimeToFileTime(&st, &ft))
		throw EnvironmentError("SystemTimeToFileTime failed with error: " + util::ErrorString(GetLastError()));

	scoped_holder<HANDLE, BOOL (__stdcall *)(HANDLE)>
		h(CreateFile(ConvertW(path).c_str(), GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr), CloseHandle);
	// error handling etc.
	if (!SetFileTime(h, nullptr, nullptr, &ft))
		throw EnvironmentError("SetFileTime failed with error: " + util::ErrorString(GetLastError()));
}

void Rename(const std::string& from, const std::string& to) {
	acs::CheckFileWrite(from);

	try {
		acs::CheckFileWrite(to);
	} catch (FileNotFoundError const&) {
		acs::CheckDirWrite(Path::DirName(to));
	}

	if (!MoveFileEx(ConvertW(from).c_str(), ConvertW(to).c_str(), MOVEFILE_REPLACE_EXISTING))
		throw FileNotAccessibleError("Can not overwrite file: " + util::ErrorString(GetLastError()));
}

void Remove(std::string const& path) {
	if (!DeleteFile(ConvertW(path).c_str())) {
		DWORD err = GetLastError();
		if (err != ERROR_FILE_NOT_FOUND)
			throw FileNotAccessibleError("Can not remove file: " + util::ErrorString(err));
	}
}

void Copy(std::string const& from, std::string const& to) {
	acs::CheckFileRead(from);
	CreateDirectory(Path::DirName(to));
	acs::CheckDirWrite(Path::DirName(to));

	if (!CopyFile(ConvertW(from).c_str(), ConvertW(to).c_str(), false)) {
		switch (GetLastError()) {
			case ERROR_FILE_NOT_FOUND:
				throw FileNotFoundError(from);
			case ERROR_ACCESS_DENIED:
				throw acs::Write("Could not overwrite " + to);
			default:
				throw acs::Write("Could not copy: " + util::ErrorString(GetLastError()));
		}
	}
}

struct DirectoryIterator::PrivData {
	scoped_holder<HANDLE, BOOL (__stdcall *)(HANDLE)> h;
	PrivData() : h(INVALID_HANDLE_VALUE, FindClose) { }
};

DirectoryIterator::DirectoryIterator() { }
DirectoryIterator::DirectoryIterator(std::string const& path, std::string const& filter)
: privdata(new PrivData)
{
	WIN32_FIND_DATA data;
	privdata->h = FindFirstFileEx(ConvertW(Path::Combine(path, filter)).c_str(), FindExInfoBasic, &data, FindExSearchNameMatch, nullptr, 0);
	if (privdata->h == INVALID_HANDLE_VALUE) return;

	value = ConvertW(data.cFileName);
}

bool DirectoryIterator::operator==(DirectoryIterator const& rhs) const {
	return privdata.get() == rhs.privdata.get();
}

DirectoryIterator& DirectoryIterator::operator++() {
	WIN32_FIND_DATA data;
	if (FindNextFile(privdata->h, &data))
		value = ConvertW(data.cFileName);
	else
		privdata.reset();
	return *this;
}

DirectoryIterator::~DirectoryIterator() { }

} }
