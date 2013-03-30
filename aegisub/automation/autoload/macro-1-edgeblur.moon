-- Automation 4 demo script
-- Macro that adds \be1 tags in front of every selected line

tr = aegisub.gettext

export script_name = tr'Add edgeblur'
export script_description = tr'A demo macro showing how to do simple line modification in Automation 4'
export script_author = 'Niels Martin Hansen'
export script_version = '2'

add_edgeblur = (subtitles, selected_lines, active_line) ->
  for i in *selected_lines
    l = subtitles[i]
    l.text = "{\\be1}" .. l.text
    subtitles[i] = l
  aegisub.set_undo_point(script_name)

aegisub.register_macro script_name, tr"Adds \\be1 tags to all selected lines", add_edgeblur
