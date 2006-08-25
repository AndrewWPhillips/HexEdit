# Microsoft Developer Studio Project File - Name="HexEdit" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=HexEdit - Win32 FinalCheck
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "HexEdit.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "HexEdit.mak" CFG="HexEdit - Win32 FinalCheck"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "HexEdit - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "HexEdit - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "HexEdit - Win32 TrueTime" (based on "Win32 (x86) Application")
!MESSAGE "HexEdit - Win32 FinalCheck" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/HexEdit", BAAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "HexEdit - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Fr /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0xc09 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0xc09 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"Browse/HexEdit.bsc"
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 version.lib setargv.obj imagehlp.lib htmlhelp.lib winmm.lib shlwapi.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT LINK32 /debug
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=MakeHelpMap
# End Special Build Tool

!ELSEIF  "$(CFG)" == "HexEdit - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /Gi /GR /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_SECDLL" /Fr /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0xc09 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0xc09 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"Browse/HexEdit.bsc"
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 version.lib setargv.obj imagehlp.lib htmlhelp.lib winmm.lib shlwapi.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Make help ID map files
PostBuild_Cmds=MakeHelpMap
# End Special Build Tool

!ELSEIF  "$(CFG)" == "HexEdit - Win32 TrueTime"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "HexEdit___Win32_TrueTime"
# PROP BASE Intermediate_Dir "HexEdit___Win32_TrueTime"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "TrueTime"
# PROP Intermediate_Dir "TrueTime"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GR /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Fr /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0xc09 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0xc09 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"Browse\HexEdit.bsc"
# ADD BSC32 /nologo /o"Browse/HexEdit.bsc"
LINK32=link.exe
# ADD BASE LINK32 setargv.obj imagehlp.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 version.lib setargv.obj imagehlp.lib htmlhelp.lib winmm.lib shlwapi.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "HexEdit - Win32 FinalCheck"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "HexEdit___Win32_FinalCheck"
# PROP BASE Intermediate_Dir "HexEdit___Win32_FinalCheck"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "FinalCheck"
# PROP Intermediate_Dir "FinalCheck"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /Gi /GR /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_SECDLL" /Fr /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /Gi /GR /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_SECDLL" /Fr /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0xc09 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0xc09 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"Browse\HexEdit.bsc"
# ADD BSC32 /nologo /o"Browse/HexEdit.bsc"
LINK32=link.exe
# ADD BASE LINK32 version.lib setargv.obj imagehlp.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 version.lib setargv.obj imagehlp.lib htmlhelp.lib winmm.lib shlwapi.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "HexEdit - Win32 Release"
# Name "HexEdit - Win32 Debug"
# Name "HexEdit - Win32 TrueTime"
# Name "HexEdit - Win32 FinalCheck"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Algorithm.cpp
# End Source File
# Begin Source File

SOURCE=.\BGSearch.cpp
# End Source File
# Begin Source File

SOURCE=.\Bookmark.cpp
# End Source File
# Begin Source File

SOURCE=.\BookmarkDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\BookmarkFind.cpp
# End Source File
# Begin Source File

SOURCE=.\Boyer.cpp
# End Source File
# Begin Source File

SOURCE=.\CalcDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\CalcEdit.cpp
# End Source File
# Begin Source File

SOURCE=.\CFile64.cpp
# End Source File
# Begin Source File

SOURCE=.\ChildFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\ClearHistDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Control.cpp
# End Source File
# Begin Source File

SOURCE=.\CopyCSrc.cpp
# End Source File
# Begin Source File

SOURCE=.\crypto.cpp
# End Source File
# Begin Source File

SOURCE=.\DataFormatView.cpp
# End Source File
# Begin Source File

SOURCE=.\DFFDData.cpp
# End Source File
# Begin Source File

SOURCE=.\DFFDEVAL.cpp
# End Source File
# Begin Source File

SOURCE=.\DFFDFor.cpp
# End Source File
# Begin Source File

SOURCE=.\DFFDGlobal.cpp
# End Source File
# Begin Source File

SOURCE=.\DFFDIf.cpp
# End Source File
# Begin Source File

SOURCE=.\DFFDJUMP.cpp
# End Source File
# Begin Source File

SOURCE=.\DFFDMisc.cpp
# End Source File
# Begin Source File

SOURCE=.\DFFDStruct.cpp
# End Source File
# Begin Source File

SOURCE=.\DFFDSwitch.cpp
# End Source File
# Begin Source File

SOURCE=.\DFFDUseStruct.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog.cpp
# End Source File
# Begin Source File

SOURCE=.\DirDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\DocData.cpp
# End Source File
# Begin Source File

SOURCE=.\EBCDIC.cpp
# End Source File
# Begin Source File

SOURCE=.\EmailDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Expr.cpp
# End Source File
# Begin Source File

SOURCE=.\FindDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\HexEdit.cpp
# End Source File
# Begin Source File

SOURCE=.\Hlp\Hexedit.hpj

!IF  "$(CFG)" == "HexEdit - Win32 Release"

USERDEP__HEXED="$(ProjDir)\hlp\HexEdit.rtf"	
# Begin Custom Build - Building context help file from $(InputPath)
OutDir=.\Release
ProjDir=.
TargetName=HexEdit
InputPath=.\Hlp\Hexedit.hpj

"$(OutDir)\$(TargetName).hlp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	call "$(ProjDir)\makehelp.bat"

