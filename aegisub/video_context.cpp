// Copyright (c) 2005-2007, Rodrigo Braz Monteiro
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


////////////
// Includes
#include "setup.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include <wx/image.h>
#include <string.h>
#include <wx/clipbrd.h>
#include <wx/filename.h>
#include <wx/config.h>
#include "utils.h"
#include "video_display.h"
#include "video_context.h"
#include "video_provider.h"
#include "subtitles_provider.h"
#include "vfr.h"
#include "ass_file.h"
#include "ass_exporter.h"
#include "ass_time.h"
#include "ass_dialogue.h"
#include "ass_style.h"
#include "subs_grid.h"
#include "vfw_wrap.h"
#include "mkv_wrap.h"
#include "options.h"
#include "subs_edit_box.h"
#include "audio_display.h"
#include "main.h"
#include "video_slider.h"
#include "video_box.h"
#include "utils.h"


///////
// IDs
enum {
	VIDEO_PLAY_TIMER = 1300
};


///////////////
// Event table
BEGIN_EVENT_TABLE(VideoContext, wxEvtHandler)
	EVT_TIMER(VIDEO_PLAY_TIMER,VideoContext::OnPlayTimer)
END_EVENT_TABLE()


////////////
// Instance
VideoContext *VideoContext::instance = NULL;


///////////////
// Constructor
VideoContext::VideoContext() {
	// Set GL context
	glContext = NULL;
	lastTex = 0;
	lastFrame = -1;

	// Set options
	audio = NULL;
	provider = NULL;
	subsProvider = NULL;
	curLine = NULL;
	loaded = false;
	keyFramesLoaded = false;
	overKeyFramesLoaded = false;
	frame_n = 0;
	isPlaying = false;
	threaded = Options.AsBool(_T("Threaded Video"));
	nextFrame = -1;
}


//////////////
// Destructor
VideoContext::~VideoContext () {
	Reset();
	delete glContext;
	glContext = NULL;
}


////////////////
// Get Instance
VideoContext *VideoContext::Get() {
	if (!instance) instance = new VideoContext;
	return instance;
}


/////////
// Clear
void VideoContext::Clear() {
	instance->audio = NULL;
	delete instance;
	instance = NULL;
}


/////////
// Reset
void VideoContext::Reset() {
	// Clear keyframes
	KeyFrames.Clear();
	keyFramesLoaded = false;

	// Remove temporary audio provider
	if (audio && audio->temporary) {
		delete audio->provider;
		audio->provider = NULL;
		delete audio->player;
		audio->player = NULL;
		audio->temporary = false;
	}

	// Remove video data
	loaded = false;
	frame_n = 0;
	keyFramesLoaded = false;
	overKeyFramesLoaded = false;
	isPlaying = false;
	nextFrame = -1;

	// Update displays
	UpdateDisplays(true);

	// Remove textures
	UnloadTexture();

	// Clean up video data
	wxRemoveFile(tempfile);
	tempfile = _T("");
	videoName = _T("");
	tempFrame.Clear();

	// Remove provider
	if (provider && subsProvider && provider->GetAsSubtitlesProvider() != subsProvider) delete subsProvider;
	subsProvider = NULL;
	delete provider;
	provider = NULL;
}


//////////////////
// Unload texture
void VideoContext::UnloadTexture() {
	// Remove textures
	if (lastTex != 0) {
		glDeleteTextures(1,&lastTex);
		lastTex = 0;
	}
	lastFrame = -1;
}


