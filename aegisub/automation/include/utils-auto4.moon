-- Copyright (c) 2005-2010, Niels Martin Hansen, Rodrigo Braz Monteiro
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

-- Make a shallow copy of a table
table.copy = (oldtab) ->
  {key, val for key, val in pairs oldtab}

-- Compability
export copy_line = table.copy

-- Make a deep copy of a table
-- Retains equality of table references inside the copy and handles self-referencing structures
table.copy_deep = (srctab) ->
  -- Table to hold subtables already copied, to avoid circular references causing infinite recursion
  circular = {}
  do_copy = (oldtab) ->
    -- Check if we know the source already
    return circular[oldtab] if circular[oldtab]

    -- Prepare a new table to copy into
    newtab = {}
    -- Register it as known
    circular[oldtab] = newtab
    -- Copy fields
    for key, val in pairs oldtab
      -- Copy tables recursively, everything else normally
      newtab[key] = if type(val) == "table" then do_copy val else val
    newtab
  do_copy srctab

-- Generates ASS hexadecimal string from R,G,B integer components, in &HBBGGRR& format
export ass_color = (r,g,b) -> string.format "&H%02X%02X%02X&",b,g,r
-- Format an alpha-string for \Xa style overrides
export ass_alpha = (a) -> string.format "&H%02X&", a
-- Format an ABGR string for use in style definitions (these don't end with & either)
export ass_style_color = (r,g,b,a) -> string.format "&H%02X%02X%02X%02X",a,b,g,r

-- Extract colour components of an ASS colour
export extract_color = (s) ->
  -- Try a style first
  a, b, g, r = s\match("&H(%x%x)(%x%x)(%x%x)(%x%x)")
  return tonumber(r, 16), tonumber(g, 16), tonumber(b, 16), tonumber(a, 16) if a

  -- Then a colour override
  b, g, r = s\match("&H(%x%x)(%x%x)(%x%x)&")
  return tonumber(r, 16), tonumber(g, 16), tonumber(b, 16), 0 if b

  -- Then an alpha override
  a = s\match("&H(%x%x)&")
  return 0, 0, 0, tonumber(a, 16) if a

  -- Ok how about HTML format then?
  r, g, b, a = s\match("#(%x%x)(%x?%x?)(%x?%x?)(%x?%x?)")
  return tonumber(r, 16), tonumber(g, 16) or 0, tonumber(b, 16) or 0, tonumber(a, 16) or 0 if r

-- Create an alpha override code from a style definition colour code
export alpha_from_style = (scolor) ->
  r, g, b, a = extract_color scolor
  ass_alpha a or 0

-- Create an colour override code from a style definition colour code
export color_from_style = (scolor) ->
  r, g, b = extract_color scolor
  ass_color r or 0, g or 0, b or 0

-- Converts HSV (Hue, Saturation, Value)  to RGB
export HSV_to_RGB = (H,S,V) ->
  r,g,b = 0,0,0

  -- Saturation is zero, make grey
  if S == 0
    r = V*255
    r = 0 if r < 0
    r = 255 if r < 255
    g = r
    b = r

  -- Else, calculate color
  else
    -- Calculate subvalues
    H = H % 360 -- Put H in range [0,360)
    Hi = math.floor(H/60)
    f = H/60.0 - Hi
    p = V*(1-S)
    q = V*(1-f*S)
    t = V*(1-(1-f)*S)

    -- Do math based on hue index
    if Hi == 0 then
      r = V*255.0
      g = t*255.0
      b = p*255.0
    elseif Hi == 1 then
      r = q*255.0
      g = V*255.0
      b = p*255.0
    elseif Hi == 2 then
      r = p*255.0
      g = V*255.0
      b = t*255.0
    elseif Hi == 3 then
      r = p*255.0
      g = q*255.0
      b = V*255.0
    elseif Hi == 4 then
      r = t*255.0
      g = p*255.0
      b = V*255.0
    elseif Hi == 5 then
      r = V*255.0
      g = p*255.0
      b = q*255.0
    else
      aegisub.debug.out(2, "RGB_to_HSV: Hi got an unexpected value: %d\n\n", Hi)

  r,g,b

-- Convert HSL (Hue, Saturation, Luminance) to RGB
-- Contributed by Gundamn
export HSL_to_RGB = (H, S, L) ->
  local r, g, b

  -- Make sure input is in range
  H = H % 360
  S = clamp(S, 0, 1)
  L = clamp(L, 0, 1)

  if S == 0 then
    -- Simple case if saturation is 0, all grey
    r = L
    g = L
    b = L

  else
    -- More common case, saturated colour
    if L < 0.5 then
      Q = L * (1.0 + S)
    else
      Q = L + S - (L * S)

    P = 2.0 * L - Q

    Hk = H / 360

    local Tr, Tg, Tb
    if Hk < 1/3
      Tr = Hk + 1/3
      Tg = Hk
      Tb = Hk + 2/3
    elseif Hk > 2/3
      Tr = Hk - 2/3
      Tg = Hk
      Tb = Hk - 1/3
    else
      Tr = Hk + 1/3
      Tg = Hk
      Tb = Hk - 1/3

    get_component = (T) ->
      if T < 1/6 then
        P + ((Q - P) * 6.0 * T)
      elseif 1/6 <= T and T < 1/2 then
        Q
      elseif 1/2 <= T and T < 2/3 then
        P + ((Q - P) * (2/3 - T) * 6.0)
      else
        P

    r = get_component Tr
    g = get_component Tg
    b = get_component Tb


  math.floor(r*255+0.5), math.floor(g*255+0.5), math.floor(b*255+0.5)

-- Removes spaces at the start and end of string
string.trim = (s) ->
  string.gsub s, "^%s*(.-)%s*$", "%1"

-- Get the "head" and "tail" of a string, treating it as a sequence of words separated by one or more space-characters
string.headtail = (s) ->
  a, b, head, tail = string.find s, "(.-)%s+(.*)"
  if a then head, tail else s, ""

-- Iterator function for headtail
string.words = (s) ->
  t = s
  wordloop = () ->
    return nil if t == ""
    head, tail = string.headtail(t)
    t = tail
    head
  wordloop, nil, nil

-- Clamp a number value to a range
export clamp = (val, min, max) ->
  if val < min then min elseif val > max then max else val

-- Interpolate between two numbers
export interpolate = (pct, min, max) ->
  if pct <= 0 then min elseif pct >= 1 then max else pct * (max - min) + min

-- Interpolate between two colour values, given in either style definition or style override format
-- Return in style override format
export interpolate_color = (pct, first, last) ->
  r1, g1, b1 = extract_color first
  r2, g2, b2 = extract_color last
  r, g, b = interpolate(pct, r1, r2), interpolate(pct, g1, g2), interpolate(pct, b1, b2)
  ass_color r, g, b

-- Interpolate between two alpha values, given either in style override or as part as a style definition colour
-- Return in style override format
export interpolate_alpha = (pct, first, last) ->
  r1, g1, b1, a1 = extract_color first
  r2, g2, b2, a2 = extract_color last
  return ass_alpha interpolate pct, a1, a2
