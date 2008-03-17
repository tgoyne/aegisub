// Copyright (c) 2008, Rodrigo Braz Monteiro
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of the Aegisub Group nor the names of its contributors
//     may be used to endorse or promote products derived from this software
//     without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// -----------------------------------------------------------------------------
//
// AEGISUB/GORGONSUB
//
// Website: http://www.aegisub.net
// Contact: mailto:amz@aegisub.net
//

#pragma once
#include <vector>

namespace Gorgonsub {

	// Range class
	class Range {
	private:
		size_t start,end;

	public:
		Range() : start(0), end(0) {}
		Range(size_t _start,size_t _end) : start(_start), end(_end) {}

		size_t GetLine(size_t n) const;
		size_t GetSize() const { return end-start; }
		size_t GetStart() const { return start; }
		size_t GetEnd() const { return end; }
	};

	// Selection class
	class Selection {
	private:
		std::vector<Range> ranges;
		size_t count;

	public:
		Selection();

		void AddLine(size_t line) { AddRange(Range(line,line+1)); }
		void AddRange(const Range &range);
		void RemoveLine(size_t line) { RemoveRange(Range(line,line+1)); }
		void RemoveRange(const Range &range);
		void AddSelection (const Selection &param);
		void RemoveSelection (const Selection &param);

		size_t GetCount() const { return count; }
		size_t GetRanges() const { return ranges.size(); }
		size_t GetLine(size_t n) const;
		size_t GetLineInRange(size_t n,size_t range) const { return ranges.at(range).GetLine(n); }
		bool IsContiguous() const { return GetRanges() <= 1; }

	};

}
