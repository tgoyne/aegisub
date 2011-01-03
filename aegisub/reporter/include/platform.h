// Copyright (c) 2009, Amar Takhar <verm@aegisub.org>
//
// Permission to use, copy, modify, and distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//
// $Id$

/// @file platform.h
/// @brief API for gathering platform information.
/// @see platform.cpp
/// @ingroup base

#ifndef R_PRECOMP
#include <wx/platinfo.h>
#include <wx/string.h>
#include <wx/intl.h>
#endif

/// @class Platform
/// @brief Grab platform-specific information.
class Platform {
public:

	/// Constructor
	Platform() {};

	/// Destructor
	virtual ~Platform() {};

	/// Get platform instance.
	static Platform* GetPlatform();

	/// @name Platform Agonstic
	/// These are platform agnostic mostly from wx functions.
	//@{

	//   From wxPlatformInfo

	/// Architecture
	/// @return Architecture name.
	/// @retval 32 bit, 64 bit
	const char* ArchName();

	/// OS Family
	/// @return OS Family
	/// @retval Unix, Windows, Mac
	const char* OSFamily();

	/// OS Name
	/// @return OS Name
	/// @retval FreeBSD, Windows, Mac
	const char* OSName();

	/// Endian
	/// @return Endian
	/// @retval Little endian, Big endian
	const char* Endian();

	//   From <wx/gdicmn.h>

	/// Is the display colour
	/// @return true/false
	/// @retval 1, 0
	int DisplayColour();

	/// Display depth
	/// @return Depth
	/// @return Integer
	wxString DisplayDepth();

	/// Display size
	/// @return Size delimited by a space.
	/// @retval "w h"
	wxString DisplaySize();

	/// Pixels per inch
	/// @return PPI
	/// @retval Integer
	wxString DisplayPPI();

	//   Misc

	/// Report signature
	/// @return Signature
	/// @retval SHA256 hash
	std::string Signature();

	/// wxWidgets version
	/// @return Version
	/// @retval Major.Minor.Micro.Patch: 2.9.0.0
	const char* wxVersion();

	/// Locale
	/// @return Locale name
	/// @retval C,POSIX,<code>
	const char* Locale();

	/// Language currently in use
	/// @return Language reporter is currently running in
	/// @retval Language code: en_US, en_CA...
	const char* Language();

	/// System language
	/// @return Language operating system is currently running in
	/// @retval Language code: en_US, en_CA...
	const char* SystemLanguage();

	/// Date
	/// @return Date
	/// @retval Date in YYYY-MM-DD
	std::string Date();

	/// Time
	/// @return Time
	/// @retval Time in HH:MM:SS
	wxString Time();

	/// TimeZone
	/// @return TimeZone
	/// @retval EST,EDT,JST...
	wxString TimeZone();
	//@}

	/// @name Platform Specific
	/// The following are all platform-specific.
	//@{
	//   Misc

	/// Operating System version
	/// @return OS Version
	/// @retval Any
	virtual const std::string OSVersion()=0;

	//   Hardware

	/// CPU ID string
	/// @return CPU ID
	/// @retval Any, ex: Intel(R) Pentium(R) M processor 1600MHz
	virtual wxString CPUId()=0;

	/// CPU Speed
	/// @return Speed
	/// @retval Integer
	virtual wxString CPUSpeed()=0;

	/// CPU Count
	/// @return Count
	/// @retval Integer
	virtual wxString CPUCount()=0;

	/// CPU Cores
	/// @return Cores
	/// @retval Integer
	virtual wxString CPUCores()=0;

	/// CPU Features
	/// @return Features set 1
	/// @retval FPU,VME,DE,PSE,TSC,MSR...
	virtual wxString CPUFeatures()=0;

	/// CPU Features2
	/// @return Features set 2
	/// @retval CPU-specific features
	/// @note "EST,TM2" on my P-M, or "SYSCALL,NX,MMX+,LM,3DNow!+,3DNow!" on an Opteron
	virtual wxString CPUFeatures2()=0;

	/// System memory
	/// @return Memory
	/// @retval Integer in bytes
	virtual uint64_t Memory()=0;

	/// OpenGL vendor
	/// @return Vendor
	/// @retval Any
	virtual wxString OpenGLVendor();

	/// OpenGL renderer
	/// @return Renderer
	/// @retval Any
	virtual wxString OpenGLRenderer();

	/// OpenGL version
	/// @return Renderer version
	/// @retval Any
	virtual wxString OpenGLVersion();

	/// OpenGL extensions
	/// @return OpenGL extensions
	/// @retval Space delimited list of extensions
	virtual wxString OpenGLExt();
	//@}

	/// @name Windows
	//@{
#ifdef __WINDOWS__

	/// Service pack
	/// @return Service pack
	/// @retval Any
	virtual wxString ServicePack()=0;

	/// Graphics driver version
	/// @return Driver version
	/// @retval Any
	virtual wxString DriverGraphicsVersion()=0;

	/// Directshow filters installed
	/// @return wxXmlNode of filters installed
	/// @retval A wxXmlNode in the format of:
	/// \verbatim
	/// <filter>
	///   <name version="[version]">[name]</name>
	/// </filter>
	/// \endverbatim
	virtual wxString DirectShowFilters()=0;

	/// AntiVirus installed
	/// @return true/false
	/// @retval 1,0
	virtual wxString AntiVirus()=0;

	/// Firewall installed
	/// @return true/false
	/// @retval 1,0
	virtual wxString Firewall()=0;

	/// DLL versions used
	/// @return wxXmlNode of DLLs used
	/// @retval A wxXmlNode in the format of:
	/// \verbatim
	/// <dll>
	///   <file version="[version]">[name]</file>
	/// </dll>
	/// \endverbatim
	virtual wxString DLLVersions()=0;
#endif
	//@}

	/// @name Unix
	//@{
#ifdef __UNIX__

	/// Dynamic libraries used
	/// @return wxXmlNode of libraries used
	/// @retval A wxXmlNode in the format of:
	/// \verbatim
	/// <lib>
	///   <file version="[version]">[name]</file>
	/// </lib>
	/// \endverbatim
	virtual wxString UnixLibraries()=0;

	/// Desktop environment
	/// @return Environment
	/// @retval Gnome, KDE, WindowMaker...
	virtual wxString DesktopEnvironment()=0;
#endif
	//@}

	/// @name OS X
	//@{
#ifdef __APPLE__

	/// OS patch level
	/// @return Patch level
	/// @retval Any
	virtual wxString PatchLevel()=0;

	/// QuickTime extensions
	/// @return wxXmlNode of extensions used
	/// @retval A wxXmlNode in the format of:
	/// \verbatim
	/// <quicktime>
	///   <ext version="[version]">[name]</file>
	/// </quicktime>
	/// \endverbatim
	virtual wxString QuickTimeExt()=0;

	/// Hardware model
	/// @return Model
	/// @retval Any
	virtual wxString HardwareModel()=0;
#endif
	//@}

private:
	void Init();

	/// wxPlatformInfo struct.
	const wxPlatformInfo plat;

	/// wxLocale instance.
	wxLocale *locale;


	/// Available video information.
	enum VideoInfo {
		VIDEO_RENDERER,	///< Renderer
		VIDEO_VENDOR,	///< Vendor
		VIDEO_VERSION,	///< Version
		VIDEO_EXT		///< Extensions
	};

	/// Retrieve OpenGL video information.
	/// @param which Requested information
	/// @return Video info.
	wxString GetVideoInfo(enum Platform::VideoInfo which);

};
