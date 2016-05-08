This include sub-directory contains header files used to interface to external libraries.
These files are not part of HexEdit but are placed here for convenience.
Note that not all Boost and Crypto++ headers are included - only those need to build HexEdit.

Current library versions used are Boost 1.60, FreeImage 3.17.0, MPIR 2.3.0, Crypto++ 5.6.3.

boost/tuple/tuple.hpp - for when pair<> is not enough
boost/crc.hpp - fast standard CRCs plus custom CRCs
boost/random/mersenne_twister.hpp - random integers
boost/* - other boost 1.60.0 supporting headers
FreeImage.h - defines interface to the FreeImage library used in many parts of HexEdit (esp. Aerial View)
mpir.h/mpirxx.h - C and C++ interfaces to MPIR (mult. precision ints & rationals) used in calculator
Crypto++/cryptlib.h - primary header needed for Crypto++ (currently just used for digests)
Crypto++/md5.h - Crypto++ MD5 digest
Crypto++/sha.h - Crypto++ SHA-1 and SHA-2 variants
Crypto++/sha3.h - Crypto++ SHA-3 variants
Crypto++/base64.h - Crypto++ base64 encoding/decoding
Crypto++/* - other Crypto++ 5.6.3 support files
