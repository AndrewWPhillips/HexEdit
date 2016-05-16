RegHelper is a project that is used to support HexEdit.

HexEdit cannot modify registry settings affecting all users, even if it is run by an Administrator.
A separate program (RegHelper.exe) is used to allow global registry changes. When HexEdit runs RegHelper.exe
a UAC (User Access Control) window is displayed asking the user if they want to allow the changes.

Command Line Options
--------------------

There are three command line options: (See "Usage" in the code for details.)

EXT - register file extensions that are openable with HexEdit (This is needed for Task Bar Jump Lists to work.)

REGALL - allow "Open With" shell menu options (Use when "Open With HexEdit" option is turned on - see Options/System/General/Global Settings.)

UNREGALL - remove "Open With" shell menu options (Use when "Open With HexEdit" option is turned off.)

------ END OF README --------
