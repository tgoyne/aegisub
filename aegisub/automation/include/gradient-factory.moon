smatch = string.match
tinsert = table.insert
_ = require 'modash'

has_gradient = (line) ->
  nil != smatch line.effect, "^gradient @%x+ %d+$"

colorkeys: {'primary', 'secondary', 'outline', 'shadow'}

colors = (config) =>
  colors = {}
  for colorkey in *colorkeys
    if config["#{colorkey}_mode"] == "Ignore"
      colors[colorkey] = {}
      continue

    count, __ = string.headtail config["#{colorkey}_mode"]
    colors[key] = {ass_color extract_color config["#{colorkey}_color#{j}"] for j = 1, count}

  if #colors.primary + #colors.secondary+ #colors.outline + #colors.shadow == 0
    aegisub.debug.out 0, "Operation failed because you did not configure gradient count for primary/secondary/outline/shadow colors."
    nil
  colors

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

-- Module exports
validate_undo: (subtitles, selected_lines, active_line) ->
  #selected_lines > 0 and has_gradient subtitles[selected_lines[1]]

undo: (subtitles, selected_lines, active_line) ->
  tag = smatch subtitles[selected_lines[1]].effect, '@%x+'
  pattern = "^gradient #{tag} (%d+)$"

  to_delete = {}
  for i = 1, #subtitles
    line = subtitles[i]
    if line.class == 'dialogue'
      c = smatch line.effect, pattern
      if c == '0' then
        subtitles[i] = _.extend line, comment: false, effect: ''
      elseif c
        tinsert to_delete, i

  subtitles.delete unpack to_delete

process: (meta, styles, config, subtitles, selected_lines, active_line) ->
  return unless colors = @_colors config

  math.randomseed os.time()

  -- Mode
  config.mode = {Smooth: 1, 'Smooth (Vertical)': 2, 'By character': 3, 'By syllable': 4}[config.mode]

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

