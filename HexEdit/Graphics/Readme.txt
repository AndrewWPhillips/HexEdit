This directory contains the files used to create the graphics used in HexEdit and the installer.  There are "source" files (checked in to the repository) and working files.  Following are some notes on creating working files from source files as the process cannot always be automated.


Pliers
------

Pliers.cdr is a Corel Draw file containing the source image for the HexEdit icon/logo.  It is used as the source of several .BMP files as described below.

- created a high-resolution bitmap (PliersBlack.png) of 1400x2100 pixels
- shaded handles blue (using Gradient tool in Paint.Net)
- shaded rest red (background is transparent)
- saved as PliersShaded.png (1400x2100)

Icon
----

- TBD


Background
----------

The background of the main HexEdit window must be a .BMP file called Backgrnd.BMP.

- Using pliers.cdr create a high-resolution monochrome .PNG
- load the .PNG file into Paint.Net
- apply the Emboss effect (Effects/Stylize/Emboss) with an angle of -45
- increase brightness (Adjustments/Brightness+Contrast) so that the background is (192,192,192)
  - with PDN 1.36 a brightness value of 64 seems to do this
- resize the image (Image/Resize) to something appropriate such as a height of 200
- save as an 8-bit .BMP files called Backgrnd.BMP


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


Installer
---------

The installer requires two images...


