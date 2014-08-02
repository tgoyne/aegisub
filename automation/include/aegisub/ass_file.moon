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

ffi.cdef[[
  typedef struct agi_ass_dialogue {
    bool comment;
    int layer;
    int start_time;
    int end_time;
    const char *style;
    const char *actor;
    const char *effect;
    int margin[3];
    const char *text;
  } agi_ass_dialogue;

  typedef struct agi_ass_info {
    const char *key;
    const char *value;
  } agi_ass_info;

  typedef struct agi_ass_style {
  } agi_ass_style;
]]

agi_ass_dialogue = ffi.typeof 'agi_ass_dialogue'
agi_ass_info = ffi.typeof 'agi_ass_info'
agi_ass_style = ffi.typeof 'agi_ass_style'

to_auto4 = (e) ->
  assert e
  if ffi.istype agi_ass_info, e
    section: '[Script Info]'
    class: 'info'
    raw: e.key .. ': ' .. e.value
    key: e.key
    value: e.value
  elseif ffi.istype agi_ass_style, e
    section: '[V4+ Styles]'
    class: 'style'
    raw: 'stuff'
    name: e.name
    fontname: e.fontname
    fontsize: e.fontsize
    color1: e.color1
    color2: e.color2
    color3: e.color3
    color4: e.color4
    bold: e.bold
    italic: e.italic
    underline: e.underline
    strikeout: e.strikeout
    scale_x: e.scale_x
    scale_y: e.scale_y
    spacing: e.spacing
    angle: e.angle
    borderstyle: e.borderstyle
    outline: e.outline
    shadow: e.shadow
    align: e.align
    margin_l: e.margin_l
    margin_r: e.margin_r
    margin_t: e.margin_t
    margin_b: e.margin_b
    encoding: e.encoding
    relative_to: 2
  elseif ffi.istype agi_ass_dialogue, e
    section: '[Events]'
    class: 'dialogue'
    raw: 'stuff'
    comment: e.comment
    layer: e.layer
    start_time: e.start_time
    end_time: e.end_time
    style: e.event
    actor: e.actor
    effect: e.effect
    margin_l: e.margin_l
    margin_r: e.margin_r
    margin_t: e.margin_t
    margin_b: e.margin_b
    text: e.text
  else
    error 'Unrecognized entry type'

get = (tbl, key, ty) ->
  v = tbl[key]
  error "Missing field '#{key}' in #{tbl.class} table" if v == nil
  error "Field '#{key}' should be a #{ty}, is a #{type(v)}" if type(v) != ty
  v

get_string = (tbl, key) -> get tbl, key, 'string'
get_double = (tbl, key) -> get tbl, key, 'number'
get_color = (tbl, key) ->
  v = get_string tbl, key
  c = ass.parse_color v
  error "Could not parse '#{v}' in field #{key} as a color" unless c
  c
get_bool = (tbl, key) ->
  v = get_double tbl, key
  not not v
get_int = (tbl, key) -> math.floor get_double tbl, key

from_auto4 = check'table' (tbl) ->
  error "Table lacks 'class' field, can't convert to AssEntry" if type(tbl.class) != 'string'

  switch tbl.class
    when 'info'
      key: get_string tbl, 'key'
      value: get_string tbl, 'key'
    when 'style'
      name: get_string tbl, 'name'
      font: get_string tbl, 'fontname'
      font_size: get_double tbl, 'fontsize'
      fill_color: get_color tbl, 'color1'
      karaoke_color: get_color tbl, 'color2'
      border_color: get_color tbl, 'color3'
      shadow_color: get_color tbl, 'color4'
      bold: get_bool tbl, 'bold'
      italic: get_bool tbl, 'italic'
      underline: get_bool tbl, 'underline'
      strikeout: get_bool tbl, 'strikeout'
      scale_x: get_double tbl, 'scale_x'
      scale_y: get_double tbl, 'scale_y'
      spacing: get_double tbl, 'spacing'
      angle: get_int tbl, 'angle'
      border_style: get tbl, 'borderstyle'
      border: get_double tbl, 'outline'
      shadow: get_double tbl, 'shadow'
      alignment: get_int tbl, 'align'
      margin: {get_int(tbl, 'margin_l'), get_int(tbl, 'margin_r'), get_int(tbl, 'margin_t')}
      encoding: get_int tbl, 'encoding'
    when 'dialogue'
      comment: get_bool tbl, 'comment'
      layer: get_int tbl, 'layer'
      start: get_int tbl, 'start_time'
      end: get_int tbl, 'end_time'
      style: get_string tbl, 'style'
      actor: get_string tbl, 'actor'
      margin: {get_int(tbl, 'margin_l'), get_int(tbl, 'margin_r'), get_int(tbl, 'margin_t')}
      effect: get_string tbl, 'effect'
      text: get_string tbl, 'text'
    else
      error "Invalid table class #{tbl.class}"
  if not tbl.class then error 'Invalid entry table', 2
  switch tbl.class
    when 'info'

ass = require 'aegisub.ass'

