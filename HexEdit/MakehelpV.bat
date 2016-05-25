@echo off
REM MakeHelpV.bat - make help files (updated for use with VS2010)

REM It creates the help map files used by RoboHelp to do context
REM help and What's this help when building HexEdit.chm.

ECHO Command IDs
echo // MAKEHELP.BAT generated Help Map file.  >"HTMLHELP\CmdIdMap.h"
echo // Commands (ID_* and IDM_*)  >>"HTMLHELP\CmdIdMap.h"
makehm  ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h  >"CmdIdMap.tmp"
bin\sed  "s/^HID/#define HID/"  <"CmdIdMap.tmp"  >>"HTMLHELP\CmdIdMap.h"
del "CmdIdMap.tmp"

ECHO MFC Command IDs
copy /y HTMLHelp\afxhelp.hm  AfxIdMap.tmp
bin\sed  "s/^HID/#define HID/"  <AfxIdMap.tmp  >AfxId2.tmp
bin\sed  "s/^AFX_HID/#define AFX_HID/"  <AfxId2.tmp  >"HTMLHELP\AfxIdMap.h"
del AfxIdMap.tmp
del AfxId2.tmp

ECHO Dialog IDs
echo // MAKEHELP.BAT generated Help Map file.  >"HTMLHELP\DlgIdMap.h"
echo // Dialogs (IDD_*)  >>"HTMLHELP\DlgIdMap.h"
makehm  IDD_,HIDD_,0x20000 resource.h  >"DlgIdMap.tmp"
bin\sed  "s/^HID/#define HID/"  <"DlgIdMap.tmp"  >>"HTMLHELP\DlgIdMap.h"
del "DlgIdMap.tmp"

ECHO Control IDs (for What's This help)
copy /y helpid.hm+resource.hm  HTMLHelp\CtlIdMap.h
