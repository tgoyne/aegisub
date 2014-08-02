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
--
-- Aegisub Project http://www.aegisub.org/

impl = require 'aegisub.__progress_impl'
ffi_util = require 'aegisub.ffi'

sformat = string.format
type = type
tinsert = table.insert

setup = (env) ->
  log = (level, fmt, ...) ->
    impl.log if type(level) == 'number'
      if level > impl.get_trace_level(env)
        return
      sformat fmt, ...
    else
      sformat level, fmt, ...

  _G.aegisub.progress =
    set: (value) -> impl.set_progress env, value
    task: (value) -> impl.set_task env, value
    title: (value) -> impl.set_title env, value
    is_cancelled: -> impl.get_cancelled env
  _G.aegisub.debug =
    out: log
  _G.aegisub.log = log
  if dialog_allowed
    _G.aegisub.dialog
      display: impl.display
      open: check'string string? string? string? boolean? boolean?' (msg, dir='', file='', wildcard='', multiple=false, must_exist=false) ->
        ret = impl.open_dialog env, msg, dir, file, wildcard, overwrite
        if ret == nil then return nil
        if multiple
          paths = {}
          i = 0
          while ret[i] != nil
            tinsert paths, ffi_util.string ret[i]
            i += 1
          ffi.C.free ret
          paths
        else
          path = ffi_util.string ret[0]
          ffi.C.free ret
          path
      save: check'string string? string? string? boolean?' (msg, dir='', file='', wildcard='', prompt_overwrite=true) ->
        ffi_util.string impl.save_dialog env, msg, dir, file, wildcard, prompt_overwrite

teardown = ->
  _G.aegisub.debug = nil
  _G.aegisub.dialog = nil
  _G.aegisub.log = nil
  _G.aegisub.progress = nil

{:setup, :teardown}
