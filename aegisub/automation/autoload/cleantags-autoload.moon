-- "Clean Tags" -- An Auto4 LUA script for cleaning up ASS subtitle lines of badly-formed override blocks and redundant/duplicate tags

-- Copyright (c) 2007-2009 Muhammad Lukman Nasaruddin (aka ai-chan, Aegisub's forum member)

-- Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
-- associated documentation files (the "Software"), to deal in the Software without restriction,
-- including without limitation the rights to use, copy, modify, merge, publish, distribute,
-- sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
-- furnished to do so, subject to the following conditions:

-- The above copyright notice and this permission notice shall be included in all copies or substantial
-- portions of the Software.

-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
-- NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
-- NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
-- OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
-- CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

local tr = aegisub.gettext

export script_name = tr'Clean Tags'
export script_description = tr'Clean subtitle lines by re-arranging ASS tags and override blocks within the lines'
export script_author = 'Muhammad Lukman Nasaruddin (ai-chan)'
export script_version = '1.20'
export script_modified = '25 February 2009'

cleantags = require 'cleantags'

cleantags_subs = (subtitles) ->
  linescleaned = 0
  for i = 1, #subtitles
    aegisub.progress.set i * 100 / #subtitles
    line = subtitles[i]
    if line.class == 'dialogue' and not line.comment and line.text ~= ''
      line.text = cleantags line.text
      subtitles[i] = line
      linescleaned += 1
      aegisub.progress.task "#{linescleaned} lines cleaned"

cleantags_macro = (subtitles, selected_lines, active_line) ->
  cleantags_subs(subtitles)
  aegisub.set_undo_point(script_name)

aegisub.register_macro(script_name, script_description, cleantags_macro)
aegisub.register_filter(script_name, script_description, 0, cleantags_subs)
