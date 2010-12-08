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
// Aegisub Project http://www.aegisub.org/
//
// $Id$

/// @file video_context.cpp
/// @brief Keep track of loaded video
/// @ingroup video
///

#include "config.h"

#ifndef AGI_PRE
#include <string.h>

#include <wx/clipbrd.h>
#include <wx/config.h>
#include <wx/filename.h>
#include <wx/image.h>
#endif

#ifdef __APPLE__
#include <OpenGL/GL.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include "include/aegisub/audio_player.h"
#include "include/aegisub/audio_provider.h"
#include "ass_dialogue.h"
#include "ass_file.h"
#include "ass_style.h"
#include "ass_time.h"
#include "audio_controller.h"
#include "compat.h"
#include "include/aegisub/audio_player.h"
#include "include/aegisub/audio_provider.h"
#include "include/aegisub/video_provider.h"
#include "keyframe.h"
#include <libaegisub/access.h>
#include "main.h"
#include "mkv_wrap.h"
#include "standard_paths.h"
#include "selection_controller.h"
#include "subs_edit_box.h"
#include "subs_grid.h"
#include "threaded_frame_source.h"
#include "utils.h"
#include "video_box.h"
#include "video_context.h"
#include "video_frame.h"

/// IDs
enum {
	VIDEO_PLAY_TIMER = 1300
};

BEGIN_EVENT_TABLE(VideoContext, wxEvtHandler)
	EVT_TIMER(VIDEO_PLAY_TIMER,VideoContext::OnPlayTimer)
END_EVENT_TABLE()

/// @brief Constructor 
///
VideoContext::VideoContext()
: startFrame(-1)
, endFrame(-1)
, playNextFrame(-1)
, nextFrame(-1)
, isPlaying(false)
, keepAudioSync(true)
, frame_n(0)
, length(0)
, arValue(1.)
, arType(0)
, hasSubtitles(false)
, playAudioOnStep(OPT_GET("Audio/Plays When Stepping Video"))
, grid(NULL)
, audio(NULL)
, VFR_Input(videoFPS)
, VFR_Output(ovrFPS)
{
	Bind(EVT_VIDEO_ERROR, &VideoContext::OnVideoError, this);
	Bind(EVT_SUBTITLES_ERROR, &VideoContext::OnSubtitlesError, this);

	OPT_SUB("Subtitle/Provider", &VideoContext::Reload, this);
	OPT_SUB("Video/Provider", &VideoContext::Reload, this);

	// It would be nice to find a way to move these to the individual providers
	OPT_SUB("Provider/Avisynth/Allow Ancient", &VideoContext::Reload, this);
	OPT_SUB("Provider/Avisynth/Memory Max", &VideoContext::Reload, this);

	OPT_SUB("Provider/Video/FFmpegSource/Decoding Threads", &VideoContext::Reload, this);
	OPT_SUB("Provider/Video/FFmpegSource/Unsafe Seeking", &VideoContext::Reload, this);
}

VideoContext::~VideoContext () {
}

VideoContext *VideoContext::Get() {
	static VideoContext instance;
	return &instance;
}

void VideoContext::Reset() {
	StandardPaths::SetPathValue(_T("?video"), "");

	keyFrames.clear();
	videoFPS = agi::vfr::Framerate();
	keyframesRevision++;

	// Remove video data
	frame_n = 0;
	length = 0;
	isPlaying = false;
	nextFrame = -1;

	// Clean up video data
	videoName.clear();

	// Remove provider
	videoProvider.reset();
	provider.reset();
}

