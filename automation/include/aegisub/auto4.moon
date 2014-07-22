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

check = require 'aegisub.check'
ffi = require 'ffi'
ffi_util = require 'aegisub.ffi'
lfs = require 'aegisub.lfs'

(script_filename, env, api) ->
  -- Replace loadfile and dofile with include
  _G.loadfile = nil
  _G.dofile = nil
  _G.include = check'string' (filename) ->
    path = if not filename\find '[/\\]'
      script_filename\gsub('[^/\\]*$', '') .. filename
    else
      p = nil
      for dir in include_path
        p = dir .. '/' .. filename
        if lfs.attributes p, 'mode'
          break
      p

    if not lfs.attributes path, 'mode'
      error "Lua include not found: #{filename}", 2

    fn, err = api.load_file env, path
    if not fn
      error "Failed to load Lua include #{filename}: #{err}", 2
    fn()

  append = (tbl, ...) ->
    c = select '#', ...
    local do_append
    do_append = (n, i) ->
      return if i > c
      v = select i, ...
      if v != nil
        tbl[n+1] = v
        return n+1, do_append n+1, i+1
      return 0, do_append n, i+1
    do_append c, 1

  to_assentry = (tbl) ->
    ffi.gc api.lua_to_assentry(env, tbl), api.free_assentry

  -- Build the public API table
  _G.aegisub =
    register_macro: check'string ?string function ?function ?function' (display, help, run, validate, active) ->
      api.register_macro env, display, help, append aegisub.functions run, validate, active

    register_filter: check'string ?string ?number function ?function' (name, description, priority, run, config) ->
      api.register_filter env, name, tostring(description), tonumber(priority), append aegisub.functions run, config

    text_extents: check'table string' (style, str) ->
      if not style.class == 'style'
        error 'First argument to aegisub.text_extents must be a valid style table', 2
      entry = to_assentry style

      result = ffi.new'double[4]'
      if api.text_extents entry, str, result
        return result[0], result[1], result[2], result[3]
      error 'Unexpected internal error calculating text extents', 2

    frame_from_ms: check'number' (ms) ->
      if api.timecodes_loaded env
        api.frame_from_ms env, ms
      else
        nil

    ms_from_frame: check'number' (frame) ->
      if api.timecodes_loaded env
        api.ms_from_frame frame
      else
        nil

    video_size: ->
      height, width, ar_value, ar_type = ffi.new'int', ffi.new'int', ffi.new'int', ffi'double'
      if api.video_size env, height, width, ar_value, ar_type
        height, width, ar_value, ar_type

    keyframes: ->
    decode_path: check'string' (str) -> ffi_util.string api.decode_path str
    cancel: -> error nil
    lua_automation_version: 4
    file_name: -> api.file_name env
    gettext: check'string' (str) -> ffi_util.string api.gettext str
    project_properties: ->

  fn, err = api.load_file env, script_filename
  if not fn
    error err

  fn() -- maybe pcall?

  if _G.script_version == 3
    error 'Automation 3 is no longer supported'

  api.set_properties env, _G.script_name or '', _G.script_description or '', _G.script_author or '', _G.script_version or ''

