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
#include "vector2d.h"


///////////////
// Curve types
enum CurveType {
	CURVE_INVALID,
	CURVE_POINT,
	CURVE_LINE,
	CURVE_BICUBIC
};


////////////////
// Spline curve
class SplineCurve {
private:
	float GetClosestSegmentPart(Vector2D p1,Vector2D p2,Vector2D p3) const;
	float GetClosestSegmentDistance(Vector2D p1,Vector2D p2,Vector2D p3) const;

public:
	Vector2D p1,p2,p3,p4;
	CurveType type;

	SplineCurve();
	void Split(SplineCurve &c1,SplineCurve &c2,float t=0.5);
	void Smooth(Vector2D prev,Vector2D next,float smooth=1.0f);

	Vector2D GetPoint(float t) const;
	Vector2D GetStartPoint() const;
	Vector2D GetEndPoint() const;
	Vector2D GetClosestPoint(Vector2D ref) const;
	float GetClosestParam(Vector2D ref) const;
	float GetQuickDistance(Vector2D ref) const;
};
