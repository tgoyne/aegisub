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
#include <wx/wxprec.h>
#include <GL/gl.h>
#include "gl/glext.h"
#include "gl_wrap.h"
#include "options.h"


//////////////////////////
// Extension get function
#ifdef __WIN32__
void* glGetProc(const char *str) { return wglGetProcAddress(str); }
#else
#ifdef __WXMAC_OSX__
void* glGetProc(const char *str) { return aglGetProcAddress(str); }
#else
void* glGetProc(const char *str) { return glXGetProcAddress((const GLubyte *)str); }
#endif
#endif


//////////////////////////////////////
// OpenGL extension function pointers
PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB = NULL;
PFNGLDELETEOBJECTARBPROC glDeleteObjectARB = NULL;
PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB = NULL;
PFNGLATTACHOBJECTARBPROC glAttachObjectARB = NULL;
PFNGLLINKPROGRAMARBPROC glLinkProgramARB = NULL;
PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB = NULL;
PFNGLSHADERSOURCEARBPROC glShaderSourceARB = NULL;
PFNGLCOMPILESHADERARBPROC glCompileShaderARB = NULL;
PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB = NULL;
PFNGLUNIFORM1IARBPROC glUniform1iARB = NULL;
PFNGLUNIFORM2FARBPROC glUniform2fARB = NULL;


////////////////
// GLEW library
//#if __VISUALC__ >= 1200
//#pragma comment(lib,"glew32.lib")
//#endif


/////////////
// Draw line
void OpenGLWrapper::DrawLine(float x1,float y1,float x2,float y2) {
	SetModeLine();
	glBegin(GL_LINES);
		glVertex2f(x1,y1);
		glVertex2f(x2,y2);
	glEnd();
}


///////////////
// Draw circle
void OpenGLWrapper::DrawEllipse(float x,float y,float radiusX,float radiusY) {
	DrawRing(x,y,radiusY,radiusY,radiusX/radiusY);
}


//////////////////
// Draw rectangle
void OpenGLWrapper::DrawRectangle(float x1,float y1,float x2,float y2) {
	// Fill
	if (a2 != 0.0) {
		SetModeFill();
		glBegin(GL_QUADS);
			glVertex2f(x1,y1);
			glVertex2f(x2,y1);
			glVertex2f(x2,y2);
			glVertex2f(x1,y2);
		glEnd();
	}

	// Outline
	if (a1 != 0.0) {
		SetModeLine();
		glBegin(GL_LINE_LOOP);
			glVertex2f(x1,y1);
			glVertex2f(x2,y1);
			glVertex2f(x2,y2);
			glVertex2f(x1,y2);
		glEnd();
	}
}


///////////////////////
// Draw ring (annulus)
void OpenGLWrapper::DrawRing(float x,float y,float r1,float r2,float ar,float arcStart,float arcEnd) {
	// Make r1 bigger
	if (r2 > r1) {
		float temp = r1;
		r1 = r2;
		r2 = temp;
	}

	// Arc range
	bool hasEnds = arcStart != arcEnd;
	float pi = 3.1415926535897932384626433832795f;
	arcEnd *= pi / 180.f;
	arcStart *= pi / 180.f;
	if (arcEnd <= arcStart) arcEnd += 2.0f*pi;
	float range = arcEnd - arcStart;

	// Math
	int steps = int((r1 + r1*ar) * range / (2.0f*pi))*4;
	if (steps < 12) steps = 12;
	//float end = arcEnd;
	float step = range/steps;
	float curAngle = arcStart;

	// Fill
	if (a2 != 0.0) {
		SetModeFill();

		// Annulus
		if (r1 != r2) {
			glBegin(GL_QUADS);
			for (int i=0;i<steps;i++) {
				glVertex2f(x+sin(curAngle)*r1*ar,y+cos(curAngle)*r1);
				glVertex2f(x+sin(curAngle)*r2*ar,y+cos(curAngle)*r2);
				curAngle += step;
				glVertex2f(x+sin(curAngle)*r2*ar,y+cos(curAngle)*r2);
				glVertex2f(x+sin(curAngle)*r1*ar,y+cos(curAngle)*r1);
			}
			glEnd();
		}

		// Circle
		else {
			glBegin(GL_POLYGON);
			for (int i=0;i<steps;i++) {
				glVertex2f(x+sin(curAngle)*r1,y+cos(curAngle)*r1);
				curAngle += step;
			}
			glEnd();
		}

		// Reset angle
		curAngle = arcStart;
	}

	// Outlines
	if (a1 != 0.0) {
		// Outer
		steps++;
		SetModeLine();
		glBegin(GL_LINE_STRIP);
		for (int i=0;i<steps;i++) {
			glVertex2f(x+sin(curAngle)*r1,y+cos(curAngle)*r1);
			curAngle += step;
		}
		glEnd();

		// Inner
		if (r1 != r2) {
			curAngle = arcStart;
			glBegin(GL_LINE_STRIP);
			for (int i=0;i<steps;i++) {
				glVertex2f(x+sin(curAngle)*r2,y+cos(curAngle)*r2);
				curAngle += step;
			}
			glEnd();

			// End caps
			if (hasEnds) {
				glBegin(GL_LINES);
					glVertex2f(x+sin(arcStart)*r1,y+cos(arcStart)*r1);
					glVertex2f(x+sin(arcStart)*r2,y+cos(arcStart)*r2);
					glVertex2f(x+sin(arcEnd)*r1,y+cos(arcEnd)*r1);
					glVertex2f(x+sin(arcEnd)*r2,y+cos(arcEnd)*r2);
				glEnd();
			}
		}
	}
}