# End Custom Build

!ELSEIF  "$(CFG)" == "HexEdit - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "HexEdit - Win32 TrueTime"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "HexEdit - Win32 FinalCheck"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\HexEdit.rc
# End Source File
# Begin Source File

SOURCE=.\HexEditDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\HexEditMacro.cpp
# End Source File
# Begin Source File

SOURCE=.\HexEditSplitter.cpp
# End Source File
# Begin Source File

SOURCE=.\HexEditView.cpp
# End Source File
# Begin Source File

SOURCE=.\HexFileList.cpp
# End Source File
# Begin Source File

SOURCE=.\HexPrintDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\IntelHex.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\Misc.cpp
# End Source File
# Begin Source File

SOURCE=.\NavManager.cpp
# End Source File
# Begin Source File

SOURCE=.\NewFile.cpp
# End Source File
# Begin Source File

SOURCE=.\NewScheme.cpp
# End Source File
# Begin Source File

SOURCE=.\OpenDiskDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\OpenSpecialDlg.cpp

!IF  "$(CFG)" == "HexEdit - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "HexEdit - Win32 Debug"

!ELSEIF  "$(CFG)" == "HexEdit - Win32 TrueTime"

!ELSEIF  "$(CFG)" == "HexEdit - Win32 FinalCheck"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Options.cpp
# End Source File
# Begin Source File

SOURCE=.\Password.cpp
# End Source File
# Begin Source File

SOURCE=.\Print.cpp
# End Source File
# Begin Source File

SOURCE=.\Prop.cpp
# End Source File
# Begin Source File

SOURCE=.\RecentFileDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Register.cpp
# End Source File
# Begin Source File

SOURCE=.\ResizeCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\SaveDffd.cpp
# End Source File
# Begin Source File

SOURCE=.\ScrView.cpp
# End Source File
# Begin Source File

SOURCE=.\Security.cpp
# End Source File
# Begin Source File

SOURCE=.\SimpleSplitter.cpp
# End Source File
# Begin Source File

SOURCE=.\SpecialList.cpp
# End Source File
# Begin Source File

SOURCE=.\SRecord.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\SystemSound.cpp
# End Source File
# Begin Source File

SOURCE=.\TabView.cpp
# End Source File
# Begin Source File

SOURCE=.\TipDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\TipWnd.cpp
# End Source File
# Begin Source File

SOURCE=.\TParseDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\TParser.cpp
# End Source File
# Begin Source File

SOURCE=.\UserTool.cpp
# End Source File
# Begin Source File

SOURCE=.\Xmltree.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Algorithm.h
# End Source File
# Begin Source File

SOURCE=.\BCGMisc.h
# End Source File
# Begin Source File

SOURCE=.\Bookmark.h
# End Source File
# Begin Source File

SOURCE=.\BookmarkDlg.h
# End Source File
# Begin Source File

SOURCE=.\BookmarkFind.h
# End Source File
# Begin Source File

SOURCE=.\boyer.h
# End Source File
# Begin Source File

SOURCE=.\CalcDlg.h
# End Source File
# Begin Source File

SOURCE=.\CalcEdit.h
# End Source File
# Begin Source File

SOURCE=.\CFile64.h
# End Source File
# Begin Source File

SOURCE=.\ChildFrm.h
# End Source File
# Begin Source File

SOURCE=.\ClearHistDlg.h
# End Source File
# Begin Source File

SOURCE=.\Control.h
# End Source File
# Begin Source File

SOURCE=.\CoordAp.h
# End Source File
# Begin Source File

SOURCE=.\CopyCSrc.h
# End Source File
# Begin Source File

SOURCE=.\crypto.h
# End Source File
# Begin Source File

SOURCE=.\DataFormatView.h
# End Source File
# Begin Source File

SOURCE=.\DFFDData.h
# End Source File
# Begin Source File

SOURCE=.\DFFDEVAL.h
# End Source File
# Begin Source File

SOURCE=.\DFFDFor.h
# End Source File
# Begin Source File

SOURCE=.\DFFDGlobal.h
# End Source File
# Begin Source File

SOURCE=.\DFFDIf.h
# End Source File
# Begin Source File

SOURCE=.\DFFDJUMP.h
# End Source File
# Begin Source File

SOURCE=.\DFFDMisc.h
# End Source File
# Begin Source File

SOURCE=.\DFFDStruct.h
# End Source File
# Begin Source File

SOURCE=.\DFFDSwitch.h
# End Source File
# Begin Source File

SOURCE=.\DFFDUseStruct.h
# End Source File
# Begin Source File

SOURCE=.\Dialog.h
# End Source File
# Begin Source File

SOURCE=.\DirDialog.h
# End Source File
# Begin Source File

SOURCE=.\EmailDlg.h
# End Source File
# Begin Source File

SOURCE=.\Expr.h
# End Source File
# Begin Source File

SOURCE=.\FindDlg.h
# End Source File
# Begin Source File

SOURCE=.\HexEdit.h
# End Source File
# Begin Source File

SOURCE=.\HLP\HEXEDIT.HM
# End Source File
# Begin Source File

SOURCE=.\HexEditDoc.h
# End Source File
# Begin Source File

SOURCE=.\HexEditMacro.h
# End Source File
# Begin Source File

SOURCE=.\HexEditSplitter.h
# End Source File
# Begin Source File

