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
// AEGISUB
//
// Website: http://aegisub.cellosoft.com
// Contact: mailto:zeratul@cellosoft.com
//


#pragma once


///////////
// Headers
#include "subtitle_format_dvd.h"
#include "video_provider_dummy.h"
#include "subtitles_provider.h"
#include "ass_dialogue.h"
#include "ass_file.h"
#ifdef _OPENMP
#include <omp.h>
#endif


///////////////
// Format name
wxString DVDSubtitleFormat::GetName() {
	return _T("DVD Subpictures");
}


//////////////
// Extensions
wxArrayString DVDSubtitleFormat::GetWriteWildcards() {
	wxArrayString results;
	results.Add(_T("sup"));
	return results;
}


/////////////
// Can write
bool DVDSubtitleFormat::CanWriteFile(wxString filename) {
	return (filename.Lower().EndsWith(_T(".sup")));
}


///////////////////////
// Get subpicture list
void DVDSubtitleFormat::GetSubPictureList(std::vector<SubPicture> &pics) {
	// Create video frame
	int w = 720;
	int h = 480;
	VideoProvider *video = new DummyVideoProvider(10,1,w,h,wxColour(255,0,0),false);
	AegiVideoFrame srcFrame = video->GetFrame(0);
	delete video;

	// Prepare subtitles
	CreateCopy();
	SortLines();
	//Merge(true,true,true);

	// Count and index lines
	using std::list;
	int count = 0;
	std::vector<AssDialogue*> diags;
	for (list<AssEntry*>::iterator cur=Line->begin();cur!=Line->end();cur++) {
		AssDialogue *current = AssEntry::GetAsDialogue(*cur);
		if (current) {
			diags.push_back(current);
			count++;
		}
	}
	pics.resize(count);

	SubtitlesProvider *provider = NULL;
	provider = SubtitlesProviderFactory::GetProvider();
	provider->LoadSubtitles(GetAssFile());

	// Write lines
	int i;
	#pragma omp parallel for shared(diags,pics,provider) private(i)
	for (i=0;i<count;i++) {
		// Dialogue
		AssDialogue *current = diags[i];

		// Get the image
		AegiVideoFrame dst;
		dst.CopyFrom(srcFrame);
		#pragma omp critical
		{
			provider->DrawSubtitles(dst,current->Start.GetMS()/1000.0);
		}
		wxImage img = dst.GetImage();
		dst.Clear();

		// Perform colour reduction on image
		unsigned char r,g,b;
		unsigned char *data = img.GetData();
		const unsigned char *dataRead = data;
		unsigned char *dataWrite = data;
		int startY = 0;
		int endY = 0;
		int startX = w;
		int endX = 0;

		// For each line
		for (int y=h;--y>=0;) {
			bool hasData = false;
			int lineStartX = 0;
			int lineEndX;

			// Scan line
			for (int x=w;--x>=0;) {
				// Read
				r = *(dataRead++);
				g = *(dataRead++);
				b = *(dataRead++);

				// Background
				if (r > 127 && g < 20) {
					r = 255;
					g = 0;
					b = 0;
				}

				// Text
				else {
					// Mark coordinates
					hasData = true;
					if (lineStartX == 0) lineStartX = w-x-1;
					lineEndX = w-x-1;

					// Set colour
					if (r > 170 && g > 170) {
						r = 255;
						g = 255;
						b = 255;
					}
					else if (r > 85 && g > 85) {
						r = 127;
						g = 127;
						b = 127;
					}
					else {
						r = 0;
						g = 0;
						b = 0;
					}
				}

				// Write
				*(dataWrite++) = r;
				*(dataWrite++) = g;
				*(dataWrite++) = b;
			}

			// Mark as last found so far
			if (hasData) {
				if (startY == 0) startY = h-y-1;
				endY = h-y-1;
				if (lineStartX < startX) startX = lineStartX;
				if (lineEndX > endX) endX = lineEndX;
			}
		}
		
		// Get subpicture
		wxImage subPic = img.GetSubImage(wxRect(startX,startY,endX-startX+1,endY-startY+1));

		// Save image
		if (startX > endX) endX = startX;
		if (startY > endY) endY = startY;
		pics[i].img = subPic;
		pics[i].x = startX;
		pics[i].y = startY;
	}

	// Clean up
	delete provider;
	srcFrame.Clear();
}


///////////////////////
// Actually write them
void DVDSubtitleFormat::WriteFile(wxString filename,wxString encoding) {
	// Get subpictures
	std::vector<SubPicture> pics;
	GetSubPictureList(pics);

	// Dump as PNG
	//for (size_t i=0;i<pics.size();i++) pics[i].img.SaveFile(filename + wxString::Format(_T("_%03i.png"),i));
}
