This directory (including certain subirectories) contains
all the files required to build HexEdit 4.0 (HexEdit.exe) and
its installer.  It includes all the C++ source code (*.cpp and
*.h files), resource source files (HexEdit.rc, etc), etc.

The help source files are not included as these require RoboHelp,
but the latest help file (HexEdit.chm) is included.

Also included is the Visual Studio project For VS2010 and later and
the project file for VS2008 (HexEditVS2008.vcproj and .sln files).

A description of some of the files is provided below.

If you extracted these files from a .ZIP file then you must have a
means of extracting using long file names.  Also you must use any
options that extract sub-directories, since there are files in the
RES and HLP sub-directories.

Note that a prebuilt release of HexEdit is available separately
at the web site http://www.hexedit.com.


About the Code
--------------

The source code for HexEdit is completely mine (apart, of course,
from standard library and MFC code supplied with the compiler).
Please send any bug fixes or enhancements to hexeditsup@hotmail.com.

There are probably some bugs still lurking in the code.  However, I
have used the program basically unchanged for the last 6 months and
have found no serious problems.

Another good metric that the program is relatively stable is that
Bounds Checker found few problems (although I haven't used it since
June 1998).  I exercised every aspect and condition of the program
that I could think of, but Bounds Checker only reported a few
problems deep in the MFC code none of which I could track back to
anything in my code.  I also used the Final Check option but as it
was so slow and was not finding any problems I gave up on that.  


As the code was, at least initially, just a test program as I learnt
to use MFC it is not of the highest quality as far as maintainability
or reuseability go.  Nevertheless, there are a few components that
you can easily use in other programs.

In particular, range_set.h implements a class that when used as a
container for integral types is completely compatible with the STL
container std::set.  When used with sets that have large contiguous
chunks it is more space and time efficient than std::set.  (For more on
range_set see my article in the June 99 issue of C/C++ Users Journal.)

Also timer.h implements a simple, but useful, low-resolution timer
class.  Boyer.h/boyer.cpp implements a class for doing Boyer-Moore searches
on a chunk of memory.

The class CScrView (ScrView.cpp/.h) was originally intended as a
better CScrollView.  I wrote it because I had major problems with
CScrollView under Windows 95, mainly due to the use of 16 bit values
to store pixel coordinates.  However, CScrView became a bit messy
when I tried to make it mapping mode independent.  It is probably
also more tightly coupled with CHexEditView than necessary.


How HexEdit was Developed
-------------------------

HexEdit was written in my spare time mainly during the first half of
1998.  I wrote it to become familiar with C++, Windows and MFC
programming.  Prior to this I had written only a little C++ and
Windows C code and no MFC, although I have been using C under
UNIX and MSDOS for more than 15 years.

HexEdit was written, built and tested entirely using Microsoft Visual
C++ 5.0 in the Developer Studio 97 IDE.  All knowledge about MFC
comes from the book "The Revolutionary Guide to MFC 4 Programming
with Visual C++" by Mike Blaszczak, which despite its name is a good
book.  All Knowledge of C++ comes from "The C++ Programming Language"
1st, 2nd and 3rd editions, by Bjarne Stroustrup.


Description of Source Files
---------------------------

HexEdit.dsp - Dev Studio project file 

HexEdit.h - main header that declares CHexEditApp class
HexEdit.cpp - main source file implementing ChexEditApp
HexEditDoc.h - declares the document class (CHexEditDoc)
HexEditDoc.cpp - implements CHexEditDoc (along with DocData.cpp)
HexEditView.h - declares the view class (ChexEditView)
HexEditView.cpp - implements the view class, including window drawing
Mainfrm.h - declares the mainframe windows class (CMainFrame)
Mainfrm.cpp - implements CMainFrame including toolbars and status bar
Childfrm.h - declares the MDI child frame class (CChildFrame)
Childfrm.cpp - implements CChildFrame (frame window for each view)
StdAfx.h - includes the headers to be added to the precompiled headers file
StdAfx.cpp - build this to rebuild the precompiled headers
HexFileList.h - declares a class derived from MFC CRecentFileList class
HexFileList.cpp - ssave/restores settings for file in the recent file list
Scrview.h - declares CScrView the base class of CHexEditView
Scrview.cpp - implements CScrView supporting scrolling up to 2^31 pixels
Search.h - declares a class that overrides standard CFindReplaceDialog
Search.cpp - implements dialog and control classes used in Find Dialog
control.h - declares classes to subclass MFC control/bar classes
control.cpp - implements edit control, status and dialog bar subclasses
Docdata.cpp - implements data access part of CHexEditDoc
HexEditMacro.h - declarations for implementing keystroke macros
HexEditMacro.cpp - implements keystroke macros as part of CHexEditApp
options.h/.cpp - options property sheet and property pages classes
Prop.h/.cpp - porperties property sheet and property pages classes
Dialog.h/.cpp - declares and implements miscellaneous dialogs
Tipdlg.h/.cpp -    "      "      "    the Tip of the Day dialog
EmailDlg.h/.cpp -  "      "      "    the email tech support dialog
Misc.h/.cpp -      "      "      "    a few utility routines
Boyer.h/.cpp -     "      "      "    a class for Boyer-Moore searches
Partition.h - declares structure use in "Colours" option page
Timer.h - an implementation of a low-resolution timer class
range_set.h - STL compatible container: set of "chunks of integers"

HexEdit.mak - exported makefile generated from HexEdit.dsp

MakeHelp.bat - batch file used in generation of HexEdit.hlp
HLP\HexEdit.rtf - contains the text of all the HexEdit help topics
HLP\HexEdit.cnt - help contents file
HLP\Hexedit.hpj - help project file
HLP\*.BMP - graphics used in the help file
HexEdit.tip - text file containing tips used by "Tip of the Day"

HexEdit.rc - defines or includes all HexEdit resources
resource.h - Dev Studio generated include file of resource IDs
RES\HexEdit.ico - application icon (at 16x16 and 32x32)
RES\HexEditDoc.ico - icon used for MDI child windows
RES\*.* - icons, bitmaps etc that are included in HexEdit.rc

------ END OF README --------