SOURCE=.\HexEditView.h
# End Source File
# Begin Source File

SOURCE=.\HexFileList.h
# End Source File
# Begin Source File

SOURCE=.\HexPrintDialog.h
# End Source File
# Begin Source File

SOURCE=.\IntelHex.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\Misc.h
# End Source File
# Begin Source File

SOURCE=.\NavManager.h
# End Source File
# Begin Source File

SOURCE=.\NewFile.h
# End Source File
# Begin Source File

SOURCE=.\NewScheme.h
# End Source File
# Begin Source File

SOURCE=.\ntapi.h
# End Source File
# Begin Source File

SOURCE=.\OpenSpecialDlg.h
# End Source File
# Begin Source File

SOURCE=.\Options.h
# End Source File
# Begin Source File

SOURCE=.\optypes.h
# End Source File
# Begin Source File

SOURCE=.\Password.h
# End Source File
# Begin Source File

SOURCE=.\Prop.h
# End Source File
# Begin Source File

SOURCE=.\range_set.h
# End Source File
# Begin Source File

SOURCE=.\RecentFileDlg.h
# End Source File
# Begin Source File

SOURCE=.\Register.h
# End Source File
# Begin Source File

SOURCE=.\ResizeCtrl.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\SaveDffd.h
# End Source File
# Begin Source File

SOURCE=.\Scheme.h
# End Source File
# Begin Source File

SOURCE=.\ScrView.h
# End Source File
# Begin Source File

SOURCE=.\Security.h
# End Source File
# Begin Source File

SOURCE=.\SimpleSplitter.h
# End Source File
# Begin Source File

SOURCE=.\SpecialList.h
# End Source File
# Begin Source File

SOURCE=.\SRecord.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\SystemSound.h
# End Source File
# Begin Source File

SOURCE=.\TabView.h
# End Source File
# Begin Source File

SOURCE=.\timer.h
# End Source File
# Begin Source File

SOURCE=.\TipDlg.h
# End Source File
# Begin Source File

SOURCE=.\TipWnd.h
# End Source File
# Begin Source File

SOURCE=.\TParseDlg.h
# End Source File
# Begin Source File

SOURCE=.\TParser.h
# End Source File
# Begin Source File

SOURCE=.\UserTool.h
# End Source File
# Begin Source File

SOURCE=.\w2k_def.h
# End Source File
# Begin Source File

SOURCE=.\Xmltree.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Group "Bar Buttons"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\res\AddrDn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\AddrSel.bmp
# End Source File
# Begin Source File

SOURCE=.\res\AddrUp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\AddrX.bmp
# End Source File
# Begin Source File

SOURCE=.\res\AscEbcDn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\AscEbcSl.bmp
# End Source File
# Begin Source File

SOURCE=.\res\AscEbcUp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\AscEbcX.bmp
# End Source File
# Begin Source File

SOURCE=.\res\AutoDn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\AutoSel.bmp
# End Source File
# Begin Source File

SOURCE=.\res\AutoUp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\AutoX.bmp
# End Source File
# Begin Source File

SOURCE=.\res\CharDn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\CharSel.bmp
# End Source File
# Begin Source File

SOURCE=.\res\CharUp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\CharX.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ContDn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ContSel1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ContSel2.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ContUp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ContX.bmp
# End Source File
# Begin Source File

SOURCE=.\res\CopyDn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\CopyUp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\CopyX.bmp
# End Source File
# Begin Source File

SOURCE=.\res\CutDn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\CutUp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\CutX.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\fdecd.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\fdecu.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\fdecx.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\fincd.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\fincu.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\fincx.bmp
# End Source File
# Begin Source File

SOURCE=.\res\FindNeDn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\FindNeUp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\FindNeX.bmp
# End Source File
# Begin Source File

SOURCE=.\res\FindPrDn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\FindPrUp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\FindPrX.bmp
# End Source File
# Begin Source File

SOURCE=.\res\FontDn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\FontUp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\FontX.bmp
# End Source File
# Begin Source File

SOURCE=.\res\FOpenDn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\FOpenUp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\FSaveDn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\FSaveUp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\FSaveX.bmp
# End Source File
# Begin Source File

SOURCE=.\res\GoMarkDn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\GoMarkUp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\GoMarkX.bmp
# End Source File
# Begin Source File

SOURCE=.\res\GraphDn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\GraphSel.bmp
# End Source File
# Begin Source File

SOURCE=.\res\GraphUp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\GraphX.bmp
# End Source File
# Begin Source File

SOURCE=.\res\MarkDn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\MarkUp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\MarkX.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ModsDn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ModsSel.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ModsUp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ModsX.bmp
# End Source File
# Begin Source File

SOURCE=.\res\PasteDn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\PasteUp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\PasteX.bmp
# End Source File
# Begin Source File

SOURCE=.\res\PlayDn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\PlayUp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\PlayX.bmp
# End Source File
# Begin Source File

SOURCE=.\res\PrintDn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\PrintUp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\PrintX.bmp
# End Source File
# Begin Source File

SOURCE=.\res\RecDn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\RecUp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\RecX.bmp
# End Source File
# Begin Source File

SOURCE=.\res\RSBackDn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\RSBackUp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\RSBackX.bmp
# End Source File
# Begin Source File

