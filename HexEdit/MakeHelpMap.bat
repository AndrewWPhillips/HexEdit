echo Creating map files
copy /y helpid.hm+resource.hm+hlp\HexEdit.hm+I:\Micros~1\vc98\mfc\include\afxhelp.hm+D:\bcg590\BCGControlBar\help\BCGControlBar.hm HexeditMap.tmp
sed "s/^HID/#define HID/"  <HexeditMap.tmp >HexeditMap2.tmp
del HexeditMap.tmp
sed "s/^AFX_HID/#define AFX_HID/"  <HexeditMap2.tmp >HTMLHelp\HexeditMap.hm
del HexeditMap2.tmp
