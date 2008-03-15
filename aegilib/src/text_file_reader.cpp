// Copyright (c) 2005, Rodrigo Braz Monteiro
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
#include <algorithm>
#include <string>
#include <wx/wfstream.h>
#include "text_file_reader.h"
using namespace Gorgonsub;

#ifdef WITH_UNIVCHARDET
#include "charset_detect.h"
#endif


///////////////
// Constructor
TextFileReader::TextFileReader(wxInputStream &stream,Gorgonsub::String enc,bool _trim,bool prefetch)
: file(stream)
{
	// Setup
	trim = _trim;
	threaded = prefetch && false;
	thread = NULL;

	// Set encoding
	encoding = enc.c_str();
	if (encoding == _T("binary")) return;
	SetEncodingConfiguration();
}


//////////////
// Destructor
TextFileReader::~TextFileReader()
{
	wxCriticalSectionLocker locker(mutex);
	if (thread) thread->Delete();
}


//////////////////////////////
// Set encoding configuration
void TextFileReader::SetEncodingConfiguration()
{
	// Set encoding configuration
	swap = false;
	Is16 = false;
	//conv = shared_ptr<wxMBConv>();
	if (encoding == _T("UTF-8")) {
		conv = shared_ptr<wxMBConv> (new wxMBConvUTF8);
	}
	else if (encoding == _T("UTF-16LE")) {
		Is16 = true;
	}
	else if (encoding == _T("UTF-16BE")) {
		Is16 = true;
		swap = true;
	}
	else if (encoding == _T("UTF-7")) {
		conv = shared_ptr<wxMBConv>(new wxCSConv(encoding));
	}
	else if (encoding == _T("Local")) {
		conv = shared_ptr<wxMBConv> (wxConvCurrent,NullDeleter());
	}
	else {
		conv = shared_ptr<wxMBConv> (new wxCSConv(encoding));
	}

	// Allocate buffer
	if (!Is16) buffer1.Alloc(4096);
	else buffer2.Alloc(4096);
}


////////////////////
// Helper functions
wxString GetString(char *read,shared_ptr<wxMBConv> conv) { return wxString(read,*conv); }
wxString GetString(wchar_t *read,shared_ptr<wxMBConv> conv) { (void)conv; return wxString(read); }
inline void Swap(wchar_t &a) {
	char *c = (char*) &a;
	char aux = c[0];
	c[0] = c[1];
	c[1] = aux;
}
inline void Swap(char &a) { (void) a; }


////////////////
// Parse a line
template <typename T>
void ParseLine(FastBuffer<T> &_buffer,wxInputStream &file,wxString &stringBuffer,shared_ptr<wxMBConv> conv,bool swap)
{
	// Look for a new line
	int newLinePos = -1;
	T newLineChar = 0;
	size_t size = _buffer.GetSize();

	// Find first line break
	if (size) _buffer.FindLineBreak(0,size,newLinePos,newLineChar);

	// If no line breaks were found, load more data into file
	while (newLinePos == -1) {
		// Read 2048 bytes
		const size_t readBytes = 2048;
		const size_t read = readBytes/sizeof(T);
		size_t oldSize = _buffer.GetSize();
		T *ptr = _buffer.GetWritePtr(read);
		file.Read(ptr,readBytes);
		size_t lastRead = file.LastRead()/sizeof(T);
		_buffer.AssumeSize(_buffer.GetSize()+lastRead-read);

		// Swap
		if (swap) {
			T* ptr2 = ptr;
			for (size_t i=0;i<lastRead;i++) {
				Swap(*ptr2++);
			}
		}

		// Find line break
		_buffer.FindLineBreak(oldSize,lastRead+oldSize,newLinePos,newLineChar);

		// End of file, force a line break
		if (file.Eof() && newLinePos == -1) newLinePos = (int) _buffer.GetSize();
	}

	// Found newline
	if (newLinePos != -1) {
		T *read = _buffer.GetMutableReadPtr();
		// Replace newline with null character and convert to proper charset
		if (newLinePos) {
			read[newLinePos] = 0;
			stringBuffer = GetString(read,conv);
		}

		// Remove an extra character if the new is the complement of \n,\r (13^7=10, 10^7=13)
		if (read[newLinePos+1] == (newLineChar ^ 7)) newLinePos++;
		_buffer.ShiftLeft(newLinePos+1);
	}
}


//////////////////////////
// Reads a line from file
Gorgonsub::String TextFileReader::ActuallyReadLine()
{
	wxString stringBuffer;
	size_t bufAlloc = 1024;
	stringBuffer.Alloc(bufAlloc);
	std::string buffer = "";

	// Read UTF-16 line from file
	if (Is16) ParseLine<wchar_t>(buffer2,file,stringBuffer,conv,swap);

	// Read ASCII/UTF-8 line from file
	else ParseLine<char>(buffer1,file,stringBuffer,conv,false);

	// Remove BOM
	size_t startPos = 0;
	if (stringBuffer.Length() > 0 && stringBuffer[0] == 0xFEFF) startPos = 1;

	// Trim
	if (trim) return String(StringTrim(stringBuffer,startPos));
	if (startPos) return String(stringBuffer.c_str() + startPos);
	return stringBuffer;
}


//////////////////////////////////
// Checks if there's more to read
bool TextFileReader::HasMoreLines()
{
	if (cache.size()) return true;
	wxCriticalSectionLocker locker(mutex);
	return (!file.Eof() || buffer1.GetSize() || buffer2.GetSize());
}


////////////////////////////////
// Ensure that charset is valid
void TextFileReader::EnsureValid(Gorgonsub::String enc)
{
	if (enc == _T("unknown") || enc == _T("UTF-32BE") || enc == _T("UTF-32LE")) {
		wxString error = _T("Character set ");
		error += enc;
		error += _T(" is not supported.");
		throw error.c_str();
	}
}


///////////////////////////
// Get encoding being used
String TextFileReader::GetCurrentEncoding()
{
	return encoding.c_str();
}


///////////////////////////
// Reads a line from cache
String TextFileReader::ReadLineFromFile()
{
	// Not threaded, just return it
	if (!threaded) return ActuallyReadLine();

	// Load into cache if needed
	String final;
	{
		wxCriticalSectionLocker locker(mutex);
		if (cache.size() == 0) {
			cache.push_back(ActuallyReadLine());
		}
	}

	{
		// Retrieve from cache
		wxCriticalSectionLocker locker(mutex);
		if (cache.size()) {
			final = cache.front();
			cache.pop_front();
		}

		// Start the thread to prefetch more
		if (cache.size() < 3 && thread == NULL) {
			thread = new PrefetchThread(this);
			thread->Create();
			thread->Run();
		}
	}

	return final;
}


////////////////
// Thread entry
wxThread::ExitCode PrefetchThread::Entry()
{
	// Lock
	bool run = true;
	while (run) {
		if (TestDestroy()) {
			parent->thread = NULL;
			return 0;
		}
		{
			wxCriticalSectionLocker locker(parent->mutex);
			if (parent->cache.size() < 6) {
				if (!parent->file.Eof()) {
					// Get line
					parent->cache.push_back(parent->ActuallyReadLine());
				}
				else run = false;
			}
		}
		Sleep(50);
	}

	// Die
	wxCriticalSectionLocker locker(parent->mutex);
	parent->thread = NULL;
	return 0;
}
