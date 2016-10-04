This ..\ThirdParty\include sub-directory contains header files used to 
interface to external libraries. These files are not part of HexEdit.

Note that NOT all Boost headers are included - only those needed to build HexEdit.

Current library versions used are Boost 1.60, FreeImage 3.17.0, MPIR 2.3.0.


boost/tuple/tuple.hpp - for when pair<> is not enough
boost/crc.hpp - fast standard CRCs plus custom CRCs
boost/random/mersenne_twister.hpp - random integers
boost/* - other boost 1.60.0 supporting headers
FreeImage.h - defines interface to the FreeImage library used in many parts of HexEdit (esp. Aerial View)
mpir.h/mpirxx.h - C and C++ interfaces to MPIR (mult. precision ints & rationals) used in calculator

Note: Crypto++ header files are now found in ..\..\ThirdParty\CryptoPP.
