@echo off

ECHO Get revision number from git
echo // Automatically generated header file created by Prebuild.bat > GitRevision.h

REM We use SET/P to do an echo without newline
set /p="#define GITREVISION "                                       >> GitRevision.h

REM Count number of commits (relies on Git being installed somewhere)
IF NOT EXIST "C:\Program Files (x86)\Git\bin\git.exe" goto next1
  "C:\Program Files (x86)\Git\bin\git.exe" rev-list HEAD | find /v /c "" >> GitRevision.h
  goto end
:next1

IF NOT EXIST "C:\Program Files (x86)\SmartGit\git\bin\git.exe" goto next2
  "C:\Program Files (x86)\SmartGit\git\bin\git.exe" rev-list HEAD | find /v /c "" >> GitRevision.h
  goto end
:next2

REM Just try running git and hope it's in the path
  git rev-list HEAD | find /v /c ""                                 >> GitRevision.h

:end
