echo Creating map files

REM Get MFC help IDs
rem copy /y I:\Micros~1\vc98\mfc\include\afxhelp.hm  afxhelp.tmp
copy /y "C:\Program Files\Microsoft Visual Studio .NET\Vc7\atlmfc\include\afxhelp.hm"  afxhelp.tmp

REM Get BCG help IDs
rem copy /y D:\bcg590\BCGControlBar\help\BCGControlBar.hm  bcghelp.tmp
copy /y "I:\Devel\BCG6_2\BCGControlBar\Help\BCGControlBar.hm"  .\bcghelp.tmp

REM Copy all help IDs into one file
copy /y helpid.hm+resource.hm+hlp\HexEdit.hm+afxhelp.tmp+bcghelp.tmp  HexeditMap.tmp

del afxhelp.tmp
del bcghelp.tmp

REM Make sure they all start with #define which is what RoboHelp expects
sed "s/^HID/#define HID/"  <HexeditMap.tmp >HexeditMap2.tmp
del HexeditMap.tmp
sed "s/^AFX_HID/#define AFX_HID/"  <HexeditMap2.tmp >HTMLHelp\HexeditMap.h
del HexeditMap2.tmp
