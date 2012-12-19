#include <wx/textctrl.h>

template<typename Ctrl>
Ctrl *SetToolTip(Ctrl *ctrl, wxString const& tooltip) {
	if (!tooltip.empty())
		ctrl->SetToolTip(tooltip);
	return ctrl;
}

inline wxTextCtrl *TextCtrl(wxWindow *parent, wxString const& value = wxEmptyString, wxSize const& size = wxDefaultSize, long style = 0, wxString const& tooltip = wxEmptyString, wxValidator const& validator = wxDefaultValidator) {
	return SetToolTip(new wxTextCtrl(parent, -1, value, wxDefaultPosition, size, style, validator), tooltip);
}

inline wxTextCtrl *TextCtrl(wxWindow *parent, const char *value = "", wxSize const& size = wxDefaultSize, long style = 0, wxString const& tooltip = wxEmptyString, wxValidator const& validator = wxDefaultValidator) {
	return SetToolTip(new wxTextCtrl(parent, -1, value, wxDefaultPosition, size, style, validator), tooltip);
}

template<typename T>
inline wxTextCtrl *TextCtrl(wxWindow *parent, T *value, wxSize const& size = wxDefaultSize, long style = 0, wxString const& tooltip = wxEmptyString, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr) {
	return TextCtrl(parent, wxEmptyString, size, style, tooltip, wxMakeIntegerValidator(value));
}

inline wxStaticText *StaticText(wxWindow *parent, wxString const& text, long flags = 0) {
	return new wxStaticText(parent, -1, text, wxDefaultPosition, wxDefaultSize, flags);
}

inline wxComboBox *DropDownList(wxWindow *parent, wxString const& value = wxEmptyString, wxArrayString const& items = wxArrayString(), wxSize const& size = wxDefaultSize, wxString const& tooltip = wxEmptyString, wxValidator const& validator = wxDefaultValidator) {
	return SetToolTip(new wxComboBox(parent, -1, value, wxDefaultPosition, size, items, wxCB_READONLY, validator), tooltip);
}

template<int N>
inline wxComboBox *DropDownList(wxWindow *parent, wxString const& value, wxString (&items)[N], wxSize const& size = wxDefaultSize, wxString const& tooltip = wxEmptyString, wxValidator const& validator = wxDefaultValidator) {
	return SetToolTip(new wxComboBox(parent, -1, value, wxDefaultPosition, size, N, items, wxCB_READONLY, validator), tooltip);
}

template<typename T>
inline wxComboBox *DropDownList(wxWindow *parent, int sel, T &items, wxSize const& size = wxDefaultSize, wxString const& tooltip = wxEmptyString, wxValidator const& validator = wxDefaultValidator) {
	auto ctrl = DropDownList(parent, wxEmptyString, items, size, tooltip, validator);
	ctrl->SetSelection(sel);
	return ctrl;
}

inline wxBitmapButton *BitmapButton(wxWindow *parent, wxBitmap const& bmp, wxString const& tooltip = wxEmptyString) {
	return SetToolTip(new wxBitmapButton(parent, -1, bmp), tooltip);
}

inline wxRadioButton *RadioButton(wxWindow *parent, wxString const& text, long flags = 0, wxString const& tooltip = wxEmptyString) {
	return SetToolTip(new wxRadioButton(parent, -1, text, wxDefaultPosition, wxDefaultSize, flags), tooltip);
}

template<typename Tuple>
struct add_to_sizer {
	wxSizerFlags flags;

	void process(wxSizer *, wxSizerFlags flags) {
		this->flags = flags;
	}

	template<typename T>
	void process(wxSizer *sizer, T win) {
		sizer->Add(win, flags);
	}

	template<int N>
	void add(wxSizer *sizer, Tuple const& t) {
		process(sizer, std::get<N>(t));
		add<N + 1>(sizer, t);
	}

	template<>
	void add<std::tuple_size<Tuple>::value>(wxSizer *sizer, Tuple const&) { }
};

template<typename Tuple>
inline wxSizer *BoxSizer(wxOrientation dir, Tuple const& args) {
	wxSizer *sizer = new wxBoxSizer(dir);
	add_to_sizer<Tuple>().add<0>(sizer, args);
	return sizer;
}

#define HORZBOX(...) BoxSizer(wxHORIZONTAL, std::make_tuple(__VA_ARGS__))
#define VERTBOX(...) BoxSizer(wxVERTICAL, std::make_tuple(__VA_ARGS__))
