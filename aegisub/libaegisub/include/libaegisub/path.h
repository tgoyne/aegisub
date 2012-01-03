// Copyright (c) 2010-2011, Amar Takhar <verm@aegisub.org>
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

/// @file path.h
/// @brief Common paths.
/// @ingroup libaegisub

#ifndef LAGI_PRE
#include <map>
#include <string>
#endif

#include <libaegisub/exception.h>

namespace agi {
	class Options;

/// Class for handling everything path-related in Aegisub
class Path {
	typedef std::map<std::string, std::string> str_map;

	/// The directory separator character for the platform
	static char dir_sep;
	/// The directory separator character for other platforms
	static char non_dir_sep;

	/// Token -> Path map
	str_map tokens;

	/// Path -> Token map
	str_map paths;

	/// Options file for stored paths; may be NULL
	Options *opt;

	/// Platform-specific code to fill in the default paths, called in the constructor
	void FillPlatformSpecificPaths();

public:
	/// Constructor
	/// @param opt Options object to use for GetPath/SetPath; may be NULL
	Path(Options *opt);

	/// Decode and normalize a path which may begin with a registered token
	/// @param path Path which is either already absolute or begins with a token
	/// @return Absolute path
	std::string Decode(std::string const& path) const;

	/// Encode an absolute path to begin with a token if there are any applicable
	/// @param path Absolute path to encode
	/// @return path untouched, or with some portion of the beginning replaced with a token
	std::string Encode(std::string path) const;

	/// Get a fully-decoded and normalized path from the configuration
	/// @param path_opt_name Name of the option to get
	/// @return Absolute path from the option
	std::string GetPath(std::string const& path_opt_name) const;

	/// Store a path in the options, making it relative to a token if possible
	/// @param path_opt_name Name of the option to set
	/// @param value New value of the option
	void SetPath(std::string const& path_opt_name, std::string const& value);

	/// Set a prefix token to use for encoding and decoding paths
	/// @param token_name A single word token beginning with '?'
	/// @param token_value An absolute path to a directory
	void SetToken(std::string const& token_name, std::string const& token_value);

	/// Set a prefix token to use for encoding and decoding paths
	/// @param token_name A single word token beginning with '?'
	/// @param token_value An absolute path to a file
	void SetTokenFile(std::string const& token_name, std::string const& token_value);

	/// Normalize a path
	/// @param path Path to normalize
	/// @return The absolute normalized path
	///
	/// Convert a path to an absolute path with slashes in the correct
	/// direction for the current platform
	static std::string Normalize(std::string const& path);

	/// Combine and normalize two path chunks, handling trailing/leading slashes
	/// @param prefix First part of path, with trailing slash optional
	/// @param suffix Second part of path. If this path is absolute, prefix is ignored
	/// @return Combined path
	static std::string Combine(std::string const& prefix, std::string const& suffix);

	/// Get the normalized full path to the directory of the path
	/// @param path Path to get the containing directory of
	/// @return Path to the directory containing the passed path, with trailing slash
	///
	/// Note that as this returns the containing directory, passing the result
	/// of it to GetDir again will result in truncating one level of directories.
	static std::string GetDir(std::string const& path);

	/// Get the filename from a path
	static std::string GetFileName(std::string const& path);

	/// Is a path absolute?
	static bool IsAbsolute(std::string const& path);
};

}
