// Copyright (c) 2022, Thomas Goyne <plorkyeran@aegisub.org>
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

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

template<typename T>
void check(T& stream, std::filesystem::path const& name) {
	if (!stream.good()) {
		std::cout << "Failed to open " << name << "\n";
		exit(1);
	}
}

int main(int argc, const char *argv[]) {
	if (argc != 3) {
		std::cout << "Usage: respack <manifest> <output base name>\n";
		return 1;
	}

	std::filesystem::path manifest_path(argv[1]);
	std::filesystem::path out_path(argv[2]);
	manifest_path = std::filesystem::absolute(manifest_path);
	out_path = std::filesystem::absolute(out_path);

	std::cout << "Generating " << argv[2] << ".{h,cpp} from " << argv[1] << "\n";

	std::ifstream manifest(argv[1]);
	std::ofstream cpp(out_path.string() + ".cpp"), h(out_path.string() + ".h");
	check(manifest, manifest_path);
	check(cpp, out_path);
	check(h, out_path);

	auto parent = manifest_path.parent_path();

	cpp << "#include \"libresrc.h\"\n";

	std::string file;
	while (std::getline(manifest, file)) {
		if (file.empty()) continue;
		std::cout << "packing " << file << "\n";

		auto full_path = parent / file;
		std::ifstream ifs(full_path, std::ios_base::binary);
		check(ifs, full_path);

		auto name = std::filesystem::path(file).filename().replace_extension().string();

		cpp << "const unsigned char " << name << "[] = {";

		size_t length = 0;
		for (std::istreambuf_iterator<char> it(ifs), end; it != end; ++it) {
			if (length > 0) cpp << ",";
			cpp << (unsigned int)(unsigned char)*it;
			++length;
		}

		cpp << "};\n";

		h << "extern const unsigned char " << name << "[" << length << "];\n";
	}

	return 0;
}
