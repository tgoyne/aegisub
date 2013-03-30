 Copyright (c) 2007, 2010, Niels Martin Hansen, Rodrigo Braz Monteiro
 Copyright (c) 2013, Thomas Goyne <plorkyeran@aegisub.org>

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

include "utils.lua"
include "unicode.lua"

local *

tinsert = table.insert

-- bc breaks:
--   doesn't touch _G
--   fuck table.n
--   kara[0] is no longer a thing
--   drop use_classic_adv

karaskel =
  furigana_scale: 0.5
  collect_head: collect_head
  preproc_line: preproc_line
  preproc_line_text: preproc_line_text
  preproc_line_size: preproc_line_size
  preproc_line_pos: preproc_line_pos
  do_basic_layout: do_basic_layout
  do_furigana_layout: do_furigana_layout

-- Collect styles and metadata from the subs
collect_head = (subs, generate_furigana) ->
  meta = video_x_correct_factor: 1.0

  styles = {}

  for l in *subs
    aegisub.cancel() if aegisub.progress.is_cancelled()

    if l.class == 'style'
      styles[l.name] = l
      l.margin_v = l.margin_t -- convenience
    elseif l.class == 'info'
      meta[l.key\lower()] = l.value

  if generate_furigana
    make_furi_style (style) ->
      fs = table.copy style
      fs.fontsize *= karaskel.furigana_scale
      fs.outline *= karaskel.furigana_scale
      fs.shadow *= karaskel.furigana_scale
      fs.name ..= '-furigana'

    subs.append unpack [make_furi_style(s) for name, s in pairs styles
                        when not styles["#{name}-furigana"]
                        when not name\match 'furigana']

  -- Fix resolution data
  meta.res_x = math.floor(meta.playresx or 0)
  meta.res_y = math.floor(meta.playresy or 0)
  if meta.res_x == 0 and meta_res_y == 0
    meta.res_x = 384
    meta.res_y = 288
  elseif meta.res_x == 0
    -- This is braindead, but it's how TextSub does things...
    if meta.res_y == 1024
      meta.res_x = 1280
    else
      meta.res_x = meta.res_y / 3 * 4
  elseif meta.res_y == 0
    -- As if 1280x960 didn't exist
    if meta.res_x == 1280
      meta.res_y = 1024
    else
      meta.res_y = meta.res_x * 3 / 4

  video_x, video_y = aegisub.video_size()
  if video_y
    -- Correction factor for TextSub weirdness when render resolution does
    -- not match script resolution. Text pixels are considered square in
    -- render resolution rather than in script resolution, which is
    -- logically inconsistent. Correct for that.
    meta.video_x_correct_factor =
      (video_y / video_x) / (meta.res_y / meta.res_x)
  aegisub.debug.out(4, "Karaskel: Video X correction factor = %f\n\n", meta.video_x_correct_factor)

  meta, styles

_copy = (src, dst, fields) -> dst[f] = src[f] for f in *fields

