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


///////////
// Headers
#include "config.h"

#include "video_provider_cache.h"


///////////////
// Constructor
VideoProviderCache::VideoProviderCache(VideoProvider *parent) {
	master = parent;
	cacheMax = 0;
	SetCacheMax(parent->GetDesiredCacheSize());
}


//////////////
// Destructor
VideoProviderCache::~VideoProviderCache() {
	delete master;
	ClearCache();
}


/////////////
// Get frame
const AegiVideoFrame VideoProviderCache::GetFrame(int n) {
	// See if frame is cached
	CachedFrame cached;
	for (std::list<CachedFrame>::iterator cur=cache.begin();cur!=cache.end();cur++) {
		cached = *cur;
		if (cached.n == n) {
			cache.erase(cur);
			cache.push_back(cached);
			return cached.frame;
		}
	}

	// Not cached, retrieve it
	const AegiVideoFrame frame = master->GetFrame(n);
	const AegiVideoFrame *srcFrame = &frame;

	// Cache frame
	pos = n;
	Cache(n,*srcFrame);
	return *srcFrame;
}


////////////////
// Get as float
void VideoProviderCache::GetFloatFrame(float* buffer, int n) {
	const AegiVideoFrame frame = GetFrame(n);
	frame.GetFloat(buffer);
}


//////////////////////////
// Set maximum cache size
void VideoProviderCache::SetCacheMax(int n) {
	if (n < 0) n = 0;
	cacheMax = n;
}


////////////////
// Add to cache
void VideoProviderCache::Cache(int n,const AegiVideoFrame frame) {
	// Cache enabled?
	if (cacheMax == 0) return;

	// Cache full, use frame at front
	if (cache.size() >= cacheMax) {
		cache.push_back(cache.front());
		cache.pop_front();
	}

	// Cache not full, insert new one
	else {
		cache.push_back(CachedFrame());
	}

	// Cache
	cache.back().n = n;
	cache.back().frame.CopyFrom(frame);
}


///////////////
// Clear cache
void VideoProviderCache::ClearCache() {
	while (cache.size()) {
		cache.front().frame.Clear();
		cache.pop_front();
	}
}


///////////////////
// Wrapper methods
int VideoProviderCache::GetPosition() {
	return pos;
}
int VideoProviderCache::GetFrameCount() {
	return master->GetFrameCount();
}
int VideoProviderCache::GetWidth() {
	return master->GetWidth();
}
int VideoProviderCache::GetHeight() {
	return master->GetHeight();
}
double VideoProviderCache::GetFPS() {
	return master->GetFPS();
}
bool VideoProviderCache::IsVFR() {
	return master->IsVFR();
}
bool VideoProviderCache::AreKeyFramesLoaded() {
	return master->AreKeyFramesLoaded();
}
wxArrayInt VideoProviderCache::GetKeyFrames() {
	return master->GetKeyFrames();
}
FrameRate VideoProviderCache::GetTrueFrameRate() {
	return master->GetTrueFrameRate();
}
void VideoProviderCache::OverrideFrameTimeList(std::vector<int> list) {
	master->OverrideFrameTimeList(list);
}
bool VideoProviderCache::IsNativelyByFrames() {
	return master->IsNativelyByFrames();
}
bool VideoProviderCache::NeedsVFRHack() {
	return master->NeedsVFRHack();
}
wxString VideoProviderCache::GetWarning() {
	return master->GetWarning();
}
wxString VideoProviderCache::GetDecoderName() {
	return master->GetDecoderName();
}