///////////////////
// Set line colour
void OpenGLWrapper::SetLineColour(wxColour col,float alpha,int width) {
	r1 = col.Red()/255.0f;
	g1 = col.Green()/255.0f;
	b1 = col.Blue()/255.0f;
	a1 = alpha;
	lw = width;
}


///////////////////
// Set fill colour
void OpenGLWrapper::SetFillColour(wxColour col,float alpha) {
	r2 = col.Red()/255.0f;
	g2 = col.Green()/255.0f;
	b2 = col.Blue()/255.0f;
	a2 = alpha;
}


////////
// Line
void OpenGLWrapper::SetModeLine() {
	glColor4f(r1,g1,b1,a1);
	if (a1 == 1.0f) glDisable(GL_BLEND);
	else {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	}
	glLineWidth(lw);
	//glEnable(GL_LINE_SMOOTH);
}


////////
// Fill
void OpenGLWrapper::SetModeFill() {
	glColor4f(r2,g2,b2,a2);
	if (a2 == 1.0f) glDisable(GL_BLEND);
	else {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	}
}


//////////////////////////
// Are shaders available?
bool OpenGLWrapper::ShadersAvailable() {
	return IsExtensionSupported("GL_ARB_vertex_shader") && IsExtensionSupported("GL_ARB_fragment_shader");
}


////////////////
// Use shaders?
bool OpenGLWrapper::UseShaders() {
	return Options.AsBool(_T("Video Use Pixel Shaders")) && ShadersAvailable();
}


///////////////////////////
// Is extension supported?
bool OpenGLWrapper::IsExtensionSupported(const char *ext) {
    char *extList = (char*) glGetString(GL_EXTENSIONS);
	if (!extList) return false;
    return strstr(extList, ext) != NULL;
}


///////////////////
// Initialize GLEW
void OpenGLWrapper::Initialize() {
	static bool initialized = false;
	if (!initialized) {
		initialized = true;

		glUseProgramObjectARB = (PFNGLUSEPROGRAMOBJECTARBPROC) glGetProc("glUseProgramObjectARB");
		glDeleteObjectARB = (PFNGLDELETEOBJECTARBPROC) glGetProc("glDeleteObjectARB");
		glCreateProgramObjectARB = (PFNGLCREATEPROGRAMOBJECTARBPROC) glGetProc("glCreateProgramObjectARB");
		glAttachObjectARB = (PFNGLATTACHOBJECTARBPROC) glGetProc("glAttachObjectARB");
		glLinkProgramARB = (PFNGLLINKPROGRAMARBPROC) glGetProc("glLinkProgramARB");
		glCreateShaderObjectARB = (PFNGLCREATESHADEROBJECTARBPROC) glGetProc("glCreateShaderObjectARB");
		glShaderSourceARB = (PFNGLSHADERSOURCEARBPROC) glGetProc("glShaderSourceARB");
		glCompileShaderARB = (PFNGLCOMPILESHADERARBPROC) glGetProc("glCompileShaderARB");
		glGetUniformLocationARB = (PFNGLGETUNIFORMLOCATIONARBPROC) glGetProc("glGetUniformLocationARB");
		glUniform1iARB = (PFNGLUNIFORM1IARBPROC) glGetProc("glUniform1iARB");
		glUniform2fARB = (PFNGLUNIFORM2FARBPROC) glGetProc("glUniform2fARB");
	}
}


