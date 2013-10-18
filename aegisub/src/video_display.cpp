#include "config.h"

#include "video_display.h"

#include "compat.h"
#include "include/aegisub/context.h"
#include "options.h"
#include "subs_controller.h"
#include "threaded_frame_source.h"
#include "utils.h"
#include "video_context.h"
#include "video_frame.h"
#include "visual_tool.h"

#include <libaegisub/util.h>

#include <algorithm>

#include <wx/dcclient.h>

#include <GLES2/gl2.h>

const char *vertex_shader =
	"attribute vec4 Position;"
	"attribute vec4 SourceColor;"
	""
	"varying vec4 DestinationColor;"
	""
	"uniform mat4 Projection;"
	"uniform mat4 Modelview;"
	""
	"attribute vec2 TexCoordIn;"
	"varying vec2 TexCoordOut;"
	""
	"void main(void) {"
	"    DestinationColor = SourceColor;"
	"    gl_Position = Projection * Modelview * Position;"
	"    TexCoordOut = TexCoordIn;"
	"}";

const char *fragment_shader =
	"varying lowp vec4 DestinationColor;"
	""
	"varying lowp vec2 TexCoordOut;"
	"uniform sampler2D Texture;"
	""
	"void main(void) {"
	"    gl_FragColor = DestinationColor * texture2D(Texture, TexCoordOut);"
	"}";

GLuint compile_shader(const char *shader, int length, GLenum type) {
	GLuint handle = glCreateShader(type);
	glShaderSource(handle, 1, &shader, &length);
	glCompileShader(handle);
	return handle;
}

typedef struct {
	float Position[3];
	float Color[4];
	float TexCoord[2];
} Vertex;

#define TEX_COORD_MAX   4

const Vertex Vertices[] = {
	{{1, -1, 0}, {1, 0, 0, 1}, {TEX_COORD_MAX, 0}},
	{{1, 1, 0}, {0, 1, 0, 1}, {TEX_COORD_MAX, TEX_COORD_MAX}},
	{{-1, 1, 0}, {0, 0, 1, 1}, {0, TEX_COORD_MAX}},
	{{-1, -1, 0}, {0, 0, 0, 1}, {0, 0}},
};

const GLubyte Indices[] = {
	0, 1, 2,
	2, 3, 0,
};


/// Attribute list for gl canvases; set the canvases to doublebuffered rgba with an 8 bit stencil buffer
int attribList[] = { WX_GL_RGBA , WX_GL_DOUBLEBUFFER, WX_GL_STENCIL_SIZE, 8, 0 };

/// @class VideoOutRenderException
/// @extends VideoOutException
/// @brief An OpenGL error occurred while uploading or displaying a frame
class OpenGlException : public agi::Exception {
public:
	OpenGlException(const char *func, int err)
	: agi::Exception(from_wx(wxString::Format("%s failed with error code %d", func, err)))
	{ }
	const char * GetName() const { return "video/opengl"; }
	Exception * Copy() const { return new OpenGlException(*this); }
};

#define E(cmd) cmd; if (GLenum err = glGetError()) throw OpenGlException(#cmd, err)

VideoDisplay::VideoDisplay(wxToolBar *visualSubToolBar, bool freeSize, wxComboBox *zoomBox, wxWindow* parent, agi::Context *c)
: wxGLCanvas(parent, -1, attribList)
, con(c)
, zoomValue(1)
{
	con->videoController->Bind(EVT_FRAME_READY, &VideoDisplay::UploadFrameData, this);
	slots.push_back(con->videoController->AddVideoOpenListener(&VideoDisplay::UpdateSize, this));

	Bind(wxEVT_PAINT, std::bind(&VideoDisplay::Render, this));

	c->videoDisplay = this;
	if (con->videoController->IsLoaded())
		con->videoController->JumpToFrame(con->videoController->GetFrameN());
}

VideoDisplay::~VideoDisplay () {
	con->videoController->Unbind(EVT_FRAME_READY, &VideoDisplay::UploadFrameData, this);
}

bool VideoDisplay::InitContext() {
	if (!IsShownOnScreen())
		return false;

	// If this display is in a minimized detached dialog IsShownOnScreen will
	// return true, but the client size is guaranteed to be 0
	if (GetClientSize() == wxSize(0, 0))
		return false;

	if (!glContext)
		glContext = agi::util::make_unique<wxGLContext>(this);

	SetCurrent(*glContext);

	GLuint vertexShader = compile_shader(vertex_shader, sizeof(vertex_shader), GL_VERTEX_SHADER);
	GLuint fragmentShader = compile_shader(fragment_shader, sizeof(fragment_shader), GL_FRAGMENT_SHADER);

	program_handle = glCreateProgram();
	glAttachShader(program_handle, vertexShader);
	glAttachShader(program_handle, fragmentShader);
	glLinkProgram(program_handle);
	glUseProgram(program_handle);

	position_attrib = glGetAttribLocation(program_handle, "Position");
	color_attrib = glGetAttribLocation(program_handle, "SourceColor");
	glEnableVertexAttribArray(position_attrib);
	glEnableVertexAttribArray(color_attrib);

	projection_uniform = glGetUniformLocation(program_handle, "Projection");
	modelview_uniform = glGetUniformLocation(program_handle, "Modelview");

	tex_coord_attrib = glGetAttribLocation(program_handle, "TexCoordIn");
	glEnableVertexAttribArray(tex_coord_attrib);
	texture_uniform = glGetUniformLocation(program_handle, "Texture");

	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);

	return true;
}

