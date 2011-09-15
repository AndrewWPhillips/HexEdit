This directory contains the files used to create the graphics used in HexEdit and the installer.  There are "source" files (checked in to the repository) and working files.  Following are some notes on creating working files from source files as the process cannot always be automated.

Currently there are 3 images distributed with HexEdit:

About.bmp - background to about dialog
Backgrnd.bmp - background to main window
Splash.bmp - displayed breifly at startup

If these have a plain background (as currently backgrnd.bmp and splash.bmp do) then they should be saved as 8-bit BMP files with RLE on.  PDN 3.36 supports 8-bit BMP files but does not use RLE so use something like Photoshop Elements to get the RLE option.


Pliers
------

Pliers.cdr is a Corel Draw file containing the source image for the HexEdit icon/logo.

It was created using Corel Draw's vector editing tools (spline etc).  For example, one handle
was simply created by drawing a bezier line with 3 bezier points and width of 10 mm.  It was
then converted to an outline using Arrange/Outline To Object, then one end was made round
by adding a point and making the lines into curves and rounding the points.  Finally,
the other handle was created using the Windows/Dockers/Transformations/Scale and reflecting
about a vertical axis.

Pliers.cdr is used as the source of several .BMP files as described below.

- created a high-resolution bitmap (PliersBlack.png) of 1400x2100 pixels
  - make white areas transparent before resizing to get edges antialiased with background
- shaded handles blue (using Gradient tool in Paint.Net)
- shaded rest red (background is transparent)
- saved as PliersShaded.png (1400x2100)


Background
----------

The background of the main HexEdit window must be a .BMP file called Backgrnd.BMP.

- Using pliers.cdr create a high-resolution monochrome .PNG
- load the .PNG file into Paint.Net
- apply the Emboss effect (Effects/Stylize/Emboss) with an angle of -45
- increase brightness (Adjustments/Brightness+Contrast) so that the background is (192,192,192)
  - with PDN 1.36 use brightness:44, contrast:32
- resize the image (Image/Resize) to something appropriate such as a height of 200
- save as an 8-bit .BMP files called Backgrnd.BMP
- load into Photoshop Elements and resave with RLE


About box
---------

The controls in the About box are transparent.  The background image is called About.BMP.

- Created About.PDN used as source for About.BMP
  - has text of left side
  - right side is left blankish for controls
- Used above PliersShaded.png (from pliers.CDR) to create Pliers layer
  - resized (IMage/Resize) PliersShaded.png from height 2100 to 180 pixels (best quality)
  - selected all and pasted into a new layer into About.PDN
  - note: get the size right using Image/Resize since resize after paste has artifacts
- Added text layer
  - Used font Swis721 Blk Rnd BT (from Corel X4 CD)
  - remember to make it antialiased
  - added "he" above the pliers at size 48 and shaded blue
  - added "edit" below the pliers at size 36 and shaded red
- Added version layer
  - added "3.4" at bottom of left side
- Added background layer and filled with (192,192,192)
  - this may be changed to embossed or hextile background later
- Added layer for drop-shadow above background
  - duplicated all layers to have shadows (pliers. text, version)
  - changed their colour to black using fill
  - made into shadow using Guassian blur (Effects/Blurs/Guassian) (Radius = 4)
  - offset the shadow down and to right using Move Selected Pixels tool
- Added another layer above background layer and below drop-shadow layer
  - paste hextile into it then pasted again next to it to fill up the whole image size
  - used gradient fill tool (transprency mode) so background shows through towards bottom right
  - the combined effect is a slightly more interesting background
- Saved as .BMP (best size and quality via Photoshop Elements)
  - save as 1 24-bit .BMP from Paint.Net
  - load into Photoshop Elements and convert to 8-bit (Image/Mode/Indexed color)
  - save as 8-bit Windows .BMP file with RLE compression


Splash
------

