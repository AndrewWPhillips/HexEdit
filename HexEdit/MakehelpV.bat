@echo off
REM This is the version of MakeHelp.bat for use with VS2005 on Veronica
REM Changes were for where the MFC help map file (afxhelp.hm) is found
REM the BCG help map file (BCGControlBar.hm) and where the release
REM version of HexEdit.exe is found (now in D:\Andrew\cpp_out\HexEdit\).

REM It creates the help map files used by RoboHelp to do context
REM help and What's this help using HexEdit.chm.
REM It also replaces the icon in the release version of HexEdit.exe.

ECHO Command IDs
echo // MAKEHELP.BAT generated Help Map file. >"HTMLHELP\CmdIdMap.h"
echo // Commands (ID_* and IDM_*) >>"HTMLHELP\CmdIdMap.h"
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >"CmdIdMap.tmp"
"C:\Program Files\Utilities\sed"  "s/^HID/#define HID/"  <"CmdIdMap.tmp" >>"HTMLHELP\CmdIdMap.h"
del "CmdIdMap.tmp"

ECHO MFC Command IDs
rem copy /y I:\Micros~1\vc98\mfc\include\afxhelp.hm  AfxIdMap.tmp
rem copy /y "C:\Program Files\Microsoft Visual Studio .NET\Vc7\atlmfc\include\afxhelp.hm"  AfxIdMap.tmp
rem copy /y "C:\Program Files\Microsoft Visual Studio 8\VC\atlmfc\include\afxhelp.hm"  AfxIdMap.tmp
rem copy /y "C:\Program Files\Microsoft Visual Studio 9.0\VC\atlmfc\include\afxhelp.hm"  AfxIdMap.tmp
copy /y "C:\Program Files\Microsoft Visual Studio 10.0\VC\atlmfc\include\afxhelp.hm"  AfxIdMap.tmp
"C:\Program Files\Utilities\sed"  "s/^HID/#define HID/"  <AfxIdMap.tmp >AfxId2.tmp
"C:\Program Files\Utilities\sed"  "s/^AFX_HID/#define AFX_HID/"  <AfxId2.tmp   >"HTMLHELP\AfxIdMap.h"
del AfxIdMap.tmp
del AfxId2.tmp

ECHO Dialog IDs
echo // MAKEHELP.BAT generated Help Map file. >"HTMLHELP\DlgIdMap.h"
echo // Dialogs (IDD_*) >>"HTMLHELP\DlgIdMap.h"
makehm IDD_,HIDD_,0x20000 resource.h >"DlgIdMap.tmp"
"C:\Program Files\Utilities\sed"  "s/^HID/#define HID/"  <"DlgIdMap.tmp" >>"HTMLHELP\DlgIdMap.h"
del "DlgIdMap.tmp"

ECHO Control IDs (for What's This help)
copy /y helpid.hm+resource.hm  HTMLHelp\CtlIdMap.h

rem ECHO BCG Control IDs
rem copy /y "D:\Devel\BCG6_2\BCGControlBar\Help\BCGControlBar.hm" BcgIdMap.tmp
rem "C:\Program Files\Utilities\sed"  "s/^HID/#define HID/"  <BcgIdMap.tmp >"HTMLHELP\BcgIdMap.h"
rem del BcgIdMap.tmp

ECHO Fix up hexedit.exe icon since the resource compiler cannot handle the new icon format
rem ReplaceVistaIcon  D:\Andrew\cpp_out\HexEdit\ReleaseVS2005\hexedit.exe   res\hexedit2.ico
"C:\Program Files\Utilities\ReplaceVistaIcon"  "C:\andrew\cpp_out\HexEdit\Release\HexEdit.exe"   res\hexedit2.ico
