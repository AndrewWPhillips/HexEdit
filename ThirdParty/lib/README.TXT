This ThirdParty\lib sub-directory contains libraries that are used to build 
HexEdit and for which no source code is included.

These libs are linked into HexEdit.exe and should work irrespective of which
version of VC++ (Visual Studio) is used to build HexEdit.

[In contrast the Crypto++ library depends on using the same CRT as HexEdit
and so the source is included in the ThirdParty\CryptoPP directory.]

Although the source code is not included they are open-source and you can
download and build them yourself if you want to.  I should warn that building
the correct version of MPIR is not simple.  Also, if you want to use a
different version of MPIR (eg, for later a later Intel instruction set) then
you can probably find some pre-built libs that others have kindly made.

Note that these also require DLLs at runtime (FreeImage.DLL, mpir.DLL) which
are included in the Git repository in the HexEdit\Output\Release directory.


mpir.lib - C Multi-precision maths library forked from GMP
mpirxx.lib - C++ class wrapper for mpir.lib   (see http://mpir.org/)

FreeImage.lib - FreeImage open source library (see freeimage.sourceforge.net/)
