This lib sub-directory contains libraries that are used to build HexEdit.

Checked in libraries may be specific to VS2010 as noted below.
How to download and build them with later versions of VS is also explained.

Note that some libs also require a DLL at runtime (eg FreeImage.DLL,
MLtaskdialog.DLL and mpir.DLL).


FreeImage.lib - FreeImage open source library     (see http://freeimage.sourceforge.net/)
MLtaskdialogs.lib - open source library for creating Vista style task dialogs
mpir.lib - C Multi-precision maths library forked from GMP
mpirxx.lib - C++ class wrapper for mpir.lib               (see http://mpir.org/)
cryptlib.lib - Crypto++ library - release version for v100 (VS2010)
cryptlibD.lib - Crypto++ library - debug version for v100 (see https://www.cryptopp.com/)


Visual Studio

1. HexEdit is a 32-bit WIN32 project currently built with Visual Studio (v100
   toolset). It uses the CRT (and MFC) via a shared multi-threaded DLL.
2. It can only be built with VS2010 or later due to use of SSE2 intrinsics.
3. You can build HexEdit with later versions of Visual Studio than VS2010 as
   long as you use the Visual Studio 2010 Platform Toolset (v100). But you
   probably only have the v100 toolset if you also have VS2010 installed.
4. To build HexEdit with later toolsets (v110, v120, etc) then you need to
   rebuild (or obtain somehow) new Crypto++ and MPIR libraries (see below).


Crypto++

1. HexEdit links with the static library (not DLL) version (cryptlib.lib).
2. You must link to the correct version of the library (debug or release).
   The main reason is that Crypto++ conventions require that memory allocated
   in the main project be freed within cryptlib.lib. The memory allocation
   routines in the MS CRT release and debug libraries have different block formats,
   hence you can't mix allocation and freeing of a block between debug/release.
3. To build Crypto++ with a later version of VS for use with HexEdit:
	- download and extract Crypto++ (eg cryptopp563.zip)
	- download/extract vs2010.zip (overwrites crypttest.sln from above)
	- open crypttest.sln in Visual Studio
	- ensure correct toolset is selected (Properties/General/Platform Toolset)
	- change project to use CRT (C run-time library) in multi-threaded DLL
	  - Properties/C-C++/Code Generation/Runtime library
	    - for Release config use: Multi-threaded DLL
	    - for Debug config use: Multi-threaded Debug DLL
	- fix SHA-3 code (should be OK if Crypto++ 5.7 and later)
	  - near the end of sha3.cpp change this line:
		m_state.BytePtr()[m_counter] ^= 1;
	  - to this:
		m_state.BytePtr()[m_counter] ^= 6;
	- build release and debug versions of cryptlib.lib
	- copy ...Win32\Output\Release\cryptlib.lib to HexEdit\lib\cryptlib.lib
	- copy ...\Debug\cryptlib.lib to HexEdit\lib\cryptlibD.lib (note extra D)


MPIR

1. You need the 32-bit libraries specific to the version of VS (platform toolset).
2. You may be able to obtain versions of the lib/dll files for other versions of VS here:
   https://github.com/Legrandin/mpir-windows-builds
3. MPIR is highly optimized with some assembler and requires YASM to build.
4. To build MPIR with a later version of VS for use with HexEdit:
	- download and extract MPIR
	- download and install YASM.EXE into the VS bin folder
	  - eg: C:\Program Files\Microsoft Visual Studio 10.0\VC\bin\
	- open the project for the specific version of VS you are using
	  - eg for VS2012: build.vc11\mpir.sln
	- build the C++ wrapper (lib_mpir_cxx)
	- build the mpir version for the minimum processor required
	  - eg: lib_mpir_p0
	- copy mpir.lib and mpirxx.lib to the HexEdit\lib sub-directory
	  - eg from: build.vc11\lib\Win32\Release

FreeImage

1. Does not depend on specific MS CRT libraries so you can use the same version.
2. FreeImage.DLL is required at run-time.

MLTasksDialog

1. Does not depend on specific MS CRT libraries so you can use the same version.
2. MLTasksDialog.DLL is required at run-time.
