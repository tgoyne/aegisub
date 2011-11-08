// Copyright (c) 2006, 2007, Niels Martin Hansen
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

/// @file auto4_lua_dialog.cpp
/// @brief Lua 5.1-based scripting engine (configuration-dialogue interface)
/// @ingroup scripting
///


#include "config.h"

#ifdef WITH_AUTO4_LUA

#include "auto4_lua.h"

#ifndef AGI_PRE
#include <assert.h>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/combobox.h>
#include <wx/gbsizer.h>
#include <wx/log.h>
#include <wx/panel.h>
#include <wx/spinctrl.h>
#include <wx/tokenzr.h>
#include <wx/validate.h>
#include <wx/window.h>
#endif

#include <libaegisub/log.h>

#include "ass_style.h"
#include "colour_button.h"
#include "string_codec.h"
#include "utils.h"

// These must be after the headers above.
#ifdef __WINDOWS__
#include "../../contrib/lua51/src/lualib.h"
#include "../../contrib/lua51/src/lauxlib.h"
#else
#include <lua.hpp>
#endif

namespace {
	inline void get_if_right_type(lua_State *L, wxString &def)
	{
		if (lua_isstring(L, -1))
			def = wxString(lua_tostring(L, -1), wxConvUTF8);
	}

	inline void get_if_right_type(lua_State *L, double &def)
	{
		if (lua_isnumber(L, -1))
			def = std::max(lua_tonumber(L, -1), def);
	}

	inline void get_if_right_type(lua_State *L, int &def)
	{
		if (lua_isnumber(L, -1))
			def = std::max<int>(lua_tointeger(L, -1), def);
	}

	inline void get_if_right_type(lua_State *L, bool &def)
	{
		if (lua_isboolean(L, -1))
			def = !!lua_toboolean(L, -1);
	}

	template<class T>
	T get_field(lua_State *L, const char *name, T def)
	{
		lua_getfield(L, -1, name);
		get_if_right_type(L, def);
		lua_pop(L, 1);
		return def;
	}

	inline wxString get_field(lua_State *L, const char *name)
	{
		return get_field(L, name, wxString());
	}

	template<class T>
	void read_string_array(lua_State *L, T &cont)
	{
		lua_pushnil(L);
		while (lua_next(L, -2)) {
			if (lua_isstring(L, -1))
				cont.push_back(wxString(lua_tostring(L, -1), wxConvUTF8));
			lua_pop(L, 1);
		}
		lua_pop(L, 1);
	}
}

namespace Automation4 {
	// LuaDialogControl
	LuaDialogControl::LuaDialogControl(lua_State *L)
	// Assume top of stack is a control table (don't do checking)
	: name(get_field(L, "name"))
	, hint(get_field(L, "hint"))
	, x(get_field(L, "x", 0))
	, y(get_field(L, "y", 0))
	, width(get_field(L, "width", 1))
	, height(get_field(L, "height", 1))
	{
		LOG_D("automation/lua/dialog") << "created control: '" << STD_STR(name) << "', (" << x << "," << y << ")(" << width << "," << height << "), "<< STD_STR(hint);
	}

	namespace LuaControl {
		/// A static text label
		class Label : public LuaDialogControl {
			wxString label;
		public:
			Label(lua_State *L)
			: LuaDialogControl(L)
			, label(get_field(L, "label"))
			{
			}

			wxControl *Create(wxWindow *parent)
			{
				return new wxStaticText(parent, -1, label);
			}

			int GetSizerFlags() const { return wxALIGN_CENTRE_VERTICAL | wxALIGN_LEFT; }

			void ControlReadBack() { }

			void LuaReadBack(lua_State *L)
			{
				// Label doesn't produce output, so let it be nil
				lua_pushnil(L);
			}
		};

		/// A single-line text edit control
		class Edit : public LuaDialogControl {
		protected:
			wxString text;
			wxTextCtrl *cw;
		public:

			Edit(lua_State *L)
			: LuaDialogControl(L)
			, text(get_field(L, "value"))
			{
				// Undocumented behaviour, 'value' is also accepted as key for text,
				// mostly so a text control can stand in for other things.
				// This shouldn't be exploited and might change later.
				text = get_field(L, "text", text);
			}

			bool CanSerialiseValue() const { return true; }

			wxString SerialiseValue() const
			{
				return inline_string_encode(text);
			}

			void UnserialiseValue(const wxString &serialised)
			{
				text = inline_string_decode(serialised);
			}

