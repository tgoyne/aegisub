// Copyright (c) 2010, Thomas Goyne <plorkyeran@aegisub.org>
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
// $Id$

/// @file mmap.cpp
/// @brief 
/// @ingroup libaegisub

#include "libaegisub/mmap.h"

#ifndef LAGI_PRE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "libaegisub/log.h"

namespace agi {
namespace mmc {

static std::string get_err_msg(DWORD error) {
	char *msg;
	// Locale-independent errors are all in English so it should be safe to
	// simply use the A variant rather than converting UTF-16LE -> UTF-8
	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, (LPSTR)&msg, 0, NULL);
	LOG_E("audio/background/cache") << "Error " << error << ": " << msg;
	std::string errmsg(msg);
	LocalFree(msg);
	return errmsg;
}

CacheHandle::CacheHandle(void *ptr, size_t, int64_t offset)
: data(ptr, UnmapViewOfFile)
, offset(offset)
{
}
CacheHandle::operator char*() const {
	return reinterpret_cast<char*>(data.get()) + offset;
}

MMapCache::MMapCache(int64_t size) {
	if (size <= 0) {
		throw BadArgument("Cache size must be greater than zero");
	}
	// Try backing it with the page file first, as removes the possibility of
	// accumulating temp files due to crashes and such
	// It's also theoretically faster, but that's unlikely to be significant
	handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, DWORD(size >> 32), DWORD(size & 0xFFFFFFFF), NULL);
	if (!handle) {
		DWORD error = GetLastError();
		switch (error) {
			case ERROR_INVALID_PARAMETER:
				throw BadArgument(get_err_msg(error));
			// It appears that COMMITMENT_LIMIT means the page file was big
			// enough but too much is used already, and NOT_ENOUGH_MEMORY means
			// that the requested size was larger than the total memory available
			case ERROR_COMMITMENT_LIMIT:
			case ERROR_NOT_ENOUGH_MEMORY: {
				// Couldn't make it backed by the page file, so back it with
				// a real temp file
				wchar_t temp_path[MAX_PATH];
				wchar_t temp_file[MAX_PATH];
				GetTempPath(MAX_PATH, temp_path);
				GetTempFileName(temp_path, L"agi", 0, temp_file);

				LARGE_INTEGER lsize;
				lsize.QuadPart = size;
				handle = CreateFile(temp_file, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, NULL);
				try {
					SetFilePointerEx(handle, lsize, NULL, FILE_BEGIN);
					if (!SetEndOfFile(handle)) {
						error = GetLastError();
						if (error == ERROR_INVALID_PARAMETER)
							throw InsufficientStorage("Not enough free hard drive space");
						throw UnknownError(get_err_msg(error));
					}
				}
				catch (...) {
					CloseHandle(handle);
					throw;
				}
				break;
			}
			default:
				throw UnknownError(get_err_msg(error));
		}
	}
	SYSTEM_INFO info;
	GetNativeSystemInfo(&info);
	page_size = info.dwAllocationGranularity;
}
MMapCache::~MMapCache() {
	CloseHandle(handle);
}

CacheHandle make_handle(int64_t pos, size_t size, int64_t page_size, void *handle, DWORD mode) {
	// Positions passed to MapViewOfFile must be aligned to the allocation
	// granularity; rather than making the client code worry about this just
	// align all offsets passed
	int64_t offset = pos % page_size;
	pos -= offset;
	size += (size_t)offset;
	void *ptr = MapViewOfFile(handle, mode, DWORD(pos >> 32), DWORD(pos & 0xFFFFFFFF), size);
	if (!ptr) throw UnknownError(get_err_msg(GetLastError()));
	return CacheHandle(ptr, size, offset);
}

CacheHandle MMapCache::Read(int64_t start, size_t size) const {
	return make_handle(start, size, page_size, handle, FILE_MAP_READ);
}
CacheHandle MMapCache::Write(int64_t start, size_t size) const {
	return make_handle(start, size, page_size, handle, FILE_MAP_WRITE);
}

}
}