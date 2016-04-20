This include sub-directory contains header files used to interface to external libraries.
These files are not part of HexEdit but are placed here for convenience.

FreeImage.h - defines interface to the FreeImage library used in many parts of HexEdit (esp. Aerial View)
MLtaskdialogs.h - dialogs library for avoidable dialogs etc used throughout the GUI
mpir.h/mpirxx.h - C and C++ interfaces to GMP (Gnu mult. precision math lib) used in calculator for inf. precision integers
Crypto++/cryptlib.h - primary header needed for Crypto++ (currently justed used for digests)
Crypto++/md5.h - Crypto++ MD5 digest
Crypto++/sha.h - Crypto++ SHA-1 and SHA-2 variants
Crypto++/sha3.h - Crypto++ SHA-3 variants
Crypto++/* - other Crypto++ 5.6.3 support files
boost/tuple/tuple.hpp - for when pair<> is not enough
boost/crc.hpp - fast standard CRCs plus custom CRCs
boost/random/mersenne_twister.hpp - random integers
boost/* - other boost 1.60.0 supporting headers
