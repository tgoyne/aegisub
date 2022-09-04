//  Copyright (c) 2012 Thomas Goyne, <plorkyeran@aegisub.org>
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

/// @file util.mm
/// @brief OSX Utilities
/// @ingroup libosxutil osx

#include "libaegisub/util_osx.h"

#import <ApplicationServices/ApplicationServices.h>
#import <Foundation/Foundation.h>

static std::string EmptyIfNil(NSString *string) {
	return [string UTF8String] ?: "";
}

namespace agi::osx {
AppNapDisabler::AppNapDisabler(std::string reason) : handle(nullptr) {
	if (reason.empty()) reason = "Loading";
	auto processInfo = [NSProcessInfo processInfo];
	if ([processInfo respondsToSelector:@selector(beginActivityWithOptions:reason:)])
		handle = [[processInfo beginActivityWithOptions:NSActivityUserInitiatedAllowingIdleSystemSleep
		                                         reason:[NSString stringWithUTF8String:reason.c_str()]]
		          retain];
}

AppNapDisabler::~AppNapDisabler() {
	auto processInfo = [NSProcessInfo processInfo];
	if (handle && [processInfo respondsToSelector:@selector(endActivity:)])
		[processInfo endActivity:(id)handle];
	[(id)handle release];
}
} // namespace agi::osx

namespace agi::util {
std::string GetBundleResourcesDirectory() {
	@autoreleasepool {
		return EmptyIfNil([[[NSBundle mainBundle] resourceURL] path]);
	}
}

std::string GetBundleSharedSupportDirectory() {
	@autoreleasepool {
		return EmptyIfNil([[NSBundle mainBundle] sharedSupportPath]);
	}
}

std::string GetApplicationSupportDirectory() {
	@autoreleasepool {
		return EmptyIfNil([NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES) firstObject]);
	}
}

} // namespace agi::util
