-- Copyright (c) 2009 Muhammad Lukman Nasaruddin (ai-chan)
-- Copyright (c) 2013 Thomas Goyne <plorkyeran@aegisub.org>
--
-- Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
-- associated documentation files (the "Software"), to deal in the Software without restriction,
-- including without limitation the rights to use, copy, modify, merge, publish, distribute,
-- sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
-- furnished to do so, subject to the following conditions:
--
-- The above copyright notice and this permission notice shall be included in all copies or substantial
-- portions of the Software.
--
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
-- NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
-- NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
-- OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
-- CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

export script_name = 'Gradient Factory'
export script_description = 'Color gradient generator by ai-chan'
export script_author = 'Muhammad Lukman Nasaruddin (ai-chan)'
export script_version = '1.4'
export script_modified = '24 March 2012'

include 'karaskel.lua'
util = require 'util'

-- Util functions which should be extracted
update = (dst, src) ->
  dst[k] = v for k,v in pairs(src)
  dst

keys = (tbl) ->
  [k for k,v in pairs(tbl)]

gui =
  label: (args) -> update args, class: 'label'
  dropdown: (args) -> update args, class: 'dropdown'
  intedit: (args) -> update args, class: 'intedit'

  applyLayout: (layout, models) ->
    layout = table.deepCopy layout
    for control in *layout
      if control.name
        control[k] = v for k,v in pairs(models[control.name])
    layout

class ModalDialog
  close = false
  returnValue = nil

  close: (...) =>
    close = true
    returnValue = ...

  show: =>
    applyModel (model) ->
    while true
      controls = gui.applyLayout @layout, @model
      btn, config = aegisub.dialog.display controls, keys @events
      m.value = config[m.name] for m in @model
      @events[btn] btn
      if close
        return returnValue

class View
  getLayout: =>
    {}

class Model
  values = nil

  new: (initValues={}) =>
    values=initValues
    @initialize!

  get: (name) =>
    values[name] or @defaults[name]

  set: (name, value) =>
    values[name] = value

  viewModel: =>
    vmFields = {}
    viewModel = @fields()
    for name, field in pairs(viewModel)
      vmFields[name] = true
      field.value = values[name] or @Model.defaults[value]
    for name, value in pairs(values)
      unless name in vmFields
        vmFields[name] = true
        viewModel[name] = value: value
    for name, value in pairs(@defaults)
      unless name in vmFields
        vmFields[name] = true
        viewModel[name] = value: value


  fields: {}
  defaults: {}
  initialize: =>


-- User interface for configuring GradientFactory
class Dropdown extends View
  render: => {
    gui.label width: 3, label: @label
    gui.dropdown x: 3, width: 9, name: @name
  }

class IntEdit extends View
  render: => {
    gui.label width: 8, label: @label
    gui.intedit x: 8, width: 4, name: @name
  }

class ColorMode extends View
  render: => {
    gui.label label: @label
    gui.dropdown y: 1, name: @name
  }

class GradientFactoryViewModel extends Model
  fields: =>
    applyto:
      items: {}
    mode:
      items: {'Smooth', 'Smooth (Vertical)', 'By character', 'By syllable'}
    stripx:
      min: 1
      max: 100
    karamode:
      items: {'As Is', 'Strip karaoke', '\\k', '\\kf', '\\ko'}
    primary_mode:
      items: {}
    secondary_mode:
      items: {}
    outline_mode:
      items: {}
    shadow_mode:
      items: {}

  defaults:
    applyto: nil
    mode: 'Smooth'
    stripx: 10
    karamode: 'As Is'
    primary_mode: 'Ignore'
    secondary_mode: 'Ignore'
    outline_mode: 'Ignore'
    shadow_mode: 'Ignore'

