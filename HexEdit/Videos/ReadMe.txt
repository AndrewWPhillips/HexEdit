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
  - playback rate 50 f/sec (makes editing easier)
- audio (from mike)
  - 22.05 KHz, mono, 16-bit
  - PCM
    - switch to MP3 then back to PCM to make sure it is correct
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
- edit the video
  - use SPACEBAR to play/stop video playback
  - use Del, Ctrl+X/C/V, Ctrl+Z/Y
- delete unwanted frames
  - turn on audio track display (View/Audio Display) for easier editing
  - use Home/End to mark area for deletion
    - IMPORTANT: turn off selection (Ctrl+D) before saving otherwise you only save the selection
- mask frames where you don't want the video but want to keep the audio (Edit/Mask Selection)
- save often to working copies as one time I could not save all my changes
- save final version with 5 fps and high compression
- Set Compression
  - no compression if saving for later edit
  - XVID MPEG4 gives good results
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

The scripts list was moved to EverNote (Devel/HexEdit/Video Scripts)