///////////////////////
// Sets video filename
void VideoContext::SetVideo(const wxString &filename) {
	// Unload video
	Reset();
	
	// Load video
	if (!filename.IsEmpty()) {
		try {
			grid->CommitChanges(true);
			bool isVfr = false;
			double overFps = 0;
			FrameRate temp;

			// Unload timecodes
			//int unload = wxYES;
			//if (VFR_Output.IsLoaded()) unload = wxMessageBox(_("Do you want to unload timecodes, too?"),_("Unload timecodes?"),wxYES_NO | wxICON_QUESTION);
			//if (unload == wxYES) VFR_Output.Unload();

			// Read extra data from file
			bool mkvOpen = MatroskaWrapper::wrapper.IsOpen();
			wxString ext = filename.Right(4).Lower();
			KeyFrames.Clear();
			if (ext == _T(".mkv") || mkvOpen) {
				// Parse mkv
				if (!mkvOpen) MatroskaWrapper::wrapper.Open(filename);

				// Get keyframes
				KeyFrames = MatroskaWrapper::wrapper.GetKeyFrames();
				keyFramesLoaded = true;

				// Ask to override timecodes
				int override = wxYES;
				if (VFR_Output.IsLoaded()) override = wxMessageBox(_("You already have timecodes loaded. Replace them with the timecodes from the Matroska file?"),_("Replace timecodes?"),wxYES_NO | wxICON_QUESTION);
				if (override == wxYES) {
					MatroskaWrapper::wrapper.SetToTimecodes(temp);
					isVfr = temp.GetFrameRateType() == VFR;
					if (isVfr) {
						overFps = temp.GetCommonFPS();
						MatroskaWrapper::wrapper.SetToTimecodes(VFR_Input);
	 					MatroskaWrapper::wrapper.SetToTimecodes(VFR_Output);
					}
				}

				// Close mkv
				MatroskaWrapper::wrapper.Close();
			}
#ifdef __WINDOWS__
			else if (ext == _T(".avi")) {
				KeyFrames = VFWWrapper::GetKeyFrames(filename);
				keyFramesLoaded = true;
			}
#endif

			// Choose a provider
			provider = VideoProviderFactory::GetProvider(filename,overFps);

			// Get subtitles provider
			subsProvider = provider->GetAsSubtitlesProvider();
			if (!subsProvider) subsProvider = SubtitlesProviderFactory::GetProvider();
			loaded = provider != NULL;

			// Set frame rate
			fps = provider->GetFPS();
			if (!isVfr) {
				VFR_Input.SetCFR(fps);
				if (VFR_Output.GetFrameRateType() != VFR) VFR_Output.SetCFR(fps);
			}
			else provider->OverrideFrameTimeList(temp.GetFrameTimeList());

			// Gather video parameters
			length = provider->GetFrameCount();
			w = provider->GetWidth();
			h = provider->GetHeight();

			// Set filename
			videoName = filename;
			Options.AddToRecentList(filename,_T("Recent vid"));

			// Get frame
			UpdateDisplays(true);
			frame_n = 0;
			Refresh(true,true);
		}
		
		catch (wxString &e) {
			wxMessageBox(e,_T("Error setting video"),wxICON_ERROR | wxOK);
		}
	}
	loaded = provider != NULL;
}


///////////////////
// Add new display
void VideoContext::AddDisplay(VideoDisplay *display) {
	for (std::list<VideoDisplay*>::iterator cur=displayList.begin();cur!=displayList.end();cur++) {
		if ((*cur) == display) return;
	}
	displayList.push_back(display);
}


//////////////////
// Remove display
void VideoContext::RemoveDisplay(VideoDisplay *display) {
	displayList.remove(display);
}


///////////////////
// Update displays
void VideoContext::UpdateDisplays(bool full) {
	for (std::list<VideoDisplay*>::iterator cur=displayList.begin();cur!=displayList.end();cur++) {
		VideoDisplay *display = *cur;
		
		if (full) {
			display->UpdateSize();
			display->ControlSlider->SetRange(0,GetLength()-1);
		}
		display->ControlSlider->SetValue(GetFrameN());
		display->ControlSlider->Update();
		display->UpdatePositionDisplay();
		display->Refresh();
		display->Update();
	}
}


/////////////////////
// Refresh subtitles
void VideoContext::Refresh (bool video, bool subtitles) {
	// Reset frame
	lastFrame = -1;

	// Get provider
	if (subtitles && subsProvider) {
		AssExporter exporter(grid->ass);
		exporter.AddAutoFilters();
		subsProvider->LoadSubtitles(exporter.ExportTransform());
	}

	// Jump to frame
	JumpToFrame(frame_n);
}


///////////////////////////////////////
// Jumps to a frame and update display
void VideoContext::JumpToFrame(int n) {
	// Loaded?
	if (!loaded) return;

	// Prevent intervention during playback
	if (isPlaying && n != playNextFrame) return;

	// Set frame number
	frame_n = n;
	GetFrameAsTexture(n);

	// Display
	UpdateDisplays(false);

	// Update grid
	if (!isPlaying && Options.AsBool(_T("Highlight subs in frame"))) grid->Refresh(false);
}


////////////////////////////
// Jumps to a specific time
void VideoContext::JumpToTime(int ms) {
	JumpToFrame(VFR_Output.GetFrameAtTime(ms));
}


//////////////////
// Get GL context
wxGLContext *VideoContext::GetGLContext(wxGLCanvas *canvas) {
	if (!glContext) glContext = new wxGLContext(canvas);
	return glContext;
}


