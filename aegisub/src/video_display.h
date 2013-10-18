#include <libaegisub/signal.h>

#include "vector2d.h"

#include <deque>
#include <memory>
#include <typeinfo>
#include <wx/glcanvas.h>

// Prototypes
struct FrameReadyEvent;
class VideoContext;
class VideoOutGL;
class VisualToolBase;
class wxComboBox;
class wxTextCtrl;
class wxToolBar;
struct VideoFrame;

namespace agi {
	struct Context;
	class OptionValue;
}

class VideoDisplay : public wxGLCanvas {
	/// Signals the display is connected to
	std::deque<agi::signal::Connection> slots;

	agi::Context *con;

	/// The size of the video in screen at the current zoom level, which may not
	/// be the same as the actual client size of the display
	wxSize videoSize;

	/// The current zoom level, where 1.0 = 100%
	double zoomValue;

	int program_handle;
	int position_attrib;
	int color_attrib;
	int projection_uniform;
	int modelview_uniform;
	int tex_coord_attrib;
	int texture_uniform;
	unsigned int vertex_buffer;
	unsigned int index_buffer;

	/// The OpenGL context for this display
	std::unique_ptr<wxGLContext> glContext;

	/// Frame which will replace the currently visible frame on the next render
	std::shared_ptr<VideoFrame> pending_frame;

	/// Upload the image for the current frame to the video card
	void UploadFrameData(FrameReadyEvent&);

	/// @brief Initialize the gl context and set the active context to this one
	/// @return Could the context be set?
	bool InitContext();

	/// @brief Set the size of the display based on the current zoom and video resolution
	void UpdateSize();

public:
	/// @brief Constructor
	VideoDisplay(
		wxToolBar *visualSubToolBar,
		bool isDetached,
		wxComboBox *zoomBox,
		wxWindow* parent,
		agi::Context *context);
	~VideoDisplay();

	/// @brief Render the currently visible frame
	void Render();

	/// @brief Set the zoom level
	/// @param value The new zoom level
	void SetZoom(double value);
	/// @brief Get the current zoom level
	double GetZoom() const { return zoomValue; }

	/// Get the last seen position of the mouse in script coordinates
	Vector2D GetMousePosition() const;

	void SetTool(std::unique_ptr<VisualToolBase> new_tool);

	bool ToolIsType(std::type_info const& type) const;

	/// Discard all OpenGL state
	void Unload();
};