- created Splash.PDN as source for Splash.BMP at size 400x200
- created white background layer (actually very light shading using gradient tool)
- created pliers layer by simply copying pliers from About.PDN (above)
- created text layer with "he" to left of the pliers and "edit" to the right
  - Used font Swis721 Blk Rnd BT again with size 72
  - letters were to be shaded so used shade halfway between when creating the text
  - make sure that anti-aliasing is on
  - selected letters using magic wand (tolerance of 30%) before shading
    - less tolerance means that edge pixels near top and bottom had noticeably wrong shade
    - more tolerance and too many edge pixels were being made opaque
  - shaded the letters using gradient tool
- created version layer with "3.4" in the bottom right corner
  - using light grey and Swis721 Blk Rnd BT size 24 (anti-aliased)
- reduced size to 33% and saved as Splash.bmp (about 10Kb)
- opened in PhotoShop and resaved with RLE (about 5Kb)


Installer
---------

The installer requires two images...


Icon
----

Create an icon at 256x256 and save as PNG.
Run RW Icon Editor, click "Create New" top icon in side bar.
Double-click icon from image and create a Vista application icon.
- creates 4 different sized icons at 3 different colour depths
Probably turn off "Enhance icon edges by a shadow effect".
Save the icon.

Toolbars
--------

HexEdit 3.5 moved to shaded toolbars (see #define SHADED_TOOLBARS) with separate cold/hot/disabled images. There are 4 toolbars (toolbar, editbar, navbar and formtbar), plus 2 extra toolbars (operations and misc).  Note that the "old toolbar" (mainbar.bmp) has finally been retired.

The shaded images are used using the new overload to CBCGToolbar::LoadToolbar (and AddToolBarForImageCollection), which has the following idiosynchrasies:

- there are 3 toolbars allowed for cold (normal), hot (hovered) and disabled
  - if not specified the disabled images are generated automatically but you must do the same for each toolbar
  - if not specified the hot image is the same as the cold one
- the bitmaps passed to the first call to LoadToolbar determine the quality of the images
  - I think all toolbar images are stored in the same bitmap so the first call determines this bitmaps format
  - the hot, cold and disabled images seem to be independent
  - we need to pass 24-bit .BMP images for (at least) the first toolbar (IDR_STDBAR)
- the background colour can be specified but we use the default - light grey (192,192,192) or 0xC0C0C0

The naming conventions for resource IDs and filenames are of the form (using editbar as an example):

IDB_EDITBAR_C  editbarCold.bmp
IDB_EDITBAR_D  editbarDisabled.bmp
IDB_EDITBAR_H  editbarHot.bmp

The 3 images for each toolbar can be created from there corresponding Paint.Net file (eg Graphics\editbar.pdn) in this way:

- make the "hot" image
  x change colour of background layer to XP toolbar background colour (239,236,221)
  - merge down all image layers if there are more than one
  - select layer with images
  - increase saturation of images layer to 140
  x merge images layer with background layer (merge down)
  - fill background layer with (192,192,192)
  - save as 24-bit .BMP with "hot" suffix (eg editbarHot.BMP)
- make the "cold" (normal) image
  - reopen the .PDN file (eg editbar.PDN)
  - merge down all image layers if there are more than one
  - select layer with toolbar images
  - open Hue + Saturation dialog (Adjustment/ Hue + Saturation)
  - reduce saturation from 100 down to 80
    - note that background colour is in diff layer and stays the same
  - fill background with (192,192,192)
  - save as 24-bit .BMP file (eg editbarCold.BMP)
- make the disabled image [not currently done]
  - reopen the .PDN file (eg editbar.PDN)
  - merge down all image layers if there are more than one
  - select layer with images
  - open Hue + Saturation dialog and reduce saturation to 0
  - open Brightness + Contrast dialog (Adjustment/Brightness+Contrast)
    - reduce contrast to -60, increase brightness to 50
  - fill background layer with (192,192,192)
  - save as .BMP file (eg editbarDisabled.BMP)
- reduce size of all .BMP files by making them 8-bit with RLE compression
  - load each .BMP into Photoshop Elements
  - convert to 8-bit if necessary (Image/Mode/Indexed Color) using Palette: Local (perceptual)
  - save to .BMP in RES directory with RLE option on (eg RES\editbarHot.bmp)