aegisub = aegisub or {}
ipairs = ipairs
newproxy = newproxy
pairs = pairs
tinsert = table.insert


to_auto4 = (e) ->
  assert e and type(e) == 'table'
  if e.key then
    section: '[Script Info]'
    class: 'info'
    raw: e.key .. ': ' .. e.value
    key: e.key
    value: e.value
  elseif e.name then
    section: '[V4+ Styles]'
    class: 'style'
    raw: 'stuff'
    name: e.name
    fontname: e.font
    fontsize: e.font_size
    color1: e.fill_color
    color2: e.karaoke_color
    color3: e.border_color
    color4: e.shadow_color
    bold: e.bold
    italic: e.italic
    underline: e.underline
    strikeout: e.strikeout
    scale_x: e.scale_x
    scale_y: e.scale_y
    spacing: e.spacing
    angle: e.angle
    borderstyle: e.border_style
    outline: e.border
    shadow: e.shadow
    align: e.alignment
    margin_l: e.margin[1]
    margin_r: e.margin[2]
    margin_t: e.margin[3]
    margin_b: e.margin[3]
    encoding: e.encoding
    relative_to: 2
  else
    section: '[Events]'
    class: 'dialogue'
    raw: 'stuff'
    comment: e.comment
    layer: e.layer
    start_time: e.start_time
    end_time: e.end_time
    style: e.event
    actor: e.actor
    effect: e.effect
    margin_l: e.margin[1]
    margin_r: e.margin[2]
    margin_t: e.margin[3]
    margin_b: e.margin[3]
    text: e.text

bounds_check = (tbl, idx, depth=1) ->
  if type(idx) != 'number'
    error "Subtitle object index must be number, is #{type(idx)}", depth + 2
  if idx <= 0 or idx > #tbl
    error "Requested out-of-range line from subtitle file: #{idx}", depth + 2

make_table = (file) ->
  laf = {}
  for key, value in pairs file.info
    tinsert laf, {class: 'info', key: key, value: value}
  for style in *file.styles
    tinsert laf, style
  for event in *file.events
    tinsert laf, event
  laf

argcheck = (name, n, arg, expected) ->
  if type(arg) != expected
    error "Bad argument ##{arg} to #{name}: expected '#{expected}', got #{type(arg)}", 3

create = (file, can_modify, can_set_undo) ->
  laf = make_table file

  check_write = if not can_modify
    -> -> error 'Cannot modify read-only subtitles object', 2
  else
    (fn) -> fn

  assign = ->

  delete = check_write (indexes, ...) =>
    return if indexes == nil
    if type(indexes) != 'table'
      indexes = {indexes, ...}
    table.sort indexes

    for idx in *indexes
      bounds_check laf, idx

    id_idx = 1
    out_idx = 1
    for i = 1, #laf
      if id_idx < #indexes and indexes[id_idx] == i
        -- modification_type
        id_idx += 1
      else
        laf[out_idx] = laf[i]
        out_idx += 1

    for i = out_idx, #laf
      table.remove laf

  deleterange = check_write check'number number' (min, max) =>
    if min < 1 then min = 1
    if max > #laf then max = #laf
    if min > max then return

    for i = max, min, -1
      -- modification_type
      table.remove laf, i

  append = check_write (...) =>
    for i = 1, select '#', ...
      a4 = select i, ...
      e = from_auto4 a4
      -- modification_type

      if #laf == 0
        laf[1] = e
        continue

      if a4.class == 'dialogue'
        tinsert laf, a4
      else
        pos = 1
        for j = 1, #laf
          if a4.class == 'info' and laf[j].class == 'style'
            break
          elseif laf[j].class == 'dialogue'
            break
        tinsert laf, pos, a4

  insert = check_write check'number' (before_idx, ...) =>
    if before_idx > #laf
      append ...
      return

    for i = 1, select '#', ...
      e = from_auto4 select i, ...
      -- modification_type
      tinsert laf, before_idx, e

  methods = :delete, :deleterange, :insert, :append

  index = (idx) =>
    switch type(idx)
      when 'number'
        bounds_check laf, idx
        to_auto4 laf[idx]
      when 'string'
        return #laf if idx == 'n'
        if not can_modify
          error 'Cannot modify read-only subtitles object', 2
        ret = methods[idx]
        if not ret
          error "Invalid method on subtitles object: #{idx}", 2
        ret
      else
        error "Cannot index subtitle files with values of type #{type(idx)}", 2

  newindex = check_write check'number' (idx, value) =>
    if idx < 0 then
      insert -idx, value
    else if idx == 0 or idx == #laf + 1
      append value
    else if not value
      delete idx
    else
      bounds_check laf, idx
      assign idx, value

  proxy = newproxy true
  with getmetatable proxy
    .__index = index
    .__newindex = newindex
    .__ipairs = -> ipairs laf
    .__len = -> #laf

  if can_set_undo
    aegisub.set_undo_point = (...) -> set_undo_point, laf, ...
  else
    aegisub.set_undo_point = -> error 'Attempt to set an undo point in a context where it makes no sense to do so.', 2

  proxy

:create
