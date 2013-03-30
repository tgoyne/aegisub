-- Copyright (c) 2007-2009 Muhammad Lukman Nasaruddin (aka ai-chan, Aegisub's forum member)
-- Copyright (c) 2013, Thomas Goyne <plorkyeran@aegisub.org>
--
-- Permission is hereby granted, free of charge, to any person obtaining a copy
-- of this software and associated documentation files (the "Software"), to
-- deal in the Software without restriction, including without limitation the
-- rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
-- sell copies of the Software, and to permit persons to whom the Software is
-- furnished to do so, subject to the following conditions:
--
-- The above copyright notice and this permission notice shall be included in
-- all copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
-- IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
-- FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
-- THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
-- LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
-- FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
-- DEALINGS IN THE SOFTWARE.

cleantags_version = "1.301"
cleantags_modified = "13 November 2009"

ktag = "\\[kK][fo]?%d+"

export cleantags = (text) ->
  breakChar = string.char 1
  replaced = 1
  while replaced != 0
    return if aegisub.progress.is_cancelled()
    text, replaced = string.gsub text,"{(.-)}{(.-)}",  (block1, block2) ->
      if string.find(block1, ktag) and string.find(block2, ktag)
        -- if both adjacent override blocks have \k , let them be
        "{#{block1}}#{breakChar}{#{block2}}"
      else
        -- either one or both override blocks don't have \k , so combine them into one override block
        "{#{block1}#{block2}}"

  text = string.gsub text, breakChar, "" -- removes all char(1) we inserted

  -- Move first \k tag in override blocks to the front
  text = string.gsub(text, "{([^{}]-)(" .. ktag .. ")(.-)}", "{%2%1%3}")

  -- For some reasons if one override block has more than one \k tag, push those to behind the first \k tag (which has been pushed to front already)
  repeat
    return if aegisub.progress.is_cancelled()
    text, replaced = string.gsub(text, "{([^{}]-)(#{ktag})(\\[^kK][^}]-)(#{ktag})(.-)}", "{%1%2%4%3%5}")
  until replaced == 0

  -- Move to the front all tags that affect the whole line (i.e. not affected by their positions in the line)
  linetags = ""
  first = (pattern) ->
    p_s, _, p_tag = string.find text, pattern
    if p_s then
      text = string.gsub(text, pattern, "")
      linetags = linetags .. p_tag
  firstoftwo = (pattern1, pattern2) ->
    p1_s, _, p1_tag = string.find(text, pattern1)
    p2_s, _, p2_tag = string.find(text, pattern2)
    text = string.gsub(text, pattern1, "")
    text = string.gsub(text, pattern2, "")
    if p1_s and (not p2_s or p1_s < p2_s) then
      linetags = linetags .. p1_tag
    elseif p2_s then
      linetags = linetags .. p2_tag
  -- \an or \a
  first("(\\an?%d+)")
  -- \org
  first("(\\org%([^,%)]*,[^,%)]*%))")
  -- \move and \pos (the first one wins)
  firstoftwo("(\\move%([^,%)]*,[^,%)]*,[^,%)]*,[^,%)]*%))", "(\\pos%([^,%)]*,[^,%)]*%))")
  -- \fade and \fad (the first one wins)
  firstoftwo("(\\fade%([^,%)]*,[^,%)]*,[^,%)]*,[^,%)]*,[^,%)]*,[^,%)]*,[^,%)]*%))", "(\\fad%([^,%)]*,[^,%)]*%))")

  if linetags\len() > 0
    text = if string.sub(text, 1, 1) == "{"
      "{#{linetags}#{text\sub 2}"
    else
      "{#{linetags}}text"

  -- Remove any spaces within parenteses within override blocks except for \clip tags
  local comb = function(a,b,c,d,e)
    if (c ~= "\\clip" and c ~= "\\iclip") or d:sub(-1):find("[,%({]") or e:sub(1,1):find("[,%)}]") then return a..b..d..e
    else return a..b..d..string.char(2)..e end
    repeat
        text, replaced2 = string.gsub(text, "({[^}\\]*)([^}%s]*(\\[^%(}\\%s]*))%s*(%([^%s%)}]*)%s+([^}]*)", comb)
    until replaced2 == 0
    text, _ = text:gsub(string.char(2)," ")

  string.gsub text, "{%s*}", ""