SOURCE=.\res\RSForwDn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\RSForwUp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\RSForwX.bmp
# End Source File
# Begin Source File

SOURCE=.\res\undodn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\undoup.bmp
# End Source File
# Begin Source File

SOURCE=.\res\undox.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\RES\back.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\back1.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\back_hot.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\cogs.ico
# End Source File
# Begin Source File

SOURCE=.\Res\colour.ico
# End Source File
# Begin Source File

SOURCE=.\RES\data.ico
# End Source File
# Begin Source File

SOURCE=.\DefaultToolbarImages.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Del.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\del.ico
# End Source File
# Begin Source File

SOURCE=.\RES\devimage.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\display.ico
# End Source File
# Begin Source File

SOURCE=.\RES\down.ico
# End Source File
# Begin Source File

SOURCE=.\RES\editbar.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\encombar.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\eval.ico
# End Source File
# Begin Source File

SOURCE=.\Res\filter.ico
# End Source File
# Begin Source File

SOURCE=.\RES\find.ico
# End Source File
# Begin Source File

SOURCE=.\RES\flip.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\flip1.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\flip1_ho.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\flip_hot.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\for.ico
# End Source File
# Begin Source File

SOURCE=.\RES\formtbar.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\forw.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\forw1.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\forw_hot.bmp
# End Source File
# Begin Source File

SOURCE=.\res\HexEdit.rc2
# End Source File
# Begin Source File

SOURCE=.\res\HexEditDoc.ico
# End Source File
# Begin Source File

SOURCE=.\RES\hide.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\hide_hot.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\highligh.cur
# End Source File
# Begin Source File

SOURCE=.\Res\ico00001.ico
# End Source File
# Begin Source File

SOURCE=.\Res\ico00002.ico
# End Source File
# Begin Source File

SOURCE=.\Res\idr_main.ico
# End Source File
# Begin Source File

SOURCE=.\RES\if.ico
# End Source File
# Begin Source File

SOURCE=.\RES\imagelis.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\jump.ico
# End Source File
# Begin Source File

SOURCE=.\res\litebulb.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\macro.ico
# End Source File
# Begin Source File

SOURCE=.\Res\Mainbar.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\mainfram.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\misc.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\navbar.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\new.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\new.ico
# End Source File
# Begin Source File

SOURCE=.\RES\parent.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\parent1.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\parent2.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\printer.ico
# End Source File
# Begin Source File

SOURCE=.\RES\refresh.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\refresh1.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\refresh_.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\search.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\show.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\show_hid.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\struct.ico
# End Source File
# Begin Source File

SOURCE=.\RES\switch.ico
# End Source File
# Begin Source File

SOURCE=.\RES\tmp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\up.ico
# End Source File
# Begin Source File

SOURCE=.\RES\vert.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\vert_hot.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\view.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\view1.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\view_hot.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\window.ico
# End Source File
# End Group
# Begin Group "Help Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\hlp\Bullet.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\CurArw2.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\CurArw4.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\CurHelp.bmp
# End Source File
# Begin Source File

SOURCE=.\HLP\DecATool.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\EditCopy.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\EditCut.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\EditPast.bmp
# End Source File
# Begin Source File

SOURCE=.\Hlp\file_opn.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\FileOpen.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\FilePrnt.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\FileSave.bmp
# End Source File
# Begin Source File

SOURCE=.\HLP\FindTool.bmp
# End Source File
# Begin Source File

SOURCE=.\HelpID.hm
# End Source File
# Begin Source File

SOURCE=.\HLP\HexATool.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\HexEdit.cnt
# End Source File
# Begin Source File

SOURCE=.\HLP\HexEdit.rtf
# End Source File
# Begin Source File

SOURCE=.\HLP\hexicon.bmp
# End Source File
# Begin Source File

SOURCE=.\HLP\HlpEBar.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\HlpSBar.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\HlpTBar.bmp
# End Source File
# Begin Source File

SOURCE=.\MakeHelp.bat
# End Source File
# Begin Source File

SOURCE=.\MakeHelpMap.bat
# End Source File
# Begin Source File

SOURCE=.\Hlp\opt1.bmp
# End Source File
# Begin Source File

SOURCE=.\Hlp\opt2.bmp
# End Source File
# Begin Source File

SOURCE=.\Hlp\opt3.bmp
# End Source File
# Begin Source File

SOURCE=.\Hlp\opt4.bmp
# End Source File
# Begin Source File

SOURCE=.\Hlp\opt5.bmp
# End Source File
# Begin Source File

SOURCE=.\Hlp\opt6.bmp
# End Source File
# Begin Source File

SOURCE=.\Hlp\prop1.bmp
# End Source File
# Begin Source File

SOURCE=.\Hlp\prop2.bmp
# End Source File
# Begin Source File

SOURCE=.\Hlp\prop3.bmp
# End Source File
# Begin Source File

SOURCE=.\Hlp\prop4.bmp
# End Source File
# Begin Source File

SOURCE=.\Hlp\prop5.bmp
# End Source File
# Begin Source File

SOURCE=.\HLP\ScClose.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\Scmax.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\Scmin.bmp
# End Source File
# Begin Source File

SOURCE=.\HLP\Screst.bmp
# End Source File
# Begin Source File

SOURCE=.\Hlp\shot_cust1.bmp
# End Source File
# Begin Source File

SOURCE=.\Hlp\shot_cust2.bmp
# End Source File
# Begin Source File

