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

#include <cstdint>
#include <iterator>
#include <memory>
#include <string>

namespace agi {
	namespace fs {
		bool Exists(std::string const& path);
		bool FileExists(std::string const& path);
		bool DirectoryExists(std::string const& path);

		/// Get the local-charset encoded shortname for a file
		///
		/// This is purely for compatibility with external libraries which do
		/// not support unicode filenames on Windows. On all other platforms,
		/// it is a no-op.
		std::string ShortName(std::string const& path);

		/// Check for amount of free space on a path
		uint64_t FreeSpace(std::string const& path);

		/// Get the size in bytes of the file at path
		///
		/// @throws agi::FileNotFound if path does not exist
		/// @throws agi::acs::NotAFile if path is a directory
		/// @throws agi::acs::Read if path exists but could not be read
		uint64_t Size(std::string const& path);

		/// Get the modification time of the file at path
		///
		/// @throws agi::FileNotFound if path does not exist
		/// @throws agi::acs::NotAFile if path is a directory
		/// @throws agi::acs::Read if path exists but could not be read
		uint64_t ModifiedTime(std::string const& path);

		/// Create a directory and all required intermediate directories
		/// @throws agi::acs::Write if the directory could not be created.
		///
		/// Trying to create a directory which already exists is not an error.
		void CreateDirectory(std::string const& path);

		/// Touch the given path
		///
		/// Creates the file if it does not exist, or updates the modified
		/// time if it does
		void Touch(std::string const& path);

		/// Rename a file or directory
		/// @param from Source path
		/// @param to   Destination path
		void Rename(std::string const& from, std::string const& to);

		void Copy(std::string const& from, std::string const& to);

		/// Delete a file or directory
		/// @param path Path to file or directory to delete
		/// @throws agi::FileNotAccessibleError if file exists but could not be deleted
		///
		/// If path is a non-empty directory, the contents are recursively deleted.
		void Remove(std::string const& path);

		class DirectoryIterator {
			struct PrivData;
			std::shared_ptr<PrivData> privdata;
			std::string value;
		public:
			typedef std::string value_type;
			typedef std::string* pointer;
			typedef std::string& reference;
			typedef size_t difference_type;
			typedef std::forward_iterator_tag iterator_category;

			bool operator==(DirectoryIterator const&) const;
			bool operator!=(DirectoryIterator const& rhs) const { return !(*this == rhs); }
			DirectoryIterator& operator++();
			std::string const& operator*() const { return value; }

			DirectoryIterator(std::string const& path, std::string const& filter);
			DirectoryIterator();
			~DirectoryIterator();
		};

		class FilesInDirectory {
			DirectoryIterator b, e;
		public:
			DirectoryIterator& begin() { return b; }
			DirectoryIterator& end() { return e; }

			FilesInDirectory(std::string const& path, std::string const& filter) : b(path, filter) { }

			template<typename T>
			void GetAll(T& cont) {
				copy(begin(), end(), std::back_inserter(cont));
			}
		};
	}
}
