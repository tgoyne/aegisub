--[[
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

-- Aegisub Automation 4 Lua karaoke templater tool
-- Parse and apply a karaoke effect written in ASS karaoke template language
-- See help file and wiki for more information on this

local tr = aegisub.gettext

export script_name = tr'Karaoke Templater'
export script_description = tr'Macro and export filter to apply karaoke effects using the template language'
export script_author = 'Niels Martin Hansen'
export script_version = '2.1.7'


include 'karaskel.lua'


-- Find and parse/prepare all karaoke template lines
parse_templates = (meta, styles, subs) ->
  templates = once: {}, line: {}, syl: {}, char: {}, furi: {}, styles: {}
  to_delete = {}
  num_subs = #subs
  for i = 1, num_sums
    aegisub.progress.set (i-1) / num_subs * 100
    l = subs[i]

    if l.class == "dialogue" and l.comment
      fx, mods = string.headtail l.effect
      fx = fx\lower()
      if fx == "code"
        parse_code meta, styles, l, templates, mods
      elseif fx == "template"
        parse_template meta, styles, l, templates, mods
      templates.styles[l.style] = true
    elseif l.class == "dialogue" and l.effect == "fx"
      -- this is a previously generated effect line, remove it
      tinsert to_delete, i
  aegisub.progress.set 100
  subs.delete unpack to_delete
  templates

parse_code = (meta, styles, line, templates, mods) ->
  template = code: line.text, loops: 1, style: line.style
  inserted = false

  while mods ~= ''
    m, mods = string.headtail mods
    switch m\lower()
      when 'once'
        tinsert(templates.once, template)
        inserted = true
      when 'line'
        tinsert(templates.line, template)
        inserted = true
      when 'syl'
        tinsert(templates.syl, template)
        inserted = true
      when 'furi'
        tinsert(templates.furi, template)
        inserted = true
      when 'all'
        template.style = nil
      when 'noblank'
        template.noblank = true
      when 'repeat', 'loop'
        times, t = string.headtail rest
        template.loops = tonumber times
        if not template.loops
          aegisub.debug.out(3, 'Failed reading this repeat-count to a number: %s\nIn template code line: %s\nEffect field: %s\n\n', times, line.text, line.effect)
          template.loops = 1
        else
          rest = t
      else
        aegisub.debug.out(3, 'Unknown modifier in code template: %s\nIn template code line: %s\nEffect field: %s\n\n', m, line.text, line.effect)

  if not inserted then
    aegisub.debug.out(5, 'Found implicit run-once code line: %s\n', line.text)
    tinsert(templates.once, template)

-- List of reserved words that can't be used as 'line' template identifiers
template_modifiers = {
  'pre-line', 'line', 'syl', 'furi', 'char', 'all', 'repeat', 'loop',
  'notext', 'keeptags', 'noblank', 'multi', 'fx', 'fxgroup'
}

parse_template = (meta, styles, line, templates, mods) ->
  template =
    t: ""
    pre: ""
    style: line.style
    loops: 1
    layer: line.layer
    addtext: true
    keeptags: false
    multi: false
    isline: false
    perchar: false
    noblank: false

  inserted = false

  while mods ~= "" do
    m, t = string.headtail mods
    mods = t
    m = m\lower()
    if (m == "pre-line" or m == "line") and not inserted
      aegisub.debug.out(5, "Found line template '%s'\n", line.text)
      -- should really fail if already inserted
      id, t = string.headtail mods
      id = id\lower()

      -- check that it really is an identifier and not a keyword
      for kw in *template_modifiers
        if id == kw
          id = nil
          break
      if id == ""
        id = nil
      if id
        rest = t

      -- get old template if there is one
      if id and templates.line[id]
        template = templates.line[id]
      elseif id then
        template.id = id
        templates.line[id] = template
      else
        tinsert(templates.line, template)
      inserted = true
      template.isline = true

      -- apply text to correct string
      if m == "line" then
        template.t ..= line.text
      else -- must be pre-line
        template.pre ..= line.text
    elseif m == "syl" and not template.isline then
      tinsert(templates.syl, template)
      inserted = true
    elseif m == "furi" and not template.isline then
      tinsert(templates.furi, template)
      inserted = true
    elseif (m == "pre-line" or m == "line") and inserted then
      aegisub.debug.out(2, "Unable to combine %s class templates with other template classes\n\n", m)
    elseif (m == "syl" or m == "furi") and template.isline then
      aegisub.debug.out(2, "Unable to combine %s class template lines with line or pre-line classes\n\n", m)
    elseif m == "all" then
      template.style = nil
    elseif m == "repeat" or m == "loop" then
      local times, t = string.headtail(rest)
      template.loops = tonumber(times)
      if not template.loops then
        aegisub.debug.out(3, "Failed reading this repeat-count to a number: %s\nIn template line: %s\nEffect field: %s\n\n", times, line.text, line.effect)
        template.loops = 1
      else
        rest = t
    elseif m == "notext" then
      template.addtext = false
    elseif m == "keeptags" then
      template.keeptags = true
    elseif m == "multi" then
      template.multi = true
    elseif m == "char" then
      template.perchar = true
    elseif m == "noblank" then
      template.noblank = true
    elseif m == "fx" then
      local fx, t = string.headtail(rest)
      if fx ~= "" then
        template.fx = fx
        rest = t
      else
        aegisub.debug.out(3, "No fx name following fx modifier\nIn template line: %s\nEffect field: %s\n\n", line.text, line.effect)
        template.fx = nil
    elseif m == "fxgroup" then
      local fx, t = string.headtail(rest)
      if fx ~= "" then
        template.fxgroup = fx
        rest = t
      else
        aegisub.debug.out(3, "No fxgroup name following fxgroup modifier\nIn template linee: %s\nEffect field: %s\n\n", line.text, line.effect)
        template.fxgroup = nil
    else
      aegisub.debug.out(3, "Unknown modifier in template: %s\nIn template line: %s\nEffect field: %s\n\n", m, line.text, line.effect)

  if not inserted then
    tinsert(templates.syl, template)
  if not template.isline then
    template.t = line.text

-- Iterator function, return all templates that apply to the given line
matching_templates = (templates, line, tenv) ->
  lastkey = nil
  test_next = ->
    k, t = next(templates, lastkey)
    lastkey = k
    return unless t
    if (t.style == line.style or not t.style) and
        (not t.fxgroup or
        (t.fxgroup and tenv.fxgroup[t.fxgroup] ~= false)) then
      t
    else
      test_next()
  test_next

-- Iterator function, run a loop using tenv.j and tenv.maxj as loop controllers
template_loop = (tenv, initmaxj) ->
  oldmaxj = initmaxj
  tenv.maxj = initmaxj
  tenv.j = 0
  ->
    return if tenv.j >= tenv.maxj or aegisub.progress.is_cancelled()

    tenv.j += 1
    if oldmaxj ~= tenv.maxj
      aegisub.debug.out(5, "Number of loop iterations changed from %d to %d\n", oldmaxj, tenv.maxj)
      oldmaxj = tenv.maxj
    tenv.j, tenv.maxj


-- Apply the templates
apply_templates = (meta, styles, subs, templates) ->
  -- the environment the templates will run in
  tenv =
    meta: meta
    string: string
    math: math
    _G: _G
  tenv.tenv = tenv

  -- Define helper functions in tenv

  tenv.retime = (mode, addstart=0, addend=0) ->
    line, syl = tenv.line, tenv.syl
    newstart, newend = line.start_time, line.end_time
    switch mode
      when 'syl'
        newstart = line.start_time + syl.start_time + addstart
        newend = line.start_time + syl.end_time + addend
      when 'presyl'
        newstart = line.start_time + syl.start_time + addstart
        newend = line.start_time + syl.start_time + addend
      when 'postsyl'
        newstart = line.start_time + syl.end_time + addstart
        newend = line.start_time + syl.end_time + addend
      when 'line'
        newstart = line.start_time + addstart
        newend = line.end_time + addend
      when 'preline'
        newstart = line.start_time + addstart
        newend = line.start_time + addend
      when 'postline'
        newstart = line.end_time + addstart
        newend = line.end_time + addend
      when 'start2syl'
        newstart = line.start_time + addstart
        newend = line.start_time + syl.start_time + addend
      when 'syl2end'
        newstart = line.start_time + syl.end_time + addstart
        newend = line.end_time + addend
      when 'set', 'abs'
        newstart = addstart
        newend = addend
      when 'sylpct'
        newstart = line.start_time + syl.start_time + addstart*syl.duration/100
        newend = line.start_time + syl.start_time + addend*syl.duration/100
    -- wishlist: something for fade-over effects,
    -- "time between previous line and this" and
    -- "time between this line and next"
    line.start_time = newstart
    line.end_time = newend
    line.duration = newend - newstart
    ''

  tenv.fxgroup = {}

  tenv.relayer = (layer) ->
    tenv.line.layer = layer
    ""

  tenv.restyle = (style) ->
    tenv.line.style = style
    tenv.line.styleref = styles[style]
    ""

  tenv.maxloop = (newmaxj) ->
    tenv.maxj = newmaxj
    ""

  tenv.maxloops = tenv.maxloop
  tenv.loopctl = (newj, newmaxj) ->
    tenv.j = newj
    tenv.maxj = newmaxj
    ""

  tenv.recall = {}
  tenv.recall = setmetatable({},
    decorators: {},
    decorator_line: (name) -> "_#{tenv.origline}_#{name}"
    decorator_syl: (name) -> "_#{tenv.syl}_#{name}"
    decorator_basesyl: (name) -> "_#{tenv.basesyl}_#{name}"
    __call: (tab, name, default) ->
      decorator = getmetatable(tab).decorators[name]
      if decorator
        name = decorator tostring name
      aegisub.debug.out(5, "Recalling '%s'\n", name)
      tab[name] or default

  tenv.remember = (name, value, decorator) ->
    getmetatable(tenv.recall).decorators[name] = decorator
    if decorator then
      name = decorator tostring name
    aegisub.debug.out(5, "Remembering '%s' as '%s'\n", name, tostring(value))
    tenv.recall[name] = value
    value
  tenv.remember_line = (name, value) ->
    tenv.remember(name, value, getmetatable(tenv.recall).decorator_line)
  tenv.remember_syl = (name, value) ->
    tenv.remember(name, value, getmetatable(tenv.recall).decorator_syl)
  tenv.remember_basesyl = (name, value) ->
    tenv.remember(name, value, getmetatable(tenv.recall).decorator_basesyl)
  tenv.remember_if = (name, value, condition, decorator) ->
    if condition
      tenv.remember(name, value, decorator)
    else
      value

  -- run all run-once code snippets
  for t in *templates.once
    assert(t.code, "WTF, a 'once' template without code?")
    run_code_template(t, tenv)

  -- start processing lines
  local i, n = 0, #subs
  while i < n do
    aegisub.progress.set(i/n*100)
    i += 1
    local l = subs[i]
    if l.class == "dialogue" and ((l.effect == "" and not l.comment) or l.effect:match("[Kk]araoke")) then
      l.i = i
      l.comment = false
      karaskel.preproc_line(subs, meta, styles, l)
      if apply_line(meta, styles, subs, l, templates, tenv) then
        -- Some templates were applied to this line, make a karaoke timing line of it
        l.comment = true
        l.effect = "karaoke"
        subs[i] = l

set_ctx_syl = (varctx, line, syl) ->
  varctx.sstart = syl.start_time
  varctx.send = syl.end_time
  varctx.sdur = syl.duration
  varctx.skdur = syl.duration / 10
  varctx.smid = syl.start_time + syl.duration / 2
  varctx.si = syl.i
  varctx.sleft = math.floor(line.left + syl.left+0.5)
  varctx.scenter = math.floor(line.left + syl.center+0.5)
  varctx.sright = math.floor(line.left + syl.right+0.5)
  varctx.swidth = math.floor(syl.width + 0.5)
  if syl.isfuri then
    varctx.sbottom = varctx.ltop
    varctx.stop = math.floor(varctx.ltop - syl.height + 0.5)
    varctx.smiddle = math.floor(varctx.ltop - syl.height/2 + 0.5)
  else
    varctx.stop = varctx.ltop
    varctx.smiddle = varctx.lmiddle
    varctx.sbottom = varctx.lbottom
  varctx.sheight = syl.height
  varctx.sx = switch line.halign
    when 'left'   then math.floor(line.left + syl.left + 0.5)
    when 'center' then math.floor(line.left + syl.center + 0.5)
    when 'right'  then math.floor(line.left + syl.right + 0.5)
  varctx.sy = varctx["s#{line.valign}"]

  for f in *['start', 'end', 'dur', 'kdur', 'mid', 'i', 'left', 'center', 'right', 'width', 'top', 'middle', 'bottom', 'height', 'x', 'y']
    varctx[f] = varctx['s' .. f]

apply_line = (meta, styles, subs, line, templates, tenv) ->
  -- Tell whether any templates were applied to this line, needed to know whether the original line should be removed from input
  applied_templates = false

  -- General variable replacement context
  varctx =
    lstart: line.start_time
    lend: line.end_time
    ldur: line.duration
    lmid: line.start_time + line.duration/2
    syln: #line.kara
    li: line.i

  varctx[f] = line[f] for f in *['layer', 'style', 'actor']
  varctx['l'..f] = math.floor line[f] for f in *['left', 'center', 'right', 'width', 'top', 'middle', 'bottom', 'height', 'x', 'y']
  varctx[f] = if line[f] > 0 then line[f] else line.styleref[f] for f in *['margin_l', 'margin_r', 'margin_t', 'margin_b', 'margin_v']

  tenv.orgline = line
  tenv.line = nil
  tenv.syl = nil
  tenv.basesyl = nil

  -- Apply all line templates
  aegisub.debug.out(5, "Running line templates\n")
  for t in matching_templates(templates.line, line, tenv) do
    break if aegisub.progress.is_cancelled()

    -- Set varctx for per-line variables
    varctx[f] = varctx['l'..f] for f in *['start', 'end', 'dur', 'mid', 'i', 'left', 'center', 'right', 'width', 'top', 'middle', 'bottom', 'height', 'x', 'y']
    varctx.kdur = math.floor(varctx.dur / 10)

    for j, maxj in template_loop(tenv, t.loops) do
      if t.code then
        aegisub.debug.out(5, "Code template, %s\n", t.code)
        tenv.line = line
        -- Although run_code_template also performs template looping this works
        -- by "luck", since by the time the first loop of this outer loop completes
        -- the one run by run_code_template has already performed all iterations
        -- and has tenv.j and tenv.maxj in a loop-ending state, causing the outer
        -- loop to only ever run once.
        run_code_template(t, tenv)
      else
        aegisub.debug.out(5, "Line template, pre = '%s', t = '%s'\n", t.pre, t.t)
        applied_templates = true
        local newline = table.copy(line)
        tenv.line = newline
        newline.layer = t.layer
        newline.text = ""
        if t.pre ~= "" then
          newline.text ..= run_text_template(t.pre, tenv, varctx)
        if t.t ~= "" then
          for i = 1, line.kara.n do
            local syl = line.kara[i]
            tenv.syl = syl
            tenv.basesyl = syl
            set_ctx_syl(varctx, line, syl)
            newline.text ..= run_text_template(t.t, tenv, varctx)
            if t.addtext then
              if t.keeptags then
                newline.text ..= syl.text
              else
                newline.text ..= syl.text_stripped
        else
          -- hmm, no main template for the line... put original text in
          if t.keeptags then
            newline.text ..= line.text
          else
            newline.text ..= line.text_stripped
        newline.effect = "fx"
        subs.append(newline)
  aegisub.debug.out(5, "Done running line templates\n\n")

  -- Loop over syllables
  for syl in *line.kara
    break if aegisub.progress.is_cancelled()

    aegisub.debug.out(5, "Applying templates to syllable: %s\n", syl.text)
    if apply_syllable_templates(syl, line, templates.syl, tenv, varctx, subs) then
      applied_templates = true

  -- Loop over furigana
  for furi in line.furi
    break if aegisub.progress.is_cancelled()

    aegisub.debug.out(5, "Applying templates to furigana: %s\n", furi.text)
    if apply_syllable_templates(furi, line, templates.furi, tenv, varctx, subs) then
      applied_templates = true

  applied_templates

run_code_template = (template, tenv) ->
  f, err = loadstring(template.code, "template code")
  if not f then
    aegisub.debug.out(2, "Failed to parse Lua code: %s\nCode that failed to parse: %s\n\n", err, template.code)
  else
    local pcall = pcall
    setfenv(f, tenv)
    for j, maxj in template_loop(tenv, template.loops) do
      res, err = pcall f
      if not res then
        aegisub.debug.out(2, "Runtime error in template code: %s\nCode producing error: %s\n\n", err, template.code)

run_text_template = (template, tenv, varctx) ->
  res = template
  aegisub.debug.out(5, "Running text template '%s'\n", res)

  -- Replace the variables in the string (this is probably faster than using a custom function, but doesn't provide error reporting)
  if varctx then
    aegisub.debug.out(5, "Has varctx, replacing variables\n")
    var_replacer = (varname) ->
      varname = string.lower varname
      aegisub.debug.out(5, "Found variable named '%s', ", varname)
      if varctx[varname] ~= nil then
        aegisub.debug.out(5, "it exists, value is '%s'\n", varctx[varname])
        return varctx[varname]
      else
        aegisub.debug.out(5, "doesn't exist\n")
        aegisub.debug.out(2, "Unknown variable name: %s\nIn karaoke template: %s\n\n", varname, template)
        return "$" .. varname
    res = string.gsub(res, "$([%a_]+)", var_replacer)
    aegisub.debug.out(5, "Done replacing variables, new template string is '%s'\n", res)

  -- Function for evaluating expressions
  expression_evaluator = (expression) ->
    f, err = loadstring "return (#{expression})"
    if (err) ~= nil then
      aegisub.debug.out(2, "Error parsing expression: %s\nExpression producing error: %s\nTemplate with expression: %s\n\n", err, expression, template)
      return "!#{expression}!"

    setfenv(f, tenv)
    res, val = pcall(f)
    return val if res

    aegisub.debug.out(2, "Runtime error in template expression: %s\nExpression producing error: %s\nTemplate with expression: %s\n\n", val, expression, template)
    "!#{expression}!"
  -- Find and evaluate expressions
  aegisub.debug.out(5, "Now evaluating expressions\n")
  res = string.gsub(res , "!(.-)!", expression_evaluator)
  aegisub.debug.out(5, "After evaluation: %s\nDone handling template\n\n", res)

  res

apply_syllable_templates = (syl, line, templates, tenv, varctx, subs) ->
  local applied = 0

  -- Loop over all templates matching the line style
  for t in matching_templates templates, line, tenv 
    break if aegisub.progress.is_cancelled()

    tenv.syl = syl
    tenv.basesyl = syl
    set_ctx_syl varctx, line, syl

    applied += apply_one_syllable_template(syl, line, t, tenv, varctx, subs, false, false)

  applied > 0

is_syl_blank = (syl) ->
  return true if syl.duration <= 0

  -- try to remove common spacing characters
  local t = syl.text_stripped
  return true if t\len() <= 0
  t = t\gsub("[ \t\n\r]", "") -- regular ASCII space characters
  t = t\gsub("　", "") -- fullwidth space
  t\len() <= 0

apply_one_syllable_template = (syl, line, template, tenv, varctx, subs, skip_perchar, skip_multi) ->
  return 0 if aegisub.progress.is_cancelled()
  t = template
  applied = 0

  aegisub.debug.out(5, "Applying template to one syllable with text: %s\n", syl.text)

  -- Check for right inline_fx
  if t.fx and t.fx ~= syl.inline_fx
    aegisub.debug.out(5, "Syllable has wrong inline-fx (wanted '%s', got '%s'), skipping.\n", t.fx, syl.inline_fx)
    return 0

  if t.noblank and is_syl_blank(syl)
    aegisub.debug.out(5, "Syllable is blank, skipping.\n")
    return 0

  -- Recurse to per-char if required
  if not skip_perchar and t.perchar
    aegisub.debug.out(5, "Doing per-character effects...\n")
    charsyl = table.copy syl
    tenv.syl = charsyl

    left, width = syl.left, 0
    for c in unicode.chars syl.text_stripped
      charsyl.text = c
      charsyl.text_stripped = c
      charsyl.text_spacestripped = c
      charsyl.prespace, charsyl.postspace = "", "" -- for whatever anyone might use these for
      width = aegisub.text_extents(syl.style, c)
      charsyl.left = left
      charsyl.center = left + width/2
      charsyl.right = left + width
      charsyl.prespacewidth, charsyl.postspacewidth = 0, 0 -- whatever...
      left += width
      set_ctx_syl(varctx, line, charsyl)

      applied += apply_one_syllable_template(charsyl, line, t, tenv, varctx, subs, true, false)

    return applied

  -- Recurse to multi-hl if required
  if not skip_multi and t.multi then
    aegisub.debug.out(5, "Doing multi-highlight effects...\n")
    local hlsyl = table.copy(syl)
    tenv.syl = hlsyl

    for hldata in *syl.highlights
      hlsyl[f] = hldata[f] for f in *['start_time', 'end_time', 'duration']
      set_ctx_syl varctx, line, hlsyl

      applied += apply_one_syllable_template(hlsyl, line, t, tenv, varctx, subs, true, true)

    return applied

  -- Regular processing
  if t.code then
    aegisub.debug.out(5, "Running code line\n")
    tenv.line = line
    run_code_template(t, tenv)
  else
    aegisub.debug.out(5, "Running %d effect loops\n", t.loops)
    for j, maxj in template_loop(tenv, t.loops) do
      local newline = table.copy(line)
      newline.styleref = syl.style
      newline.style = syl.style.name
      newline.layer = t.layer
      tenv.line = newline
      newline.text = run_text_template(t.t, tenv, varctx)
      if t.keeptags then
        newline.text ..= syl.text
      elseif t.addtext then
        newline.text ..= syl.text_stripped
      newline.effect = "fx"
      aegisub.debug.out(5, "Generated line with text: %s\n", newline.text)
      subs.append(newline)
      applied += 1

  applied


-- Main function to do the templating
filter_apply_templates = (subs, config) ->
  aegisub.progress.task tr"Collecting header data..."
  meta, styles = karaskel.collect_head(subs, true)

  aegisub.progress.task tr"Parsing templates..."
  templates = parse_templates(meta, styles, subs)

  aegisub.progress.task tr"Applying templates..."
  apply_templates(meta, styles, subs, templates)

macro_apply_templates = (subs, sel) ->
  filter_apply_templates(subs, ismacro: true, sel: sel)
  aegisub.set_undo_point tr"apply karaoke template"

macro_can_template = (subs) ->
  -- check if this file has templates in it, don't allow running the macro if it hasn't
  num_dia = 0
  for l in *subs
    if l.class == "dialogue"
      num_dia += 1
      -- test if the line is a template
      return true if (string.headtail(l.effect)):lower() == "template" then
      -- don't try forever, this has to be fast
      return false if num_dia > 50
  false

aegisub.register_macro(tr"Apply karaoke template", tr"Applies karaoke effects from templates", macro_apply_templates, macro_can_template)
aegisub.register_filter(tr"Karaoke template", tr"Apply karaoke effect templates to the subtitles.\n\nSee the help file for information on how to use this.", 2000, filter_apply_templates)
