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
#include <limits>
#include <tr1/functional>

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#endif

#include "libaegisub/log.h"

namespace agi {
namespace mmc {

using namespace std::tr1::placeholders;

CacheHandle::CacheHandle(void *ptr, size_t size, int64_t offset)
: offset(offset)
, data(ptr, std::tr1::bind(munmap, _1, size))
{
}
CacheHandle::operator char*() const {
	return reinterpret_cast<char*>(data.get()) + offset;
}

MMapCache::MMapCache(int64_t size) {
	// Make sure that we will actually be able to access the entire file
#ifndef HAVE_MMAP2
	if (size > std::numeric_limits<off_t>::max()) {
		throw InsufficientStorage("Your operating system does not support mmaping files over 2GB");
	}
#endif
	if (size <= 0) {
		throw BadArgument("Cache size must be greater than zero");
	}
	char file_template[25];
	strcpy(file_template, "/tmp/libaegisub.XXXXXX");
	fd = mkstemp(file_template);
	if (ftruncate64(fd, size)) {
		switch (errno) {
			case EINVAL:
			case EFBIG:
				throw InsufficientStorage("Not enough free hard drive space");

		}
	}
	page_size = sysconf(_SC_PAGESIZE);;
}
MMapCache::~MMapCache() {
	close(fd);
}

CacheHandle make_handle(int64_t pos, size_t size, int64_t page_size, int fd, int mode) {
	int64_t offset = pos % page_size;
	pos -= offset;
	size += (size_t)offset;
	void *ptr;
	if (pos > std::numeric_limits<off_t>::max()) {
#ifdef HAVE_MMAP2
		ptr = mmap2(NULL, size, mode, MAP_SHARED, fd, pos / page_size);
#else
		throw agi::InternalError("Tried to mmap past what the OS supports",  NULL);
#endif
	}
	else {
		ptr = mmap(NULL, size, mode, MAP_SHARED, fd, pos);
	}
	if (!ptr) {
		switch (errno) {
			case EINVAL:
				throw BadArgument("Cannot mmap 0 bytes");
			case ENOMEM:
				throw InsufficientStorage("Not enough free memory");
			case ENXIO:
			case EOVERFLOW:
				LOG_E("mmap/make_handle") << "Attempted to map " << pos << " to " << pos + size;
				throw BadArgument("Cannot map requested range.");
			default:
				LOG_E("mmap/make_handle") << "Unknown error " << errno;
				throw UnknownError("An unknown error occurred");
		}
	}
	return CacheHandle(ptr, size, offset);
}

CacheHandle MMapCache::Read(int64_t start, size_t size) const {
	return make_handle(start, size, page_size, fd, PROT_READ);
}
CacheHandle MMapCache::Write(int64_t start, size_t size) const {
	return make_handle(start, size, page_size, fd, PROT_WRITE);
}

}
}
