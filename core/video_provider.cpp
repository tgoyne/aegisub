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
#include "video_provider_avs.h"
#include "video_provider_lavc.h"
#include "options.h"


////////////////
// Get provider
VideoProvider *VideoProvider::GetProvider(wxString video,wxString subtitles) {
	// Check if avisynth is available
	bool avisynthAvailable = false;
	#ifdef __WINDOWS__
	try {
		// If avisynth.dll cannot be loaded, an exception will be thrown and avisynthAvailable will never be set to true
		AviSynthWrapper avs;
		avisynthAvailable = true;
	}
	catch (...) {}
	#endif

	// Initialize to null
	VideoProvider *provider = NULL;

	// See if it's OK to use LAVC
	#ifdef USE_LAVC
	if (Options.AsBool(_T("Use ffmpeg"))) {
		try {
			provider = new LAVCVideoProvider(video,subtitles,1.0);
		}
		catch (...) {
			if (avisynthAvailable) {
				wxMessageBox(_T("Failed loading FFmpeg decoder for video, falling back to Avisynth."),_T("FFmpeg error."));
				provider = NULL;
			}

			// Out of options, rethrow
			else throw;
		}
	}
	#endif

	// Use avisynth provider
	#ifdef __WINDOWS__
	bool usedDirectshow = false;
	if (!provider) provider = new AvisynthVideoProvider(video,subtitles,1.0,usedDirectshow);
	#endif

	// Return provider
	return provider;
}
