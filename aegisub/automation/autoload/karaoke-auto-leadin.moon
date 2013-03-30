--[[
 Copyright (c) 2007, Niels Martin Hansen
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

tr = aegisub.gettext

export script_name = tr"Automatic karaoke lead-in"
export script_description = tr"Join up the ends of selected lines and add \\k tags to shift karaoke"
export script_author = "Niels Martin Hansen"
export script_version = "1.0"

add_auto_leadin = (subs, sel) ->
  -- Smallest inter-line duration
  min_interdur = nil

  for i = 2, #sel do
    -- Grab two selected lines
    A = subs[sel[i-1]]
    B = subs[sel[i]]

    -- Blank duration between lines
    interdur = B.start_time - A.end_time

    if interdur > 0 then
      -- Update smallest inter-line duration
      min_interdur = interdur if not min_interdur or interdur < min_interdur

      B.start_time = A.end_time
      B.text = string.format "{\\k%d}%s", interdur/10, B.text

      subs[sel[i]] = B
    else
      aegisub.debug.out(2, "Warning: Skipping line-pair with zero or negative inter-duration:\n%s\n%s\n\n", A.text, B.text)

  if min_interdur then
    aegisub.debug.out(0, "Smallest inter-line duration: %d milliseconds", min_interdur)

    aegisub.set_undo_point(script_name)
  else
    aegisub.debug.out(2, "Warning: No lines modified")

aegisub.register_macro(script_name, script_description, add_auto_leadin, (subs, sel) -> #sel >= 2)
