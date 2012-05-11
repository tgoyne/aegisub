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

/// @file spline.cpp
/// @brief Manage vector drawings for visual typesetting tools
/// @ingroup visual_ts
///

#include "config.h"

#ifndef AGI_PRE
#include <limits>

#include <wx/tokenzr.h>
#endif

#include "spline.h"

#include "utils.h"
#include "visual_tool.h"

Spline::Spline(const VisualToolBase &tl)
: coord_translator(tl)
, scale(1)
, raw_scale(1)
{
}

Vector2D Spline::ToScript(Vector2D vec) const {
	return coord_translator.ToScriptCoords(vec) * scale;
}

Vector2D Spline::FromScript(Vector2D vec) const {
	return coord_translator.FromScriptCoords(vec / scale);
}

void Spline::SetScale(int new_scale) {
	raw_scale = new_scale;
	scale = 1 << (raw_scale - 1);
}

wxString Spline::EncodeToASS() const {
	wxString result;
	result.reserve(size() * 10);
	char last = 0;

	for (const_iterator cur = begin(); cur != end(); ++cur) {
		switch (cur->type) {
			case SplineCurve::POINT:
				if (last != 'm') {
					result += "m ";
					last = 'm';
				}
				result += ToScript(cur->p1).DStr(' ');
				break;

			case SplineCurve::LINE:
				if (last != 'l') {
					result += "l ";
					last = 'l';
				}
				result += ToScript(cur->p2).DStr(' ');
				break;

			case SplineCurve::BICUBIC:
				if (last != 'b') {
					result += "b ";
					last = 'b';
				}
				result += ToScript(cur->p2).DStr(' ') + " ";
				result += ToScript(cur->p3).DStr(' ') + " ";
				result += ToScript(cur->p4).DStr(' ');
				break;

			default: break;
		}
		result += " ";
	}
	return result;
}

void Spline::DecodeFromASS(wxString str) {
	// Clear current
	clear();
	std::vector<float> stack;

	// Prepare
	char command = 'm';
	Vector2D pt(0, 0);

	// Tokenize the string
	wxStringTokenizer tkn(str, " ");
	while (tkn.HasMoreTokens()) {
		wxString token = tkn.GetNextToken();
		double n;
		if (token.ToCDouble(&n)) {
			stack.push_back(n);

			// Move
			if (stack.size() == 2 && command == 'm') {
				pt = FromScript(Vector2D(stack[0], stack[1]));
				stack.clear();

				push_back(pt);
			}

			// Line
			if (stack.size() == 2 && command == 'l') {
				SplineCurve curve(pt, FromScript(Vector2D(stack[0], stack[1])));
				push_back(curve);

				pt = curve.p2;
				stack.clear();
			}

			// Bicubic
			else if (stack.size() == 6 && command == 'b') {
				SplineCurve curve(pt,
					FromScript(Vector2D(stack[0], stack[1])),
					FromScript(Vector2D(stack[2], stack[3])),
					FromScript(Vector2D(stack[4], stack[5])));
				push_back(curve);

				pt = curve.p4;
				stack.clear();
			}
		}

		// Got something else
		else if (token.size() == 1) {
			command = token[0];
			stack.clear();
		}
	}

	if (!empty() && front().type != SplineCurve::POINT)
		push_front(pt);
}

void Spline::MovePoint(iterator curve,int point,Vector2D pos) {
	iterator prev = curve;
	if (curve != begin()) --prev;
	iterator next = curve;
	++next;
	if (next != end() && next->type == SplineCurve::POINT)
		next = end();

	// Modify
	if (point == 0) {
		curve->p1 = pos;
		if (curve != begin() && curve->type != SplineCurve::POINT)
			prev->EndPoint() = pos;
		if (next != end() && curve->type == SplineCurve::POINT)
			next->p1 = pos;
	}
	else if (point == 1) {
		curve->p2 = pos;
		if (next != end() && curve->type == SplineCurve::LINE)
			next->p1 = pos;
	}
	else if (point == 2) {
		curve->p3 = pos;
	}
	else if (point == 3) {
		curve->p4 = pos;
		if (next != end())
			next->p1 = pos;
	}
}

void Spline::GetPointList(std::vector<float>& points, std::vector<int>& first, std::vector<int>& count) {
	points.clear();
	first.clear();
	count.clear();

	points.reserve((size() + 1) * 2);
	int curCount = 0;

	// Generate points for each curve
	for (iterator cur = begin(); cur != end(); ++cur) {
		if (cur->type == SplineCurve::POINT) {
			if (curCount > 0)
				count.push_back(curCount);

			// start new path
			first.push_back(points.size() / 2);
			curCount = 0;
		}
		curCount += cur->GetPoints(points);
	}

	count.push_back(curCount);
}

void Spline::GetPointList(std::vector<float> &points, iterator curve) {
	points.clear();
	if (curve == end()) return;
	switch (curve->type) {
		case SplineCurve::LINE:
			points.push_back(curve->p1.X());
			points.push_back(curve->p1.Y());
			points.push_back(curve->p2.X());
			points.push_back(curve->p2.Y());
			break;

		case SplineCurve::BICUBIC:
			curve->GetPoints(points);
			break;

		default: break;
	}
}

void Spline::GetClosestParametricPoint(Vector2D reference,iterator &curve,float &t,Vector2D &pt) {
	curve = end();
	t = 0.f;
	if (empty()) return;

	// Close the shape
	push_back(SplineCurve(back().EndPoint(), front().p1));

	float closest = std::numeric_limits<float>::infinity();
	for (iterator cur = begin(); cur != end(); ++cur) {
		float param = cur->GetClosestParam(reference);
		Vector2D p1 = cur->GetPoint(param);
		float dist = (p1-reference).SquareLen();
		if (dist < closest) {
			closest = dist;
			t = param;
			curve = cur;
			pt = p1;
		}
	}

	if (&*curve == &back()) {
		curve = end();
	}

	// Remove closing and return
	pop_back();
}

Vector2D Spline::GetClosestPoint(Vector2D reference) {
	iterator curve;
	float t;
	Vector2D point;
	GetClosestParametricPoint(reference, curve, t, point);
	return point;
}

void Spline::Smooth(float smooth) {
	// See if there are enough curves
	if (size() < 3) return;

	// Smooth curve
	iterator cur_curve = end();
	--cur_curve;
	for (iterator cur = begin(); cur != end(); ) {
		iterator prev_curve = cur_curve;
		cur_curve = cur;
		++cur;
		iterator next_curve = cur == end() ? begin() : cur;

		cur_curve->Smooth(prev_curve->p1, next_curve->EndPoint(), smooth);
	}
}
