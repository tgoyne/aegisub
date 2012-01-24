// Copyright (c) 2007, Rodrigo Braz Monteiro
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
// Aegisub Project http://www.aegisub.org/
//
// $Id$

/// @file subtitles_provider.h
/// @brief Declaration of base-class for subtitle renderers
/// @ingroup main_headers subtitle_rendering
///

#pragma once

#include "factory_manager.h"

#include <libaegisub/exception.h>

class AssFile;
class AegiVideoFrame;

namespace agi {
	DEFINE_BASE_EXCEPTION(SubtitleProviderError, NonFatalException)

	/// There are no subtitle providers available
	DEFINE_SIMPLE_EXCEPTION(NoSubtitleProvidersError, SubtitleProviderError, "subtitle_provide/none")

	/// Initializing the subtitle provider failed
	DEFINE_SIMPLE_EXCEPTION(SubtitleProviderInitError, SubtitleProviderError, "subtitle_provider/init")

	/// Sending subtitles to the provider failed. The inner exception may have more details.
	DEFINE_SIMPLE_EXCEPTION(SubtitleProviderLoadError, SubtitleProviderError, "subtitle_provider/load")

	/// Rendering the subtitles on the video frame failed
	DEFINE_SIMPLE_EXCEPTION(SubtitleProviderDrawError, SubtitleProviderError, "subtitle_provider/draw")
}

/// @class SubtitlesProvider
/// @brief DOCME
///
/// DOCME
class SubtitlesProvider {
public:
	/// Virtual destructor for polymorphic base class
	virtual ~SubtitlesProvider() { };

	/// Load subtitles to be used in DrawSubtitles
	/// @param subs File to load
	/// @throw SubtitleProviderLoadError if an error occurs
	virtual void LoadSubtitles(AssFile *subs)=0;

	/// Draw subtitles on a frame
	/// @param dst Frame to draw on
	/// @param time Time in milliseconds
	/// @throw SubtitleProviderDrawError if an error occurs
	///
	/// If no subtitles have been loaded this should do nothing and return
	/// without errors
	virtual void DrawSubtitles(AegiVideoFrame &dst, double time)=0;
};

/// DOCME
/// @class SubtitlesProviderFactoryManager
/// @brief DOCME
///
/// DOCME
class SubtitlesProviderFactory : public Factory1<SubtitlesProvider, std::string> {
public:
	/// Get a subtitle provider
	/// @throw agi::NoSubtitleProvidersError if none are available
	static SubtitlesProvider *GetProvider();

	/// Initialize all subtitle providers
	static void RegisterProviders();
};
