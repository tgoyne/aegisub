// Copyright (c) 2006, Rodrigo Braz Monteiro
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
// AEGISUB
//
// Website: http://aegisub.cellosoft.com
// Contact: mailto:zeratul@cellosoft.com
//


///////////
// Headers
#include "audio_provider_stream.h"
#include "utils.h"


///////////////
// Constructor
StreamAudioProvider::StreamAudioProvider() {
	bufLen = 8192;
	startPos = 0;
	endPos = 8192;
}


//////////////
// Destructor
StreamAudioProvider::~StreamAudioProvider() {
	for (std::list<BufferChunk*>::iterator cur=buffer.begin();cur!=buffer.end();cur++) {
		delete *cur;
	}
	buffer.clear();
}


/////////////
// Get audio
void StreamAudioProvider::GetAudio(void *buf, __int64 start, __int64 count) {
	// Write
	__int64 left = count;
	__int64 written = 0;
	int toWrite;
	while (left > 0) {
		// Discard done
		if (startPos == 8192) {
			buffer.pop_front();
			startPos = 0;
		}

		// Is last?
		bool isLast = buffer.size() == 1;
		int size = 8192;
		if (isLast) size = endPos;

		// Write
		toWrite = MIN(int(size-startPos),int(left));
		memcpy(((short*)buf)+written,&(buffer.front()->buf[startPos]),toWrite);
		startPos += toWrite;
		written += toWrite;
		left -= toWrite;

		// Last
		if (isLast) break;
	}

	// Still left, fill with zero
	if (left > 0) {
		short *dst = (short*) buf;
		for (__int64 i=written;i<count;i++) {
			dst[i] = 0;
		}
	}
}


//////////////////////////
// Append audio to stream
void StreamAudioProvider::Append(void *src, __int64 count) {
	// Read
	__int64 left = count;
	__int64 read = 0;
	int toRead;
	while (left > 0) {
		// Check space
		if (endPos == 8192) {
			buffer.push_back(new BufferChunk);
			endPos = 0;
		}

		// Read
		toRead = MIN(int(8192-endPos),int(left));
		memcpy(&(buffer.back()->buf[endPos]),((short*)src)+read,toRead);
		endPos += toRead;
		read += toRead;
		left -= toRead;
	}
}


////////////////////////////
// Buffer chunk constructor
StreamAudioProvider::BufferChunk::BufferChunk() {
	buf.resize(4096);
	isFree = true;
}
