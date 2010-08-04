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

/// @file libaegisub_mmap.cpp
/// @brief 
/// @ingroup video_input

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS 1
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <libaegisub/mmap.h>

#include "main.h"

using namespace agi::mmc;

TEST(lagi_mmap, good_cache_sizes) {
	EXPECT_NO_THROW(MMapCache(1));
	EXPECT_NO_THROW(MMapCache(1024));
	EXPECT_NO_THROW(MMapCache(1073741824));
}

TEST(lagi_mmap, bad_cache_sizes) {
	EXPECT_THROW(MMapCache(1LL << 62), InsufficientStorage);
	EXPECT_THROW(MMapCache(0), BadArgument);
	EXPECT_THROW(MMapCache(-1), BadArgument);
}

TEST(lagi_mmap, cache_coherent) {
	MMapCache cache(4096);
	CacheHandle read = cache.Read(0, 5);
	strcpy(cache.Write(0, 5), "test");
	EXPECT_STREQ("test", read);
}

TEST(lagi_mmap, read_unaligned) {
	MMapCache cache(12345);
	ASSERT_NO_THROW(cache.Read(5432, 88));
	strcpy(cache.Write(0, 5), "test");
	EXPECT_STREQ("st", cache.Read(2, 3));
}

TEST(lagi_mmap, write_unaligned) {
	MMapCache cache(12345);
	ASSERT_NO_THROW(strcpy(cache.Write(1, 5), "test"));
	EXPECT_STREQ("st", cache.Read(3, 3));
}
