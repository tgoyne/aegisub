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

#include <libaegisub/exception.h>
#include <libaegisub/path.h>

#include "main.h"
#include "util.h"

using agi::Path;

#ifdef _WIN32
#define DS "\\"
#else
#define DS "/"
#endif

TEST(lagi_path, normalize) {
	EXPECT_STREQ("", Path::Normalize("").c_str());
	EXPECT_STREQ("C:" DS "", Path::Normalize("C:\\").c_str());
	EXPECT_STREQ("C:" DS "foo.txt", Path::Normalize("C:/foo.txt").c_str());
	EXPECT_STREQ("C:" DS "foo.txt", Path::Normalize("C:/.\\foo.txt").c_str());
	EXPECT_STREQ("C:" DS "foo.txt", Path::Normalize("C:/bar/../foo.txt").c_str());
	EXPECT_STREQ("C:" DS "foo", Path::Normalize("C:/foo").c_str());
	EXPECT_STREQ("C:" DS "foo" DS "", Path::Normalize("C:/foo/").c_str());
	EXPECT_STREQ("C:" DS "foo" DS "", Path::Normalize("C:/foo/.").c_str());
	EXPECT_STREQ("C:" DS "foo" DS "", Path::Normalize("C:/foo/./").c_str());
	EXPECT_STREQ("C:" DS "", Path::Normalize("C:/foo/..").c_str());
	EXPECT_STREQ("C:" DS "", Path::Normalize("C:/foo/../").c_str());
	EXPECT_STREQ("C:" DS "foo" DS "", Path::Normalize("C:/foo/bar/..").c_str());
	EXPECT_STREQ("C:" DS "bar" DS "", Path::Normalize("C:/foo/../bar/").c_str());
	EXPECT_STREQ("C:" DS "foo" DS "bar" DS "", Path::Normalize("C:/foo/./bar/").c_str());
	EXPECT_STREQ("C:" DS "foo" DS "", Path::Normalize("C:\\foo\\").c_str());
	EXPECT_STREQ("C:" DS "a" DS "b" DS "c" DS "d" DS "e", Path::Normalize("C:///a/b//c/d\\e").c_str());
}

TEST(lagi_path, combine) {
	EXPECT_STREQ("", Path::Combine("", "").c_str());
	EXPECT_STREQ("C:" DS "foo.txt", Path::Combine("C:" DS "", "foo.txt").c_str());
	EXPECT_STREQ("C:" DS "bar" DS "foo.txt", Path::Combine("C:" DS "bar", "foo.txt").c_str());
	EXPECT_STREQ("C:" DS "bar" DS "foo.txt", Path::Combine("C:" DS "bar" DS "", "foo.txt").c_str());
	EXPECT_STREQ("C:" DS "baz" DS "foo.txt", Path::Combine("C:" DS "bar" DS "", "C:\\baz\\foo.txt").c_str());
}

TEST(lagi_path, is_absolute) {
	EXPECT_FALSE(Path::IsAbsolute(""));
	EXPECT_FALSE(Path::IsAbsolute("C"));
	EXPECT_FALSE(Path::IsAbsolute("C:"));
	EXPECT_TRUE(Path::IsAbsolute("C:\\"));
	EXPECT_TRUE(Path::IsAbsolute("C:/"));
	EXPECT_FALSE(Path::IsAbsolute("/"));
	EXPECT_FALSE(Path::IsAbsolute("/bin/sh"));
	EXPECT_FALSE(Path::IsAbsolute("\\bin\\sh"));
	EXPECT_TRUE(Path::IsAbsolute("\\\\bin/sh"));
}

TEST(lagi_path, get_dir) {
	EXPECT_STREQ("", Path::DirName("").c_str());
	EXPECT_STREQ("C:" DS "", Path::DirName("C:\\").c_str());
	EXPECT_STREQ("C:" DS "", Path::DirName("C:\\foo.txt").c_str());
	EXPECT_STREQ("C:" DS "", Path::DirName("C:\\bar").c_str());
	EXPECT_STREQ("C:" DS "", Path::DirName("C:\\bar\\").c_str());
	EXPECT_STREQ("C:" DS "bar" DS "", Path::DirName("C:\\bar\\foo.txt").c_str());
}