void VideoDisplay::UploadFrameData(FrameReadyEvent &evt) {
	pending_frame = evt.frame;
	Render();
}

void VideoDisplay::Render() try {
	if (!con->videoController->IsLoaded() || !InitContext())
		return;

	//if (!videoOut)
	//	videoOut = agi::util::make_unique<VideoOutGL>();

	//try {
	//	if (pending_frame) {
	//		videoOut->UploadFrameData(*pending_frame);
	//		pending_frame.reset();
	//	}
	//}
	//catch (const VideoOutInitException& err) {
	//	wxLogError(
	//		"Failed to initialize video display. Closing other running "
	//		"programs and updating your video card drivers may fix this.\n"
	//		"Error message reported: %s",
	//		err.GetMessage());
	//	con->videoController->SetVideo("");
	//	return;
	//}
	//catch (const VideoOutRenderException& err) {
	//	wxLogError(
	//		"Could not upload video frame to graphics card.\n"
	//		"Error message reported: %s",
	//		err.GetMessage());
	//	return;
	//}

	//if (videoSize.GetWidth() == 0) videoSize.SetWidth(1);
	//if (videoSize.GetHeight() == 0) videoSize.SetHeight(1);

	//if (!viewport_height || !viewport_width)
	//	PositionVideo();

	//videoOut->Render(viewport_left, viewport_bottom, viewport_width, viewport_height);
	//E(glViewport(0, 0, videoSize.GetWidth(), videoSize.GetHeight()));

	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glClearColor(0, 104.0/255.0, 55.0/255.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	const float projection[] = {
		2.f, 0.f, 0.f, 0.f,
		0.f, 1.333f, 0.f, 0.f,
		0.f, 0.f, -2.333f, -1.f,
		0.f, 0.f,  -13.333f, 0.f
	};

	const float modelView[] = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, -7, 1
	};

	glUniformMatrix4fv(projection_uniform, 1, 0, projection);
	glUniformMatrix4fv(modelview_uniform, 1, 0, modelView);

	E(glViewport(0, 0, videoSize.GetWidth(), videoSize.GetHeight()));

	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

	glVertexAttribPointer(position_attrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
	glVertexAttribPointer(color_attrib, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) (sizeof(float) * 3));

	glVertexAttribPointer(tex_coord_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) (sizeof(float) * 7));

	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, _floorTexture);
	//glUniform1i(_textureUniform, 0);

	glDrawElements(GL_TRIANGLES, sizeof(Indices)/sizeof(Indices[0]), GL_UNSIGNED_BYTE, 0);

	//E(glMatrixMode(GL_PROJECTION));
	//E(glLoadIdentity());
	//E(glOrtho(0.0f, videoSize.GetWidth(), videoSize.GetHeight(), 0.0f, -1000.0f, 1000.0f));

	//if (OPT_GET("Video/Overscan Mask")->GetBool()) {
	//	double ar = con->videoController->GetAspectRatioValue();

	//	// Based on BBC's guidelines: http://www.bbc.co.uk/guidelines/dq/pdf/tv/tv_standards_london.pdf
	//	// 16:9 or wider
	//	if (ar > 1.75) {
	//		DrawOverscanMask(.1f, .05f);
	//		DrawOverscanMask(0.035f, 0.035f);
	//	}
	//	// Less wide than 16:9 (use 4:3 standard)
	//	else {
	//		DrawOverscanMask(.067f, .05f);
	//		DrawOverscanMask(0.033f, 0.035f);
	//	}
	//}

	//if ((mouse_pos || !autohideTools->GetBool()) && tool)
	//	tool->Draw();

	SwapBuffers();
}
catch (const agi::Exception &err) {
	wxLogError(
		"An error occurred trying to render the video frame on the screen.\n"
		"Error message reported: %s",
		err.GetChainedMessage());
	con->videoController->SetVideo("");
}

void VideoDisplay::UpdateSize() {
	if (!con->videoController->IsLoaded() || !IsShownOnScreen()) return;

	videoSize.Set(con->videoController->GetWidth(), con->videoController->GetHeight());
	videoSize *= zoomValue;
	if (con->videoController->GetAspectRatioType() != AspectRatio::Default)
		videoSize.SetWidth(videoSize.GetHeight() * con->videoController->GetAspectRatioValue());

	wxEventBlocker blocker(this);
	SetMinClientSize(videoSize);
	SetMaxClientSize(videoSize);

	GetGrandParent()->Layout();
}

void VideoDisplay::SetZoom(double value) {
	zoomValue = std::max(value, .125);
	UpdateSize();
}

void VideoDisplay::SetTool(std::unique_ptr<VisualToolBase> new_tool) {
}

bool VideoDisplay::ToolIsType(std::type_info const& type) const {
	return false;
}

Vector2D VideoDisplay::GetMousePosition() const {
	return Vector2D();
}

void VideoDisplay::Unload() {
	glContext.reset();
	pending_frame.reset();
}