			wxControl *Create(wxWindow *parent)
			{
				cw = new wxTextCtrl(parent, -1, text);
				cw->SetToolTip(hint);
				return cw;
			}

			void ControlReadBack()
			{
				text = cw->GetValue();
			}

			void LuaReadBack(lua_State *L)
			{
				lua_pushstring(L, text.utf8_str());
			}
		};

		/// A color-picker button
		class Color : public LuaDialogControl {
			wxString text;
			ColourButton *cw;
		public:

			Color(lua_State *L)
			: LuaDialogControl(L)
			, text(get_field(L, "value"))
			{
			}

			bool CanSerialiseValue() const { return true; }

			wxString SerialiseValue() const
			{
				return inline_string_encode(text);
			}

			void UnserialiseValue(const wxString &serialised)
			{
				text = inline_string_decode(serialised);
			}

			wxControl *Create(wxWindow *parent)
			{
				AssColor colour;
				colour.Parse(text);
				cw = new ColourButton(parent, -1, wxSize(50*width,10*height), colour.GetWXColor());
				cw->SetToolTip(hint);
				return cw;
			}

			void ControlReadBack()
			{
				text = cw->GetColour().GetAsString(wxC2S_HTML_SYNTAX);
			}

			void LuaReadBack(lua_State *L)
			{
				lua_pushstring(L, text.utf8_str());
			}
		};

		/// A multiline text edit control
		class Textbox : public Edit {
		public:
			Textbox(lua_State *L)
			: Edit(L)
			{
			}

			// Same serialisation interface as single-line edit
			wxControl *Create(wxWindow *parent)
			{
				cw = new wxTextCtrl(parent, -1, text, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
				cw->SetMinSize(wxSize(0, 30));
				cw->SetToolTip(hint);
				return cw;
			}
		};


		/// Integer only edit
		class IntEdit : public Edit {
			wxSpinCtrl *cw;
			int value;
			int min, max;
		public:

			IntEdit(lua_State *L)
			: Edit(L)
			, value(get_field(L, "value", 0))
			, min(get_field(L, "min", 0))
			, max(get_field(L, "max", 0))
			{
				if (min <= max) {
					max = INT_MAX;
					min = INT_MIN;
				}
			}

			bool CanSerialiseValue() const  { return true; }

			wxString SerialiseValue() const
			{
				return wxString::Format("%d", value);
			}

			void UnserialiseValue(const wxString &serialised)
			{
				long tmp;
				if (serialised.ToLong(&tmp))
					value = tmp;
			}

			wxControl *Create(wxWindow *parent)
			{
				cw = new wxSpinCtrl(parent, -1, wxString::Format("%d", value), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, min, max, value);
				cw->SetToolTip(hint);
				return cw;
			}

			void ControlReadBack()
			{
				value = cw->GetValue();
			}

			void LuaReadBack(lua_State *L)
			{
				lua_pushinteger(L, value);
			}
		};


		// Float only edit
		class FloatEdit : public Edit {
		public:
			double value;

			FloatEdit(lua_State *L)
			: Edit(L)
			, value(get_field(L, "value", 0))
			{
				// TODO: spin button support
			}

			bool CanSerialiseValue() const  { return true; }

			wxString SerialiseValue() const
			{
				return wxString::Format("%g", value);
			}

			void UnserialiseValue(const wxString &serialised)
			{
				double newval;
				if (serialised.ToDouble(&newval))
					value = newval;
			}

			wxControl *Create(wxWindow *parent)
			{
				cw = new wxTextCtrl(parent, -1, SerialiseValue());
				cw->SetToolTip(hint);
				return cw;
			}

			void ControlReadBack()
			{
				text = cw->GetValue();
				UnserialiseValue(text);
			}

			void LuaReadBack(lua_State *L)
			{
				lua_pushnumber(L, value);
			}
		};

		/// A dropdown list
		class Dropdown : public LuaDialogControl {
			wxArrayString items;
			wxString value;
			wxComboBox *cw;
		public:

			Dropdown(lua_State *L)
			: LuaDialogControl(L)
			, value(get_field(L, "value"))
			{
				lua_getfield(L, -1, "items");
				read_string_array(L, items);
			}

			bool CanSerialiseValue() const { return true; }

			wxString SerialiseValue() const
			{
				return inline_string_encode(value);
			}

