@echo off
REM -- First make map file from Microsoft Visual C++ generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by HexEdit.HPJ. >"hlp\HexEdit.hm"
echo. >>"hlp\HexEdit.hm"
echo // Commands (ID_* and IDM_*) >>"hlp\HexEdit.hm"
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"hlp\HexEdit.hm"
echo. >>"hlp\HexEdit.hm"
echo // Prompts (IDP_*) >>"hlp\HexEdit.hm"
makehm IDP_,HIDP_,0x30000 resource.h >>"hlp\HexEdit.hm"
echo. >>"hlp\HexEdit.hm"
echo // Resources (IDR_*) >>"hlp\HexEdit.hm"
makehm IDR_,HIDR_,0x20000 resource.h >>"hlp\HexEdit.hm"
echo. >>"hlp\HexEdit.hm"
echo // Dialogs (IDD_*) >>"hlp\HexEdit.hm"
makehm IDD_,HIDD_,0x20000 resource.h >>"hlp\HexEdit.hm"
echo. >>"hlp\HexEdit.hm"
echo // Frame Controls (IDW_*) >>"hlp\HexEdit.hm"
makehm IDW_,HIDW_,0x50000 resource.h >>"hlp\HexEdit.hm"
REM -- Make help for Project HexEdit


echo Building Win32 Help files
rem start /wait c:\msdev\bin\hcrtf -x "hlp\HexEdit.hpj"
start /wait hcw /C /E /M "hlp\HexEdit.hpj"
if errorlevel 1 goto :Error
if not exist "hlp\HexEdit.hlp" goto :Error
if not exist "hlp\HexEdit.cnt" goto :Error
echo.
if exist Debug\nul copy "hlp\HexEdit.hlp" Debug
if exist Debug\nul copy "hlp\HexEdit.cnt" Debug
if exist Release\nul copy "hlp\HexEdit.hlp" Release
if exist Release\nul copy "hlp\HexEdit.cnt" Release
if exist Profile\nul copy "hlp\HexEdit.hlp" Profile
if exist Profile\nul copy "hlp\HexEdit.cnt" Profile
echo.
goto :done

:Error
echo hlp\HexEdit.hpj(1) : error: Problem encountered creating help file

:done
echo.