void VideoContext::SetVideo(const wxString &filename) {
	Stop();
	Reset();
	if (filename.empty()) return;

	try {
		provider.reset(new ThreadedFrameSource(filename, this));
		videoProvider = provider->GetVideoProvider();
		videoFile = filename;

		keyFrames = videoProvider->GetKeyFrames();

		// Set frame rate
		videoFPS = videoProvider->GetFPS();
		if (ovrFPS.IsLoaded()) {
			int ovr = wxMessageBox(_("You already have timecodes loaded. Would you like to replace them with timecodes from the video file?"), _("Replace timecodes?"), wxYES_NO | wxICON_QUESTION);
			if (ovr == wxYES) {
				ovrFPS = agi::vfr::Framerate();
				ovrTimecodeFile.clear();
			}
		}

		// Gather video parameters
		length = videoProvider->GetFrameCount();

		// Set filename
		videoName = filename;
		config::mru->Add("Video", STD_STR(filename));
		wxFileName fn(filename);
		StandardPaths::SetPathValue(_T("?video"),fn.GetPath());

		// Get frame
		frame_n = 0;

		// Show warning
		wxString warning = videoProvider->GetWarning();
		if (!warning.empty()) wxMessageBox(warning,_T("Warning"),wxICON_WARNING | wxOK);

		hasSubtitles = false;
		if (filename.Right(4).Lower() == L".mkv") {
			hasSubtitles = MatroskaWrapper::HasSubtitles(filename);
		}

		provider->LoadSubtitles(grid->ass);
		grid->ass->AddCommitListener(&VideoContext::SubtitlesChanged, this);
		VideoOpen();
		KeyframesOpen(keyFrames);
	}
	catch (agi::UserCancelException const&) { }
	catch (agi::FileNotAccessibleError const& err) {
		config::mru->Remove("Video", STD_STR(filename));
		wxMessageBox(lagi_wxString(err.GetMessage()), L"Error setting video", wxICON_ERROR | wxOK);
	}
	catch (VideoProviderError const& err) {
		wxMessageBox(lagi_wxString(err.GetMessage()), L"Error setting video", wxICON_ERROR | wxOK);
	}
}

void VideoContext::Reload() {
	if (IsLoaded()) {
		int frame = frame_n;
		SetVideo(videoFile);
		JumpToFrame(frame);
	}
}

void VideoContext::SubtitlesChanged() {
	if (!IsLoaded()) return;

	bool wasPlaying = isPlaying;
	Stop();

	provider->LoadSubtitles(grid->ass);
	GetFrameAsync(frame_n);

	if (wasPlaying) Play();
}

void VideoContext::JumpToFrame(int n) {
	if (!IsLoaded()) return;

	// Prevent intervention during playback
	if (isPlaying && n != playNextFrame) return;

	frame_n = n;

	Seek(n);
}

void VideoContext::JumpToTime(int ms, agi::vfr::Time end) {
	JumpToFrame(FrameAtTime(ms, end));
}

void VideoContext::GetFrameAsync(int n) {
	provider->RequestFrame(n,videoFPS.TimeAtFrame(n)/1000.0);
}

std::tr1::shared_ptr<AegiVideoFrame> VideoContext::GetFrame(int n, bool raw) {
	return provider->GetFrame(n, videoFPS.TimeAtFrame(n)/1000.0, raw);
}

int VideoContext::GetWidth() const {
	return videoProvider->GetWidth();
}
int VideoContext::GetHeight() const {
	return videoProvider->GetHeight();
}

void VideoContext::SaveSnapshot(bool raw) {
	// Get folder
	static const agi::OptionValue* ssPath = OPT_GET("Path/Screenshot");
	wxString option = lagi_wxString(ssPath->GetString());
	wxFileName videoFile(videoName);
	wxString basepath;
	// Is it a path specifier and not an actual fixed path?
	if (option[0] == _T('?')) {
		// If dummy video is loaded, we can't save to the video location
		if (option.StartsWith(_T("?video")) && (videoName.Find(_T("?dummy")) != wxNOT_FOUND)) {
			// So try the script location instead
			option = _T("?script");
		}
		// Find out where the ?specifier points to
		basepath = StandardPaths::DecodePath(option);
		// If where ever that is isn't defined, we can't save there
		if ((basepath == _T("\\")) || (basepath == _T("/"))) {
			// So save to the current user's home dir instead
			basepath = wxGetHomeDir();
		}
	}
	// Actual fixed (possibly relative) path, decode it
	else basepath = DecodeRelativePath(option,StandardPaths::DecodePath(_T("?user/")));
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

	GetFrame(frame_n,raw)->GetImage().SaveFile(path,wxBITMAP_TYPE_PNG);
}

void VideoContext::GetScriptSize(int &sw,int &sh) {
	grid->ass->GetResolution(sw,sh);
}

