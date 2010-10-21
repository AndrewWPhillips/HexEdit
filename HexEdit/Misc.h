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

// Macros
#define BRIGHTNESS(cc) ((GetRValue(cc)*30 + GetGValue(cc)*59 + GetBValue(cc)*11)/256)

// Why isn't there an abs() template function?
#define mac_abs(a)  ((a)<0?-(a):(a))

// Declarations
BOOL IsUs();
void get_hls(COLORREF rgb, int &hue, int &lightness, int &saturation);
COLORREF get_rgb(int hue, int luminance, int saturation);
COLORREF tone_down(COLORREF col, COLORREF bg_col, double amt = 0.75);
COLORREF same_hue(COLORREF col, int sat, int lum = -1);
COLORREF add_contrast(COLORREF col, COLORREF bg_col);

double TZDiff();
DATE FromTime_t(__int64 v);
DATE FromTime_t_80(long v);
DATE FromTime_t_mins(long v);
DATE FromTime_t_1899(long v);
bool ConvertToFileTime(time_t tt, FILETIME *ft);

CString NumScale(double val);
CString	bin_str(__int64 val,int bits);
void AddCommas(CString &str);
void AddSpaces(CString &str);
CString get_menu_text(CMenu *pmenu,int id);

#ifndef REGISTER_APP
void LoadHist(std::vector<CString> & hh, LPCSTR name, size_t smax);
void SaveHist(std::vector<CString> const & hh, LPCSTR name, size_t smax);

CString DecimalToString(const void * pdecimal, CString & sMantissa, CString & sExponent);
bool StringToDecimal(const char * ss, void * presult);

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

unsigned long rand1();
unsigned long rand2();
void rand_good_seed(unsigned long seed);
unsigned long rand_good();
#endif

unsigned short crc16(const void *buffer, size_t len);  // NOTE: also see RegisterDlg.cpp

unsigned long crc_32(const void *buffer, size_t len);
// Use the following for CRC 32 which is too big for a single buffer
void crc_32_init();
void crc_32_update(const void *buf, size_t len);
DWORD crc_32_final();

unsigned short crc_ccitt(const void *buf, size_t len);  // all in one CRC CCITT
void crc_ccitt_init();
void crc_ccitt_update(const void *buf, size_t len);
unsigned short crc_ccitt_final();

unsigned short crc_ccitt2(const void *buf, size_t len);  // all in one CRC CCITT
void crc_ccitt2_init(int init = -1);
void crc_ccitt2_update(const void *buf, size_t len);
unsigned short crc_ccitt2_final();

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
inline int SigDigits(__int64 val, int base = 10)
{
	ASSERT(base > 1);
	int retval = 0;
	for (retval = 0; val > 0; ++retval)
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
