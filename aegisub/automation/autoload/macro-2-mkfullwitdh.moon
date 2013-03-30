-- Automation 4 demo script
-- Converts halfwidth (ASCII) Latin letters to fullwidth JIS Latin letters

tr = aegisub.gettext

export script_name = tr'Make text fullwidth'
export script_description = tr'Shows how to use the unicode include to iterate over characters and a lookup table to convert those characters to something else.'
export script_author = 'Niels Martin Hansen'
export script_version = '1'

unicode = require 'unicode'

lookup =
  '!': '！', '"': '”', '#': '＃', '$': '＄'
  '%': '％', '&': '＆', "'": '’', '(': '（'
  ')': '）', '*': '＊', '+': '＋', ',': '，'
  '-': '－', '.': '．', '/': '／'
  '1': '１', '2': '２', '3': '３', '4': '４'
  '5': '５', '6': '６', '7': '７', '8': '８'
  '9': '９', '0': '０'
  ':': '：', ';': '；', '<': '＜', '=': '＝'
  '>': '＞', '?': '？', '@': '＠'
  'A': 'Ａ', 'B': 'Ｂ', 'C': 'Ｃ', 'D': 'Ｄ'
  'E': 'Ｅ', 'F': 'Ｆ', 'G': 'Ｇ', 'H': 'Ｈ'
  'I': 'Ｉ', 'J': 'Ｊ', 'K': 'Ｋ', 'L': 'Ｌ'
  'M': 'Ｍ', 'N': 'Ｎ', 'O': 'Ｏ', 'P': 'Ｐ'
  'Q': 'Ｑ', 'R': 'Ｒ', 'S': 'Ｓ', 'T': 'Ｔ'
  'U': 'Ｕ', 'V': 'Ｖ', 'W': 'Ｗ', 'X': 'Ｘ'
  'Y': 'Ｙ', 'Z': 'Ｚ'
  '': '［', '\\': '＼', ']': '］', '^': '＾'
  'a': 'ａ', 'b': 'ｂ', 'c': 'ｃ', 'd': 'ｄ'
  'e': 'ｅ', 'f': 'ｆ', 'g': 'ｇ', 'h': 'ｈ'
  'i': 'ｉ', 'j': 'ｊ', 'k': 'ｋ', 'l': 'ｌ'
  'm': 'ｍ', 'n': 'ｎ', 'o': 'ｏ', 'p': 'ｐ'
  'q': 'ｑ', 'r': 'ｒ', 's': 'ｓ', 't': 'ｔ'
  'u': 'ｕ', 'v': 'ｖ', 'w': 'ｗ', 'x': 'ｘ'
  'y': 'ｙ', 'z': 'ｚ'
  '_': '＿', '`': '‘'
  '{': '｛', '|': '｜', '}': '｝', '~': '～'

make_fullwidth = (subtitles, selected_lines, active_line) ->
  for i in *selected_lines
    l = subtitles[i]

    aegisub.debug.out string.format 'Processing line %d: "%s"\n', i, l.text
    aegisub.debug.out "Chars: \n"

    in_tags = false
    newtext = ""
    for c in unicode.chars(l.text)
      aegisub.debug.out c .. ' -> '
      in_tags = true if c == '{'
      if in_tags then
        aegisub.debug.out "#{c} (ignored, in tags)\n"
        newtext ..= c
      else
        if lookup[c]
          aegisub.debug.out "#{lookup[c]} (converted)\n"
          newtext ..= lookup[c]
        else
          aegisub.debug.out "#{lookup[c]} (not found in lookup)\n"
          newtext ..= c
      in_tags = false if c == '}'

    l.text = newtext
    subtitles[i] = l

  aegisub.set_undo_point tr'Make fullwidth'

aegisub.register_macro(tr'Make fullwidth', tr'Convert Latin letters to SJIS fullwidth letters', make_fullwidth)