void VideoContext::PlayNextFrame() {
	if (isPlaying)
		return;

	int thisFrame = frame_n;
	JumpToFrame(frame_n + 1);
	// Start playing audio
	if (playAudioOnStep->GetBool()) {
		audio->PlayRange(SampleRange(
			audio->SamplesFromMilliseconds(TimeAtFrame(thisFrame)),
			audio->SamplesFromMilliseconds(TimeAtFrame(thisFrame + 1))));
	}
}

void VideoContext::PlayPrevFrame() {
	if (isPlaying)
		return;

	int thisFrame = frame_n;
	JumpToFrame(frame_n -1);
	// Start playing audio
	if (playAudioOnStep->GetBool()) {
		audio->PlayRange(SampleRange(
			audio->SamplesFromMilliseconds(TimeAtFrame(thisFrame - 1)),
			audio->SamplesFromMilliseconds(TimeAtFrame(thisFrame))));
	}
}

void VideoContext::Play() {
	if (isPlaying) {
		Stop();
		return;
	}

	// Set variables
	startFrame = frame_n;
	endFrame = -1;

	// Start playing audio
	audio->PlayToEnd(audio->SamplesFromMilliseconds(TimeAtFrame(startFrame)));

	//audio->Play will override this if we put it before, so put it after.
	isPlaying = true;

	// Start timer
	playTime.Start();
	playback.SetOwner(this,VIDEO_PLAY_TIMER);
	playback.Start(10);
}

void VideoContext::PlayLine() {
	AssDialogue *curline = grid->GetActiveLine();
	if (!curline) return;

	// Start playing audio
	audio->PlayRange(SampleRange(
		audio->SamplesFromMilliseconds(curline->Start.GetMS()),
		audio->SamplesFromMilliseconds(curline->End.GetMS())));

	// Set variables
	isPlaying = true;
	startFrame = FrameAtTime(grid->GetActiveLine()->Start.GetMS(),agi::vfr::START);
	endFrame = FrameAtTime(grid->GetActiveLine()->End.GetMS(),agi::vfr::END);

	// Jump to start
	playNextFrame = startFrame;
	JumpToFrame(startFrame);

	// Set other variables
	playTime.Start();

	// Start timer
	playback.SetOwner(this,VIDEO_PLAY_TIMER);
	playback.Start(10);
}

void VideoContext::Stop() {
	if (isPlaying) {
		playback.Stop();
		isPlaying = false;
		audio->Stop();
	}
}