//////////////////////
// Set current shader
void OpenGLWrapper::SetShader(GLuint i) {
	Initialize();
	glUseProgramObjectARB(i);
}


//////////////////////////
// Destroy shader program
void OpenGLWrapper::DestroyShaderProgram(GLuint i) {
	Initialize();
	SetShader(0);
	glDeleteObjectARB(i);
}


////////////////////////////////////////////////////////
// Create shader program from vertex and pixel shaders
GLuint OpenGLWrapper::CreateShaderProgram(GLuint vertex,GLuint pixel) {
	// Create instance
	Initialize();
	GLuint program = glCreateProgramObjectARB();

	// Attach shaders
	glAttachObjectARB(program,vertex);
	glAttachObjectARB(program,pixel);

	// Link
	glLinkProgramARB(program);

	// Return
	return program;
}


/////////////////////////////////
// Create standard Vertex shader
GLuint OpenGLWrapper::CreateStandardVertexShader() {
	// Create instance
	Initialize();
	GLuint shader = glCreateShaderObjectARB(GL_VERTEX_SHADER);

	// Read source
	char source[] =
		"void main() {\n"
		"	gl_TexCoord[0] = gl_MultiTexCoord0;\n"
		"	gl_Position = ftransform();\n"
		"}";

	// Compile
	const GLchar *src = source;
	glShaderSourceARB(shader,1,&src,NULL);
	glCompileShaderARB(shader);

	// Return
	return shader;
}


///////////////////////////////////
// Create YV12->RGB32 Pixel Shader
GLuint OpenGLWrapper::CreateYV12PixelShader() {
	// Create instance
	Initialize();
	GLuint shader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER);

	// Read source
	char source[] =
		"uniform sampler2D tex;\n"
		"uniform vec2 off1;\n"
		"uniform vec2 off2;\n"
		"\n"
		"void main() {\n"
		"	vec2 pos = gl_TexCoord[0].st;\n"
		"	vec4 y_bias = vec4(-0.063,-0.063,-0.063,0.0);\n"
		"	vec4 y_mult = vec4(1.164,1.164,1.164,1.0);\n"
		"	vec4 color_y = (texture2D(tex,pos) + y_bias) * y_mult;\n"
		"	pos *= 0.5;\n"
		"	vec4 uv_bias = vec4(-0.5,-0.5,-0.5,0.0);\n"
		"	vec4 uv_mult = vec4(0.0,-0.391,2.018,1.0);\n"
		//"	vec4 uv_mult = vec4(0.0,-0.344,1.770,1.0);\n"
		"	vec4 color_u = (texture2D(tex,pos + off1) + uv_bias) * uv_mult;\n"
		"	uv_mult = vec4(1.596,-0.813,0.0,1.0);\n"
		//"	uv_mult = vec4(1.403,-0.714,0.0,1.0);\n"
		"	vec4 color_v = (texture2D(tex,pos + off2) + uv_bias) * uv_mult;\n"
		"	gl_FragColor = color_y + color_u + color_v;\n"
		"}";

	// Compile
	const GLchar *src = source;
	glShaderSourceARB(shader,1,&src,NULL);
	glCompileShaderARB(shader);

	// Return
	return shader;
}


/////////////////////////////////////
// Create YV12->RGB32 Shader Program
GLuint OpenGLWrapper::CreateYV12Shader(float tw,float th,float tws) {
	// Create vertex shader
	GLuint ver = OpenGLWrapper::CreateStandardVertexShader();
	if (glGetError() != 0) throw _T("Error creating generic vertex shader");

	// Create pixel shader
	GLuint pix = OpenGLWrapper::CreateYV12PixelShader();
	if (glGetError() != 0) throw _T("Error creating YV12 pixel shader");

	// Create program
	GLuint program = OpenGLWrapper::CreateShaderProgram(ver,pix);
	if (glGetError() != 0) throw _T("Error creating shader program");

	// Set shader
	OpenGLWrapper::SetShader(program);
	if (glGetError() != 0) throw _T("Error setting shader");

	// Set uniform variables
	GLuint address = glGetUniformLocationARB(program,"tex");
	glUniform1iARB(address, 0);
	address = glGetUniformLocationARB(program,"off1");
	glUniform2fARB(address, 0.0f, th);
	address = glGetUniformLocationARB(program,"off2");
	glUniform2fARB(address, tws, th);

	// Return shader
	return program;
}
