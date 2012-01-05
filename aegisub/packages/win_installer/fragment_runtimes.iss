; Copyright (c) 2007-2009, Niels Martin Hansen
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
;
;   * Redistributions of source code must retain the above copyright notice,
;     this list of conditions and the following disclaimer.
;   * Redistributions in binary form must reproduce the above copyright notice,
;     this list of conditions and the following disclaimer in the documentation
;     and/or other materials provided with the distribution.
;   * Neither the name of the Aegisub Group nor the names of its contributors
;     may be used to endorse or promote products derived from this software
;     without specific prior written permission.
;
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
; ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
; LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
; CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
; SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
; INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
; CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
; ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
; POSSIBILITY OF SUCH DAMAGE.
;
; -----------------------------------------------------------------------------
;
; AEGISUB
;
; Website: http://www.aegisub.org/
; Contact: mailto:nielsm@indvikleren.dk
;


; This file implements checking for and installing runtime libraries for Aegisub

[Files]
DestDir: {tmp}; Source: src\vcredist_x86.exe; Flags: nocompression deleteafterinstall; Check: RuntimesRequired

[Components]
Name: main/runtime; Description: Runtime libraries; Check: RuntimesRequired; Flags: fixed; Types: custom compact full; ExtraDiskSpaceRequired: 4630528

[Run]
Filename: {tmp}\vcredist_x86.exe; StatusMsg: Installing runtime libraries...; Check: RuntimesRequired; Components: main/runtime; Parameters: "/q"

[Code]
function RuntimesRequired: Boolean;
var
  DisplayVersion: string;
begin
  // Check for uninstall entry for runtimes, don't bother installing if it can be uninstalled now
  // HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{9A25302D-30C0-39D9-BD6F-21E6EC160475}
  // Check: DisplayVersion = "9.0.30729"
  try
    DisplayVersion := '';
    Result := RegQueryStringValue(HKLM, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{9A25302D-30C0-39D9-BD6F-21E6EC160475}',
        'DisplayVersion', DisplayVersion);
    Result := Result and (DisplayVersion = '9.0.30729');
  except
    // If the check fails take the safe route
    Result := False;
  end;

  Result := not Result;
end;
[/Code]