class GradientFactoryGui
  colorkeys: {'primary', 'secondary', 'outline', 'shadow'}

  events:
    'Generate': (config) => @close config
    '+colors': @add_color_button
    '-colors': @remove_color_button
    'Cancel': => @close!

  new: =>
    @layout = {
      gui.label    y: 0, label: "#{script_name} #{script_version} by ai-chan (updated #{script_modified})"
      Dropdown     y: 1, name: 'applyto', label: 'Apply to'
      Dropdown     y: 2, name: 'mode', label: 'Mode'
      IntEdit      y: 3, name: 'stripx', label: 'Pixel per strip (for Smooth modes)'
      Dropdown     y: 2, name: 'karamode', label: 'Karaoke'
      ColorMode    x: 1, y: 5, label: 'Primary', 'primary_mode'
      ColorMode    x: 2, y: 5, label: 'Secondary', 'secondary_mode'
      ColorMode    x: 3, y: 5, label: 'Outline', 'outline_mode'
      ColorMode    x: 4, y: 5, label: 'Shadow', 'shadow_mode'
      gui.label    y: 6, label: 'Colors'
    }

    @model = GradientFactoryViewModel()

    @color_button_count = 0
    @add_color_button!
    @add_color_button!

  add_color_button: =>
    @color_button_count += 1
    @controls[#@controls + 1] =
      label: "Color #{@color_button_count}"
      class: 'label'
      x: 0
      y: @color_comp_count + 6

    for i = 1, 4
      @controls[#@controls + 1] =
        name: "#{@colorkeys[i]}_color#{@color_button_count}"
        class: 'color'
        x: i
        y: @color_comp_count + 6

  remove_color_button: =>
    return if @color_button_count <= 2
    @controls[#@controls] = nil for i = 1, 5

  window_buttons: =>
    if @color_button_count > 2
      {'Generate', '+colors', '-colors', 'Cancel'}
    else
      {'Generate', '+colors', 'Cancel'}

  display: =>
    while true
      cfg_res, config = aegisub.dialog.display @controls, @window_buttons()
      if cfg_res == 'Generate'
        return true
      if cfg_res == 'Cancel'
        return false

      @set_control_values config
      if cfg_res == '+colors'
        @add_color_button!
      else
        @remove_color_button!

process = (meta, styles, config, subtitles, selected_lines, active_line) ->
  @update_control_values(config)
  @last_run = os.time()
  scfg = @serialize_config(config)

  return unless colors = @_colors config

  math.randomseed os.time()

  -- Mode
  config.mode = {Smooth: 1, 'Smooth (Vertical)': 2 'By character': 3, 'By syllable': 4}[config.mode]

  -- karaoke tag function
  config.karatagfn = switch config.karamode
    when 'Strip karaoke' then -> ''
    when 'As Is' then (syl) -> string.format "{\\%s%d}", syl.tag, syl.kdur
    else (syl) -> string.format "{%s%d}", config.karamode, syl.kdur

  -- Get lines indexes
  valid_line = (i, pred) ->
    line = subtitles[i]
    line.class == 'dialogue' and not line.comment and not @has_gradient line and (not pred or pred line)

  applyto_type, applyto_more = string.headtail(config.applyto)
  subs = switch applyto_type
    when 'All'
      [subtitles[i] for i, #subtitles when valid_line i]
    when 'Selected'
      selected_lines
    when 'Style'
      _, style = string.headtail(applyto_more)
      [subtitles[i] for i, #subtitles when valid_line i, (l) -> l.style == style]
    when 'Actor'
      _, actor = string.headtail(applyto_more)
      [subtitles[i] for i, #subtitles when valid_line i, (l) -> l.actor == actor]

  -- process them
  configstored = false
  new_lines = 0
  count = 0

  for i in *subs do
    count += 1
    aegisub.progress.set(count * 100 / #subs)
    return if aegisub.progress.is_cancelled()

    line = subtitles[i + new_lines]

    if not configstored and line.class == 'info' and line.key == "GradientFactory"
      line.value = scfg
      subtitles[i + new_lines] = line
      configstored = true

    karaskel.preproc_line subtitles, meta, styles, line
    res = @do_line meta, styles, config, colors, line
    subtitles.insert(res, i + new_lines)
    new_lines += #res

    aegisub.progress.task("#{count} / #{#subs} lines processed")

  if not configstored
    subtitles.append
      class: "info"
      section: "Script Info"
      key: "GradientFactory"
      value: scfg

  true

gradient =
  new: =>
    @last_run = 0
    @color_comp_count = 0
    while @color_comp_count < 3
      @append_color_components!

  update_control_values: (config) =>
    for control in *@controls
      if control.name
        control.value = config[control.name]

  serialize_config: (config) =>
    config.last_run = @last_run
    config.color_comp_count = @color_comp_count
    util.serialize config

  unserialize_config: (scfg) =>
    config = util.deserialize scfg
    return if @last_run > config.last_run

    @last_run = config.last_run
    while @color_comp_count < config.color_comp_count
      @append_color_components!
    while @color_comp_count > config.color_comp_count
      @unappend_color_components!

  _colors: (config) =>
    colors = primary: {}, secondary: {}, outline: {}, shadow: {}
    for k = 1,4
      key = @colorkeys[k]
      if config["#{key}_mode"] ~= "Ignore"
        count, _ = string.headtail config["#{key}_mode"]
        for j = 1,count
          htmlcolor = config["#{key}_color#{j}"]
          r, g, b = string.match(htmlcolor, "(%x%x)(%x%x)(%x%x)")
          colors[key][j] = ass_color tonumber(r,16), tonumber(g,16), tonumber(b,16)

    if #colors.primary + #colors.secondary+ #colors.outline + #colors.shadow == 0
      aegisub.debug.out 0, "Operation failed because you did not configure gradient count for primary/secondary/outline/shadow colors."
      nil
    colors

  process: (meta, styles, config, subtitles, selected_lines, active_line) =>
    @update_control_values(config)
    @last_run = os.time()
    scfg = @serialize_config(config)

    return unless colors = @_colors config

    math.randomseed os.time()

    -- Mode
    config.mode = {Smooth: 1, 'Smooth (Vertical)': 2 'By character': 3, 'By syllable': 4}[config.mode]

    -- karaoke tag function
    config.karatagfn = switch config.karamode
      when 'Strip karaoke' then -> ''
      when 'As Is' then (syl) -> string.format "{\\%s%d}", syl.tag, syl.kdur
      else (syl) -> string.format "{%s%d}", config.karamode, syl.kdur

    -- Get lines indexes
    valid_line = (i, pred) ->
      line = subtitles[i]
      line.class == 'dialogue' and not line.comment and not @has_gradient line and (not pred or pred line)

    applyto_type, applyto_more = string.headtail(config.applyto)
    subs = switch applyto_type
      when 'All'
        [subtitles[i] for i, #subtitles when valid_line i]
      when 'Selected'
        selected_lines
      when 'Style'
        _, style = string.headtail(applyto_more)
        [subtitles[i] for i, #subtitles when valid_line i, (l) -> l.style == style]
      when 'Actor'
        _, actor = string.headtail(applyto_more)
        [subtitles[i] for i, #subtitles when valid_line i, (l) -> l.actor == actor]

    -- process them
    configstored = false
    new_lines = 0
    count = 0

    for i in *subs do
      count += 1
      aegisub.progress.set(count * 100 / #subs)
      return if aegisub.progress.is_cancelled()

      line = subtitles[i + new_lines]

      if not configstored and line.class == 'info' and line.key == "GradientFactory"
        line.value = scfg
        subtitles[i + new_lines] = line
        configstored = true

      karaskel.preproc_line subtitles, meta, styles, line
      res = @do_line meta, styles, config, colors, line
      subtitles.insert(res, i + new_lines)
      new_lines += #res

      aegisub.progress.task("#{count} / #{#subs} lines processed")

    if not configstored
      subtitles.append
        class: "info"
        section: "Script Info"
        key: "GradientFactory"
        value: scfg

    return true

  -- Get the position tag for a line along with its clipper function
  _pos_and_clipper: (config) =>
    anim = false
    pos_tag = string.format("\\pos(%d,%d)", line.x, line.y)
    dx, dy, dy1, dy2, animtimes = 0, 0, 0, 0, ""
    pos = {line.text\find "{[^}]*\\pos%(([^,%)]*),([^,%)]*)%).*}"}
    mov = {line.text\find "{[^}]*\\move%(([^,%)]*),([^,%)]*),([^,%)]*),([^,%)]*)%).*}"}
    movt = {line.text\find "{[^}]*\\move%(([^,%)]*),([^,%)]*),([^,%)]*),([^,%)]*),([^,%)]*),([^,%)]*)%).*}"}

    min = pos
    if #min == 0 or #mov != 0 and mov[0] < min[0]
      min = mov
    if #min == 0 or #movt != 0 and movt[0] < min[0]
      min = movt

    for i = 3, #min
      v = string.headtail string.trim tostring min[i]
      v = tonumber(v)}
      min[i] = if v then v else 0

    if #min == 4 -- \pos
      pos_tag = string.format "\\pos(%d,%d)", unpack min[3,]
      dx, dy = min[3] - line.x, min[4] - line.y
    elseif #min == 6 -- \move without times
      pos_tag = string.format "\\move(%d,%d,%d,%d)", unpack min[3,]
      dx, dy = min[3] - line.x, min[4] - line.y
      dx2, dy2 = min[5] - line.x, min[6] - line.y
      anim = true
    elseif #min == 8 -- \move with times
      pos_tag = string.format "\\move(%d,%d,%d,%d,%d,%d)", unpack min[3,6]
      animtimes = string.format "%d,%d,", unpack min[7,8]
      dx, dy = min[3] - line.x, min[4] - line.y
      dx2, dy2 = min[5] - line.x, min[6] - line.y
      anim = true

    clipper = (x1, y1, x2, y2) ->
      outstr = string.format "\\clip(%d,%d,%d,%d)", x1+dx, y1+dy, x2+dx, y2+dy
      if anim
        outstr = string.format "%s\\t(%s\\clip(%d,%d,%d,%d))", outstr, animtimes, x1+dx2, y1+dy2, x2+dx2, y2+dy2
      outstr .. pos_tag

    pos_tag, clipper

  do_line: (meta, styles, config, colors, line) =>
    local mode = config.mode
    mode = 3 if #line.kara == 0 and mode == 4

    randomtag = math.random(273, 4095)

    local results
    linetext = ""
    unless mode == 4
      nline = table.copy(line)
      nline.comment = true
      nline.effect = string.format "gradient @%x 0", randomtag
      results = { nline }

      linetext = if #line.kara > 0
         table.concat [config.karatagfn(syl) .. syl.text_stripped for syl in *line.kara], ''
      else
        line.text_stripped

    lineleft = line.left - line.styleref.outline
    lineright = line.right + line.styleref.outline + line.styleref.shadow
    linetop = line.top - line.styleref.outline
    linebottom = line.bottom + line.styleref.outline + line.styleref.shadow

    pos_tag, clipper = @_pos_and_clipper config

    if mode == 1
      left, right = 0, config.stripx
      count = 0
      linewidth = line.width + 2*line.styleref.outline + line.styleref.shadow
      nlinewidth = linewidth-config.stripx
      while (left == 0 or left < linewidth) and not aegisub.progress.is_cancelled()
        count += 1
        nline = table.copy(line)
        nline.text = string.format "{%s%s}%s",
          clipper(left+lineleft,linetop,right+lineleft,linebottom)
          @color_interpolator(left, nlinewidth, colors)
          linetext
        nline.effect = string.format("gradient @%x %00d", randomtag, count)
        table.insert(results, nline)
        left = right
        right += config.stripx

    elseif mode == 2
      top, bottom = 0, config.stripx
      count = 0

      lineheight = line.height + 2*line.styleref.outline + line.styleref.shadow
      nlineheight = lineheight-config.stripx
      while (top == 0 or top < lineheight) and not aegisub.progress.is_cancelled()
        count += 1
        nline = table.copy(line)
        nline.text = string.format "{%s%s}%s",
          clipper(lineleft,linetop+top,lineright,linetop+bottom)
          @color_interpolator(top, nlineheight, colors)
          linetext

        nline.effect = string.format("gradient @%x %00d", randomtag, count)
        table.insert(results, nline)
        top = bottom
        bottom += config.stripx

    elseif mode == 3
      process_str = (str, left, prefix) ->
        right = 0
        text = {}
        for char in unicode.chars str
          width, height, descent, ext_lead = aegisub.text_extents line.styleref, char
          right = left + width
          colortags = @color_interpolator @calc_j(left, right, line.width), line.width, colors
          colortags = "{#{colortags}}" unless colortags == ''
          text[#syltext] = colortags .. char
          left = right
        linetext ..= prefix .. table.concat(text, '')

      if #line.kara > 0 and config.karamode ~= "Strip karaoke"
        process_str syl.text, syl.left, config.karatagfn(syl) for syl in *line.kara
      else
        process_str line.text_stripped, 0, ''
       linetext = "{#{pos_tag}}#{linetext}" if pos_mode > 0

    elseif mode == 4
      for syl in *line.kara
        colortags = @color_interpolator(@calc_j(syl.left, syl.right, line.width), line.width, colors)
        colortags = "{#{colortags}}" unless colortags == ''
        linetext ..= config.karatagfn(syl) .. colortags .. syl.text_stripped
      linetext = "{#{pos_tag}}#{linetext}" if pos_mode > 0

    if mode > 2
        line.text = linetext
        return { line }

    return results

  calc_j: (left, right, width) =>
    if left + right < width
        left + ((right - left) * left / width)
    else
        left + ((right - left) * right / width)

  color_interpolator: (j, maxj, colors) =>
    colors_out = {}
    for c = 1,4 do
      dcolors = colors[@colorkeys[c]]
      cc = #dcolors
      if cc > 1
        nmaxj = maxj/(cc-1)
        k = clamp math.floor(j / nmaxj), 0, cc-2
        nj = j - (k*nmaxj)
        table.insert colors_out, string.format "\\%dc%s", c, interpolate_color nj/nmaxj, dcolors[k+1], dcolors[k+2]
    table.concat colors_out, ''

  prepareconfig: (styles, subtitles, selected) =>
    conf_field = [f for f in *@controls[applyto] when f.name == 'applyto'][0]
    old_value = conf_field.value

    conf_field.value = 'All lines'
    items = {'All lines'}

    add_item = (value) ->
      table.insert items, value
      conf_field.value = value if old_value == value

    if #selected > 0 then
      add_item "Selected lines (#{selected})"

    for style in *styles
      add_item "Style = #{style.name}"

    actors = {}
    for line in *subtitles
      if line.class == 'dialogue' and not line.comment and line.actor ~= '' and not actors[line.actor]
        actors[line.actor] = true
        add_item "Actor = #{actor.name}"

    conf_field.items = items

    for i = 1, 4
      field = @controls[11 + i * 2]

      field.items = { 'Ignore' }
      table.insert field.items, "#{k} colors" for k, @color_comp_count

      if not {v, true for v in *field.items}[field.value]
        field.value = field.items[#field.items]

-- The actual macro entry point
has_gradient = (line) ->
  nil != string.match line.effect, "^gradient @%x+ %d+$"

macro_process = (subtitles, selected_lines, active_line) ->
  meta, styles = karaskel.collect_head subtitles

  -- configuration
  @unserialize_config meta['gradientfactory'] if meta['gradientfactory']

  -- filter selected_lines
  selected_lines = [i for i in *selected_lines when not subtitles[i].comment and not has_gradient subtitles[i]]

  -- display dialog
  window = GradientFactoryGui()
  local cfg_res, config
  @prepareconfig styles, subtitles, selected_lines
  res = window.display!

  if res
    result = @process meta, styles, config, subtitles, selected_lines, active_line
    if result then
      aegisub.set_undo_point 'Generate color gradient'
      aegisub.progress.task 'Done'
    else
      aegisub.progress.task 'Failed'
  else
    aegisub.progress.task 'Cancelled'

macro_undo = (subtitles, selected_lines, active_line) ->
  tag = string.match subtitles[selected_lines[1]].effect, '@%x+'
  pattern = "^gradient #{tag} (%d+)$"

  to_delete = {}
  for i = 1, #subtitles
    line = subtitles[i]
    if line.class == 'dialogue'
      c = string.match line.effect, pattern
      if c == '0' then
        subtitles[i] = update line, comment: false, effect: ''
      elseif c
        table.insert to_delete, i

  subtitles.delete unpack to_delete

  aegisub.set_undo_point tr'Un-gradient'

validate_undo = (subtitles, selected_lines, active_line) ->
  #selected_lines > 0 and has_gradient subtitles[selected_lines[1]]


-- register macros
aegisub.register_macro 'Generate color gradient', 'Generate color gradient', macro_process
aegisub.register_macro 'Un-gradient', 'Un-gradient', macro_undo, validate_undo
