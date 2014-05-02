-- Copyright (c) 2014, Thomas Goyne <plorkyeran@aegisub.org>
--
-- Permission to use, copy, modify, and distribute this software for any
-- purpose with or without fee is hereby granted, provided that the above
-- copyright notice and this permission notice appear in all copies.
--
-- THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
-- WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
-- MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
-- ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
-- WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
-- ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
-- OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

local ffi = require 'ffi'
ffi.cdef[[
typedef struct library library;
typedef struct string {
    const char *data;
    size_t len;
} string;

library *auto5_init();
void auto5_deinit(library *lib);
string auto5_clipboard_get(library *lib);
bool auto5_clipboard_set(library *lib, const char *str, size_t len);
]]

local get, set = pcall(function()
  local lib = ffi.C.auto5_init()
  return
    function() return ffi.C.auto5_clipboard_get(lib) end,
    function(str) return ffi.C.auto5_clipboard_set(lib, str) end
end)

if not get or not set then
  local value = ''
  get = function() return value end
  set = function(str) value = str; return true end
end

return {['get'] = get, ['set'] = set}