///////////////////////////
// Get GL Texture of frame
GLuint VideoContext::GetFrameAsTexture(int n) {
	// Already uploaded
	if (n == lastFrame || n == -1) return lastTex;

	// Get frame
	AegiVideoFrame frame = GetFrame(n);

	// Set frame
	lastFrame = n;

	// Image type
	GLenum format;
	if (frame.format == FORMAT_RGB32) {
		if (frame.invertChannels) format = GL_BGRA_EXT;
		else format = GL_RGBA;
	}
	else if (frame.format == FORMAT_RGB24) {
		if (frame.invertChannels) format = GL_BGR_EXT;
		else format = GL_RGB;
	}
	else if (frame.format == FORMAT_YV12) {
		format = GL_LUMINANCE;
	}
	isInverted = frame.flipped;

	// Set context
	GetGLContext(displayList.front())->SetCurrent(*displayList.front());
	glEnable(GL_TEXTURE_2D);
	if (glGetError() != 0) throw _T("Error enabling texturing.");

	if (lastTex == 0) {
		// Enable
		glShadeModel(GL_FLAT);

		// Generate texture with GL
		glGenTextures(1, &lastTex);
		if (glGetError() != 0) throw _T("Error generating texture.");
		glBindTexture(GL_TEXTURE_2D, lastTex);
		if (glGetError() != 0) throw _T("Error binding texture.");

		// Texture parameters
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		// Load image data into texture
		int height = frame.h;
		if (frame.format == FORMAT_YV12) height = frame.h * 3 / 2;
		int tw = SmallestPowerOf2(frame.w);
		int th = SmallestPowerOf2(frame.h);
		texW = float(frame.w)/float(tw);
		texH = float(frame.h)/float(th);
		glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,tw,th,0,format,GL_UNSIGNED_BYTE,NULL);
		if (glGetError() != 0) throw _T("Error allocating texture.");

		// Set texture
		//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
		if (glGetError() != 0) throw _T("Error setting hinting.");

		// Set priority
		float priority = 1.0f;
		glPrioritizeTextures(1,&lastTex,&priority);
	}
	
	// Load texture data
	glTexSubImage2D(GL_TEXTURE_2D,0,0,0,frame.w,frame.h,format,GL_UNSIGNED_BYTE,frame.data[0]);
	if (glGetError() != 0) throw _T("Error uploading primary plane");

	// UV planes for YV12
	if (frame.format == FORMAT_YV12) {
		glTexSubImage2D(GL_TEXTURE_2D,0,0,frame.h,frame.w/2,frame.h/2,format,GL_UNSIGNED_BYTE,frame.data[1]);
		if (glGetError() != 0) throw _T("Error uploading U plane.");
		glTexSubImage2D(GL_TEXTURE_2D,0,frame.w/2,frame.h,frame.w/2,frame.h/2,format,GL_UNSIGNED_BYTE,frame.data[2]);
		if (glGetError() != 0) throw _T("Error uploadinv V plane.");
	}

	// Return texture number
	return lastTex;
}


/////////////////
// Save snapshot
void VideoContext::SaveSnapshot() {
	// Get folder
	wxString option = Options.AsText(_("Video Screenshot Path"));
	wxFileName videoFile(videoName);
	wxString basepath;
	if (option == _T("?video")) {
		basepath = videoFile.GetPath();
	}
	else if (option == _T("?script")) {
		if (grid->ass->filename.IsEmpty()) basepath = videoFile.GetPath();
		else {
			wxFileName file2(grid->ass->filename);
			basepath = file2.GetPath();
		}
	}
	else basepath = DecodeRelativePath(option,((AegisubApp*)wxTheApp)->folderName);
	basepath += _T("/") + videoFile.GetName();

	// Get full path
	int session_shot_count = 1;
	wxString path;
	while (1) {
		path = basepath + wxString::Format(_T("_%03i_%i.png"),session_shot_count,frame_n);
		++session_shot_count;
		wxFileName tryPath(path);
		if (!tryPath.FileExists()) break;
	}

	// Save
	GetFrame(frame_n).GetImage().SaveFile(path,wxBITMAP_TYPE_PNG);
}


////////////////////////
// Requests a new frame
AegiVideoFrame VideoContext::GetFrame(int n) {
	if (n == -1) n = frame_n;
	AegiVideoFrame frame = provider->GetFrame(n);
	if (subsProvider && subsProvider->CanRaster()) {
		tempFrame.CopyFrom(frame);
		subsProvider->DrawSubtitles(tempFrame,VFR_Input.GetTimeAtFrame(n,true,true)/1000.0);
		return tempFrame;
	}
	else return frame;
}


////////////////////////////
// Get dimensions of script
void VideoContext::GetScriptSize(int &sw,int &sh) {
	grid->ass->GetResolution(sw,sh);
}


