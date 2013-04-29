-- Copyright (c) 2013, Thomas Goyne <plorkyeran@aegisub.org>
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

re = require 'aegisub.re'

BLOCK_REGEX = re.compile '({[^}]*})([^{]+)'
KTAG_RE = re.compile '\\\\[kK]fo?\\d+'

ktag = "\\[kK][fo]?%d+"

has_ktag = (str) -> nil != KTAG_RE\find str

process_line: (text) ->
  -- Extract the override blocks
  blocks = for str in re.gsplit '{'
    parts = re.split '}', false, 1
    if #parts == 1 -- No closing } so the whole thing is plain text
      ovr: '', text: parts[1]
    elseif parts[1]\find '\\' -- override block and following text
      ovr: parts[1], text: parts[2]
    else -- comment
      ovr: '', text: '{' + str

  -- Consolitdate adjacent override blocks which don't both contain ktags
  -- Turns {\k1}{\b1}foo into {}{\k1\b1}foo
  acc = ''
  for i = 1, #blocks
    cur = blocks[i]
    next = if i < #blocks then blocks[i + 1] else nil
    if next and cur.text == '' and (not has_ktag(cur.ovr) or not has_ktag(next.ovr))
      acc ..= cur.ovr
      cur.ovr = ''
    else
      cur.ovr = acc .. cur.ovr
      acc = ''

  -- Drop empty override blocks (some of which may have been made above)
  blocks = [block for block in *blocks when block.text != '' or block.ovr != '']

  -- Move ktags to the front of their override blocks
  for block in *blocks
    ktags = ''
    other = ''
    last = 1
    for ktag in KTAG_RE.gfind block.ovr
      if last < ktag.first
        other ..= block.ovr\sub last, ktag.first - 1
      ktags ..= ktag.str
      last = ktag.last + 1
    block.ovr = ktags .. other

  -- Move all line-global tags to the beginning of the line and drop any
  -- redundant ones

  -- Build the final output string
  table.concat [if block.ovr == '' then block.text else "{#{block.ovr}}#{block.text}" for block in blocks]

  -- Move to the front all tags that affect the whole line (i.e. not affected by their positions in the line)
  linetags = ''
  first = (pattern) ->
    p_s, _, p_tag = string.find text, pattern
    if p_s then
      text = string.gsub(text, pattern, '')
      linetags = linetags .. p_tag
  firstoftwo = (pattern1, pattern2) ->
    p1_s, _, p1_tag = string.find(text, pattern1)
    p2_s, _, p2_tag = string.find(text, pattern2)
    text = string.gsub(text, pattern1, '')
    text = string.gsub(text, pattern2, '')
    if p1_s and (not p2_s or p1_s < p2_s) then
      linetags = linetags .. p1_tag
    elseif p2_s then
      linetags = linetags .. p2_tag
  -- \an or \a
  first('(\\an?%d+)')
  -- \org
  first('(\\org%([^,%)]*,[^,%)]*%))')
  -- \move and \pos (the first one wins)
  firstoftwo('(\\move%([^,%)]*,[^,%)]*,[^,%)]*,[^,%)]*%))', '(\\pos%([^,%)]*,[^,%)]*%))')
  -- \fade and \fad (the first one wins)
  firstoftwo('(\\fade%([^,%)]*,[^,%)]*,[^,%)]*,[^,%)]*,[^,%)]*,[^,%)]*,[^,%)]*%))', '(\\fad%([^,%)]*,[^,%)]*%))')

  if linetags\len() > 0
    text = if string.sub(text, 1, 1) == '{'
      "{#{linetags}#{text\sub 2}"
    else
      "{#{linetags}}text"

  -- Remove any spaces within parenteses within override blocks except for \clip tags
  comb = (a,b,c,d,e) ->
    if (c != '\\clip' and c != '\\iclip') or d\sub(-1):find('[,%({]') or e:sub(1,1):find('[,%)}]') then return a..b..d..e
    else return a..b..d..string.char(2)..e end
    repeat
        text, replaced2 = string.gsub(text, '({[^}\\]*)([^}%s]*(\\[^%(}\\%s]*))%s*(%([^%s%)}]*)%s+([^}]*)', comb)
    until replaced2 == 0
    text, _ = text\gsub string.char(2), ' '

  string.gsub text, '{%s*}', ''
