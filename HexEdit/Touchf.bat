REM Change the dates of released files to be the same dates for consistency and
REM        set their time to 05:00 to match the version number of HexEdit (5.0).
touch -t 08310500 Output\Release\HexEdit.exe
touch -t 08310500 Output\Release\RegHelper.exe
touch -t 08310500 HTMLHelp\HexEdit.chm
touch -t 08310500 Graphics\Splash.bmp
touch -t 08310500 Graphics\About.jpg
touch -t 08310500 HexEdit.tip
rem touch -t 08310500 HexEdit.xml
touch -t 08310500 ReadMe.txt
pause