void VideoContext::OnPlayTimer(wxTimerEvent &event) {
	// Lock
	wxMutexError res = playMutex.TryLock();
	if (res == wxMUTEX_BUSY) return;
	playMutex.Unlock();
	wxMutexLocker lock(playMutex);

	// Get time difference
	int dif = playTime.Time();

	// Find next frame
	int startMs = TimeAtFrame(startFrame);
	int nextFrame = frame_n;
	int i=0;
	for (i=0;i<10;i++) {
		if (nextFrame >= length) break;
		if (dif < TimeAtFrame(nextFrame) - startMs) {
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
	if (audio->IsPlaying() && keepAudioSync && (nextFrame < frame_n || nextFrame > frame_n + 2)) {
		audio->ResyncPlaybackPosition(audio->SamplesFromMilliseconds(TimeAtFrame(nextFrame)));
	}

	// Jump to next frame
	playNextFrame = nextFrame;
	frame_n = nextFrame;
	JumpToFrame(nextFrame);

	// Sync audio
	if (keepAudioSync && nextFrame % 10 == 0 && audio->IsPlaying()) {
		int64_t audPos = audio->SamplesFromMilliseconds(TimeAtFrame(nextFrame));
		int64_t curPos = audio->GetPlaybackPosition();
		int delta = int(audPos-curPos);
		if (delta < 0) delta = -delta;
		int maxDelta = audio->SamplesFromMilliseconds(1000);
		if (delta > maxDelta) audio->ResyncPlaybackPosition(audPos);
	}
}

double VideoContext::GetARFromType(int type) const {
	if (type == 0) return (double)VideoContext::Get()->GetWidth()/(double)VideoContext::Get()->GetHeight();
	if (type == 1) return 4.0/3.0;
	if (type == 2) return 16.0/9.0;
	if (type == 3) return 2.35;
	return 1.0;  //error
}

void VideoContext::SetAspectRatio(int type, double value) {
	if (type != 4) value = GetARFromType(type);

	arType = type;
	arValue = MID(.5, value, 5.);
	ARChange(arType, arValue);
}

void VideoContext::LoadKeyframes(wxString filename) {
	if (filename == keyFramesFilename || filename.empty()) return;
	try {
		keyFrames = KeyFrameFile::Load(filename);
		keyFramesFilename = filename;
		KeyframesOpen(keyFrames);
	}
	catch (const wchar_t *error) {
		wxMessageBox(error, _T("Error opening keyframes file"), wxOK | wxICON_ERROR, NULL);
	}
	catch (agi::acs::AcsNotFound const&) {
		wxLogError(L"Could not open file " + filename);
		config::mru->Remove("Keyframes", STD_STR(filename));
	}
	catch (...) {
		wxMessageBox(_T("Unknown error"), _T("Error opening keyframes file"), wxOK | wxICON_ERROR, NULL);
	}
	keyframesRevision++;
}

void VideoContext::SaveKeyframes(wxString filename) {
	KeyFrameFile::Save(filename, GetKeyFrames());
	keyframesRevision++;
}

void VideoContext::CloseKeyframes() {
	keyFramesFilename.clear();
	if (videoProvider.get()) {
		keyFrames = videoProvider->GetKeyFrames();
	}
	else {
		keyFrames.clear();
	}
	KeyframesOpen(keyFrames);
	keyframesRevision++;
}

void VideoContext::LoadTimecodes(wxString filename) {
	if (filename == ovrTimecodeFile || filename.empty()) return;
	try {
		ovrFPS = agi::vfr::Framerate(STD_STR(filename));
		ovrTimecodeFile = filename;
		config::mru->Add("Timecodes", STD_STR(filename));
		SubtitlesChanged();
	}
	catch (const agi::acs::AcsError&) {
		wxLogError(L"Could not open file " + filename);
		config::mru->Remove("Timecodes", STD_STR(filename));
	}
	catch (const agi::vfr::Error& e) {
		wxLogError(L"Timecode file parse error: %s", e.GetMessage().c_str());
	}
}
void VideoContext::SaveTimecodes(wxString filename) {
	try {
		FPS().Save(STD_STR(filename), IsLoaded() ? length : -1);
		config::mru->Add("Timecodes", STD_STR(filename));
	}
	catch(const agi::acs::AcsError&) {
		wxLogError(L"Could not write to " + filename);
	}
}
void VideoContext::CloseTimecodes() {
	ovrFPS = agi::vfr::Framerate();
	ovrTimecodeFile.clear();
	SubtitlesChanged();
}

int VideoContext::TimeAtFrame(int frame, agi::vfr::Time type) const {
	if (ovrFPS.IsLoaded()) {
		return ovrFPS.TimeAtFrame(frame, type);
	}
	return videoFPS.TimeAtFrame(frame, type);
}
int VideoContext::FrameAtTime(int time, agi::vfr::Time type) const {
	if (ovrFPS.IsLoaded()) {
		return ovrFPS.FrameAtTime(time, type);
	}
	return videoFPS.FrameAtTime(time, type);
}

void VideoContext::OnVideoError(VideoProviderErrorEvent const& err) {
	wxLogError(
		L"Failed seeking video. The video file may be corrupt or incomplete.\n"
		L"Error message reported: %s",
		lagi_wxString(err.GetMessage()).c_str());
}
void VideoContext::OnSubtitlesError(SubtitlesProviderErrorEvent const& err) {
	wxLogError(
		L"Failed rendering subtitles. Error message reported: %s",
		lagi_wxString(err.GetMessage()).c_str());
}

void VideoContext::OnExit() {
	// On unix wxThreadModule will shut down any still-running threads (and
	// display a warning that it's doing so) before the destructor for
	// VideoContext runs, so manually kill the thread
	Get()->provider.reset();
}
