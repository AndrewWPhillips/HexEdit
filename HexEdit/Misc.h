#ifndef MISC_INCLUDED_
#define MISC_INCLUDED_  1

// Misc.h

// Copyright (c) 2008-2010 by Andrew W. Phillips.
//
// No restrictions are placed on the noncommercial use of this code,
// as long as this text (from the above copyright notice to the
// disclaimer below) is preserved.
//
// This code may be redistributed as long as it remains unmodified
// and is not sold for profit without the author's written consent.
//
// This code, or any part of it, may not be used in any software that
// is sold for profit, without the author's written consent.
//
// DISCLAIMER: This file is provided "as is" with no expressed or
// implied warranty. The author accepts no liability for any damage
// or loss of business that this product may cause.
//

#include <istream>                  // used in << and >>
#include <mpir.h>                   // for big integer handling (mpz_t)

// Macros
#define BRIGHTNESS(cc) ((GetRValue(cc)*30 + GetGValue(cc)*59 + GetBValue(cc)*11)/256)

// Why isn't there an abs() template function?
#define mac_abs(a)  ((a)<0?-(a):(a))

// Declarations
BOOL IsUs();

// Colour manipulation
void get_hls(COLORREF rgb, int &hue, int &lightness, int &saturation);
COLORREF get_rgb(int hue, int luminance, int saturation);
COLORREF tone_down(COLORREF col, COLORREF bg_col, double amt = 0.75);
COLORREF same_hue(COLORREF col, int sat, int lum = -1);
COLORREF add_contrast(COLORREF col, COLORREF bg_col);

// Date/time
double TZDiff();
DATE FromTime_t(__int64 v);
DATE FromTime_t_80(long v);
DATE FromTime_t_mins(long v);
DATE FromTime_t_1899(long v);
bool ConvertToFileTime(time_t tt, FILETIME *ft);

// Number conversion
CString NumScale(double val);
CString	bin_str(__int64 val,int bits);
void AddCommas(CString &str);
void AddSpaces(CString &str);

// Round to nearest integer allowing for -ve values, halves always go away from zero eg -1.5 => -2.0
inline double fround(double r) { return (r > 0.0) ? floor(r + 0.5) : ceil(r - 0.5); }

CString get_menu_text(CMenu *pmenu,int id);

#ifndef REGISTER_APP
void LoadHist(std::vector<CString> & hh, LPCSTR name, size_t smax);
void SaveHist(std::vector<CString> const & hh, LPCSTR name, size_t smax);

CString Decimal2String(const void * pdecimal, CString & sMantissa, CString & sExponent);
bool String2Decimal(const char * ss, void * presult);

#ifdef _DEBUG
void TestDecimalRoutines();
#endif

bool make_real48(unsigned char pp[6], double val, bool big_endian = false);
double real48(const unsigned char *pp, int *pexp = NULL, long double *pmant = NULL, bool big_endian = false);
bool make_ibm_fp32(unsigned char pp[4], double val, bool little_endian = false);
bool make_ibm_fp64(unsigned char pp[8], double val, bool little_endian = false);
long double ibm_fp32(const unsigned char *pp, int *pexp = NULL,
			 long double *pmant = NULL, bool little_endian = false);
long double ibm_fp64(const unsigned char *pp, int *pexp = NULL,
			 long double *pmant = NULL, bool little_endian = false);

__int64 strtoi64(const char *, int radix = 0);
__int64 strtoi64(const char *, int radix, const char **endptr);
unsigned __int64 mpz_get_ui64(mpz_srcptr p);
void mpz_set_ui64(mpz_ptr p, unsigned __int64 i);
const char * mpz_set_bytes(mpz_ptr p, FILE_ADDRESS addr, int count);

void BrowseWeb(UINT id);
CString GetExePath();
BOOL GetDataPath(CString &data_path, int csidl = CSIDL_APPDATA);
void SetFileCreationTime(const char *filename, time_t tt);
void SetFileAccessTime(const char *filename, time_t tt);
CString FileErrorMessage(const CFileException *fe, UINT mode = CFile::modeRead|CFile::modeWrite);
bool OutsideMonitor(CRect);
CRect MonitorMouse();
CRect MonitorRect(CRect);
bool NeedsFix(CRect &rect);
bool CopyAndConvertImage(const char *src, const char *dest);
bool AbortKeyPress();

void DummyRegAccess(unsigned int group = 0);
unsigned long str_hash(const char *str);   // hash value from string

void rand_good_seed(unsigned long seed);
unsigned long rand_good();

int next_diff(const void * buf1, const void * buf2, size_t len);

#endif

// All in one CRCs
unsigned short crc_ccitt(const void *buf, size_t len);
unsigned long crc_32(const void *buffer, size_t len);

#ifdef BOOST_CRC
unsigned short crc_16(const void *buffer, size_t len);  // Boost crc 16
void * crc_16_init();
void crc_16_update(void * handle, const void *buf, size_t len);
unsigned short crc_16_final(void * handle);

unsigned short crc_xmodem(const void *buf, size_t len);
void * crc_xmodem_init();
void crc_xmodem_update(void * handle, const void *buf, size_t len);
unsigned short crc_xmodem_final(void * handle);
#endif