SOURCE=.\Hlp\shot_cust3.bmp
# End Source File
# Begin Source File

SOURCE=.\Hlp\shot_cust4.bmp
# End Source File
# Begin Source File

SOURCE=.\Hlp\shot_cust5.bmp
# End Source File
# Begin Source File

SOURCE=.\Hlp\shot_cust6.bmp
# End Source File
# End Group
# Begin Group "Text Files"

# PROP Default_Filter "txt"
# Begin Source File

SOURCE=.\TestData\_common_types.xml
# End Source File
# Begin Source File

SOURCE=.\TestData\_standard_types.xml
# End Source File
# Begin Source File

SOURCE=.\TestData\_windows_constants.txt
# End Source File
# Begin Source File

SOURCE=.\TestData\_windows_types.xml
# End Source File
# Begin Source File

SOURCE=.\TestData\_xxx.xml
# End Source File
# Begin Source File

SOURCE=.\TestData\Address_RegUser.txt
# End Source File
# Begin Source File

SOURCE=.\TestData\BCG_code_changes.txt
# End Source File
# Begin Source File

SOURCE=.\TestData\BinaryFileFormat.dtd
# End Source File
# Begin Source File

SOURCE=.\TestData\CONTROL
# End Source File
# Begin Source File

SOURCE=.\TestData\default.xml
# End Source File
# Begin Source File

SOURCE=.\TestData\dffd.txt
# End Source File
# Begin Source File

SOURCE=.\Ebcdic.tab
# End Source File
# Begin Source File

SOURCE=.\TestData\email3.txt
# End Source File
# Begin Source File

SOURCE=.\Hexedit.cfd
# End Source File
# Begin Source File

SOURCE=.\HexEdit.tip
# End Source File
# Begin Source File

SOURCE=.\TestData\License.txt
# End Source File
# Begin Source File

SOURCE=.\TestData\NOTES.TXT
# End Source File
# Begin Source File

SOURCE=.\TestData\ParseDefns.txt
# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=.\TestData\reguser.txt
# End Source File
# Begin Source File

SOURCE=.\TestData\T1
# End Source File
# Begin Source File

SOURCE=.\TestData\T2
# End Source File
# Begin Source File

SOURCE=.\TestData\T3
# End Source File
# Begin Source File

SOURCE=.\TestData\T4
# End Source File
# Begin Source File

SOURCE=.\TestData\T5
# End Source File
# Begin Source File

SOURCE=.\TestData\T6
# End Source File
# Begin Source File

SOURCE=.\TestData\T7
# End Source File
# Begin Source File

SOURCE=.\TestData\T8
# End Source File
# Begin Source File

SOURCE=.\TestData\T9
# End Source File
# Begin Source File

SOURCE=.\touch2.bat
# End Source File
# Begin Source File

SOURCE=.\touch3.bat
# End Source File
# Begin Source File

SOURCE=.\Touchf.bat
# End Source File
# Begin Source File

SOURCE=.\TestData\WebSearch.txt
# End Source File
# End Group
# Begin Group "Grid Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\GridBtnCell_src\BtnDataBase.cpp
# End Source File
# Begin Source File

SOURCE=.\GridBtnCell_src\BtnDataBase.h
# End Source File
# Begin Source File

SOURCE=.\GridCtrl_src\CellRange.h
# End Source File
# Begin Source File

SOURCE=.\GridBtnCell_src\GridBtnCell.cpp
# End Source File
# Begin Source File

SOURCE=.\GridBtnCell_src\GridBtnCell.h
# End Source File
# Begin Source File

SOURCE=.\GridBtnCell_src\GridBtnCellBase.cpp
# End Source File
# Begin Source File

SOURCE=.\GridBtnCell_src\GridBtnCellBase.h
# End Source File
# Begin Source File

SOURCE=.\GridBtnCell_src\GridBtnCellCombo.cpp

!IF  "$(CFG)" == "HexEdit - Win32 Release"

!ELSEIF  "$(CFG)" == "HexEdit - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "HexEdit - Win32 TrueTime"

!ELSEIF  "$(CFG)" == "HexEdit - Win32 FinalCheck"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\GridBtnCell_src\GridBtnCellCombo.h

!IF  "$(CFG)" == "HexEdit - Win32 Release"

!ELSEIF  "$(CFG)" == "HexEdit - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "HexEdit - Win32 TrueTime"

!ELSEIF  "$(CFG)" == "HexEdit - Win32 FinalCheck"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\GridCtrl_src\GridCell.cpp
# End Source File
# Begin Source File

SOURCE=.\GridCtrl_src\GridCell.h
# End Source File
# Begin Source File

SOURCE=.\GridCtrl_src\GridCellBase.cpp
# End Source File
# Begin Source File

SOURCE=.\GridCtrl_src\GridCellBase.h
# End Source File
# Begin Source File

SOURCE=.\NewCellTypes\GridCellCombo.cpp
# End Source File
# Begin Source File

SOURCE=.\NewCellTypes\GridCellCombo.h
# End Source File
# Begin Source File

SOURCE=.\NewCellTypes\GridCellDateTime.cpp
# End Source File
# Begin Source File

SOURCE=.\NewCellTypes\GridCellDateTime.h
# End Source File
# Begin Source File

SOURCE=.\GridCtrl_src\GridCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\GridCtrl_src\GridCtrl.h
# End Source File
# Begin Source File

