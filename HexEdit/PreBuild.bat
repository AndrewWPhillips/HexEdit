ECHO Get revision number from git
echo // Pre build automatically generated header file> GitRevision.h
REM We use SET/P to do an echo without newline
set /p="#define GITREVISION "                        >> GitRevision.h
REM Count number of commits (relies on Git being installed)
git rev-list HEAD | find /v /c ""                    >> GitRevision.h