// Use the following for CRC calcs that are too big to do all at once
void * crc_32_init();
void crc_32_update(void * handle, const void *buf, size_t len);
unsigned long crc_32_final(void * handle);

void * crc_ccitt_init();
void crc_ccitt_update(void *, const void *buf, size_t len);
unsigned short crc_ccitt_final(void *);

unsigned short crc_ccitt_b(const void *buf, size_t len);  // not the real CRC CCITT
void * crc_ccitt_b_init();
void crc_ccitt_b_update(void * handle, const void *buf, size_t len);
unsigned short crc_ccitt_b_final(void * handle);

// General CRC
struct crc_params
{
	int bits;               // Number of bits (taken from bits_idx_)
	int dummy;              // not used
	unsigned __int64 poly;
	unsigned __int64  init_rem;
	unsigned __int64  final_xor;
	BOOL reflect_in;
	BOOL reflect_rem;
};

void load_crc_params(struct crc_params *par, LPCTSTR strParams);

void * crc_4bit_init(const struct crc_params * par);
void crc_4bit_update(void *hh, const void *buf, size_t len);
unsigned char crc_4bit_final(void *hh);

void * crc_8bit_init(const struct crc_params * par);
void crc_8bit_update(void *hh, const void *buf, size_t len);
unsigned char crc_8bit_final(void *hh);

void * crc_16bit_init(const struct crc_params * par);
void crc_16bit_update(void *hh, const void *buf, size_t len);
unsigned short crc_16bit_final(void *hh);

void * crc_32bit_init(const struct crc_params * par);
void crc_32bit_update(void *hh, const void *buf, size_t len);
unsigned long crc_32bit_final(void *hh);

// Encryption routines NOTE: also see RegisterDlg.cpp
void set_key(const char *pp, size_t len);
void encrypt(void *buffer, size_t len);
void decrypt(void *buffer, size_t len);

char letter_valid(char cc);
int letter_decode(const char *str, size_t str_len, void *result);
void letter_encode(const void *buf, size_t len, char *result);

// flip_bytes is typically used to switch between big- and little-endian byte order but
// works with any number of bytes (including an odd number whence middle byte not moved)
inline void flip_bytes(unsigned char *pp, size_t count)
{
	unsigned char cc;                   // Temp byte used for swap
	unsigned char *end = pp + count - 1;

	while (pp < end)
	{
		cc = *end;
		*(end--) = *pp;
		*(pp++) = cc;
	}
}

// flip bytes in each consecutive word
// if count is odd the last byte is not moved
inline void flip_each_word(unsigned char *pp, size_t count)
{
	ASSERT(count%2 == 0);               // There should be an even number of bytes
	unsigned char cc;                   // Temp byte used for swap
	for (unsigned char *end = pp + count; pp < end; pp += 2)
	{
		cc = *pp;
		*pp = *(pp+1);
		*(pp+1) = cc;
	}
}

// The number of significant digits in a number
inline int SigDigits(unsigned __int64 val, int base = 10)
{
	ASSERT(base > 1);
	int retval = 0;
	for (retval = 0; val != 0; ++retval)
		val /= base;
	return retval;
}

#ifndef _LONGLONG
// This is needed since std streams don't support __int64
inline std::istream &operator>>(std::istream &ss, __int64 &ii)
{
	char buf[22];
	for (char *pp = buf; pp < buf + sizeof(buf) - 1; ++pp)
	{
		if (!(ss >> *pp) || !isdigit(*pp))
		{
			if (ss && *pp != '\0')
				ss.putback(*pp);
			break;
		}
	}
	*pp = '\0';
	ii = ::strtoi64(buf);
//    sscanf(buf, "%I64d", &ii);
	if (!ss && pp > buf)
		ss.clear(ss.rdstate() & ~std::ios_base::failbit); // We actually read something so clear failbit
	else if (ss && pp == buf)
		ss.setstate(std::ios_base::failbit);              // We failed to read so set failbit

	return ss;
}

inline std::ostream &operator<<(std::ostream &ss, const __int64 &ii)
{
	char buf[22];
	sprintf(buf, "%I64d", ii);
	return ss << buf;
}
#endif // ndef _LONGLONG

inline BOOL IsDevice(LPCTSTR ss) { return _tcsncmp(ss, _T("\\\\.\\"), 4) == 0; }
// 0. \\.\D:                 - volume with drive letter D (works for NT and 9X)
// 1. \\.\PhysicalDriveN     - Nth physical drive (including removeable but not floppies)
// 2. \\.\FloppyN            - Nth floppy drive
// 3. \\.\CdRomN             - Nth CD/DVD drive
// NOTE 1, 2 and 3 are converted internally into device names used in NT native
//      API (eg "\\.\Floppy0" => "\device\Floppy0") because FloppyN type names
//      do not work and CdRomN type names only work under XP.
// 4. \device\               - NO LONGER USED
BSTR GetPhysicalDeviceName(LPCTSTR name); // get device name to use with native API (for 1,2,3 above)
CString DeviceName(CString name);         // get nice display name for device file name
int DeviceVolume(LPCTSTR filename);       // get volume (0=A:, 1=B: etc) of physical device (optical/removeable only) or -1 if none

#endif

#ifdef _DEBUG
void test_misc();
#endif
