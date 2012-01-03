// Copyright (c) 2012, Thomas Goyne <plorkyeran@aegisub.org>
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

#include <libaegisub/exception.h>
#include <libaegisub/path.h>

#include "main.h"
#include "util.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

using agi::Path;

class lagi_path : public libagi {
protected:
	wchar_t old_working_dir[MAX_PATH+1];

	// Set a constant working directory that's very likely to exist so that
	// the tests for relative paths can have hardcoded results
	void SetUp() {
		GetCurrentDirectoryW(MAX_PATH, old_working_dir);
		BOOL dicks = SetCurrentDirectoryW(L"C:\\Program Files\\");
	}

	void TearDown() {
		SetCurrentDirectoryW(old_working_dir);
	}
};

TEST_F(lagi_path, normalize) {
	EXPECT_STREQ("", Path::Normalize("").c_str());
	EXPECT_STREQ("C:\\Program Files\\a", Path::Normalize("a").c_str());
	EXPECT_STREQ("C:\\", Path::Normalize("/").c_str());
	EXPECT_STREQ("C:\\Program Files\\", Path::Normalize("C:").c_str());
	EXPECT_STREQ("C:\\", Path::Normalize("C:\\").c_str());
	EXPECT_STREQ("C:\\foo.txt", Path::Normalize("C:/foo.txt").c_str());
	EXPECT_STREQ("C:\\foo.txt", Path::Normalize("C:/.\\foo.txt").c_str());
	EXPECT_STREQ("C:\\foo.txt", Path::Normalize("C:/bar/../foo.txt").c_str());
	EXPECT_STREQ("C:\\foo", Path::Normalize("C:/foo").c_str());
	EXPECT_STREQ("C:\\foo\\", Path::Normalize("C:/foo/").c_str());
	EXPECT_STREQ("C:\\foo\\", Path::Normalize("C:/foo/.").c_str());
	EXPECT_STREQ("C:\\foo\\", Path::Normalize("C:/foo/./").c_str());
	EXPECT_STREQ("C:\\", Path::Normalize("C:/foo/..").c_str());
	EXPECT_STREQ("C:\\", Path::Normalize("C:/foo/../").c_str());
	EXPECT_STREQ("C:\\foo\\", Path::Normalize("C:/foo/bar/..").c_str());
	EXPECT_STREQ("C:\\bar\\", Path::Normalize("C:/foo/../bar/").c_str());
	EXPECT_STREQ("C:\\foo\\bar\\", Path::Normalize("C:/foo/./bar/").c_str());
	EXPECT_STREQ("C:\\foo\\", Path::Normalize("C:\\foo\\").c_str());
	EXPECT_STREQ("C:\\a\\b\\c\\d\\e", Path::Normalize("C:///a/b//c/d\\e").c_str());
	EXPECT_STREQ("C:\\Program Files\\foo.txt", Path::Normalize("foo.txt").c_str());
}

TEST_F(lagi_path, combine) {
	EXPECT_STREQ("", Path::Combine("", "").c_str());
	EXPECT_STREQ("C:\\foo.txt", Path::Combine("C:\\", "foo.txt").c_str());
	EXPECT_STREQ("C:\\bar\\foo.txt", Path::Combine("C:\\bar", "foo.txt").c_str());
	EXPECT_STREQ("C:\\bar\\foo.txt", Path::Combine("C:\\bar\\", "foo.txt").c_str());
	EXPECT_STREQ("C:\\baz\\foo.txt", Path::Combine("C:\\bar\\", "C:\\baz\\foo.txt").c_str());
}

TEST_F(lagi_path, is_absolute) {
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

TEST_F(lagi_path, get_dir) {
	EXPECT_STREQ("", Path::GetDir("").c_str());
	EXPECT_STREQ("C:\\", Path::GetDir("C:\\").c_str());
	EXPECT_STREQ("C:\\", Path::GetDir("C:\\foo.txt").c_str());
	EXPECT_STREQ("C:\\", Path::GetDir("C:\\bar").c_str());
	EXPECT_STREQ("C:\\", Path::GetDir("C:\\bar\\").c_str());
	EXPECT_STREQ("C:\\bar\\", Path::GetDir("C:\\bar\\foo.txt").c_str());
}

TEST_F(lagi_path, get_file_name) {
	EXPECT_STREQ("", Path::GetFileName("").c_str());
	EXPECT_STREQ("", Path::GetFileName("C:").c_str());
	EXPECT_STREQ("", Path::GetFileName("C:\\").c_str());
	EXPECT_STREQ("foo.txt", Path::GetFileName("C:\\foo.txt").c_str());
	EXPECT_STREQ("bar", Path::GetFileName("C:\\bar").c_str());
	EXPECT_STREQ("", Path::GetFileName("C:\\bar\\").c_str());
	EXPECT_STREQ("foo.txt", Path::GetFileName("C:\\bar\\foo.txt").c_str());
	EXPECT_STREQ("foo.txt", Path::GetFileName("C:/bar/foo.txt").c_str());
}

TEST_F(lagi_path, set_token) {
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

TEST_F(lagi_path, decode_path) {
	Path p(0);
	ASSERT_NO_THROW(p.SetTokenFile("?video", "C:/a/b/file.mkv"));
	ASSERT_NO_THROW(p.SetTokenFile("?audio", "C:/a/b/file.mka"));
	ASSERT_NO_THROW(p.SetTokenFile("?script", "C:/a/b/c/subs.ass"));

	EXPECT_STREQ("", p.Decode("").c_str());
	EXPECT_STREQ("C:\\foo.txt", p.Decode("C:/foo.txt").c_str());

	EXPECT_STREQ("C:\\a\\b\\foo.txt", p.Decode("?video/foo.txt").c_str());
	EXPECT_STREQ("C:\\a\\b\\foo.txt", p.Decode("?audio/foo.txt").c_str());
	EXPECT_STREQ("C:\\a\\b\\foo.txt", p.Decode("?audiofoo.txt").c_str());
	EXPECT_STREQ("C:\\a\\b\\c\\foo.txt", p.Decode("?script/foo.txt").c_str());

	EXPECT_STREQ("C:\\a\\b\\", p.Decode("?video").c_str());
	EXPECT_STREQ("C:\\a\\b\\", p.Decode("?audio").c_str());
	EXPECT_STREQ("C:\\a\\b\\c\\", p.Decode("?script").c_str());

	ASSERT_NO_THROW(p.SetTokenFile("?script", ""));
	EXPECT_STREQ("?script", p.Decode("?script").c_str());
}

TEST_F(lagi_path, encode_path) {
	Path p(0);
	ASSERT_NO_THROW(p.SetTokenFile("?video", "C:/a/b/file.mkv"));
	ASSERT_NO_THROW(p.SetTokenFile("?audio", "C:/a/b/c/file.mka"));
	ASSERT_NO_THROW(p.SetTokenFile("?script", ""));

	EXPECT_STREQ("", p.Encode("").c_str());

	EXPECT_STREQ("?video", p.Encode(p.Decode("?video")).c_str());
	EXPECT_STREQ("?audio", p.Encode(p.Decode("?audio")).c_str());
	EXPECT_STREQ("?script", p.Encode(p.Decode("?script")).c_str());

	EXPECT_STREQ("?video\\foo.txt", p.Encode(p.Decode("?video/foo.txt")).c_str());
	EXPECT_STREQ("?audio\\foo.txt", p.Encode(p.Decode("?audio/foo.txt")).c_str());
	EXPECT_STREQ("?script\\foo.txt", p.Encode(p.Decode("?script/foo.txt")).c_str());
}
