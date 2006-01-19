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


#ifndef VIDEO_DISPLAY_H
#define VIDEO_DISPLAY_H


///////////
// Headers
#include <wx/wxprec.h>
#include <windows.h>
#include "avisynth_wrap.h"
#include <time.h>


//////////////
// Prototypes
class SubtitlesGrid;
class VideoDisplay;
class VideoSlider;
class AudioProvider;
class AudioDisplay;
class AssDialogue;


////////////////////
// Get Frame thread
class GetFrameThread: public wxThread {
private:
	VideoDisplay *display;
	int image_n;

public:
	GetFrameThread(VideoDisplay *parent,int n);
	wxThread::ExitCode Entry();
};


//////////////
// Main class
class VideoDisplay: public wxWindow, public AviSynthWrapper {
	friend class GetFrameThread;
	friend class AudioProvider;

private:
	int mouse_x,mouse_y;
	wxBitmap *backbuffer;
	wxString subsName;
	wxString VSFilterPath;

	PClip sublessVideo;
	PClip video;
	double zoom;
	wxSize origSize;
	bool isLocked;
	bool threaded;
	bool gettingFrame;
	int nextFrame;
	int framesSkipped;
	wxMutex BitmapMutex;

	clock_t PlayTime;
	clock_t StartTime;
	wxTimer Playback;
	int StartFrame;
	int EndFrame;
	int PlayNextFrame;

	void GetFrameImage(int n);
	void GetFrame(int n);
	void FrameReady();

	void SetZoom(double value);
	wxString GetVSFilter();
	void SaveSnapshot();

	void OnPaint(wxPaintEvent& event);
    void OnMouseEvent(wxMouseEvent& event);
	void OnMouseLeave(wxMouseEvent& event);
	void OnCopyToClipboard(wxCommandEvent &event);
	void OnSaveSnapshot(wxCommandEvent &event);
	void OnCopyCoords(wxCommandEvent &event);
	void OnPlayTimer(wxTimerEvent &event);

public:
	wxBitmap *curFrame;
	wxArrayInt KeyFrames;
	SubtitlesGrid *grid;
	wxString videoName;
	int arType;
	int w,h;
	int orig_w,orig_h;
	int frame_n;
	int length;
	bool loaded;
	bool IsPlaying;
	double fps;
	VideoSlider *ControlSlider;
	//wxSlider *zoomSlider;
	wxComboBox *zoomBox;
	wxTextCtrl *PositionDisplay;
	wxTextCtrl *SubsPosition;
	AssDialogue *curLine;
	AudioDisplay *audio;

	VideoDisplay(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0, const wxString& name = wxPanelNameStr);
	~VideoDisplay();

	void OpenAVSVideo();
	void OpenAVSSubs();
	void PrepareAfterAVS();
	void SetVideo(const wxString &filename);
	void SetSubtitles(const wxString &filename);
	void Reset();
	void Unload();
	void JumpToFrame(int n);
	void JumpToTime(int ms);
	void RefreshVideo(bool force=false);
	void UpdatePositionDisplay();
	void SetAspectRatio(int type);
	int GetAspectRatio() { return arType; }
	void SetZoomPos(int pos);
	void Locked(bool state);
	void UpdateSubsRelativeTime();
	void GetScriptSize(int &w,int &h);

	void Play();
	void PlayLine();
	void Stop();

	DECLARE_EVENT_TABLE()
};


#endif
