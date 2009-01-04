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


///////////
// Headers
#include "config.h"

#include "spline_curve.h"
#include "utils.h"


/////////////////////
// Curve constructor
SplineCurve::SplineCurve() {
	type = CURVE_INVALID;
}


/////////////////////////////////////////////////////////
// Split a curve in two using the de Casteljau algorithm
void SplineCurve::Split(SplineCurve &c1,SplineCurve &c2,float t) {
	// Split a line
	if (type == CURVE_LINE) {
		c1.type = CURVE_LINE;
		c2.type = CURVE_LINE;
		c1.p1 = p1;
		c2.p2 = p2;
		c1.p2 = p1*(1-t)+p2*t;
		c2.p1 = c1.p2;
	}

	// Split a bicubic
	else if (type == CURVE_BICUBIC) {
		c1.type = CURVE_BICUBIC;
		c2.type = CURVE_BICUBIC;

		// Sub-divisions
		float u = 1-t;
		Vector2D p12 = p1*u+p2*t;
		Vector2D p23 = p2*u+p3*t;
		Vector2D p34 = p3*u+p4*t;
		Vector2D p123 = p12*u+p23*t;
		Vector2D p234 = p23*u+p34*t;
		Vector2D p1234 = p123*u+p234*t;

		// Set points
		c1.p1 = p1;
		c2.p4 = p4;
		c1.p2 = p12;
		c1.p3 = p123;
		c1.p4 = p1234;
		c2.p1 = p1234;
		c2.p2 = p234;
		c2.p3 = p34;
	}
}


//////////////////////
// Smoothes the curve
// Based on http://antigrain.com/research/bezier_interpolation/index.html
void SplineCurve::Smooth(Vector2D P0,Vector2D P3,float smooth) {
	// Validate
	if (type != CURVE_LINE) return;
	if (p1 == p2) return;
	smooth = MID(0.0f,smooth,1.0f);

	// Get points
	Vector2D P1 = p1;
	Vector2D P2 = p2;

	// Calculate intermediate points
	Vector2D c1 = (P0+P1)/2.0f;
	Vector2D c2 = (P1+P2)/2.0f;
	Vector2D c3 = (P2+P3)/2.0f;
	float len1 = (P1-P0).Len();
	float len2 = (P2-P1).Len();
	float len3 = (P3-P2).Len();
	float k1 = len1/(len1+len2);
	float k2 = len2/(len2+len3);
	Vector2D m1 = c1+(c2-c1)*k1;
	Vector2D m2 = c2+(c3-c2)*k2;

	// Set curve points
	p4 = p2;
	p2 = m1+(c2-m1)*smooth + P1 - m1;
	p3 = m2+(c2-m2)*smooth + P2 - m2;
	type = CURVE_BICUBIC;
}


///////////////
// Get a point
Vector2D SplineCurve::GetPoint(float t) const {
	// Point
	if (type == CURVE_POINT) return p1;

	// Line
	else if (type == CURVE_LINE) {
		return p1*(1.0f-t) + p2*t;
	}

	// Bicubic
	else if (type == CURVE_BICUBIC) {
		float u = 1.0f-t;
		return p1*u*u*u + 3*p2*t*u*u + 3*p3*t*t*u + p4*t*t*t;
	}

	else return Vector2D(0,0);
}


///////////////////////
// Get start/end point
Vector2D SplineCurve::GetStartPoint() const {
	return p1;
}
Vector2D SplineCurve::GetEndPoint() const {
	switch (type) {
		case CURVE_POINT: return p1;
		case CURVE_LINE: return p2;
		case CURVE_BICUBIC: return p4;
		default: return p1;
	}
}


//////////////////////////////////
// Get point closest to reference
Vector2D SplineCurve::GetClosestPoint(Vector2D ref) const {
	return GetPoint(GetClosestParam(ref));
}


///////////////////////////////////////////
// Get value of parameter closest to point
float SplineCurve::GetClosestParam(Vector2D ref) const {
	// Line
	if (type == CURVE_LINE) {
		return GetClosestSegmentPart(p1,p2,ref);
	}

	// Bicubic
	if (type == CURVE_BICUBIC) {
		int steps = 100;
		float bestDist = 80000000.0f;
		float bestT = 0.0f;
		for (int i=0;i<=steps;i++) {
			float t = float(i)/float(steps);
			float dist = (GetPoint(t)-ref).Len();
			if (dist < bestDist) {
				bestDist = dist;
				bestT = t;
			}
		}
		return bestT;
	}

	// Something else
	return 0.0f;
}


//////////////////
// Quick distance
float SplineCurve::GetQuickDistance(Vector2D ref) const {
	// Bicubic
	if (type == CURVE_BICUBIC) {
		float len1 = GetClosestSegmentDistance(p1,p2,ref);
		float len2 = GetClosestSegmentDistance(p2,p3,ref);
		float len3 = GetClosestSegmentDistance(p3,p4,ref);
		float len4 = GetClosestSegmentDistance(p4,p1,ref);
		float len5 = GetClosestSegmentDistance(p1,p3,ref);
		float len6 = GetClosestSegmentDistance(p2,p4,ref);
		return MIN(MIN(MIN(len1,len2),MIN(len3,len4)),MIN(len5,len6));
	}

	// Something else
	else return (GetClosestPoint(ref)-ref).Len();
}


//////////////////////////////////////////
// Closest t in segment p1-p2 to point p3
float SplineCurve::GetClosestSegmentPart(Vector2D pt1,Vector2D pt2,Vector2D pt3) const {
	return MID(0.0f,(pt3-pt1).Dot(pt2-pt1)/(pt2-pt1).SquareLen(),1.0f);
}


/////////////////////////////////////////////////
// Closest distance between p3 and segment p1-p2
float SplineCurve::GetClosestSegmentDistance(Vector2D pt1,Vector2D pt2,Vector2D pt3) const {
	float t = GetClosestSegmentPart(pt1,pt2,pt3);
	return (pt1*(1.0f-t)+pt2*t-pt3).Len();
}