TEST(lagi_path, get_file_name) {
	EXPECT_STREQ("", Path::FileName("").c_str());
	EXPECT_STREQ("", Path::FileName("C:").c_str());
	EXPECT_STREQ("", Path::FileName("C:\\").c_str());
	EXPECT_STREQ("foo.txt", Path::FileName("C:\\foo.txt").c_str());
	EXPECT_STREQ("bar", Path::FileName("C:\\bar").c_str());
	EXPECT_STREQ("", Path::FileName("C:\\bar\\").c_str());
	EXPECT_STREQ("foo.txt", Path::FileName("C:\\bar\\foo.txt").c_str());
	EXPECT_STREQ("foo.txt", Path::FileName("C:/bar/foo.txt").c_str());
}

TEST(lagi_path, set_token) {
	Path p(0);

	// These are InternalError because the tokens are currently always hardcoded
	EXPECT_THROW(p.SetToken("no ?", "path"), agi::InternalError);
	EXPECT_THROW(p.SetToken("?bad", "path"), agi::InternalError);

	// Set to paths
	EXPECT_NO_THROW(p.SetToken("?video", "path/to/video"));
	EXPECT_NO_THROW(p.SetToken("?audio", "path/to/audio"));
	EXPECT_NO_THROW(p.SetToken("?script", "path/to/subs"));

	// Clear
	EXPECT_NO_THROW(p.SetToken("?video", ""));
	EXPECT_NO_THROW(p.SetToken("?audio", ""));
	EXPECT_NO_THROW(p.SetToken("?script", ""));
	EXPECT_NO_THROW(p.SetTokenFile("?video", ""));
	EXPECT_NO_THROW(p.SetTokenFile("?audio", ""));
	EXPECT_NO_THROW(p.SetTokenFile("?script", ""));
}

TEST(lagi_path, decode_path) {
	Path p(0);
	ASSERT_NO_THROW(p.SetTokenFile("?video", "C:/a/b/file.mkv"));
	ASSERT_NO_THROW(p.SetTokenFile("?audio", "C:/a/b/file.mka"));
	ASSERT_NO_THROW(p.SetTokenFile("?script", "C:/a/b/c/subs.ass"));

	EXPECT_STREQ("", p.Decode("").c_str());
	EXPECT_STREQ("C:" DS "foo.txt", p.Decode("C:/foo.txt").c_str());

	EXPECT_STREQ("C:" DS "a" DS "b" DS "foo.txt", p.Decode("?video/foo.txt").c_str());
	EXPECT_STREQ("C:" DS "a" DS "b" DS "foo.txt", p.Decode("?audio/foo.txt").c_str());
	EXPECT_STREQ("C:" DS "a" DS "b" DS "foo.txt", p.Decode("?audiofoo.txt").c_str());
	EXPECT_STREQ("C:" DS "a" DS "b" DS "c" DS "foo.txt", p.Decode("?script/foo.txt").c_str());

	EXPECT_STREQ("C:" DS "a" DS "b" DS "", p.Decode("?video").c_str());
	EXPECT_STREQ("C:" DS "a" DS "b" DS "", p.Decode("?audio").c_str());
	EXPECT_STREQ("C:" DS "a" DS "b" DS "c" DS "", p.Decode("?script").c_str());

	ASSERT_NO_THROW(p.SetTokenFile("?script", ""));
	EXPECT_STREQ("?script", p.Decode("?script").c_str());
}

TEST(lagi_path, encode_path) {
	Path p(0);
	ASSERT_NO_THROW(p.SetTokenFile("?video", "C:/a/b/file.mkv"));
	ASSERT_NO_THROW(p.SetTokenFile("?audio", "C:/a/b/c/file.mka"));
	ASSERT_NO_THROW(p.SetTokenFile("?script", ""));

	EXPECT_STREQ("", p.Encode("").c_str());

	EXPECT_STREQ("?video", p.Encode(p.Decode("?video")).c_str());
	EXPECT_STREQ("?audio", p.Encode(p.Decode("?audio")).c_str());
	EXPECT_STREQ("?script", p.Encode(p.Decode("?script")).c_str());

	EXPECT_STREQ("?video" DS "foo.txt", p.Encode(p.Decode("?video/foo.txt")).c_str());
	EXPECT_STREQ("?audio" DS "foo.txt", p.Encode(p.Decode("?audio/foo.txt")).c_str());
	EXPECT_STREQ("?script" DS "foo.txt", p.Encode(p.Decode("?script/foo.txt")).c_str());
}
