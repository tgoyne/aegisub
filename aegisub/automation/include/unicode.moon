﻿--[[
 Copyright (c) 2007, Niels Martin Hansen, Rodrigo Braz Monteiro
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.
   * Neither the name of the Aegisub Group nor the names of its contributors
     may be used to endorse or promote products derived from this software
     without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
]]

-- Unicode (UTF-8) support functions for Aegisub Automation 4 Lua
-- http://www.ietf.org/rfc/rfc2279.txt

module 'unicode'

-- Return the number of bytes occupied by the character starting at the i'th byte in s
charwidth = (s, i) ->
  b = s\byte(i or 1)
  if not b then
    --aegisub.debug.out(3, "unicode.charwidth of '%s' @ %d, nil byte\n", s, i)
    -- FIXME, something in karaskel results in this case, shouldn't happen
    -- What would "proper" behaviour be? Zero? Or just explode?
    1
  elseif b < 128 then
    1
  elseif b < 224 then
    2
  elseif b < 240 then
    3
  else
    4

-- Returns an iterator function for iterating over the characters in s
chars = (s) ->
  curchar, i = 0, 1
  ->
    return if i > s\len()

    width = charwidth(s, i)
    j = i
    curchar = curchar + 1
    i = i + width
    s\sub(j, i-1), curchar

-- Returns the number of characters in s
-- Runs in O(s:len()) time!
len = (s) ->
  n = 0
  for c in chars s
    n += 1
  n

-- Get codepoint of first char in s
codepoint = (s) ->
  -- Basic case, ASCII
  b = s\byte(1)
  return b if b < 128

  -- Use a naive decoding algorithm, and assume input is valid
  local res, w = 0

  if b < 224 then
    -- prefix byte is 110xxxxx
    res = b - 192
    w = 2
  elseif b < 240 then
    -- prefix byte is 11100000
    res = b - 224
    w = 3
  elseif b < 248 then
    -- prefix byte is 11110000
    res = b - 240
    w = 4
  elseif b < 252 then
    -- prefix byte is 11111000
    res = b - 248
    w = 5
  else
    -- prefix byte is 11111100
    res = b - 252
    w = 6

  for i = 2, w
    res = res*64 + s\byte(i) - 128
  res
