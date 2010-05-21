#ifndef AGI_PRE
#include <string>

#include <wx/colour.h>
#include <wx/arrstr.h>
#include <wx/string.h>
#endif

#include <libaegisub/colour.h>

#define STD_STR(x) std::string(x.mb_str())

inline wxColour lagi_wxColour(const agi::Colour &colour) { return wxColour(colour); }
inline wxString lagi_wxString(const std::string &str) { return wxString(str); }
wxArrayString lagi_MRU_wxAS(const wxString &list);
