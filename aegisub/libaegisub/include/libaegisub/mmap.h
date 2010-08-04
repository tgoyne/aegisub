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

/// @file mmap.h
/// @brief mmap-backed blob of memory
/// @ingroup libaegisub

#if !defined(AGI_PRE) && !defined(LAGI_PRE)
#ifdef _WIN32
#include <memory>
#else
#include <tr1/memory>
#endif

#include <stdint.h>
#endif

#include <libaegisub/exception.h>

namespace agi {
namespace mmc {

class MMapCache;

/// @class CacheHandle
/// @brief RAII wrapper around a pointer returned from MMapCache
///
/// Storing the wrapped address beyond the lifespan of the CacheHandle is
/// extremely unlikely to work and should not be done. Generally the CacheHandle
/// should not be stored either, and should be re-requested from the containing
/// MMapCache for every use.
class CacheHandle {
	int64_t offset;
	size_t size;
	std::tr1::shared_ptr<void> data;
public:
	/// @brief Constructor
	/// @param ptr    Pointer to wrap
	/// @param size   Size of the allocated chunk
	/// @param offset Amount that the pointer was offset from the requested
	///               address for alignment purposes
	CacheHandle(void *ptr, size_t size, int64_t offset);
	/// @brief Implicit char* conversion operation
	operator char*() const;
};

/// @class MMapCache
/// @brief mmap-backed blob of memory
///
/// Intended for uses when a blob of memory which is larger than is practical
/// to simply allocate is desired. When supported by the OS, this includes blobs
/// larger than can be addressed by 32-bit programs.
class MMapCache {
	void *handle;
	int fd;
	/// The system's minimum addressing granularity which mappings must be
	/// aligned to. Note that on windows this is not necessarily the same thing
	/// as the page size; POSIX requires that it is.
	int64_t page_size;
public:
	/// @brief Constructor
	/// @param size Size of cache in bytes
	MMapCache(int64_t size);
	~MMapCache();
	/// @brief Get read pointer to the requested address
	/// @param start Offset in the block to read from
	/// @param size  Length in bytes to make readable
	/// @return Read-only pointer to requested location in the cache; only
	///         valid until the end of the statement
	CacheHandle Read(int64_t start, size_t size) const;
	/// @brief Get write pointer to the requested address
	/// @param start Offset in the block to write to
	/// @param size  Length in bytes to make writable
	/// @return Read-write pointer to requested location in the cache; only
	///         valid until the end of the statement
	CacheHandle Write(int64_t start, size_t size) const;
	/// @brief Get the system's page size
	/// @return The system's page size
	///
	/// Although not required, reading and writing addresses aligned to the
	/// system's page size may be faster
	int64_t GetPageSize() const { return page_size; }
};

DEFINE_BASE_EXCEPTION_NOINNER(Error, agi::Exception)
DEFINE_SIMPLE_EXCEPTION_NOINNER(UnknownError, Error, "mmap/unknown")
DEFINE_SIMPLE_EXCEPTION_NOINNER(InsufficientStorage, Error, "mmap/insufficientstorage")
DEFINE_SIMPLE_EXCEPTION_NOINNER(BadArgument, Error, "mmap/badargument")

}
}
