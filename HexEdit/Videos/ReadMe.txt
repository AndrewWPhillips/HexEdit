This directory contains video demos of HexEdit captured with CamStudio, using Workers Collection Magniifying Glass to zoom.  Joining clips and editing with VirtualDub and/or Adobe Premiere.

The Work sub-directory contains the current video's work files.  Completed videos are here with a filename of the form HexEditDemoAaaBbb.avi where Aaa is the category and Bbb is the description.  Similarly scripts for the demos are kept with a name like ScriptAaaBbb.txt.  The categories and descriptions are below.


Process
-------

Recording demo with CamStudio
- capture short clips to later be joined using VirtualDub
- video settings
  - CamStudio Lossless Codec v1.4
  - Quality 100
  - Key frames every 1 (not used for this codec?)
  - Capture 200 msec
  - playback rate 5 f/sec
- audio (from mike)
  - 22.05 KHz, mono, 16-bit
  - PCM
- region
  - fixed
  - 640x480
  - auto pan off
- do zoom using Magnifying Glass
  - Ctrl+Alt+Z
  - 1280x960 fully visible
  - Zoom x 2, update @ 0.1 sec
  - Position under cursor

VirtualDub
- load 1st video (File/Open Video File)
- append others (File/Append AVI segment)
- delete unwanted frames
- save often to working copies as one time I could not save all my changes
- Set Compression
  - CamStudio Lossless Codec v1.4 sometimes gives wrong colours
  - ffdshow video codec has sound out of sync with video
- Add logo (filter)
- save (File /Save As AVI)

Upload to YouTube


Alternative
-----------

- work in h:\tmp\video
- capture using MS Video1 codec in CamStudio
  - no sound
- open new project in Adobe Premiere
  - Video Frame Size 640x480
  - audio 32 KHz
- import video
- add voice over
- export using Microsoft AVI output
  - use CamStudio Lossless codec
  - change video width and frame rate

Categories
==========

Viewing
-------

> Intro

* Display

> Format

> Highlighter

* Status Bar

> Ruler

Colour Schemes
- uses (not just pretty) - see specific bytes
- colour editing
- adding ranges
- use with Aerial View
- tool

Active File Options
- options pages
  - scroll zone
- saving as default

* Aerial View
- border and ants
- selection
- context menu

* Templates
- split/tabbed
- columns
- tree view
- tree keys
- context menus
- sync and autosync
- refresh after modifying the data

Ebcdic
- status bar
- find
- clipboard
- ebcdic.tab
- ebcdic code pages (Prop/Byte)

Unicode
- Prop/Byte page
- Find dialog
- template strings
- clipboard

Editing
-------

> Editing

> Mark

> Selecting

* Clipboard

Special Clipboard formats
- copy as hex
- comma separated values etc (C source)

Undo
- view vs file level undo
- warning if undo change made in other window
- undo changes
- intelligent undo

Large Files
- aborting long operations
- file/New
- building a large file using Edit/Append and Append Same File


Properties Dialog & File Properties

Properties Numbers
- Integer
- Real

Properties Date and Character Sets
- Date
- Byte


Info Tips
- list
  - checkboxes
  - add: predefined values, formats
  - delete, reorder
- expressions
- formatting - depends on type of expression
  - unsigned (eg address)
  - signed int
  - fp
  - date
  - not useful: string/bool
- example: Current time:, now(), "%H:%M"
- bookmarks
- predefined tips
  - address
  - sector/offset

Customization
-------------

Toolbars
- copy/move
- edit image

Keyboard


Context Menus


Global Formatting
- hex case
- abbrev ints

Modeless dialogs
- float/dock
- get out of the way
- roll up or hide

Printing
- WYSIWYG: highlights etc are printed
- preview
- print selection
- templates

Default Display


Navigation
----------

Jump Tools
- history
- expressions - variables and bookmarks
- quick base conversion/calculator
- addresses update as cursor if moved and during long searches/compares
- cf calculator

Navigations Points
- back/forw
- drop down lists - strictly chronological
- file will be reopened

Bookmarks
- named and unnamed
- prev/next
- bookmark tool
- can jump to even if the file is closed

Advanced Bookmarks
- same bookmark name in different files
- info tip window
- context menu
- remove all in current file
- bookmark name in jump tool

Calculator
- base/bits
- layout binop on right, unop on left
- expressions with brackets for precedence
- file operations
- memory/mark buttons
- integration with macros
- endian
- overflow

Variables
- types
- calculator
- jumps tools
- arrays
- functions
- clear


Macros
------

Macros
- recording/playing
- options
- stop on error to avoid mangled files
- Abort macros at any time by pressing Escape 

Macro Files
- macros files
- nested macros (unlimited)
- instructions
- AutoExec macros
- _ext macros

Advanced Macros
- multiples windows - next window or by name
- mark
- calc
- operations


Searching
---------

Find Tool
- exact
- case-insensitive
- hex
- background search occurrences
- Abort searches with the Escape key
- EBCDIC in EBC mode
- hist list

Find Dialog
- same hist list as find tool
- scope: after whole file it switches to rest of file
- text/hex wildcard (dv = ?)
- updates other pages and find tool

Masked Find
- individual bits

Aligned Find
- eg: number in field of db rec (header + fixed sizes db recs)


Tools
-----

Compare
- highlight

Conversions
- EBC<->ASC, OEM<->ANSI, ucase etc

Unary Arithmetic Operations

Operations Using Calc (Binary + Assign)

Encryption

Checksums

* Zlib

Import/Export


Help
----

Dialog
- options dialog
- modeless dialogs

Command
- status bar
- menu
- toolbar buttons
- tooltips

Misc
- TOD
- keymap
- email

Help Topics
- overview
- bin files
- browse seq
- index


File Handling
-------------

Opening Files
- dialog
  - multiple files
- command line
- recent file dialog
- filter list
- drag onto main window
- shell menu item

Recent File Dialog
- RO
- sort
- remove from list

File Type Filters
- editing
- open dialog
- explorer

Explorer
- split view incl orientation
- filtering incl history and cust list
- folder history
- HexEdit specific columns
- select/resize/move columns (retained)
- hidden files display
- context menu- open/ro

File Operations
- deleting
- change attr

File Windows
- tabs at top or bottom
- context menu
- multiple windows on same file
- next/prev window - Ctrl+Tab, Ctrl+Shift+Tab
- Windows menu
- Windows dialog

Templates
---------

Provided templates
- .EXE
- .BMP
- .IFF - recursive
- user supplied (thanks)

Template Creation
- settings
- dialogs
- previous field

Template Fields and Types
- int, real, date, string
- arrays
- global vars

Template domains
- check for errors
- display using enums

Template Expressions
- GetInt/GetBool/GetString

> Parser
http://www.youtube.com/watch?v=snL0_rfBDNo

* Floating Templates
- using mark
- using cursor

Advanced Parser Features
- display (format) in data fields
- enums
- display in struct
- unnamed "none" field for unused bytes

Template Files
- edit: change union (jumps with structs) into a switch

Template Parser Customization
- to match compiler
  - _standard_types.xml
  - keep backup
- own (custom) types and constants
- _Windows_types - all from windows headers + some MFC structs/classes