SOURCE=.\GridCtrl_src\GridDropTarget.cpp
# End Source File
# Begin Source File

SOURCE=.\GridCtrl_src\GridDropTarget.h
# End Source File
# Begin Source File

SOURCE=.\GridTreeBtnCell_src\GridTreeBtnCell.cpp
# End Source File
# Begin Source File

SOURCE=.\GridTreeBtnCell_src\GridTreeBtnCell.h
# End Source File
# Begin Source File

SOURCE=.\TreeColumn_src\GridTreeCell.cpp
# End Source File
# Begin Source File

SOURCE=.\TreeColumn_src\GridTreeCell.h
# End Source File
# Begin Source File

SOURCE=.\TreeColumn_src\GridTreeCellBase.cpp
# End Source File
# Begin Source File

SOURCE=.\TreeColumn_src\GridTreeCellBase.h
# End Source File
# Begin Source File

SOURCE=.\GridCtrl_src\InPlaceEdit.cpp
# End Source File
# Begin Source File

SOURCE=.\GridCtrl_src\InPlaceEdit.h
# End Source File
# Begin Source File

SOURCE=.\GridCtrl_src\MemDC.h
# End Source File
# Begin Source File

SOURCE=.\GridCtrl_src\TitleTip.cpp
# End Source File
# Begin Source File

SOURCE=.\GridCtrl_src\TitleTip.h
# End Source File
# Begin Source File

SOURCE=.\TreeColumn_src\TreeColumn.cpp
# End Source File
# Begin Source File

SOURCE=.\TreeColumn_src\TreeColumn.h
# End Source File
# End Group
# Begin Group "Install"

# PROP Default_Filter "wxs"
# Begin Source File

SOURCE=.\Install\HexEdit.url
# End Source File
# Begin Source File

SOURCE=.\Install\HexEdit.wxs
# End Source File
# Begin Source File

SOURCE=.\Install\wixui\HexEditOptionsDlg.wxs
# End Source File
# Begin Source File

SOURCE=.\Install\wixui\License.rtf
# End Source File
# Begin Source File

SOURCE=.\Install\make.bat
# End Source File
# Begin Source File

SOURCE=.\Install\wixui\makelib.bat
# End Source File
# Begin Source File

SOURCE=.\Install\Order.Txt
# End Source File
# Begin Source File

SOURCE=.\Install\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=.\Install\ReadMeTemplate.txt
# End Source File
# Begin Source File

SOURCE=.\Install\Register.url
# End Source File
# Begin Source File

SOURCE=.\Install\RelNotes.txt
# End Source File
# Begin Source File

SOURCE=.\Install\Support.url
# End Source File
# Begin Source File

SOURCE=.\Install\wixui\WixUI_HexEdit.wxs
# End Source File
# End Group
# Begin Source File

SOURCE=.\manifest.xml
# End Source File
# Begin Source File