////////
// Play
void VideoContext::Play() {
	// Stop if already playing
	if (isPlaying) {
		Stop();
		return;
	}

	// Set variables
	isPlaying = true;
	startFrame = frame_n;
	endFrame = -1;

	// Start playing audio
	audio->Play(VFR_Output.GetTimeAtFrame(startFrame),-1);

	// Start timer
	startTime = clock();
	playTime = startTime;
	playback.SetOwner(this,VIDEO_PLAY_TIMER);
	playback.Start(1);
}


/////////////
// Play line
void VideoContext::PlayLine() {
	// Get line
	AssDialogue *curline = grid->GetDialogue(grid->editBox->linen);
	if (!curline) return;

	// Start playing audio
	audio->Play(curline->Start.GetMS(),curline->End.GetMS());

	// Set variables
	isPlaying = true;
	startFrame = VFR_Output.GetFrameAtTime(curline->Start.GetMS(),true);
	endFrame = VFR_Output.GetFrameAtTime(curline->End.GetMS(),false);

	// Jump to start
	playNextFrame = startFrame;
	JumpToFrame(startFrame);

	// Set other variables
	startTime = clock();
	playTime = startTime;

	// Start timer
	playback.SetOwner(this,VIDEO_PLAY_TIMER);
	playback.Start(1);
}


////////
// Stop
void VideoContext::Stop() {
	if (isPlaying) {
		playback.Stop();
		isPlaying = false;
		audio->Stop();
	}
}


//////////////
// Play timer
void VideoContext::OnPlayTimer(wxTimerEvent &event) {
	// Lock
	wxMutexError res = playMutex.TryLock();
	if (res == wxMUTEX_BUSY) return;
	playMutex.Unlock();
	wxMutexLocker lock(playMutex);

	// Get time difference
	clock_t cur = clock();
	clock_t dif = (clock() - startTime)*1000/CLOCKS_PER_SEC;
	playTime = cur;

	// Find next frame
	int startMs = VFR_Output.GetTimeAtFrame(startFrame);
	int nextFrame = frame_n;
	int i=0;
	for (i=0;i<10;i++) {
		if (nextFrame >= length) break;
		if (dif < VFR_Output.GetTimeAtFrame(nextFrame) - startMs) {
			break;
		}
		nextFrame++;
	}

	// End
	if (nextFrame >= length || (endFrame != -1 && nextFrame > endFrame)) {
		Stop();
		return;
	}

	// Same frame
	if (nextFrame == frame_n) return;

	// Next frame is before or over 2 frames ahead, so force audio resync
	if (nextFrame < frame_n || nextFrame > frame_n + 2) audio->player->SetCurrentPosition(audio->GetSampleAtMS(VFR_Output.GetTimeAtFrame(nextFrame)));

	// Jump to next frame
	playNextFrame = nextFrame;
	frame_n = nextFrame;
	JumpToFrame(nextFrame);

	// Sync audio
	if (nextFrame % 10 == 0) {
		__int64 audPos = audio->GetSampleAtMS(VFR_Output.GetTimeAtFrame(nextFrame));
		__int64 curPos = audio->player->GetCurrentPosition();
		int delta = int(audPos-curPos);
		if (delta < 0) delta = -delta;
		int maxDelta = audio->provider->GetSampleRate();
		if (delta > maxDelta) audio->player->SetCurrentPosition(audPos);
	}
}


//////////////////////////////
// Get name of temp work file
wxString VideoContext::GetTempWorkFile () {
	if (tempfile.IsEmpty()) {
		tempfile = wxFileName::CreateTempFileName(_T("aegisub"));
		wxRemoveFile(tempfile);
		tempfile += _T(".ass");
	}
	return tempfile;
}


/////////////////
// Get keyframes
wxArrayInt VideoContext::GetKeyFrames() {
	if (OverKeyFramesLoaded()) return overKeyFrames;
	return KeyFrames;
}


/////////////////
// Set keyframes
void VideoContext::SetKeyFrames(wxArrayInt frames) {
	KeyFrames = frames;
}


/////////////////////////
// Set keyframe override
void VideoContext::SetOverKeyFrames(wxArrayInt frames) {
	overKeyFrames = frames;
	overKeyFramesLoaded = true;
}


///////////////////
// Close keyframes
void VideoContext::CloseOverKeyFrames() {
	overKeyFrames.Clear();
	overKeyFramesLoaded = false;
}


//////////////////////////////////////////
// Check if override keyframes are loaded
bool VideoContext::OverKeyFramesLoaded() {
	return overKeyFramesLoaded;
}


/////////////////////////////////
// Check if keyframes are loaded
bool VideoContext::KeyFramesLoaded() {
	return overKeyFramesLoaded || keyFramesLoaded;
}
