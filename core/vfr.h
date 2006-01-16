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


#pragma once


///////////
// Headers
#include <list>
#include <vector>
#include <wx/wxprec.h>


///////////////////////
// Framerate type enum
enum ASS_FrameRateType {
	NONE,
	CFR,
	VFR
};


///////////////////////
// Base abstract class
class VFR_Base {
public:
	virtual int GetFrameAtTime(int ms)=0;
	virtual int GetTimeAtFrame(int frame)=0;
	virtual double GetAverage()=0;
};


////////////////////////
// V1 Timecodes Classes
class VFR_v1_Range {
public:
	bool isDefault;
	int start;
	int end;
	double fps;
};

class VFR_v1 : public VFR_Base {
private:
	std::list<VFR_v1_Range> Range;
	double DefaultFPS;

public:
	VFR_v1();
	~VFR_v1();
	void Clear();
	void AddRange(int start,int end,double fps,bool isdefault);
	int GetFrameAtTime(int ms);
	int GetTimeAtFrame(int frame);
	double GetAverage();
};


//////////////////////
// V2 Timecodes Class
class VFR_v2 : public VFR_Base {
public:
	std::vector<double> Frame;

	VFR_v2();
	~VFR_v2();
	void Clear();
	void AddFrame(double fps);
	int GetFrameAtTime(int ms);
	int GetTimeAtFrame(int frame);
	double GetAverage();
};


///////////////////
// Framerate class
class FrameRate {
public:
	FrameRate();
	~FrameRate();

	wxString vfrFile;
	bool loaded;
	ASS_FrameRateType FrameRateType;
	double AverageFrameRate;
	VFR_Base *vfr;

	void SetCFR(double fps,bool ifunset=false);
	void Load(wxString file);
	void Unload();
	int GetFrameAtTime(int ms);
	int GetTimeAtFrame(int frame);
	int CorrectFrameAtTime(int ms,bool start);
	int CorrectTimeAtFrame(int frame,bool start);
	double GetTrueRate(double fps);
};


///////////
// Globals
extern FrameRate VFR_Output;
extern FrameRate VFR_Input;