			void UnserialiseValue(const wxString &serialised)
			{
				value = inline_string_decode(serialised);
			}

			wxControl *Create(wxWindow *parent)
			{
				cw = new wxComboBox(parent, -1, value, wxDefaultPosition, wxDefaultSize, items, wxCB_READONLY);
				cw->SetToolTip(hint);
				return cw;
			}

			void ControlReadBack()
			{
				value = cw->GetValue();
			}

			void LuaReadBack(lua_State *L)
			{
				lua_pushstring(L, value.utf8_str());
			}
		};

		class Checkbox : public LuaDialogControl {
			wxString label;
			bool value;
			wxCheckBox *cw;
		public:

			Checkbox(lua_State *L)
			: LuaDialogControl(L)
			, label(get_field(L, "label"))
			, value(get_field(L, "value", false))
			{
			}

			bool CanSerialiseValue() const { return true; }

			wxString SerialiseValue() const
			{
				return value ? "1" : "0";
			}

			void UnserialiseValue(const wxString &serialised)
			{
				// fixme? should this allow more different "false" values?
				value = serialised != "0";
			}

			wxControl *Create(wxWindow *parent)
			{
				cw = new wxCheckBox(parent, -1, label);
				cw->SetToolTip(hint);
				cw->SetValue(value);
				return cw;
			}

			void ControlReadBack()
			{
				value = cw->GetValue();
			}

			void LuaReadBack(lua_State *L)
			{
				lua_pushboolean(L, value);
			}
		};
	};

	// LuaDialog
	LuaDialog::LuaDialog(lua_State *L, bool include_buttons)
	: use_buttons(include_buttons)
	, button_pushed(0)
	{
		LOG_D("automation/lua/dialog") << "creating LuaDialoug, addr: " << this;

		// assume top of stack now contains a dialog table
		if (!lua_istable(L, 1))
			luaL_error(L, "Cannot create config dialog from something non-table");

		// Ok, so there is a table with controls
		lua_pushvalue(L, 1);
		lua_pushnil(L); // initial key
		while (lua_next(L, -2)) {
			wxString controlclass;

			if (lua_istable(L, -1))
				controlclass = get_field(L, "class");

			controlclass.LowerCase();

			LuaDialogControl *ctl;

			// Check control class and create relevant control
			if (controlclass == "label") {
				ctl = new LuaControl::Label(L);
			} else if (controlclass == "edit") {
				ctl = new LuaControl::Edit(L);
			} else if (controlclass == "intedit") {
				ctl = new LuaControl::IntEdit(L);
			} else if (controlclass == "floatedit") {
				ctl = new LuaControl::FloatEdit(L);
			} else if (controlclass == "textbox") {
				ctl = new LuaControl::Textbox(L);
			} else if (controlclass == "dropdown") {
				ctl = new LuaControl::Dropdown(L);
			} else if (controlclass == "checkbox") {
				ctl = new LuaControl::Checkbox(L);
			} else if (controlclass == "color") {
				ctl = new LuaControl::Color(L);
			} else if (controlclass == "coloralpha") {
				// FIXME
				ctl = new LuaControl::Edit(L);
			} else if (controlclass == "alpha") {
				// FIXME
				ctl = new LuaControl::Edit(L);
			} else {
				luaL_error(L, "bad control table entry");
			}

			controls.push_back(ctl);
			lua_pop(L, 1);
		}

		if (include_buttons && lua_istable(L, 2)) {
			lua_pushvalue(L, 2);
			read_string_array(L, buttons);
		}
	}

	LuaDialog::~LuaDialog()
	{
		delete_clear(controls);
	}

	wxWindow* LuaDialog::CreateWindow(wxWindow *parent)
	{
		wxWindow *w = new wxPanel(parent);
		wxGridBagSizer *s = new wxGridBagSizer(4, 4);

		for (size_t i = 0; i < controls.size(); ++i) {
			LuaDialogControl *c = controls[i];
			s->Add(c->Create(w), wxGBPosition(c->y, c->x), wxGBSpan(c->height, c->width), c->GetSizerFlags());
		}

		if (use_buttons) {
			wxStdDialogButtonSizer *bs = new wxStdDialogButtonSizer();
			if (buttons.size() > 0) {
				LOG_D("automation/lua/dialog") << "creating user buttons";
				for (size_t i = 0; i < buttons.size(); ++i) {
					LOG_D("automation/lua/dialog") << "button '" << STD_STR(buttons[i]) << "' gets id " << 1001+(wxWindowID)i;

					bs->Add(new wxButton(w, 1001+(wxWindowID)i, buttons[i]));
				}
			} else {
				LOG_D("automation/lua/dialog") << "creating default buttons";
				bs->Add(new wxButton(w, wxID_OK));
				bs->Add(new wxButton(w, wxID_CANCEL));
			}
			bs->Realize();

			w->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &LuaDialog::OnButtonPush, this);

			wxBoxSizer *ms = new wxBoxSizer(wxVERTICAL);
			ms->Add(s, 0, wxBOTTOM, 5);
			ms->Add(bs);
			w->SetSizerAndFit(ms);
		} else {
			w->SetSizerAndFit(s);
		}

