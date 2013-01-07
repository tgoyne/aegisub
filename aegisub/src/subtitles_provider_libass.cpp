// Copyright (c) 2006-2007, Rodrigo Braz Monteiro, Evgeniy Stepanov
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

/// @file subtitles_provider_libass.cpp
/// @brief libass-based subtitle renderer
/// @ingroup subtitle_rendering
///

#include "config.h"

#ifdef WITH_LIBASS

#include <wx/filefn.h>
#include <wx/utils.h>

#ifdef __APPLE__
#include <sys/param.h>
#include <libaegisub/util_osx.h>
#endif

#include <libaegisub/log.h>

#include "ass_file.h"
#include "dialog_progress.h"
#include "frame_main.h"
#include "main.h"
#include "standard_paths.h"
#include "subtitles_provider_libass.h"
#include "utils.h"
#include "video_context.h"
#include "video_frame.h"

static void msg_callback(int level, const char *fmt, va_list args, void *) {
	if (level >= 7) return;
	char buf[1024];
#ifdef _WIN32
	vsprintf_s(buf, sizeof(buf), fmt, args);
#else
	vsnprintf(buf, sizeof(buf), fmt, args);
#endif

	if (level < 2) // warning/error
		LOG_I("subtitle/provider/libass") << buf;
	else // verbose
		LOG_D("subtitle/provider/libass") << buf;
}

class FontConfigCacheThread : public wxThread {
	ASS_Library *ass_library;
	ASS_Renderer *ass_renderer;
	FontConfigCacheThread** thisPtr;
	ExitCode Entry() {
		const char *config_path = nullptr;
#ifdef __APPLE__
		std::string conf_path = agi::util::OSX_GetBundleResourcesDirectory() + "/etc/fonts/fonts.conf";
		config_path = conf_path.c_str();
#endif

		if (ass_library) ass_renderer = ass_renderer_init(ass_library);
		ass_set_fonts(ass_renderer, nullptr, "Sans", 1, config_path, true);
		if (ass_library) ass_renderer_done(ass_renderer);
		*thisPtr = nullptr;
		return EXIT_SUCCESS;
	}
public:
	FontConfigCacheThread(ASS_Library *ass_library, FontConfigCacheThread **thisPtr)
	: ass_library(ass_library)
	, ass_renderer(nullptr)
	, thisPtr(thisPtr)
	{
		*thisPtr = this;
		Create();
		Run();
	}
	FontConfigCacheThread(ASS_Renderer *ass_renderer, FontConfigCacheThread **thisPtr)
		: ass_library(nullptr)
		, ass_renderer(ass_renderer)
		, thisPtr(thisPtr)
	{
		*thisPtr = this;
		Create();
		Run();
	}
};

static void wait_for_cache_thread(FontConfigCacheThread const * const * const cache_worker) {
	if (!*cache_worker) return;

	DialogProgress progress(wxGetApp().frame, "Updating font index", "This may take several minutes");
	progress.Run([=](agi::ProgressSink *ps) {
		ps->SetIndeterminate();
		while (*cache_worker && !ps->IsCancelled())
			wxMilliSleep(100);
	});
}

LibassSubtitlesProvider::LibassSubtitlesProvider(std::string) {
	wait_for_cache_thread(&cache_worker);

	// Initialize renderer
	ass_track = nullptr;
	ass_renderer = ass_renderer_init(ass_library);
	if (!ass_renderer) throw "ass_renderer_init failed";
	ass_set_font_scale(ass_renderer, 1.);
	new FontConfigCacheThread(ass_renderer, &cache_worker);
	wait_for_cache_thread(&cache_worker);
}

LibassSubtitlesProvider::~LibassSubtitlesProvider() {
	if (ass_track) ass_free_track(ass_track);
	if (ass_renderer) ass_renderer_done(ass_renderer);
}

void LibassSubtitlesProvider::LoadSubtitles(AssFile *subs) {
	// Prepare subtitles
	std::vector<char> data;
	subs->SaveMemory(data);

	// Load file
	if (ass_track) ass_free_track(ass_track);
	ass_track = ass_read_memory(ass_library, &data[0], data.size(),(char *)"UTF-8");
	if (!ass_track) throw "libass failed to load subtitles.";
}

#define _r(c)  ((c)>>24)
#define _g(c)  (((c)>>16)&0xFF)
#define _b(c)  (((c)>>8)&0xFF)
#define _a(c)  ((c)&0xFF)

void LibassSubtitlesProvider::DrawSubtitles(AegiVideoFrame &frame,double time) {
	// Set size
	ass_set_frame_size(ass_renderer, frame.w, frame.h);

	// Get frame
	ASS_Image* img = ass_render_frame(ass_renderer, ass_track, int(time * 1000), nullptr);

	// libass actually returns several alpha-masked monochrome images.
	// Here, we loop through their linked list, get the colour of the current, and blend into the frame.
	// This is repeated for all of them.
	while (img) {
		// Get colours
		unsigned int opacity = 255 - ((unsigned int)_a(img->color));
		unsigned int r = (unsigned int)_r(img->color);
		unsigned int g = (unsigned int)_g(img->color);
		unsigned int b = (unsigned int) _b(img->color);

		// Prepare copy
		int src_stride = img->stride;
		int dst_stride = frame.pitch;
		const unsigned char *src = img->bitmap;

		unsigned char *dst = frame.data;
		if (frame.flipped) {
			dst += dst_stride * (frame.h - 1);
			dst_stride *= -1;
		}
		dst += (img->dst_y * dst_stride + img->dst_x * 4);

		// Copy image to destination frame
		for (int y = 0; y < img->h; y++) {
			unsigned char *dstp = dst;

			for (int x = 0; x < img->w; ++x) {
				unsigned int k = ((unsigned)src[x]) * opacity / 255;
				unsigned int ck = 255 - k;

				*dstp = (k * b + ck * *dstp) / 255;
				++dstp;

				*dstp = (k * g + ck * *dstp) / 255;
				++dstp;

				*dstp = (k * r + ck * *dstp) / 255;
				++dstp;

				++dstp;
			}

			dst += dst_stride;
			src += src_stride;
		}

		// Next image
		img = img->next;
	}
}

void LibassSubtitlesProvider::CacheFonts() {
	ass_library = ass_library_init();
	ass_set_message_cb(ass_library, msg_callback, nullptr);
	new FontConfigCacheThread(ass_library, &cache_worker);
}

ASS_Library* LibassSubtitlesProvider::ass_library;
FontConfigCacheThread* LibassSubtitlesProvider::cache_worker = nullptr;

#endif // WITH_LIBASS
