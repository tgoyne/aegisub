// Copyright (c) 2008, Rodrigo Braz Monteiro
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

#include "plugin_manager.h"
#include "video_provider_manager.h"
#include "audio_provider_manager.h"
#include "audio_player_manager.h"
#include "subtitles_provider_manager.h"
#include "spellchecker_manager.h"


///////////////
// Constructor
PluginManager::PluginManager() {
	init = false;

#ifdef WITH_AUTO4_LUA
	lua = NULL;
#endif
#ifdef WITH_PERL
	perl = NULL;
#endif
#ifdef WITH_AUTO3
	auto3 = NULL;
#endif
#ifdef WITH_RUBY
	ruby = NULL;
#endif
	
}


//////////////
// Destructor
PluginManager::~PluginManager() {
	VideoProviderFactoryManager::ClearProviders();
	AudioProviderFactoryManager::ClearProviders();
	AudioPlayerFactoryManager::ClearProviders();
	SubtitlesProviderFactoryManager::ClearProviders();
	SpellCheckerFactoryManager::ClearProviders();

#ifdef WITH_AUTO4_LUA
	if (lua) {
		lua->Unregister(lua);
		delete lua;
		lua = NULL;
	}
#endif
#ifdef WITH_PERL
	if (perl) {
		perl->Unregister(perl);
		delete perl;
		perl = NULL;
	}
#endif
#ifdef WITH_AUTO3
	if (auto3) {
		auto3->Unregister(auto3);
		delete auto3;
		auto3 = NULL;
	}
#endif
#ifdef WITH_RUBY
	if (ruby) {
		ruby->Unregister(ruby);
		delete ruby;
		ruby = NULL;
	}
#endif
}


//////////////////////////////////
// Registers all built-in plugins
void PluginManager::RegisterBuiltInPlugins() {
	if (!init) {
		// Managers
		VideoProviderFactoryManager::RegisterProviders();
		AudioProviderFactoryManager::RegisterProviders();
		AudioPlayerFactoryManager::RegisterProviders();
		SubtitlesProviderFactoryManager::RegisterProviders();
		SpellCheckerFactoryManager::RegisterProviders();

		// Automation languages
#ifdef WITH_AUTO4_LUA
		lua = new Automation4::LuaScriptFactory();
		lua->RegisterFactory();
#endif
#ifdef WITH_PERL
		perl = new Automation4::PerlScriptFactory();
		perl->RegisterFactory();
#endif
#ifdef WITH_AUTO3
		auto3 = new Automation4::Auto3ScriptFactory();
#endif
#ifdef WITH_RUBY
		ruby = new Automation4::RubyScriptFactory();
#endif
	}

	// Done
	init = true;
}