		return w;
	}

	int LuaDialog::LuaReadBack(lua_State *L)
	{
		// First read back which button was pressed, if any
		if (use_buttons) {
			LOG_D("automation/lua/dialog") << "reading back button_pushed";
			int btn = button_pushed;
			if (btn == 0) {
				LOG_D("automation/lua/dialog") << "was zero, cancelled";
				// Always cancel/closed
				lua_pushboolean(L, 0);
			} else if (buttons.size() == 0 && btn == 1) {
				LOG_D("automation/lua/dialog") << "default buttons, button 1 bushed, Ok button";
				lua_pushboolean(L, 1);
			} else {
				LOG_D("automation/lua/dialog") << "user button: " << STD_STR(buttons.at(btn-1));
				// button_pushed is index+1 to reserve 0 for Cancel
				lua_pushstring(L, buttons.at(btn-1).utf8_str());
			}
		}

		// Then read controls back
		lua_newtable(L);
		for (size_t i = 0; i < controls.size(); ++i) {
			controls[i]->LuaReadBack(L);
			lua_setfield(L, -2, controls[i]->name.utf8_str());
		}

		if (use_buttons) {
			return 2;
		} else {
			return 1;
		}
	}

	wxString LuaDialog::Serialise()
	{
		wxString res;

		// Format into "name1:value1|name2:value2|name3:value3|"
		for (size_t i = 0; i < controls.size(); ++i) {
			if (controls[i]->CanSerialiseValue()) {
				wxString sn = inline_string_encode(controls[i]->name);
				wxString sv = controls[i]->SerialiseValue();
				res += wxString::Format("%s:%s|", sn, sv);
			}
		}

		// Remove trailing pipe
		if (!res.empty())
			res.RemoveLast();

		return res;
	}

	void LuaDialog::Unserialise(const wxString &serialised)
	{
		// Split by pipe
		wxStringTokenizer tk(serialised, "|");
		while (tk.HasMoreTokens()) {
			// Split by colon
			wxString pair = tk.GetNextToken();
			wxString name = inline_string_decode(pair.BeforeFirst(':'));
			wxString value = pair.AfterFirst(':');

			// Hand value to all controls matching name
			for (size_t i = 0; i < controls.size(); ++i) {
				if (controls[i]->name == name && controls[i]->CanSerialiseValue()) {
					controls[i]->UnserialiseValue(value);
				}
			}
		}
	}

	void LuaDialog::ReadBack()
	{
		for (size_t i = 0; i < controls.size(); ++i) {
			controls[i]->ControlReadBack();
		}
	}

	void LuaDialog::OnButtonPush(wxCommandEvent &evt)
	{
		// Let button_pushed == 0 mean "cancelled", such that pushing Cancel or closing the dialog
		// will both result in button_pushed == 0
		if (evt.GetId() == wxID_OK) {
			LOG_D("automation/lua/dialog") << "was wxID_OK";
			button_pushed = 1;
		} else if (evt.GetId() == wxID_CANCEL) {
			LOG_D("automation/lua/dialog") << "was wxID_CANCEL";
			button_pushed = 0;
		} else {
			LOG_D("automation/lua/dialog") << "was user button";
			// Therefore, when buttons are numbered from 1001 to 1000+n, make sure to set it to i+1
			button_pushed = evt.GetId() - 1000;

			// hack to make sure the dialog will be closed
			// only do this for non-colour buttons
			if (dynamic_cast<ColourButton*> (evt.GetEventObject())) return;
			evt.SetId(wxID_OK);
		}
		LOG_D("automation/lua/dialog") << "button_pushed set to: " << button_pushed;
		evt.Skip();
	}
};

#endif // WITH_AUTO4_LUA
