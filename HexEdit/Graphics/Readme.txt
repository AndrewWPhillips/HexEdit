This directory contains the files used to create the graphics used in HexEdit and the installer.  There are "source" files (checked in to the repository) and working files.  Following are some notes on creating working files from source files as the process cannot always be automated.


Pliers
------

Pliers.cdr is a Corel Draw file containing the source image for the HexEdit icon/logo.  It is used as the source of several .BMP files as described below.


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

- TBD


Installer
---------

The installer requires two images...


