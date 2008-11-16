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
// AEGISUB/ATHENASUB
//
// Website: http://www.aegisub.net
// Contact: mailto:amz@aegisub.net
//

#pragma once
#include "athenasub.h"
#include "model.h"
#include "tr1.h"

namespace Athenasub {

	// Format handler interface
	class CFormatHandler : public IFormatHandler {
	protected:
		virtual ~CFormatHandler() {}

		void AddSection(IModel &model,String name) const { model.AddSection(name); }
		ConstSection GetSection(const IModel &model,String name) const { return model.GetSection(name); }
		ConstSection GetSectionByIndex(const IModel &model,size_t index) const { return model.GetSectionByIndex(index); }
		Section GetSection(IModel &model,String name) const { return model.GetMutableSection(name); }
		Section GetSectionByIndex(IModel &model,size_t index) const { return model.GetMutableSectionByIndex(index); }
		size_t GetSectionCount(const IModel &model) const { return model.GetSectionCount(); }

	public:
		//CFormatHandler(IModel& _model) : model(_model) {}

		virtual void Load(IModel &model,wxInputStream &file,const String encoding) = 0;
		virtual void Save(const IModel &model,wxOutputStream &file,const String encoding) const = 0;
	};

}