SOURCE=.\TestData\ToDo.txt
# End Source File
# End Target
# End Project
# Section HexEdit : {BFCB90BB-84B1-473B-BA6D-006EEBE44953}
# 	1:13:IDD_FIND_TEXT:251
# 	2:16:Resource Include:resource.h
# 	2:13:IDD_FIND_TEXT:IDD_FIND_TEXT
# 	2:15:CLASS: CHexPage:CHexPage
# 	2:11:FindDlg.cpp:FindDlg5.cpp
# 	2:9:FindDlg.h:FindDlg5.h
# 	2:18:CLASS: CNumberPage:CNumberPage
# 	2:17:CLASS: CFindSheet:CFindSheet
# 	2:10:ENUM: enum:enum
# 	2:18:CLASS: CSimplePage:CSimplePage
# 	2:19:Application Include:hexedit.h
# 	2:16:CLASS: CTextPage:CTextPage
# 	2:19:CLASS: CReplacePage:CReplacePage
# End Section
# Section HexEdit : {09260E41-10E0-4D1B-A98D-4AA0545FBC71}
# 	1:12:IDD_FIND_HEX:145
# 	2:16:Resource Include:resource.h
# 	2:15:CLASS: CHexPage:CHexPage
# 	2:11:FindDlg.cpp:FindDlg2.cpp
# 	2:9:FindDlg.h:FindDlg2.h
# 	2:18:CLASS: CNumberPage:CNumberPage
# 	2:17:CLASS: CFindSheet:CFindSheet
# 	2:12:IDD_FIND_HEX:IDD_FIND_HEX
# 	2:10:ENUM: enum:enum
# 	2:18:CLASS: CSimplePage:CSimplePage
# 	2:19:Application Include:hexedit.h
# 	2:16:CLASS: CTextPage:CTextPage
# 	2:19:CLASS: CReplacePage:CReplacePage
# End Section
# Section HexEdit : {B1438180-DE69-11D3-92C0-8D211DACDB6A}
# 	1:8:IDD_CALC:117
# 	2:16:Resource Include:resource.h
# 	2:15:CLASS: CCalcDlg:CCalcDlg
# 	2:10:ENUM: enum:enum
# 	2:11:CalcDlg.cpp:CalcDlg.cpp
# 	2:9:CalcDlg.h:CalcDlg.h
# 	2:19:Application Include:HexEdit.h
# 	2:8:IDD_CALC:IDD_CALC
# End Section
# Section HexEdit : {E3996A26-5496-46F6-85F5-3C345557410B}
# 	1:15:IDD_FIND_NUMBER:148
# 	2:16:Resource Include:resource.h
# 	2:15:CLASS: CHexPage:CHexPage
# 	2:11:FindDlg.cpp:FindDlg5.cpp
# 	2:9:FindDlg.h:FindDlg5.h
# 	2:15:IDD_FIND_NUMBER:IDD_FIND_NUMBER
# 	2:18:CLASS: CNumberPage:CNumberPage
# 	2:17:CLASS: CFindSheet:CFindSheet
# 	2:10:ENUM: enum:enum
# 	2:18:CLASS: CSimplePage:CSimplePage
# 	2:19:Application Include:hexedit.h
# 	2:16:CLASS: CTextPage:CTextPage
# 	2:19:CLASS: CReplacePage:CReplacePage
# End Section
# Section HexEdit : {CCECD0F9-A594-476F-98BA-7885AECB0FB2}
# 	1:15:IDD_FIND_SIMPLE:144
# 	2:16:Resource Include:resource.h
# 	2:15:CLASS: CHexPage:CHexPage
# 	2:11:FindDlg.cpp:FindDlg1.cpp
# 	2:9:FindDlg.h:FindDlg1.h
# 	2:18:CLASS: CNumberPage:CNumberPage
# 	2:17:CLASS: CFindSheet:CFindSheet
# 	2:10:ENUM: enum:enum
# 	2:18:CLASS: CSimplePage:CSimplePage
# 	2:19:Application Include:hexedit.h
# 	2:15:IDD_FIND_SIMPLE:IDD_FIND_SIMPLE
# 	2:16:CLASS: CTextPage:CTextPage
# 	2:19:CLASS: CReplacePage:CReplacePage
# End Section
# Section HexEdit : {70193149-2B86-4114-ADD5-F9A3737EC982}
# 	1:15:IDD_FIND_NUMBER:152
# 	2:16:Resource Include:resource.h
# 	2:15:CLASS: CHexPage:CHexPage
# 	2:11:FindDlg.cpp:FindDlg2.cpp
# 	2:9:FindDlg.h:FindDlg2.h
# 	2:15:IDD_FIND_NUMBER:IDD_FIND_NUMBER
# 	2:18:CLASS: CNumberPage:CNumberPage
# 	2:17:CLASS: CFindSheet:CFindSheet
# 	2:10:ENUM: enum:enum
# 	2:18:CLASS: CSimplePage:CSimplePage
# 	2:19:Application Include:hexedit.h
# 	2:16:CLASS: CTextPage:CTextPage
# 	2:19:CLASS: CReplacePage:CReplacePage
# End Section
# Section HexEdit : {26CA68D5-21C8-41AB-8D51-D07163AB7EFE}
# 	1:12:IDD_FIND_HEX:150
# 	2:16:Resource Include:resource.h
# 	2:15:CLASS: CHexPage:CHexPage
# 	2:11:FindDlg.cpp:FindDlg1.cpp
# 	2:9:FindDlg.h:FindDlg1.h
# 	2:18:CLASS: CNumberPage:CNumberPage
# 	2:17:CLASS: CFindSheet:CFindSheet
# 	2:12:IDD_FIND_HEX:IDD_FIND_HEX
# 	2:10:ENUM: enum:enum
# 	2:18:CLASS: CSimplePage:CSimplePage
# 	2:19:Application Include:hexedit.h
# 	2:16:CLASS: CTextPage:CTextPage
# 	2:19:CLASS: CReplacePage:CReplacePage
# End Section
# Section HexEdit : {1F3A9279-1AD8-4938-A8CF-882AFD7E9512}
# 	1:16:IDD_FIND_REPLACE:147
# 	2:16:Resource Include:resource.h
# 	2:15:CLASS: CHexPage:CHexPage
# 	2:11:FindDlg.cpp:FindDlg4.cpp
# 	2:9:FindDlg.h:FindDlg4.h
# 	2:18:CLASS: CNumberPage:CNumberPage
# 	2:17:CLASS: CFindSheet:CFindSheet
# 	2:10:ENUM: enum:enum
# 	2:18:CLASS: CSimplePage:CSimplePage
# 	2:19:Application Include:hexedit.h
# 	2:16:CLASS: CTextPage:CTextPage
# 	2:19:CLASS: CReplacePage:CReplacePage
# 	2:16:IDD_FIND_REPLACE:IDD_FIND_REPLACE
# End Section
# Section HexEdit : {9617F36F-6119-46DE-BCF3-E1AE8B0077A1}
# 	1:15:IDD_FIND_SIMPLE:250
# 	2:16:Resource Include:resource.h
# 	2:15:CLASS: CHexPage:CHexPage
# 	2:11:FindDlg.cpp:FindDlg4.cpp
# 	2:9:FindDlg.h:FindDlg4.h
# 	2:18:CLASS: CNumberPage:CNumberPage
# 	2:17:CLASS: CFindSheet:CFindSheet
# 	2:10:ENUM: enum:enum
# 	2:18:CLASS: CSimplePage:CSimplePage
# 	2:19:Application Include:hexedit.h
# 	2:15:IDD_FIND_SIMPLE:IDD_FIND_SIMPLE
# 	2:16:CLASS: CTextPage:CTextPage
# 	2:19:CLASS: CReplacePage:CReplacePage
# End Section
# Section HexEdit : {6983D927-AF4F-4A19-9202-7ED6A588DDEF}
# 	1:16:IDD_FIND_REPLACE:249
# 	2:16:Resource Include:resource.h
# 	2:15:CLASS: CHexPage:CHexPage
# 	2:11:FindDlg.cpp:FindDlg3.cpp
# 	2:9:FindDlg.h:FindDlg3.h
# 	2:18:CLASS: CNumberPage:CNumberPage
# 	2:17:CLASS: CFindSheet:CFindSheet
# 	2:10:ENUM: enum:enum
# 	2:18:CLASS: CSimplePage:CSimplePage
# 	2:19:Application Include:hexedit.h
# 	2:16:CLASS: CTextPage:CTextPage
# 	2:19:CLASS: CReplacePage:CReplacePage
# 	2:16:IDD_FIND_REPLACE:IDD_FIND_REPLACE
# End Section
# Section HexEdit : {A837962D-5482-44EE-8002-B882ACF051B4}
# 	2:15:CLASS: CHexPage:CHexPage
# 	2:11:FindDlg.cpp:FindDlg.cpp
# 	2:9:FindDlg.h:FindDlg.h
# 	2:18:CLASS: CNumberPage:CNumberPage
# 	2:17:CLASS: CFindSheet:CFindSheet
# 	2:10:ENUM: enum:enum
# 	2:18:CLASS: CSimplePage:CSimplePage
# 	2:19:Application Include:hexedit.h
# 	2:16:CLASS: CTextPage:CTextPage
# 	2:19:CLASS: CReplacePage:CReplacePage
# End Section
# Section HexEdit : {20586576-6F43-746E-726F-6C20636F6E74}
# 	1:17:CG_IDS_DIDYOUKNOW:113
# 	1:22:CG_IDS_TIPOFTHEDAYMENU:112
# 	1:18:CG_IDS_TIPOFTHEDAY:111
# 	1:22:CG_IDS_TIPOFTHEDAYHELP:116
# 	1:19:CG_IDP_FILE_CORRUPT:115
# 	1:7:IDD_TIP:109
# 	1:13:IDB_LIGHTBULB:104
# 	1:18:CG_IDS_FILE_ABSENT:114
# 	2:17:CG_IDS_DIDYOUKNOW:CG_IDS_DIDYOUKNOW
# 	2:7:CTipDlg:CTipDlg
# 	2:22:CG_IDS_TIPOFTHEDAYMENU:CG_IDS_TIPOFTHEDAYMENU
# 	2:18:CG_IDS_TIPOFTHEDAY:CG_IDS_TIPOFTHEDAY
# 	2:12:CTIP_Written:OK
# 	2:22:CG_IDS_TIPOFTHEDAYHELP:CG_IDS_TIPOFTHEDAYHELP
# 	2:2:BH:
# 	2:19:CG_IDP_FILE_CORRUPT:CG_IDP_FILE_CORRUPT
# 	2:7:IDD_TIP:IDD_TIP
# 	2:8:TipDlg.h:TipDlg.h
# 	2:13:IDB_LIGHTBULB:IDB_LIGHTBULB
# 	2:18:CG_IDS_FILE_ABSENT:CG_IDS_FILE_ABSENT
# 	2:10:TipDlg.cpp:TipDlg.cpp
# End Section
# Section HexEdit : {B1438181-DE69-11D3-92C0-8D211DACDB6A}
# 	2:10:CalcEdit.h:CalcEdit.h
# 	2:12:CalcEdit.cpp:CalcEdit.cpp
# 	2:19:Application Include:HexEdit.h
# 	2:16:CLASS: CCalcEdit:CCalcEdit
# End Section
# Section HexEdit : {A7E81C4C-FC7F-43DA-8BB4-A315C7FBD7A7}
# 	1:13:IDD_FIND_TEXT:146
# 	2:16:Resource Include:resource.h
# 	2:13:IDD_FIND_TEXT:IDD_FIND_TEXT
# 	2:15:CLASS: CHexPage:CHexPage
# 	2:11:FindDlg.cpp:FindDlg3.cpp
# 	2:9:FindDlg.h:FindDlg3.h
# 	2:18:CLASS: CNumberPage:CNumberPage
# 	2:17:CLASS: CFindSheet:CFindSheet
# 	2:10:ENUM: enum:enum
# 	2:18:CLASS: CSimplePage:CSimplePage
# 	2:19:Application Include:hexedit.h
# 	2:16:CLASS: CTextPage:CTextPage
# 	2:19:CLASS: CReplacePage:CReplacePage
# End Section
# Section HexEdit : {A42C171C-E7FF-4C37-8EE2-A6E729E5EC34}
# 	2:15:CLASS: CHexPage:CHexPage
# 	2:11:FindDlg.cpp:FindDlg.cpp
# 	2:9:FindDlg.h:FindDlg.h
# 	2:18:CLASS: CNumberPage:CNumberPage
# 	2:17:CLASS: CFindSheet:CFindSheet
# 	2:10:ENUM: enum:enum
# 	2:18:CLASS: CSimplePage:CSimplePage
# 	2:19:Application Include:hexedit.h
# 	2:16:CLASS: CTextPage:CTextPage
# 	2:19:CLASS: CReplacePage:CReplacePage
# End Section