-- Pre-process line, determining stripped text, karaoke data and splitting off furigana data
-- Modifies the object passed for line
preproc_line_text: (meta, styles, line) ->
  -- Assume line is class=dialogue
  kara = aegisub.parse_karaoke_data line
  line.kara = {}
  line.furi = {}

  line.text_stripped = ''
  line.duration = line.end_time - line.start_time

  worksyl = highlights: {}, furi: {}
  cur_inline_fx = ""
  for syl in *kara
    -- Detect any inline-fx tags
    inline_fx = syl.text\match '%{.*\\%-([^}\\]+)'
    cur_inline_fx = inline_fx if inline_fx

    -- Strip spaces (only basic ones, no fullwidth etc.)
    prespace, syltext, postspace = syl.text_stripped\match '^([ \t]*)(.-)([ \t]*)$'

    -- See if we've broken a (possible) multi-hl stretch
    -- If we did it's time for a new worksyl
    prefix = syltext\sub 1, unicode.charwidth syltext 1
    if prefix ~= '#' and prefix ~= '＃'
      tinsert line.kara, worksyl
      worksyl = highlights: {}, furi: {}

    -- Add highlight data
    tinsert worksyl.highlights,
      start_time: syl.start_time,
      end_time: syl.end_time,
      duration: syl.duration

    -- Detect furigana (both regular and fullwidth pipes work)
    -- Furigana is stored independantly from syllables
    if syltext\find('|') or syltext\find('｜')
      -- Replace fullwidth pipes, they aren't regex friendly
      syltext = syltext\gsub '｜', '|'
      -- Get before/after pipe text
      syltext, furitext = syltext\match '^(.-)|(.-)$'

      prefix = furitext\sub 1, unicode.charwidth(furitext, 1)

      -- Some of these may seem superflous, but a furi should ideally have the same "interface" as a syllable
      furi =
        syl: worksyl
        start_time: syl.start_time
        end_time: syl.end_time
        duration: syl.duration
        kdur: syl.duration / 10
        line: line
        tag: syl.tag
        inline_fx: cur_inline_fx
        i: line.kara.n
        prespace: ''
        postspace: ''
        highlights: {worksyl.highlight[#worksyl.highlights]}
        isfuri: true
        -- isbreak = Don't join this furi visually with previous furi, even if their main texts are adjacent
        isbreak: prefix == '!' or prefix == '！' or prefix == '<' or prefix == '＜'
        -- spillback = Allow this furi text to spill over the left edge of the main text
        -- (Furi is always allowed to spill over the right edge of main text.)
        spillback: prefix == '<' or prefix == '＜'

      -- Remove the prefix character from furitext, if there was one
      if furi.isbreak
        furitext = furitext\sub prefix\len()

      furi.text = furitext
      furi.text_stripped = furitext
      furi.text_spacestripped = furitext

      tinsert line.furi, furi
      tinsert worksyl.furi, furi

    -- Syllables that aren't part of a multi-highlight generate a new output-syllable
    if not worksyl.text or (prefix ~= '#' and prefix ~= '＃')
      -- Update stripped line-text
      line.text_stripped ..= prespace .. syltext .. postspace

      -- Copy data from syl to worksyl
      _copy syl, worksyl, ['text', 'duration', 'start_time', 'end_time', 'tag']
      worksyl.kdur = syl.duration / 10
      worksyl.line = line

      -- And add new data to worksyl
      worksyl.i = #line.kara
      worksyl.text_stripped = prespace .. syltext .. postspace -- be sure to include the spaces so the original line can be built from text_stripped
      worksyl.inline_fx = cur_inline_fx
      worksyl.text_spacestripped = syltext
      worksyl.prespace = prespace
      worksyl.postspace = postspace
    else
      -- This is just an extra highlight
      worksyl.duration += syl.duration
      worksyl.kdur = worksyl.kdur + syl.duration / 10
      worksyl.end_time = syl.end_time

  -- Add the last syllable
  tinsert line.kara, worksyl

-- Pre-calculate sizing information for the given line, no layouting is done
-- Modifies the object passed for line
preproc_line_size: (meta, styles, line) ->
    karaskel.preproc_line_text meta, styles, line unless line.kara

  -- Add style information
  if styles[line.style]
    line.styleref = styles[line.style]
  else
    aegisub.debug.out(2, "WARNING: Style not found: #{line.style}\n")
    line.styleref = next(styles) -- just pick an arbitrary one

  -- Calculate whole line sizing
  line.width, line.height, line.descent, line.extlead = aegisub.text_extents line.styleref, line.text_stripped
  line.width *= meta.video_x_correct_factor

  -- Calculate syllable sizing
  for syl in *line.kara
    syl.style = line.styleref
    syl.width, syl.height = aegisub.text_extents syl.style, syl.text_spacestripped
    syl.width *= meta.video_x_correct_factor
    syl.prespacewidth = aegisub.text_extents(syl.style, syl.prespace) * meta.video_x_correct_factor
    syl.postspacewidth = aegisub.text_extents(syl.style, syl.postspace) * meta.video_x_correct_factor

  -- Calculate furigana sizing
  if styles["#{line.style}-furigana"]
    line.furistyle = styles["#{line.style}-furigana"]
  else
    aegisub.debug.out(4, "No furigana style defined for style '#{line.style}'\n")
    line.furistyle = nil

  if line.furistyle
    for furi in *line.furi
      furi.style = line.furistyle
      furi.width, furi.height = aegisub.text_extents furi.style, furi.text
      furi.width *= meta.video_x_correct_factor
      furi.prespacewidth = 0
      furi.postspacewidth = 0


-- Layout a line, including furigana layout
-- Modifies the object passed for line
preproc_line_pos: (meta, styles, line) ->
  karaskel.preproc_line_size meta, styles, line unless line.styleref

  -- Syllable layouting must be done before the rest, since furigana layout may change the total width of the line
  if line.furistyle
    karaskel.do_furigana_layout meta, styles, line
  else
    karaskel.do_basic_layout meta, styles, line

  -- Effective margins
  line.margin_v = line.margin_t
  for field in *['margin_l', 'margin_r', 'margin_t', 'margin_b', 'margin_v']
    line["eff_#{field}"] = if line[field] > 0 then line[field] else line.styleref[field]

  -- And positioning
  with line
    switch .styleref.align % 3
      when 1 -- Left aligned
        .left = .eff_margin_l
        .center = .left + .width / 2
        .right = .left + .width
        .x = .left
        .halign = "left"
      when 2 -- Centered
        .left = (meta.res_x - .eff_margin_l - .eff_margin_r - .width) / 2 + .eff_margin_l
        .center = .left + .width / 2
        .right = .left + .width
        .x = .center
        .halign = "center"
      when 0 -- Right aligned
        .left = meta.res_x - .eff_margin_r - .width
        .center = .left + .width / 2
        .right = .left + .width
        .x = .right
        .halign = "right"
    .hcenter = .center
    switch math.floor (.styleref.align - 1) / 3
      when 0 -- Bottom aligned
        .bottom = meta.res_y - .eff_margin_b
        .middle = .bottom - .height / 2
        .top = .bottom - .height
        .y = .bottom
        .valign = "bottom"
      when 1 -- Mid aligned
        .top = (meta.res_y - .eff_margin_t - .eff_margin_b - .height) / 2 + .eff_margin_t
        .middle = .top + .height / 2
        .bottom = .top + .height
        .y = .middle
        .valign = "middle"
      when 2
        -- Top aligned
        .top = .eff_margin_t
        .middle = .top + .height / 2
        .bottom = .top + .height
        .y = .top
        .valign = "top"
    .vcenter = .middle

-- Do simple syllable layouting (no furigana)
do_basic_layout: (meta, styles, line) ->
  curx = 0
  for syl in *line.kara
    with syl
      .left   = curx + .prespacewidth
      .center = .left + .width / 2
      .right  = .left + .width
      curx += .prespacewidth + .width + .postspacewidth

_layout_groups (line) ->
  -- Start by building layout groups
  -- Two neighboring syllables with furigana that join together are part of the same layout group
  -- A forced split creates a new layout group
  lgroups = {}
  -- Start-sentinel
  lgsentinel = basewidth: 0, furiwidth: 0, syls: {}, furi: {}, spillback: false, left: 0, right: 0
  tinsert lgroups, lgsentinel
  -- Create groups
  last_had_furi = false
  lg = table.copy lgsentinel
  for syl in *line.kara
    -- Furigana-less syllables always generate a new layout group
    -- So do furigana-endowed syllables that are marked as split
    -- But if current lg has no width (usually only first) don't create a new
    if (#syl.furi == 0 or syl.furi[1].isbreak or not last_had_furi) and lg.basewidth > 0
      tinsert lgroups, lg
      lg = table.copy lgsentinel
      last_had_furi = false

    -- Add this syllable to lg
    lg.basewidth += syl.prespacewidth + syl.width + syl.postspacewidth
    tinsert lg.syls, syl

    for furi in *syl.furi
      lg.furiwidth += furi.width
      lg.spillback or= furi.spillback
      tinsert lg.furi, furi
      last_had_furi = true

  tinsert lgroups, lg
  tinsert lgroups, lgsentinel
  lgroups

-- Do advanced furigana layout algorithm
do_furigana_layout: (meta, styles, line) ->
  lgroups = _layout_groups line

  -- Layout the groups at macro-level
  -- Skip sentinel at ends in loop
  curx = 0
  for i = 2, #lgroups-1 do
    lg = lgroups[i]
    prev = lgroups[i-1]

    -- Three cases: No furigana, furigana smaller than base and furigana larger than base
    if lg.furiwidth == 0
      -- Here wa can basically just place the base text
      lg.left = curx
      lg.right = lg.left + lg.basewidth
      -- If there was any spillover from a previous group, add it to here
      if prev.rightspill > 0
        lg.rightspill = prev.rightspill - lg.basewidth
        prev.rightspill = 0
      curx += lg.basewidth

    elseif lg.furiwidth <= lg.basewidth
      -- If there was any rightspill from previous group, we have to stay 100% clear of that
      if prev.rightspill > 0
        curx += prev.rightspill
        prev.rightspill = 0
      lg.left = curx
      lg.right = lg.left + lg.basewidth
      curx += lg.basewidth
      -- Negative spill here
      lg.leftspill = (lg.furiwidth - lg.basewidth) / 2
      lg.rightspill = lg.leftspill

    else
      -- Furigana is wider than base, we'll have to spill in some direction
      if prev.rightspill > 0
        curx += prev.rightspill
        prev.rightspill = 0
      -- Do we spill only to the right or in both directions?
      if lg.spillback
        -- Both directions
        lg.leftspill = (lg.furiwidth - lg.basewidth) / 2
        lg.rightspill = lg.leftspill
        -- If there was any furigana or spill on previous syllable we can't overlap it
        lg.left = if prev.rightspill then curx + lg.leftspill else curx
        --- fffff
        -- nil vs 0 is used?
      else
        -- Only to the right
        lg.leftspill = 0
        lg.rightspill = lg.furiwidth - lg.basewidth
        lg.left = curx
      lg.right = lg.left + lg.basewidth
      curx = lg.right

  -- Now the groups are layouted, so place the individual syllables/furigana
  for lg in *lgroups
    basecenter = (lg.left + lg.right) / 2 -- centered furi is centered over this
    curx = lg.left -- base text is placed from here on

    -- Place base syllables
    for syl in *lg.syls
      syl.left = curx + syl.prespacewidth
      syl.center = syl.left + syl.width/2
      syl.right = syl.left + syl.width
      curx = syl.right + syl.postspacewidth

    line.width = curx if curx > line.width

    -- Place furigana
    curx = if lg.furiwidth < lg.basewidth or lg.spillback
      lg.left + (lg.basewidth - lg.furiwidth) / 2 -- Center over group
    else
      lg.left -- Left aligned

    for furi in *lg.furi
      furi.left = curx
      furi.center = furi.left + furi.width/2
      furi.right = furi.left + furi.width
      curx = furi.right


-- Precalc some info on a line
-- Modifies the line parameter
preproc_line: (subs, meta, styles, line) ->
  -- subs parameter is never used and probably won't ever be
  -- (it wouldn't be fun if some lines suddenly changed index here)
  -- pass whatever you want, but be careful calling preproc_line_pos directly, that interface might change
  karaskel.preproc_line_pos meta, styles, line


-- An actual "skeleton" function
-- Parses the first word out of the Effect field of each dialogue line and runs "fx_"..effect on that line
-- Lines with empty Effect field run fx_none
-- Lines with unimplemented effects are left alone
-- If the effect function returns true, the original line is kept in output,
-- otherwise the original line is converted to a comment
-- General prototype of an fx function: function(subs, meta, styles, line, fxdata)
-- fxdata are extra data after the effect name in the Effect field
local fx_library_registered = false
use_fx_library_furi: (use_furigana, macrotoo) ->
  fx_library_main = (subs) ->
    aegisub.progress.task("Collecting header info")
    meta, styles = karaskel.collect_head(subs, use_furigana)

    aegisub.progress.task("Processing subs")
    local i, maxi = 1, #subs
    while i <= maxi do
      aegisub.progress.set(i/maxi*100)
      l = subs[i]

      if l.class == "dialogue"
        aegisub.progress.task(l.text)
        karaskel.preproc_line(subs, meta, styles, l)
        keep = true
        fx, fxdata = string.headtail(l.effect)
        if fx == "" then fx = "none"
        if _G["fx_" .. fx]
          -- note to casual readers: _G is a special global variable that points to the global environment
          -- specifically, _G["_G"] == _G
          keep = _G["fx_" .. fx](subs, meta, styles, l, fxdata)
        unless keep
          l = subs[i]
          l.comment = true
          subs[i] = l

      i += 1

  return if fx_library_registered
  aegisub.register_filter(script_name or "fx_library", script_description or "Apply karaoke effects (fx_library skeleton)", 2000, fx_library_main)

  if macrotoo
    fxlibmacro = (subs) ->
      fx_library_main subs
      aegisub.set_undo_point script_name or "karaoke effect"
    aegisub.register_macro(script_name or "fx_library", script_description or "Apply karaoke effects (fx_library skeleton)", fxlibmacro)

use_fx_library: (macrotoo) ->
  karaskel.use_fx_library_furi(false, macrotoo)
