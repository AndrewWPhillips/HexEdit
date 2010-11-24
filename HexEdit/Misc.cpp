// Misc.cpp : miscellaneous routines
//
// Copyright (c) 1999-2010 by Andrew W. Phillips.
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

/*

MODULE NAME:    NUMSCALE - OLD version see below for new one

Usage:          CString NumScale(double val)

Returns:        A string containing the scaled number (see below)

Parameters:     val = a value to be scaled

Description:    Converts a number to a string which is more easily read.
				At most 5 significant digits are generated (and hence
				there is a loss of precision) to aid readability.  If
				scaling is necessary a metric modifier is appended to
				the string.  Eg:

				0.0 becomes "0 "
				0.1 becomes "0 "
				1.0 becomes "1 "
				100.0 becomes "100 "
				99999.0 becomes "99,999 "
				100000.0 becomes "100 K"
				99999000.0 becomes "99,999 K"
				100000000.0 becomes "100 M"
				99999000000.0 becomes "99,999 M"
				100000000000.0 becomes "100 G"
				99999000000000.0 becomes "99,999 G"
				100000000000000.0 becomes "100 T"
				99999000000000000.0 becomes "99,999 T"
				100000000000000000.0 returns "1.000e16 "

				Note that scaling values between 1 and -1  (milli, micro, etc)
				are not supported (produce 0).  Negative numbers are scaled
				identically to positive ones but preceded by a minus sign.
				Numbers are rounded to the nearest whole number.

------------------------------------------------------
MODULE NAME:    NUMSCALE - Creat abbreviated number

Usage:          CString NumScale(double val)

Returns:        A 6 char string containing the abbreviated number (see below)

Parameters:     val = a value to be scaled

Description:    Converts a number to a string which is more easily read.
				At most 3 significant digits are generated (and hence
				there is a loss of precision) to aid readability.  If
				scaling is necessary a metric modifier is appended to
				the string.  Eg:

				0.0     becomes  "    0  "
				0.1     becomes  "    0  "
				1.0     becomes  "    1  "
				999     becomes  "  999  "
				-999    becomes  " -999  "
				9999    becomes  " 9.99 K"
				-9999   becomes  "-9.99 K"
				99999   becomes  " 99.9 K"
				999999  becomes  "  999 K"
				1000000 becomes  " 1.00 M"
				1e7     becomes  " 10.0 M"
				1e8     becomes  "  100 M"
				1e9     becomes  " 1.00 G"
				1e12    becomes  " 1.00 T"
				1e15    becomes  " 1.00 P"
				1e18    becomes  " 1.00 E"
				1e21    becomes  " 1.00 Z"
				1e24    becomes  " 1.00 Y"
				1e27    becomes  " 1.0e27"

				Note that scaling values between 1 and -1  (milli, micro, etc)
				are not supported (produce 0).  Negative numbers are scaled
				identically to positive ones but preceded by a minus sign.
				Numbers are rounded to the nearest whole number.

------------------------------------------------------
MODULE NAME:    ADDCOMMAS - Add commas to string of digits

Usage:          void AddCommas(CString &str)

Parameters:     str = string to add commas to

Description:    Adds commas to a string of digits.  The addition of commas
				stops when a character which is not a digit (or comma or
				space) is found and the rest of the string is just appended
				to the result.

				The number may have a leading plus or minus sign and contain
				commas and spaces.  The sign is preserved but any existing commas
				and spaces are removed, unless they occur after the comma
				addition has stopped.

				eg. "" becomes ""
				eg. "1" becomes "1"
				eg. "A,B C" becomes "A,B C"
				eg. "1234567" becomes "1,234,567"
				eg. " - 1 , 234" becomes "-1,234"
				eg. "+1234 ABC" becomes "+1,234 ABC"
				eg. "1234 - 1" becomes "1,234 - 1"
------------------------------------------------------
MODULE NAME:    ADDSPACES - Add spaces to string of hex digits

Usage:          void AddSpaces(CString &str)

Parameters:     str = string to add spaces to

Description:    Like AddCommas() above but adds spaces to a hex number rather
				than commas to a decimal number.

*/

#include "stdafx.h"
#include <MultiMon.h>

#ifndef REGISTER_APP
#include "HexEdit.h"
#endif
#include <math.h>
#include <ctype.h>
#include <assert.h>
#include <locale.h>
#include <sys/stat.h>           // For _stat()
#include <sys/utime.h>          // For _utime()
#ifdef BOOST_CRC
#include <boost/crc.hpp>
#endif
#include <imagehlp.h>           // For ::MakeSureDirectoryPathExists()
#include <winioctl.h>           // For DISK_GEOMETRY, IOCTL_DISK_GET_DRIVE_GEOMETRY etc
#include <direct.h>             // For _getdrive()
#include <FreeImage.h>

#include "misc.h"
#include "Security.h"
#include "ntapi.h"
#include "BigInteger.h"        // Used in C# Decimal conversions

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifndef REGISTER_APP

//-----------------------------------------------------------------------------
// Routines for loading/saving history lists in the registry

// LoadHist reads a list of history strings from the registry
void LoadHist(std::vector<CString> & hh, LPCSTR name, size_t smax)
{
	CString ss, entry;
	CString fmt = CString(name) + "%d";

	hh.clear();
	for (int ii = smax; ii > 0; ii--)
	{
		entry.Format(fmt, ii);
		ss = theApp.GetProfileString("History", entry);
		if (!ss.IsEmpty())
			hh.push_back(ss);
	}
}

// SaveHist writes history strings to the registry for later reading by LoadHist
void SaveHist(std::vector<CString> const & hh, LPCSTR name, size_t smax)
{
	CString entry;
	CString fmt = CString(name) + "%d";

	int ii, num = min(hh.size(), smax);

	// Write th new entries
	for (ii = 1; ii <= num; ++ii)
	{
		// Check that we don't write any empty strings as this will cause problems
		ASSERT(!hh[hh.size()-ii].IsEmpty());

		entry.Format(fmt, ii);
		theApp.WriteProfileString("History", entry, hh[hh.size()-ii]);
	}
	// Wipe out any old entries past the new end
	for ( ; ; ++ii)
	{
		entry.Format(fmt, ii);

		// Stop when there are no more entries in the registry to be blanked out.
		if (theApp.GetProfileString("History", entry).IsEmpty())
			break;

		theApp.WriteProfileString("History", entry, NULL);
	}
}

/* Copies an image file from source to dest, converting to a 24-bit .BMP file in the process.
 *  src  = file name of an existing image file
 *  dest = created file
 *         - overwritten if it already exists
 *         - the path folders are created if necessary
 */
bool CopyAndConvertImage(const char *src, const char *dest)
{
	FREE_IMAGE_FORMAT fmt = FreeImage_GetFileType(src);  // Try to work out the file type
	if (fmt == FIF_UNKNOWN)
	{
		fmt = FreeImage_GetFIFFromFilename(src);    // That didn't work so try this
		if (fmt == FIF_UNKNOWN)
			return false;
	}

	// Load image letting FreeImage decide what type it is
	FIBITMAP * dib, * dib24;
	if ((dib = FreeImage_Load(fmt, src)) == NULL)
		return false;

	// We always save as a 24-bit BMP - so first convert to 24 bits
	dib24 = FreeImage_ConvertTo24Bits(dib);
	FreeImage_Unload(dib);

	// Save image as a BMP
	bool retval = true;
	if (dib24 == NULL ||
		!::MakeSureDirectoryPathExists(dest) ||
		!FreeImage_Save(FIF_BMP, dib24, dest))
	{
		retval = false;
	}

	FreeImage_Unload(dib24);

	// Make the file times of the copied file the same as the original file
	struct _stat status;                // used to get file times of source file
	if (retval && _stat(src, &status) == 0)
	{
		struct _utimbuf times;          // Structure used to change file times
		times.actime = times.modtime = status.st_mtime;
		_utime(dest, &times);
	}

	return retval;
}

//-----------------------------------------------------------------------------
// Routines for handling C# Decimal type

// Notes on behaviour of C# Deciaml type:
// If mantissa is zero, exponent is ignored
// If any bit of bottom byte is one then it is -ve
// If exponent is > 28 then is printed as stored but when used in calcs is treated as if exponent is zero
// If exponent byte is 127 then calcs crash with null reference exception

// Example decimals and how they display:
// 12345678901234567890123456789  28   -> 1.2345678901234567890123456789
// 10000000000000000000000000000  28   -> 1.0000000000000000000000000000
// 10000000000000000000000000000  27   -> 10.000000000000000000000000000
// 12345678901234567890123456789  0    -> 12345678901234567890123456789
// 1                              -27  -> 0.000000000000000000000000001

bool StringToDecimal(const char *ss, void *presult)
{
	// View the decimal (result) as four 32 bit integers
	BigInteger::Unit *dd = (BigInteger::Unit *)presult;

	BigInteger mant;            // Mantisssa - initially zero
	BigInteger ten(10L);        // Just used to multiply values by 10
	int exp = 0;                // Exponent
	bool dpseen = false;        // decimal point seen yet?
	bool neg = false;           // minus sign seen?

	// Scan the characters of the value
	const char *pp;
	for (pp = ss; *pp != '\0'; ++pp)
	{
		if (*pp == '-')
		{
			if (pp != ss)
				return false;      // minus sign not at start
			neg = true;
		}
		else if (isdigit(*pp))
		{
			BigInteger::Mul(mant, ten, mant);
			BigInteger::Add(mant, BigInteger(long(*pp - '0')), mant);
			if (dpseen) ++exp;  // Keep track of digits after decimal pt
		}
		else if (*pp == '.')
		{
			if (dpseen)
				return false;    // more than one decimal point
			dpseen = true;
		}
		else if (*pp == 'e' || *pp == 'E')
		{
			char *end;
			exp -= strtol(pp+1, &end, 10);
			pp = end;
			break;
		}
		else
			return false;       // unexpected character
	}
	if (*pp != '\0')
		return false;           // extra characters after end

	if (exp < -28 || exp > 28)
		return false;          // exponent outside valid range

	// Adjust mantissa for -ve exponent
	if (exp < 0)
	{
		BigInteger tmp;
		BigInteger::Exp(ten, BigInteger(long(-exp)), tmp);
		BigInteger::Mul(mant, tmp, mant);
		exp = 0;
	}

	// Check for mantissa too big.
	if (mant.GetUnitLength() > 4)
		return false;

	// Set integer part
	dd[2] = mant.GetUnit(2);
	dd[1] = mant.GetUnit(1);
	dd[0] = mant.GetUnit(0);

	// Set exponent and sign
	dd[3] = exp << 16;
	if (neg && !mant.IsZero())
		dd[3] |= 0x80000000;

	return true;
}

CString DecimalToString(const void *pdecimal, CString &sMantissa, CString &sExponent)
{
	// View the decimal as four 32 bit integers
	const BigInteger::Unit *dd = (const BigInteger::Unit *)pdecimal;

	// Get the 96 bit integer part (mantissa)
	BigInteger big(dd, 3);
	big.ToString(sMantissa.GetBuffer(64), 10);
	sMantissa.ReleaseBuffer();

	// Get the exponent
	int exp = (dd[3] >> 16) & 0xFF;
	sExponent.Format("%d", - exp);

	// Check that unused bits are zero and that exponent is within range
	if ((dd[3] & 0x0000FFFF) != 0 || exp > 28)
		return CString("Invalid Decimal");

	// Create a string in which we insert a decimal point
	CString ss = sMantissa;
	int len = ss.GetLength();

	if (exp >= len)
	{
		// Add leading zeroes
		ss = CString('0', exp - len + 1) + ss;
		len = exp + 1;
	}

	// if any bit of last byte is on then assume the value is -ve
	if ((dd[3] & 0xFF000000) != 0)
	{
		sMantissa.Insert(0, '-');
		return "-" + ss.Left(len - exp) + CString(".") + ss.Right(exp);
	}
	else
		return ss.Left(len - exp) + CString(".") + ss.Right(exp);
}
#endif  // #ifndef REGISTER_APP

// -------------------------------------------------------------------------
// Colour routines

static int hlsmax = 100;            // This determines the ranges of values returned
static int rgbmax = 255;

void get_hls(COLORREF rgb, int &hue, int &luminance, int &saturation)
{

	int red = GetRValue(rgb);
	int green = GetGValue(rgb);
	int blue = GetBValue(rgb);
	int cmax = max(red, max(green, blue));
	int cmin = min(red, min(green, blue));

	// Work out luminance
	luminance = ((cmax+cmin)*hlsmax + rgbmax)/(2*rgbmax);

	if (cmax == cmin)
	{
		hue = -1;
		saturation = 0;
	}
	else
	{
		// Work out saturation
		if (luminance <= hlsmax/2)
			saturation = ((cmax-cmin)*hlsmax + (cmax+cmin)/2) / (cmax+cmin);
		else
			saturation = ((cmax-cmin)*hlsmax + (2*rgbmax-cmax-cmin)/2)/(2*rgbmax-cmax-cmin);

		// Work out hue
		int Rdelta = ((cmax-red  )*(hlsmax/6) + (cmax-cmin)/2) / (cmax-cmin);
		int Gdelta = ((cmax-green)*(hlsmax/6) + (cmax-cmin)/2) / (cmax-cmin);
		int Bdelta = ((cmax-blue )*(hlsmax/6) + (cmax-cmin)/2) / (cmax-cmin);
		if (red == cmax)
			hue = Bdelta - Gdelta;
		else if (green == cmax)
			hue = (hlsmax/3) + Rdelta - Bdelta;
		else
		{
			ASSERT(blue == cmax);
			hue = ((2*hlsmax)/3) + Gdelta - Rdelta;
		}
		
		if (hue < 0)
			hue += hlsmax;
		if (hue > hlsmax)
			hue -= hlsmax;
	}
}

static int hue2rgb(int n1, int n2, int hue)
{
	// Keep hue within range
	if (hue < 0)
		hue += hlsmax;
	else if (hue > hlsmax)
		hue -= hlsmax;

	if (hue < hlsmax/6)
		return n1 + ((n2-n1)*hue + hlsmax/12)/(hlsmax/6);
	else if (hue < hlsmax/2)
		return n2;
	else if (hue < (hlsmax*2)/3)
		return n1 + ((n2-n1)*((hlsmax*2)/3 - hue) + hlsmax/12)/(hlsmax/6);
	else
		return n1;
} 

COLORREF get_rgb(int hue,int luminance, int saturation)
{
	int red, green, blue;
	int magic1, magic2;

	if (hue == -1 || saturation == 0)
	{
		red = green = blue = (luminance*rgbmax)/hlsmax;
	}
	else
	{
		if (luminance <= hlsmax/2)
			magic2 = (luminance*(hlsmax + saturation) + hlsmax/2)/hlsmax;
		else
			magic2 = luminance + saturation - (luminance*saturation + hlsmax/2)/hlsmax;
		magic1 = 2*luminance - magic2;

		// get RGB, changing units from HLSMAX to RGBMAX
		red =   (hue2rgb(magic1, magic2, hue + hlsmax/3)*rgbmax + hlsmax/2)/hlsmax;
		green = (hue2rgb(magic1, magic2, hue           )*rgbmax + hlsmax/2)/hlsmax;
		blue =  (hue2rgb(magic1, magic2, hue - hlsmax/3)*rgbmax + hlsmax/2)/hlsmax;
	}

   return RGB(red, green, blue);
} 

// Make a colour (col) closer in tone (ie brightness or luminance) to another
// colour (bg_col) but the otherwise keep the same colour (hue/saturation).
// The parameter amt determines the amount of adjustment.
//  0 = no change
//  0.5 = default means half way to background
//  1 = fully toned down (same as background colour)
//  -ve values = tone "up"
// Note. Despite the name if bg_col is brighter than col it will "tone up".
COLORREF tone_down(COLORREF col, COLORREF bg_col, double amt /* = 0.75*/)
{
	int hue, luminance, saturation;
	get_hls(col, hue, luminance, saturation);
	
	int bg_hue, bg_luminance, bg_saturation;
	get_hls(bg_col, bg_hue, bg_luminance, bg_saturation);
	
	int diff = bg_luminance - luminance;
	
	// If toning "up" make sure that there is some change in the colour
	if (diff == 0 && amt < 0.0)
		diff = luminance > 50 ? 1 : -1;
	luminance += int(diff*amt);
	if (luminance > 100)
		luminance = 100;
	else if (luminance < 0)
		luminance = 0;
	
	// Make colour the same shade as col but less "bright"
	return get_rgb(hue, luminance, saturation);
}

// Increase contrast (tone up) if necessary.
// If a colour (col) does not have enough contrast with a background
// colour (bg_col) then increase the contrast.  This is similar to
// passing a -ve value  to tone_up (above) but "tones up" conditionally.
// Returns col with adjusted luminance which may be exactly the
// colour of col if therre is already enough contrast.
COLORREF add_contrast(COLORREF col, COLORREF bg_col)
{
	int hue, luminance, saturation;
	get_hls(col, hue, luminance, saturation);
	
	int bg_hue, bg_luminance, bg_saturation;
	get_hls(bg_col, bg_hue, bg_luminance, bg_saturation);
	
	if (bg_luminance >= 50 && luminance > bg_luminance - 25)
	{
		// Decrease luminance to increase contrast
		luminance = bg_luminance - 25;
	}
	else if (bg_luminance < 50 && luminance < bg_luminance + 25)
	{
		// Increase luminance to increase contrast
		luminance = bg_luminance + 25;
	}
	assert(luminance <= 100 && luminance >= 0);
	
	// Make colour the same shade as col but less "bright"
	return get_rgb(hue, luminance, saturation);
}

// This gets a colour of the same hue but with adjusted luminance and/or
// saturation.  This can be used for nice colour effects.
// Supply values for lum and sat in the range 0 - 100 or
// use -1 for lum or sat to not change them.
COLORREF same_hue(COLORREF col, int sat, int lum /* = -1 */)
{
	int hue, luminance, saturation;
	get_hls(col, hue, luminance, saturation);

	if (lum > -1)
		luminance = lum;
	if (sat > -1)
		saturation = sat;
	
	return get_rgb(hue, luminance, saturation);
}

// -------------------------------------------------------------------------
// Time routines

double TZDiff()
{
	static double tz_diff = -1e30;

	if (tz_diff != -1e30)
		return tz_diff;
	time_t dummy = time_t(1000000L);
	tz_diff = (1000000L - mktime(gmtime(&dummy)))/86400.0;
	return tz_diff;
}

DATE FromTime_t(__int64 v)  // Normal time_t and time64_t (secs since 1/1/1970)
{
	return (365.0*70.0 + 17 + 2) + v/(24.0*60.0*60.0) + TZDiff();
}

DATE FromTime_t_80(long v)
{
	return (365.0*80.0 + 19 + 2) + v/(24.0*60.0*60.0) + TZDiff();
}

DATE FromTime_t_mins(long v)
{
	return (365.0*70.0 + 17 + 2) + v/(24.0*60.0) + TZDiff();
}

DATE FromTime_t_1899(long v)
{
	return v/(24.0*60.0*60.0) + TZDiff();
}

// Convert time_t to FILETIME
bool ConvertToFileTime(time_t tt, FILETIME *ft)
{
	assert(ft != NULL);
	bool retval = false;

	struct tm * ptm = ::localtime(&tt);
	if (ptm != NULL)
	{
		SYSTEMTIME st;
		st.wYear   = ptm->tm_year + 1900;
		st.wMonth  = ptm->tm_mon + 1;
		st.wDay    = ptm->tm_mday;
		st.wHour   = ptm->tm_hour;
		st.wMinute = ptm->tm_min;
		st.wSecond = ptm->tm_sec;
		st.wMilliseconds = 500;

		FILETIME ftemp;
		if (::SystemTimeToFileTime(&st, &ftemp) && ::LocalFileTimeToFileTime(&ftemp, ft))
			retval = true;
	}
	return retval;
}

//-----------------------------------------------------------------------------
// Make nice looking numbers etc

#if 0
CString NumScale(double val)
{
	CString retval;
	char unitmod = '\0';
	char sep_char = ',';                // Used to separate groups of digits
	struct lconv *plconv = localeconv(); // Pointer to locale info

	// Note we're always grouping digits into thousands (groups of 3) but if
	// native locale also used groups of 3 then use its thousands separator.
	if (strlen(plconv->thousands_sep) == 1 && *plconv->grouping == 3)
		sep_char = *plconv->thousands_sep;

	// Allow for negative values (may give "-0")
	if (val < 0.0)
	{
		val = -val;
		retval = "-";
	}
	else
		retval = "";

	// If too big just print in scientific notation
	if (val >= 99999.5e12)
	{
		retval.Format("%.2e ", val);
		return retval;
	}

	if (val >= 99999.5)
	{
		unitmod = 'K';
		val = val / 1000.0;
	}
	if (val >= 99999.5)
	{
		unitmod = 'M';
		val = val / 1000.0;
	}
	if (val >= 99999.5)
	{
		unitmod = 'G';
		val = val / 1000.0;
	}
	if (val >= 99999.5)
	{
		unitmod = 'T';
		val = val / 1000.0;
	}

	val += 0.5;         /* Round rather than truncate */

	if (val >= 1000.0)
		retval.Format("%ld%c%03ld %c", (long)val/1000L, sep_char, (long)val%1000L, unitmod);
	else
		retval.Format("%d %c", (int)val, unitmod);

	return retval;
}
#endif

CString NumScale(double val)
{
	double dd = val;
	CString retval;
	bool negative = false;
	static const char *unit_str = " KMGTPE";  // (unit), kilo, mega, giga, tera, peta, exa

	// Allow for negative values (may give "-0")
	if (dd < 0.0)
	{
		dd = -dd;
		negative = true;
	}

	size_t idx;
	for (idx = 0; dd + 0.5 >= (idx >= theApp.k_abbrev_ ? 1000.0 : 1024.0); ++idx)
		dd = dd / (idx >= theApp.k_abbrev_ ? 1000.0 : 1024.0);

	// If too big just print in scientific notation
	if (idx >= strlen(unit_str))
	{
		retval.Format("%6.1g ", val);   // Just output original value with exponent
		return retval;
	}

	if (idx == 0 || dd + 0.5 >= 100.0)
	{
		dd = floor(dd + 0.5);                   // Remove fractional part so %g does not try to show it
		retval.Format("%5g %c", negative ? -dd : dd, unit_str[idx]);
	}
	else
		retval.Format("%#5.3g %c", negative ? -dd : dd, unit_str[idx]);

	return retval;
}

// Make a string of binary digits from a number (64 bit int)
CString bin_str(__int64 val, int bits)
{
	ASSERT(bits <= 64);
	char buf[100], *pp = buf;

	for (int ii = 0; ii < bits; ++ii)
	{
		if (ii%8 == 0)
			*pp++ = ' ';
		*pp++ = (val&(_int64(1)<<ii)) ? '1' : '0';
	}
	*pp = '\0';

	CString retval(buf+1);
	retval.MakeReverse();
	return retval;
}

#ifndef REGISTER_APP

// Add commas every 3 digits (or as appropriate to the locale) to a decimal string
void AddCommas(CString &str)
{
	const char *pp, *end;                   // Ptrs into orig. string
	int ndigits = 0;                        // Number of digits in string
//	struct lconv *plconv = localeconv(); // Locale settings (grouping etc) are now just read once in InitInstance

	// Allocate enough space (allowing for space padding)
	char *out = new char[(str.GetLength()*(theApp.dec_group_+1))/theApp.dec_group_ + 2]; // Result created here
	char *dd = out;                     // Ptr to current output char

	// Skip leading whitespace
	pp = str.GetBuffer(0);
	while (isspace(*pp))
		pp++;

	// Keep initial sign if any
	if (*pp == '+' || *pp == '-')
		*dd++ = *pp++;

	// Find end of number and work out how many digits it has
	end = pp + strspn(pp, ", \t0123456789");
	for (const char *qq = pp; qq < end; ++qq)
		if (isdigit(*qq))
			++ndigits;

	for ( ; ndigits > 0; ++pp)
		if (isdigit(*pp))
		{
			*dd++ = *pp;
			if (--ndigits > 0 && ndigits%theApp.dec_group_ == 0)
				*dd++ = theApp.dec_sep_char_;
		}
	ASSERT(pp <= end);
	strcpy(dd, pp);

	str = out;
	delete[] out;
}

// Add spaces to make a string of hex digits easier to read
void AddSpaces(CString &str)
{
	const char *pp, *end;               // Ptrs into orig. string
	int ndigits = 0;                    // Number of digits in string
	const char sep_char = ' ';  // Used to separate groups of digits
	const int group = 4;                // How many is in a group

	// Allocate enough space (allowing for space padding)
	char *out = new char[(str.GetLength()*(group+1))/group + 2]; // Result created here
	char *dd = out;                     // Ptr to current output char

	// Skip leading whitespace
	pp = str.GetBuffer(0);
	while (isspace(*pp))
		pp++;

	// Find end of number and work out how many digits it has
	end = pp + strspn(pp, " \t0123456789ABCDEFabcdef");
	for (const char *qq = pp; qq < end; ++qq)
		if (isxdigit(*qq))
			++ndigits;

	for ( ; ndigits > 0; ++pp)
		if (isxdigit(*pp))
		{
			*dd++ = *pp;
			if (--ndigits > 0 && ndigits%group == 0)
				*dd++ = sep_char;
		}
	ASSERT(pp <= end);
	strcpy(dd, pp);

	str = out;
	delete[] out;
}

// When a menu item is selected we only get an id which is usually a command ID
// but for the menu created with make_var_menu_tree the id is only a unique number.
// What we really want is the menu text which contains a variable name.
// Given a menu ptr and an id the menu is searched and the menu item text for
// that id is returned or an empty string if it is not found.
CString get_menu_text(CMenu *pmenu, int id)
{
	CString retval;

	// Check menu items first
	if (pmenu->GetMenuString(id, retval, MF_BYCOMMAND) > 0)
		return retval;

	// Check ancestor menus
	int item_count = pmenu->GetMenuItemCount();
	for (int ii = 0; ii < item_count; ++ii)
	{
		CMenu *psub = pmenu->GetSubMenu(ii);
		if (psub != NULL && psub->GetMenuString(id, retval, MF_BYCOMMAND) > 0)
			return retval;
	}

	return CString("");
}

//-----------------------------------------------------------------------------
// Conversions

static const long double two_pow40 = 1099511627776.0L;

// Make a real48 (8 bit exponent, 39 bit mantissa) from a double.
// Returns false on overflow, returns true (and zero value) on underflow
bool make_real48(unsigned char pp[6], double val, bool big_endian /*=false*/)
{
	int exp;                    // Base 2 exponent of val
	val = frexp(val, &exp);

	if (val == 0.0 || exp < -128)       // zero or underflow
	{
		memset(pp, 0, 6);
		return true;
	}
	else if (exp== -1 || exp > 127)     // inf or overflow
		return false;

	assert(exp + 128 >= 0 && exp + 128 < 256);
	pp[0] = exp + 128;                  // biassed exponent in first byte

	bool neg = val < 0.0;               // remember if -ve
	val = fabs(val);

	// Work out mantissa bits
	__int64 imant = (__int64)(val * two_pow40);
	assert(imant < two_pow40);
	pp[1] = unsigned char((imant) & 0xFF);
	pp[2] = unsigned char((imant>>8) & 0xFF);
	pp[3] = unsigned char((imant>>16) & 0xFF);
	pp[4] = unsigned char((imant>>24) & 0xFF);
	pp[5] = unsigned char((imant>>32) & 0x7F);  // masks off bit 40 (always on)
	if (neg)
		pp[5] |= 0x80;                  // set -ve bit

	if (big_endian)
		flip_bytes(pp, 6);
	return true;
}

// Make a double from a Real48
// Also returns the exponent (base 2) and mantissa if pexp/pmant are not NULL
double real48(const unsigned char *pp, int *pexp, long double *pmant, bool big_endian /*=false*/)
{
	// Copy bytes containing the real48 in case we have to flip byte order
	unsigned char buf[6];
	memcpy(buf, pp, 6);
	if (big_endian)
		flip_bytes(buf, 6);

	int exponent = (int)buf[0] - 128;
	if (pexp != NULL) *pexp = exponent;

	// Build integer mantissa (without implicit leading bit)
	__int64 mantissa = buf[1] + ((__int64)buf[2]<<8) + ((__int64)buf[3]<<16) +
					((__int64)buf[4]<<24) + ((__int64)(buf[5]&0x7F)<<32);

	// Special check for zero
	if (::memcmp(buf, "\0\0\0\0\0", 5)== 0 && (buf[5] & 0x7F) == 0)
	{
		if (pmant != NULL) *pmant = 0.0;
		return 0.0;
	}

	// Add implicit leading 1 bit
	mantissa +=  (_int64)1<<39;

	if (pmant != NULL)
	{
		if ((buf[5] & 0x80) == 0)
			*pmant = mantissa / (two_pow40 / 2);
		else
			*pmant = -(mantissa / (two_pow40 / 2));
	}
	
	// Check sign bit and return appropriate result
	if (buf[0] == 0 && mantissa < two_pow40/4)
		return 0.0;                            // underflow
	else if ((buf[5] & 0x80) == 0)
		return (mantissa / two_pow40) * powl(2, exponent);
	else
		return -(mantissa / two_pow40) * powl(2, exponent);
}

static const long double two_pow24 = 16777216.0L;
static const long double two_pow56 = 72057594037927936.0L;

bool make_ibm_fp32(unsigned char pp[4], double val, bool little_endian /*=false*/)
{
	int exp;                    // Base 2 exponent of val
	
	val = frexp(val, &exp);

	if (val == 0.0 || exp < -279)
	{
		// Return IBM zero for zero or exponent too small
		memset(pp, '\0', 4);
		return true;
	}
	else if (exp > 252)
		return false;           // Rteurn error for exponent too big

	// Change signed exponent into +ve biassed exponent (-256=>0, 252=>508)
	exp += 256;

	// normalize exponent to base 16 (with corresp. adjustment to mantissa)
	while (exp < 0 || (exp&3) != 0)   // While bottom 2 bits are not zero we don't have a base 16 exponent
	{
		val /= 2.0;
		++exp;
	}
	exp = exp>>2;               // Convert base 2 exponent to base 16

	// At this point we have a signed base 16 exponent in range 0 to 127 and
	// a signed mantissa in the range 0.0625 (1/16) to (just less than) 1.0.
	// (although val may be < 0.625 if the exponent was < -256).

	assert(exp >= 0 && exp < 128);
	assert(val < 1.0);

	// Store exponent and sign bit
	if (val < 0.0)
	{
		val = -val;
		pp[0] = exp | 0x80;     // Turn on sign bit
	}
	else
	{
		pp[0] = exp;
	}

	long imant = long(val * two_pow24);
	assert(imant < two_pow24);
	pp[1] = unsigned char((imant>>16) & 0xFF);
	pp[2] = unsigned char((imant>>8) & 0xFF);
	pp[3] = unsigned char((imant) & 0xFF);

	if (little_endian)
	{
		unsigned char cc = pp[0]; pp[0] = pp[3]; pp[3] = cc;
		cc = pp[1]; pp[1] = pp[2]; pp[2] = cc;
	}
	return true;
}

bool make_ibm_fp64(unsigned char pp[8], double val, bool little_endian /*=false*/)
{
	int exp;                    // Base 2 exponent of val
	
	val = frexp(val, &exp);

	if (val == 0.0 || exp < -279)
	{
		// Return IBM zero for zero or exponent too small
		memset(pp, '\0', 8);
		return true;
	}
	else if (exp > 252)
		return false;           // Rteurn error for exponent too big

	// Change signed exponent into +ve biassed exponent (-256=>0, 252=>508)
	exp += 256;

	// normalize exponent to base 16 (with corresp. adjustment to mantissa)
	while (exp < 0 || (exp&3) != 0)   // While bottom 2 bits are not zero we don't have a base 16 exponent
	{
		val /= 2.0;
		++exp;
	}
	exp = exp>>2;               // Convert base 2 exponent to base 16

	// At this point we have a signed base 16 exponent in range 0 to 127 and
	// a signed mantissa in the range 0.0625 (1/16) to (just less than) 1.0.
	// (although val may be < 0.625 if the exponent was < -256).

	assert(exp >= 0 && exp < 128);
	assert(val < 1.0);

	// Store exponent and sign bit
	if (val < 0.0)
	{
		val = -val;
		pp[0] = exp | 0x80;     // Turn on sign bit
	}
	else
	{
		pp[0] = exp;
	}

	__int64 imant = (__int64)(val * two_pow56);
	assert(imant < two_pow56);
	pp[1] = unsigned char((imant>>48) & 0xFF);
	pp[2] = unsigned char((imant>>40) & 0xFF);
	pp[3] = unsigned char((imant>>32) & 0xFF);
	pp[4] = unsigned char((imant>>24) & 0xFF);
	pp[5] = unsigned char((imant>>16) & 0xFF);
	pp[6] = unsigned char((imant>>8) & 0xFF);
	pp[7] = unsigned char((imant) & 0xFF);

	if (little_endian)
	{
		unsigned char cc = pp[0]; pp[0] = pp[7]; pp[7] = cc;
		cc = pp[1]; pp[1] = pp[6]; pp[6] = cc;
		cc = pp[2]; pp[2] = pp[5]; pp[5] = cc;
		cc = pp[3]; pp[3] = pp[4]; pp[4] = cc;
	}
	return true;
}

long double ibm_fp32(const unsigned char *pp, int *pexp /*=NULL*/,
					 long double *pmant /*=NULL*/, bool little_endian /*=false*/)
{
	unsigned char tt[4];                // Used to store bytes in little-endian order
	int exponent;                       // Base 16 exponent of IBM FP value
	long mantissa;                      // Unsigned integer mantissa

	if (little_endian)
	{
		tt[0] = pp[3];
		tt[1] = pp[2];
		tt[2] = pp[1];
		tt[3] = pp[0];
		pp = tt;
	}

	// Work out the binary exponent:
	// - remove the high (sign) bit from the first byte
	// - subtract the bias of 64
	// - convert hex exponent to binary [note 16^X == 2^(4*X)]
	exponent = ((int)(pp[0] & 0x7F) - 64);
	if (pexp != NULL) *pexp = exponent;
	// - convert hex exponent to binary
	//   note that we multiply the exponent by 4 since 16^X == 2^(4*X)
	exponent *= 4;

	// Get the mantissa and return zero if there are no bits on
	mantissa = ((long)pp[1]<<16) + ((long)pp[2]<<8) + pp[3];
	if (mantissa == 0)
	{
		if (pmant != NULL) *pmant = 0.0;
		return 0.0;
	}

	if (pmant != NULL)
	{
		if ((pp[0] & 0x80) == 0)
			*pmant = mantissa / two_pow24;
		else
			*pmant = -(mantissa / two_pow24);
	}
	
	// Check sign bit and return appropriate result
	if ((pp[0] & 0x80) == 0)
		return (mantissa / two_pow24) * powl(2, exponent);
	else
		return -(mantissa / two_pow24) * powl(2, exponent);
}

long double ibm_fp64(const unsigned char *pp, int *pexp /*=NULL*/,
				long double *pmant /*=NULL*/, bool little_endian /*=false*/)
{
	unsigned char tt[8];                // Used to store bytes in little-endian order
	int exponent;                       // Base 16 exponent of IBM FP value
	__int64 mantissa;                   // Unsigned integral mantissa

	if (little_endian)
	{
		tt[0] = pp[7];
		tt[1] = pp[6];
		tt[2] = pp[5];
		tt[3] = pp[4];
		tt[4] = pp[3];
		tt[5] = pp[2];
		tt[6] = pp[1];
		tt[7] = pp[0];
		pp = tt;
	}

	// Work out the binary exponent:
	// - remove the high (sign) bit from the first byte
	// - subtract the bias of 64
	exponent = ((int)(pp[0] & 0x7F) - 64);
	if (pexp != NULL) *pexp = exponent;
	// - convert hex exponent to binary
	//   note that we multiply the exponent by 4 since 16^X == 2^(4*X)
	exponent *= 4;

	// Get the mantissa and return zero if there are no bits on
	mantissa =  ((__int64)pp[1]<<48) + ((__int64)pp[2]<<40) + ((__int64)pp[3]<<32) +
				((__int64)pp[4]<<24) + ((__int64)pp[5]<<16) + ((__int64)pp[6]<<8) + pp[7];
	if (mantissa == 0)
	{
		if (pmant != NULL) *pmant = 0.0;
		return 0.0;
	}

	if (pmant != NULL)
	{
		if ((pp[0] & 0x80) == 0)
			*pmant = mantissa / two_pow56;
		else
			*pmant = -(mantissa / two_pow56);
	}

	// Check sign bit and return appropriate result
	if ((pp[0] & 0x80) == 0)
		return (mantissa / two_pow56) * powl(2, exponent);
	else
		return -(mantissa / two_pow56) * powl(2, exponent);
}
#endif  // #ifndef REGISTER_APP

// The compiler does not provide a function for reading a 64 bit int from a string?!!
__int64 strtoi64(const char *ss, int radix /*=0*/)
{
	if (radix == 0) radix = 10;

	__int64 retval = 0;

	for (const char *src = ss; *src != '\0'; ++src)
	{
		// Ignore everything except valid digits
		unsigned int digval;
		if (isdigit(*src))
			digval = *src - '0';
		else if (isalpha(*src))
			digval = toupper(*src) - 'A' + 10;
		else
			continue;                   // Ignore separators (or any other garbage)

		if (digval >= radix)
		{
			ASSERT(0);                  // How did this happen?
			continue;                   // Ignore invalid digits
		}

		// Ignore overflow
		retval = retval * radix + digval;
	}

	return retval;
}

// Slightly better version with overflow checking and returns ptr to 1st char not used
__int64 strtoi64(const char *ss, int radix, const char **endptr)
{
	if (radix == 0) radix = 10;

	__int64 retval = 0;

	unsigned __int64 maxval = _UI64_MAX / radix;

	const char * src;
	for (src = ss; *src != '\0'; ++src)
	{
		// Ignore everything except valid digits
		unsigned int digval;
		if (isdigit(*src))
			digval = *src - '0';
		else if (isalpha(*src))
			digval = toupper(*src) - 'A' + 10;
		else
			break;   // Not a digit

		if (digval >= radix)
			break;   // Digit too large for radix

		if (retval < maxval || (retval == maxval && (unsigned __int64)digval <= _UI64_MAX % radix))
			retval = retval * radix + digval;
		else
			break;  // overflow
	}

	if (endptr != NULL)
		*endptr = src;

	return retval;
}

//-----------------------------------------------------------------------------
void BrowseWeb(UINT id)
{
	CString str;
	VERIFY(str.LoadString(id));

	::ShellExecute(AfxGetMainWnd()->m_hWnd, _T("open"), str, NULL, NULL, SW_SHOWNORMAL);
}

//-----------------------------------------------------------------------------
// File handling

// Try to find the absolute path the .EXE and return it (including trailing backslash)
CString GetExePath()
{
	char fullpath[_MAX_PATH];
	char *end;                          // End of path of exe file

	// First try argv[0] which is usually the full pathname of the .EXE
	strncpy(fullpath, __argv[0], sizeof(fullpath)-1);

	// Check if we did not get an absolute path (if fired up from command line it may be relative)
	if (fullpath[1] != ':')
	{
		// Use path of help file which is stored in the same directory as the .EXE
		strncpy(fullpath, AfxGetApp()->m_pszHelpFilePath, sizeof(fullpath)-1);
	}

	fullpath[sizeof(fullpath)-1] = '\0';  // make sure it is nul-terminated

	// Remove any trailing filename
	if ((end = strrchr(fullpath, '\\')) == NULL && (end = strrchr(fullpath, ':')) == NULL)
		end = fullpath;
	else
		++end;   // move one more so backslash is included
	*end = '\0';

	return CString(fullpath);
}

// Get the place to store user data files
BOOL GetDataPath(CString &data_path, int csidl /*=CSIDL_APPDATA*/)
{
#ifndef REGISTER_APP
	if (theApp.win95_)
	{
		// Windows 95 does not support app data special folder so just use .exe location
		data_path = ::GetExePath();
		return TRUE;
	}
#endif

	BOOL retval = FALSE;
	LPTSTR pbuf = data_path.GetBuffer(MAX_PATH);

#if 0 // SHGetSpecialFolderPath requires W2K
	if (SHGetSpecialFolderPath(NULL, pbuf, csidl, TRUE) != NOERROR)
		retval = TRUE;
#else
	LPMALLOC pMalloc;
	if (SUCCEEDED(SHGetMalloc(&pMalloc)))
	{
		ITEMIDLIST *itemIDList;
		if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, csidl, &itemIDList)))
		{
			SHGetPathFromIDList(itemIDList, pbuf);
			pMalloc->Free(itemIDList);
			retval = TRUE;
		}
		pMalloc->Release();
	}
#endif

	data_path.ReleaseBuffer();
	if (!retval)
		data_path.Empty();     // Make string empty if we failed just in case
	else if (csidl == CSIDL_APPDATA || csidl == CSIDL_COMMON_APPDATA)
		data_path += '\\' + CString(AfxGetApp()->m_pszRegistryKey) + 
					 '\\' + CString(AfxGetApp()->m_pszProfileName) +
					 '\\';
	return retval;
}

// Change a file's "creation" time
void SetFileCreationTime(const char *filename, time_t tt)
{
	HANDLE hh = ::CreateFile(filename, FILE_WRITE_ATTRIBUTES, FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hh == INVALID_HANDLE_VALUE)
		return;

	FILETIME ft;
	if (ConvertToFileTime(tt, &ft))
		::SetFileTime(hh, &ft, NULL, NULL);
	::CloseHandle(hh);
}

// Change a file's "access" time
void SetFileAccessTime(const char *filename, time_t tt)
{
	HANDLE hh = ::CreateFile(filename, FILE_WRITE_ATTRIBUTES, FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hh == INVALID_HANDLE_VALUE)
		return;

	FILETIME ft;
	if (ConvertToFileTime(tt, &ft))
		::SetFileTime(hh, NULL, &ft, NULL);
	::CloseHandle(hh);
}

// FileErrorMessage - generate a meaningful error string from a CFileException
// fe = pointer to error information (including file name)
// mode = this parameter is used to make the message more meaningful depending
//        on the type of operations being performed on the file.  If the only
//        file operations being performed are reading file(s) the use
//        CFile::modeRead.  If only creating/writing files use CFile::modeWrite.
//        Otherwise use the default (both flags combined) to get messages that
//        are less specific but will not mislead the user if the flag does not
//        match the operations.
CString FileErrorMessage(const CFileException *fe, UINT mode /*=CFile::modeRead|CFile::modeWrite*/)
{
		CString retval;                                         // Returned error message
		CFileStatus fs;                                         // Current file status (if access error)
		UINT drive_type;                                        // Type of drive where the file is
	char rootdir[4] = "?:\\";			// Use with GetDriveType

		if (mode == CFile::modeRead)
				retval.Format("An error occurred reading from the file \"%s\".\n", fe->m_strFileName);
		else if (mode = CFile::modeWrite)
				retval.Format("An error occurred writing to the file \"%s\".\n", fe->m_strFileName);
		else
				retval.Format("An error occurred using the file \"%s\".\n", fe->m_strFileName);

		switch (fe->m_cause)
		{
		case CFileException::none:
				ASSERT(0);                                                      // There should be an error for this function to be called
				retval += "Apparently there was no error!";
				break;
#if _MSC_VER < 1400  // generic is now a C++ reserved word
		case CFileException::generic:
#else
		case CFileException::genericException:
#endif
				retval += "The error is not specified.";
				break;
		case CFileException::fileNotFound:
				ASSERT(mode != CFile::modeWrite);       // Should be reading from an existing file
				retval += "The file does not exist.";
				break;
		case CFileException::badPath:
				retval += "The file name contains invalid characters or the drive\n"
							  "or one or more component directories do not exist.";
				break;
		case CFileException::tooManyOpenFiles:
				retval += "There are too many open files in this system.\n"
								  "Try closing some programs or rebooting the system.";
				break;
		case CFileException::accessDenied:
				// accessDenied (or errno == EACCES) is a general purpose error
				// value and can mean many things.  We try to find out more about
				// the file to work out what exactly went wrong.
				if (!CFile::GetStatus(fe->m_strFileName, fs))
				{
						if (fe->m_strFileName.Compare("\\") == 0 ||
							fe->m_strFileName.GetLength() == 3 && fe->m_strFileName[1] == ':' && fe->m_strFileName[2] == '\\')
						{
								retval += "This is the root directory.\n"
												  "You cannot use it as a file.";
						}
						else if (mode == CFile::modeWrite)
						{
								retval += "Check that you have permission to write\n"
												  "to the directory and that the disk is\n"
												  "not write-protected or read-only media.";
						}
						else
								retval += "Access denied. Check that you have permission\n"
												  "to read the directory and use the file.";
				}
				else if (fs.m_attribute & CFile::directory)
						retval += "This is a directory - you cannot use it as a file.";
				else if (fs.m_attribute & (CFile::volume|CFile::hidden|CFile::system))
						retval += "The file is a special system file.";
				else if ((fs.m_attribute & CFile::readOnly) && mode != CFile::modeRead)
						retval += "You cannot write to a read-only file.";
				else
						retval += "You cannot access this file.";
				break;
		case CFileException::invalidFile:
				ASSERT(0);                                                              // Uninitialised or corrupt file handle
				retval += "A software error occurred: invalid file handle.\n"
								  "Please report this defect to the software vendor.";
				break;
		case CFileException::removeCurrentDir:
				retval += "An attempt was made to remove the current directory.";
				break;
		case CFileException::directoryFull:
				ASSERT(mode != CFile::modeRead);        // Must be creating a file
				retval += "The file could not be created because the directory\n"
								  "for the file is full.  Delete some files from the\n"
								  "root directory or use a sub-directory.";
				break;
		case CFileException::badSeek:
				ASSERT(0);                                                      // Normally caused by a bug
				retval += "A software error occurred: seek to bad file position.";
				break;
		case CFileException::hardIO:
				if (fe->m_strFileName.GetLength() > 2 && fe->m_strFileName[1] == ':')
						rootdir[0] = fe->m_strFileName[0];
				else
						rootdir[0] = _getdrive() + 'A' - 1;

				drive_type = GetDriveType(rootdir);
				switch (drive_type)
				{
				case DRIVE_REMOVABLE:
				case DRIVE_CDROM:
						retval += "There was a problem accessing the drive.\n"
										  "Please ensure that the medium is present.";
						break;
				case DRIVE_REMOTE:
						retval += "There was a problem accessing the file.\n"
										  "There may be a problem with your network.";
						break;
				default:
						retval += "There was a hardware error.  There may be\n"
										  "a problem with your computer or disk drive.";
						break;
				}
				break;
		case CFileException::sharingViolation:
				retval += "SHARE.EXE is not loaded or the file is in use.\n"
								  "Check that SHARE.EXE is installed, then try\n"
								  "closing other programs or rebooting the system";
				break;
		case CFileException::lockViolation:
				retval += "The file (or part thereof) is in use.\n"
								  "Try closing other programs or rebooting the system";
				break;
		case CFileException::diskFull:
				ASSERT(mode != CFile::modeRead);        // Must be writing to a file
				retval += "The file could not be written as the disk is full.\n"
								  "Please delete some files to make room on the disk.";
				break;
		case CFileException::endOfFile:
				ASSERT(mode != CFile::modeWrite);       // Should be reading from a file
				retval += "An attempt was made to access an area past the end of the file.";
				break;
		default:
				ASSERT(0);
				retval += "An undocumented file error occurred.";
				break;
		}
		return retval;
}

#ifndef REGISTER_APP
//-----------------------------------------------------------------------------
// Multiple monitor handling

// Gets rect of the monitor which contains the mouse
CRect MonitorMouse()
{
	CPoint pt;
	GetCursorPos(&pt);

	CRect rect(pt.x, pt.y, pt.x+1, pt.y+1);
	return MonitorRect(rect);
}

// Gets rect of monitor which contains most of rect.
// In non-multimon environment it just returns the rect of the
// screen work area (excludes "always on top" docked windows).
CRect MonitorRect(CRect rect)
{
	CRect cont_rect;

	if (theApp.mult_monitor_)
	{
		// Use rect of containing monitor as "container"
		HMONITOR hh = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi;
		mi.cbSize = sizeof(mi);
		if (hh != 0 && GetMonitorInfo(hh, &mi))
			cont_rect = mi.rcWork;  // work area of nearest monitor
		else
		{
			// Shouldn't happen but if it does use the whole virtual screen
			ASSERT(0);
			cont_rect = CRect(::GetSystemMetrics(SM_XVIRTUALSCREEN),
				::GetSystemMetrics(SM_YVIRTUALSCREEN),
				::GetSystemMetrics(SM_XVIRTUALSCREEN) + ::GetSystemMetrics(SM_CXVIRTUALSCREEN),
				::GetSystemMetrics(SM_YVIRTUALSCREEN) + ::GetSystemMetrics(SM_CYVIRTUALSCREEN));
		}
	}
	else if (!::SystemParametersInfo(SPI_GETWORKAREA, 0, &cont_rect, 0))
	{
		// I don't know if this will ever happen since the Windows documentation
		// is pathetic and does not say when or why SystemParametersInfo might fail.
		cont_rect = CRect(0, 0, ::GetSystemMetrics(SM_CXFULLSCREEN),
								::GetSystemMetrics(SM_CYFULLSCREEN));
	}

	return cont_rect;
}

// Returns true if rect is wholly or partially off the screen.
// In multimon environment it also returns true if rect extends over more than one monitor.
bool OutsideMonitor(CRect rect)
{
	CRect cont_rect = MonitorRect(rect);

	return rect.left   < cont_rect.left  ||
		   rect.right  > cont_rect.right ||
		   rect.top    < cont_rect.top   ||
		   rect.bottom > cont_rect.bottom;
}

// Check if most of window is off all monitors.  If so it returns true and
// adjusts the parameter (rect) so the rect is fully within the closest monitor.
bool NeedsFix(CRect &rect)
{
	CRect cont_rect = MonitorRect(rect);
	CRect small_rect(rect);
	small_rect.DeflateRect(rect.Width()/4, rect.Height()/4);
	CRect tmp;
	if (!tmp.IntersectRect(cont_rect, small_rect))
	{
		// Fix the rect
#if _MSC_VER >= 1300
		if (rect.right > cont_rect.right)
			rect.MoveToX(rect.left - (rect.right - cont_rect.right));
		if (rect.bottom > cont_rect.bottom)
			rect.MoveToY(rect.top - (rect.bottom - cont_rect.bottom));
		if (rect.left < cont_rect.left)
			rect.MoveToX(cont_rect.left);
		if (rect.top < cont_rect.top)
			rect.MoveToY(cont_rect.top);
#endif
		return true;
	}
	else
		return false;  // its OK
}

// Check if a lengthy operation should be aborted.
// Updates display and checks for user pressing Escape key.
bool AbortKeyPress()
{
	bool retval = false;
	MSG msg;

	// Do any redrawing, but nothing else
	while (::PeekMessage(&msg, NULL, WM_PAINT, WM_PAINT, PM_REMOVE))
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

	// Check if any key has been pressed
	if (::PeekMessage(&msg, NULL, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE))
	{
		int cc = msg.wParam;

		// Windows does not like to miss key down events (needed to match key up events)
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);

		// Remove any characters resulting from keypresses (so they are not inserted into the active file)
		while (::PeekMessage(&msg, NULL, WM_CHAR, WM_CHAR, PM_REMOVE))
			;

		// Abort is signalled with Escape key or SPACE bar
		retval = cc == VK_ESCAPE || cc == VK_SPACE;
	}

	return retval;
}

/////////////////////////////////////////////////////////////////////////////
// Windows NT native API functions
HINSTANCE hNTDLL;        // Handle to NTDLL.DLL

PFRtlInitUnicodeString pfInitUnicodeString;
PFNtOpenFile pfOpenFile;
PFNtClose pfClose;
PFNtDeviceIoControlFile pfDeviceIoControlFile;
PFNtQueryInformationFile pfQueryInformationFile;
PFNtSetInformationFile pfSetInformationFile;
PFNtQueryVolumeInformationFile pfQueryVolumeInformationFile;
PFNtReadFile pfReadFile;
PFNtWriteFile pfWriteFile;

void GetNTAPIFuncs()
{
	if (!theApp.is_nt_)
		return;

	if (hNTDLL != (HINSTANCE)0)
		return;                 // Already done

	if ((hNTDLL = ::LoadLibrary("ntdll.dll")) == (HINSTANCE)0)
		return;

	pfInitUnicodeString    = (PFRtlInitUnicodeString)  ::GetProcAddress(hNTDLL, "RtlInitUnicodeString");
	pfOpenFile             = (PFNtOpenFile)            ::GetProcAddress(hNTDLL, "NtOpenFile");
	pfClose                = (PFNtClose)               ::GetProcAddress(hNTDLL, "NtClose");
	pfDeviceIoControlFile  = (PFNtDeviceIoControlFile) ::GetProcAddress(hNTDLL, "NtDeviceIoControlFile");
	pfQueryInformationFile = (PFNtQueryInformationFile)::GetProcAddress(hNTDLL, "NtQueryInformationFile");
	pfSetInformationFile   = (PFNtSetInformationFile)  ::GetProcAddress(hNTDLL, "NtSetInformationFile");
	pfQueryVolumeInformationFile = (PFNtQueryVolumeInformationFile)::GetProcAddress(hNTDLL, "NtQueryVolumeInformationFile");
	pfReadFile             = (PFNtReadFile)             ::GetProcAddress(hNTDLL, "NtReadFile");
	pfWriteFile            = (PFNtWriteFile)            ::GetProcAddress(hNTDLL, "NtWriteFile");
}

// Given a "fake" device file name return a NT native API device name. Eg \\.\Floppy1 => \device\Floppy1
// Note: For \device\HarddiskN entries the sub-entry "Partition0" is not really a partition but
//       allows access to the whole disk.  (This is only a real device under NT, but is a valid
//       alias under 2K/XP to something like \device\HarddiskN\DRM (where N != M, although N == M
//       for fixed devices and M > N for removeable (M increases every time media is swapped).
BSTR GetPhysicalDeviceName(LPCTSTR name)
{
	CString dev_name;

	if (_tcsncmp(name, _T("\\\\.\\PhysicalDrive"), 17) == 0)
		dev_name.Format(_T("\\device\\Harddisk%d\\Partition0"), atoi(name + 17));
	else if (_tcsncmp(name, _T("\\\\.\\CdRom"), 9) == 0)
		dev_name.Format(_T("\\device\\Cdrom%d"), atoi(name + 9));
	else if (_tcsncmp(name, _T("\\\\.\\Floppy"), 10) == 0)
		dev_name.Format(_T("\\device\\Floppy%d"), atoi(name + 10));
	else
		ASSERT(0);

	// Return as a BSTR (so Unicode string is available)
	return dev_name.AllocSysString();
}

// Get volume number (0-25) assoc. with a physical device or -1 if not found.
// Note: This currently only works for optical and removeable drives (not floppy or fixed drives).
int DeviceVolume(LPCTSTR filename)
{
	const char *dev_str;                // Start of device name that we are looking for
	UINT drive_type;                    // Type of drive looking for
	int dev_num;                        // The device number we are looking for

	if (_tcsncmp(filename, _T("\\\\.\\PhysicalDrive"), 17) == 0)
	{
		drive_type = DRIVE_REMOVABLE;
		dev_num = atoi(filename + 17);
		dev_str = "\\device\\Harddisk";
	}
	else if (_tcsncmp(filename, _T("\\\\.\\CdRom"), 9) == 0)
	{
		drive_type = DRIVE_CDROM;
		dev_num = atoi(filename + 9);
		dev_str = "\\device\\Cdrom";
	}
	//else if (_tcsncmp(filename, _T("\\\\.\\Floppy"), 10) == 0)
	//{
	//	dev_num = atoi(filename + 10);
	//	dev_str = "\\device\\Floppy";
	//}
	else
	{
		ASSERT(0);  // invalid physical device name
		return -1;
	}

	// First scan all volumes (A: to Z:) for drives of the desired type
	DWORD dbits = GetLogicalDrives();   // Bit is on for every drive present
	char vol[4] = "?:\\";               // Vol name used for volume enquiry calls
	for (int ii = 0; ii < 26; ++ii)
	{
		vol[0] = 'A' + (char)ii;
		if ((dbits & (1<<ii)) != 0 && ::GetDriveType(vol) == drive_type)
		{
			char dev[3] = "?:";
			dev[0] = 'A' + (char)ii;
			char buf[128];
			buf[0] = '\0';
			::QueryDosDevice(dev, buf, sizeof(buf));  // eg \device\cdrom0
			int offset = strlen(dev_str);
			if (_strnicmp(buf, dev_str, offset) == 0 &&
				isdigit(buf[offset]) &&
				atoi(buf + offset) == dev_num)
			{
				return ii;
			}
		}
	}
	return -1;
}

// Get display name for device file name
CString DeviceName(CString name) // TODO get rid of this (use SpecialList::name() instead)
{
	CString retval;
	if (name.Left(17) == _T("\\\\.\\PhysicalDrive"))
		retval = _T("Physical Drive ") + name.Mid(17);
	else if (name.Left(9) == _T("\\\\.\\CdRom"))
		retval = _T("Optical Drive ") + name.Mid(9);
	else if (name.Left(10) == _T("\\\\.\\Floppy"))
		retval = _T("Floppy Drive ") + name.Mid(10);
#if 0
	if (name.Left(14) == _T("\\device\\Floppy"))
		retval.Format("Floppy Drive %d", atoi(name.Mid(14)));
	else if (name.Left(16) == _T("\\device\\Harddisk"))
		retval.Format("Hard Drive %d", atoi(name.Mid(16)));
	else if (name.Left(13) == _T("\\device\\Cdrom"))
		retval.Format("Optical Drive %d", atoi(name.Mid(13)));
#endif
	//else if (name.Left(17) == _T("\\\\.\\PhysicalDrive"))
	//	return _T("Physical Drive ") + name.Mid(17);
	//else if (name.Left(9) == _T("\\\\.\\CdRom"))
	//	return _T("Optical Drive ") + name.Mid(9);
	//else if (name == _T("\\\\.\\a:"))
	//	return CString(_T("Floppy Drive 0"));
	//else if (name == _T("\\\\.\\b:"))
	//	return CString(_T("Floppy Drive 1"));
	else if (name.GetLength() == 6)
	{
		_TCHAR vol[4] = _T("?:\\");     // Vol name used for volume enquiry calls
		vol[0] = name[4];

		// Find out what sort of drive it is
		retval.Format(_T(" (%c:)"), vol[0]);
		switch (::GetDriveType(vol))
		{
		default:
			ASSERT(0);
			break;
		case DRIVE_NO_ROOT_DIR:
		case DRIVE_REMOVABLE:
			if (vol[0] == 'A' || vol[0] == 'B')
				retval = "Floppy Drive " + retval;
			else
				retval = "Removable " + retval;
			break;
		case DRIVE_FIXED:
			retval = "Fixed Drive " + retval;
			break;
		case DRIVE_REMOTE:
			retval = "Network Drive " + retval;
			break;
		case DRIVE_CDROM:
			retval = "Optical Drive " + retval;
			break;
		case DRIVE_RAMDISK:
			retval = "RAM Drive " + retval;
			break;
		}
	}
	else
		ASSERT(0);

	return retval;
}

//-----------------------------------------------------------------------------
// Security funcs

// This is to confuse anyone using regmon to work out what is being done in the registry
void DummyRegAccess(unsigned int group /*=0*/)
{
	static const char * const reg_entries[3][110] =
	{
		{
			".obd\\ShellNew\\Data",
			".obt\\ShellNew\\Data",
			".mic\\ShellNew\\Data",
			".mix\\ShellNew\\Data",
			".fpx\\ShellNew\\Data",
			".cdr\\ShellNew\\Data",
			".tga\\ShellNew\\Data",
			".hpj\\ShellNew\\NullFile",
			".hpj\\ShellNew\\Data",
			".cnt\\ShellNew\\NullFile",
			".cnt\\ShellNew\\Data",
			".UDL\\ShellNew\\NullFile",
			".UDL\\ShellNew\\Data",
			".UDL\\ShellNew\\NullFile",
			".UDL\\ShellNew\\Data",
			".dsn\\ShellNew\\NullFile",
			".dsn\\ShellNew\\Data",
			".jod\\ShellNew\\NullFile",
			".aspz\\ShellNew\\NullFile",
			".aspz\\ShellNew\\Data",
			".asaz\\ShellNew\\NullFile",
			".asaz\\ShellNew\\Data",
			".cdx\\ShellNew\\NullFile",
			".cdx\\ShellNew\\Data",
			".wsc\\ShellNew\\NullFile",
			".wsc\\ShellNew\\Data",
			".sct\\ShellNew\\NullFile",
			".sct\\ShellNew\\Data",
			".aif\\Content Type",
			".snd\\Content Type",
			".mid\\Content Type",
			".rmi\\Content Type",
			".avi\\Content Type",
			".qt\\Content Type",
			".mov\\Content Type",
			".mpg\\Content Type",
			".mp2\\Content Type",
			".mpe\\Content Type",
			".mpeg\\ShellNew\\Data",
			".m1v\\ShellNew\\Data",
			".mpa\\ShellNew\\NullFile",
			".mydocs\\ShellNew\\Data",
			".nfo\\ShellNew\\NullFile",
			".set\\ShellNew\\NullFile",
			".qic\\ShellNew\\Data",
			".acw\\ShellNew\\NullFile",
			".sam\\AmiProDocument\\ShellNew\\FileName",
			".shw\\Presentations31.Show\\Data",
			".shw\\Presentations31.Show\\ShellNew\\FileName",
			".wb2\\QuattroPro.Notebook.6\\Data",
			".wb2\\QuattroPro.Notebook.6\\ShellNew\\FileName",
			".wb2\\QuattroPro.Graph.6\\Data",
			".wb2\\QuattroPro.Graph.6\\ShellNew\\FileName",
			".gif\\ShellNew\\NullFile",
			".gif\\ShellNew\\Data",
			".jpeg\\ShellNew\\NullFile",
			".jpeg\\ShellNew\\Data",
			".png\\ShellNew\\NullFile",
			".png\\ShellNew\\Data",
			NULL
		},
		{
			".wmf\\ShellNew\\NullFile",
			".wmf\\ShellNew\\Data",
			".htm\\ShellNew\\Data",
			".html\\ShellNew\\Data",
			".wk4\\ShellNew\\Data",
			".wk4\\123Worksheet\\Data",
			".wk4\\123Worksheet\\ShellNew\\FileName",
			".wpd\\ShellNew\\Data",
			".wpd\\WPWin6.1File\\ShellNew\\FileName",
			".wri\\ShellNew\\Data",
			".dcx\\ShellNew\\NullFile",
			".dcx\\ShellNew\\NullFile",
			".awd\\ShellNew\\NullFile",
			".xif\\ShellNew\\NullFile",
			".wif\\ShellNew\\NullFile",
			".WSH\\ShellNew\\NullFile",
			".VBS\\ShellNew\\NullFile",
			"gopher\\EditFlags",
			"gopher\\shell\\NullFile",
			"ftp\\EditFlags",
			"ftp\\shell\\NullFile",
			"http\\EditFlags",
			"https\\EditFlags",
			"Drive\\shell\\Backup\\NullFile",
			"inffile\\shell\\NullFile",
			".lnk\\ShellNew\\Command",
			".reg\\shell\\edit\\Command",
			".txt\\ShellNew\\NullFile",
			".css\\Content Type",
			"CSSfile\\EditFlags",
			"CSSfile\\DefaultIcon\\NullFile",
			"InternetShortcut\\IsShortcut",
			"InternetShortcut\\DefaultIcon\\NullFile",
			".JS\\ShellNew\\NullFile",
			".frm\\ShellNew\\NullFile",
			".frx\\ShellNew\\NullFile",
			".bas\\ShellNew\\NullFile",
			".cls\\ShellNew\\NullFile",
			".vbp\\ShellNew\\NullFile",
			".ctl\\ShellNew\\NullFile",
			".ctx\\ShellNew\\NullFile",
			".dob\\ShellNew\\NullFile",
			".dox\\ShellNew\\NullFile",
			".dsr\\ShellNew\\NullFile",
			".dsx\\ShellNew\\NullFile",
			".pag\\ShellNew\\NullFile",
			".pgx\\ShellNew\\NullFile",
			".vbz\\ShellNew\\NullFile",
			".vbg\\ShellNew\\NullFile",
			".vbl\\ShellNew\\NullFile",
			"MIME\\Database\\Codepage\\932\\BodyCharset",
			".oca\\ShellNew\\NullFile",
			".oca\\ShellNew\\Data",
			".swt\\ShellNew\\NullFile",
			".swt\\ShellNew\\Data",
			".vbr\\ShellNew\\NullFile",
			".vbr\\ShellNew\\Data",
			".vbw\\ShellNew\\NullFile",
			".vbw\\ShellNew\\Data",
			".wct\\ShellNew\\NullFile",
			".wct\\ShellNew\\Data",
			".odl\\ShellNew\\NullFile",
			".odl\\ShellNew\\Data",
			".idl\\ShellNew\\NullFile",
			".idl\\ShellNew\\Data",
			".mdp\\ShellNew\\NullFile",
			".mdp\\ShellNew\\Data",
			".dsw\\ShellNew\\NullFile",
			".dsw\\ShellNew\\Data",
			".dsp\\ShellNew\\NullFile",
			".dsp\\ShellNew\\Data",
			".rc\\ShellNew\\NullFile",
			".rc\\ShellNew\\Data",
			".rct\\ShellNew\\NullFile",
			".rct\\ShellNew\\Data",
			".bsc\\ShellNew\\NullFile",
			".bsc\\ShellNew\\Data",
			".dsm\\ShellNew\\NullFile",
			".dsm\\ShellNew\\Data",
			".res\\ShellNew\\NullFile",
			".res\\ShellNew\\Data",
			".tlh\\ShellNew\\NullFile",
			".tlh\\ShellNew\\Data",
			".tli\\ShellNew\\NullFile",
			".tli\\ShellNew\\Data",
			".plg\\ShellNew\\NullFile",
			".plg\\ShellNew\\Data",
			".h\\ShellNew\\NullFile",
			".h\\ShellNew\\Data",
			".hxx\\ShellNew\\NullFile",
			".hxx\\ShellNew\\Data",
			".hpp\\ShellNew\\NullFile",
			".hpp\\ShellNew\\Data",
			".c\\ShellNew\\NullFile",
			".c\\ShellNew\\Data",
			".cxx\\ShellNew\\NullFile",
			".cxx\\ShellNew\\Data",
			".cpp\\ShellNew\\NullFile",
			".cpp\\ShellNew\\Data",
			".pdb\\ShellNew\\NullFile",
			".pdb\\ShellNew\\Data",
			".idb\\ShellNew\\NullFile",
			".idb\\ShellNew\\Data",
			".sbr\\ShellNew\\NullFile",
			".sbr\\ShellNew\\Data",
			".obj\\ShellNew\\NullFile",
			".obj\\ShellNew\\Data",
			".pch\\ShellNew\\NullFile",
			".pch\\ShellNew\\Data",
			NULL
		},
		{
			".ilk\\ShellNew\\NullFile",
			".ilk\\ShellNew\\Data",
			".tlb\\ShellNew\\NullFile",
			".tlb\\ShellNew\\Data",
			".mdb\\ShellNew\\NullFile",
			".mdb\\ShellNew\\Data",
			".mdb\\ShellNew\\Command",
			".mda\\ShellNew\\NullFile",
			".mda\\ShellNew\\Data",
			".mdn\\ShellNew\\NullFile",
			".mdn\\ShellNew\\Data",
			".mdz\\ShellNew\\NullFile",
			".mdz\\ShellNew\\Data",
			".mdt\\ShellNew\\NullFile",
			".mdt\\ShellNew\\Data",
			".ldb\\ShellNew\\NullFile",
			".ldb\\ShellNew\\Data",
			".mdw\\ShellNew\\NullFile",
			".mdw\\ShellNew\\Data",
			".mat\\ShellNew\\NullFile",
			".mat\\ShellNew\\Data",
			".maq\\ShellNew\\Data",
			".maf\\ShellNew\\NullFile",
			".maf\\ShellNew\\Data",
			".mar\\ShellNew\\NullFile",
			".mar\\ShellNew\\Data",
			".mam\\ShellNew\\NullFile",
			".mam\\ShellNew\\Data",
			".mad\\ShellNew\\NullFile",
			".mad\\ShellNew\\Data",
			".mde\\ShellNew\\NullFile",
			".mde\\ShellNew\\Data",
			".DIC\\ShellNew\\NullFile",
			".DIC\\ShellNew\\Data",
			".zip\\\\ShellNew\\\\ShellNew\\NullFile",
			".zip\\\\ShellNew\\\\ShellNew\\Data",
			".xla\\ShellNew\\NullFile",
			".xla\\ShellNew\\Data",
			".xlb\\ShellNew\\NullFile",
			".xlb\\ShellNew\\Data",
			".xlc\\ShellNew\\NullFile",
			".xlc\\ShellNew\\Data",
			".xldz\\ShellNew\\NullFile",
			".xldz\\ShellNew\\Data",
			".xlk\\ShellNew\\NullFile",
			".xlk\\ShellNew\\Data",
			".xll\\ShellNew\\NullFile",
			".xll\\ShellNew\\Data",
			".xlm\\ShellNew\\NullFile",
			".xlyz\\ShellNew\\Data",
			".csv\\ShellNew\\NullFile",
			".xlv\\ShellNew\\NullFile",
			".xlw\\ShellNew\\NullFile",
			".slk\\ShellNew\\NullFile",
			".slk\\ShellNew\\Data",
			".dif\\ShellNew\\NullFile",
			".iqy\\ShellNew\\Data",
			".gra\\ShellNew\\NullFile",
			".wiz\\ShellNew\\NullFile",
			".wbk\\ShellNew\\NullFile",
			".wll\\ShellNew\\NullFile",
			".wll\\ShellNew\\Data",
			".exc\\ShellNew\\NullFile",
			".ofn\\ShellNew\\NullFile",
			".ofn\\ShellNew\\Command",
			".msg\\ShellNew\\NullFile",
			".oft\\ShellNew\\NullFile",
			".oss\\ShellNew\\NullFile",
			".fav\\ShellNew\\NullFile",
			".xnk\\ShellNew\\NullFile",
			".ocx\\ShellNew\\NullFile",
			".RLE\\ShellNew\\NullFile",
			".RLE\\ShellNew\\NullFile",
			".RLE\\ShellNew\\Data",
			".IFF\\ShellNew\\NullFile",
			".IFF\\ShellNew\\NullFile",
			".LBM\\ShellNew\\NullFile",
			".LBM\\ShellNew\\NullFile",
			".ILBM\\ShellNew\\NullFile",
			".ILBM\\ShellNew\\NullFile",
			".JIF\\ShellNew\\NullFile",
			".JIF\\ShellNew\\NullFile",
			".PIC\\ShellNew\\NullFile",
			".PIC\\ShellNew\\NullFile",
			".PSD\\ShellNew\\NullFile",
			".PSD\\ShellNew\\NullFile",
			".cfl\\ShellNew\\NullFile",
			".cc\\ShellNew\\NullFile",
			".pdf\\ShellNew\\NullFile",
			".fdf\\ShellNew\\NullFile",
			".vcp\\ShellNew\\NullFile",
			".LZH\\ShellNew\\NullFile",
			".ARJ\\ShellNew\\NullFile",
			".ARC\\ShellNew\\NullFile",
			".ARC\\ShellNew\\Data",
			".TAZ\\ShellNew\\NullFile",
			"*\\ShellNew\\NullFile",
			".frm\\ShellNew\\Data",
			".frx\\ShellNew\\Data",
			".TZ\\ShellNew\\NullFile",
			".TZ\\ShellNew\\Data",
			".UU\\ShellNew\\NullFile",
			".UUE\\ShellNew\\NullFile",
			".XXE\\ShellNew\\NullFile",
			".B64\\ShellNew\\NullFile",
			".BHX\\ShellNew\\NullFile",
			".DAT\\ShellNew\\NullFile",
			".LOG\\ShellNew\\NullFile",
			".LOG\\ShellNew\\Data",
			NULL
		}
	};
	
	HKEY hkey;
	const char * const *pre;

	group = group % 3;                  // Make sure group is 0, 1, or 2

	for (pre = reg_entries[group]; *pre != NULL; ++pre)
	{
		// Separate the key name from the sub-key name
		const char *pp = strrchr(*pre, '\\');
		ASSERT(pp != NULL);                             // Make sure it had a backslash somewhere
		char reg_data[128];
		ASSERT(pp - *pre < sizeof(reg_data)-1);
		strncpy(reg_data, *pre, pp - *pre);
		reg_data[pp-*pre] = '\0';
		++pp;                                                   // Point to sub-key name
		
		// Open the key
		if (::RegOpenKey(HKEY_CLASSES_ROOT, reg_data, &hkey) == ERROR_SUCCESS)
		{
			// Get the sub-key value
			DWORD reg_type;                     // The returned registry entry type
			DWORD reg_size = sizeof(reg_data);
			(void)::RegQueryValueEx(hkey, pp, NULL, &reg_type, (BYTE *)reg_data, &reg_size);
			::RegCloseKey(hkey);
		}
	}
}

// This just subtracts 1 from each char in a string
static void sub1(char *ss)
{
	for ( ; *ss != '\0'; ++ss)
		*ss = *ss - 1;
}
#endif  // #ifndef REGISTER_APP

//-----------------------------------------------------------------------------
// PRNGs

// Mersenne Twist PRNG
#if 1
// constants for MT11213A:
#define MERS_N 351
#define MERS_M 175
#define MERS_R 19
#define MERS_U 11
#define MERS_S 7
#define MERS_T 15
#define MERS_L 17
#define MERS_A 0xE4BD75F5
#define MERS_B 0x655E5280
#define MERS_C 0xFFD58000
#else
// constants for MT19937:
#define MERS_N 624
#define MERS_M 397
#define MERS_R 31
#define MERS_U 11
#define MERS_S 7
#define MERS_T 15
#define MERS_L 18
#define MERS_A 0x9908B0DF
#define MERS_B 0x9D2C5680
#define MERS_C 0xEFC60000
#endif

static int rand1_ind;                           // Index into rand1_state array
static unsigned long rand1_state[MERS_N];       // State vector
#ifdef _DEBUG
static bool rand1_init = false;
#endif

void rand1_seed(unsigned long seed)
{
	for (rand1_ind = 0; rand1_ind < MERS_N; rand1_ind++)
	{
		seed = seed * 29943829 - 1;
		rand1_state[rand1_ind] = seed;
	}
#ifdef _DEBUG
	rand1_init = true;
#endif
}

unsigned long rand1()
{
	ASSERT(rand1_init);

	unsigned long retval;
	
	if (rand1_ind >= MERS_N)
	{
		const unsigned long LOWER_MASK = (1LU << MERS_R) - 1;
		const unsigned long UPPER_MASK = -1L  << MERS_R;
		int kk, km;
		for (kk=0, km=MERS_M; kk < MERS_N-1; kk++)
		{
			retval = (rand1_state[kk] & UPPER_MASK) | (rand1_state[kk+1] & LOWER_MASK);
			rand1_state[kk] = rand1_state[km] ^ (retval >> 1) ^ (-(signed long)(retval & 1) & MERS_A);
			if (++km >= MERS_N) km = 0;
		}

		retval = (rand1_state[MERS_N-1] & UPPER_MASK) | (rand1_state[0] & LOWER_MASK);
		rand1_state[MERS_N-1] = rand1_state[MERS_M-1] ^ (retval >> 1) ^ (-(signed long)(retval & 1) & MERS_A);
		rand1_ind = 0;
	}

	retval = rand1_state[rand1_ind++];
	retval ^=  retval >> MERS_U;
	retval ^= (retval << MERS_S) & MERS_B;
	retval ^= (retval << MERS_T) & MERS_C;
	retval ^=  retval >> MERS_L;

	return retval;
}

// RANROT PRNG

#define RANROT_KK  17
#define RANROT_JJ  10
#define RANROT_R1  13
#define RANROT_R2   9

static int rand2_p1, rand2_p2;
static unsigned long randbuffer[RANROT_KK];
#ifdef _DEBUG
static unsigned long randbufcopy[RANROT_KK*2];
#endif

void rand2_seed(unsigned long seed)
{
	for (int ii = 0; ii < RANROT_KK; ++ii)
	{
		seed = seed*2891336453 + 1;
		randbuffer[ii] = seed;
	}

	rand2_p1 = 0;
	rand2_p2 = RANROT_JJ;

#ifdef _DEBUG
	memcpy (randbufcopy, randbuffer, RANROT_KK*sizeof(long));
	memcpy (randbufcopy + RANROT_KK, randbuffer, RANROT_KK*sizeof(long));
#endif
}

unsigned long rand2()
{
	unsigned long retval = randbuffer[rand2_p1] = _lrotl(randbuffer[rand2_p2], RANROT_R1) +
												  _lrotl(randbuffer[rand2_p1], RANROT_R2);

	if (--rand2_p1 < 0) rand2_p1 = RANROT_KK - 1;
	if (--rand2_p2 < 0) rand2_p2 = RANROT_KK - 1;

#ifdef _DEBUG
	if (randbuffer[rand2_p1] == randbufcopy[0] &&
		memcmp(randbuffer, randbufcopy + RANROT_KK - rand2_p1, RANROT_KK*sizeof(long)) == 0)
	{
		// self-test failed
		if ((rand2_p2 + RANROT_KK - rand2_p1) % RANROT_KK != RANROT_JJ)
			ASSERT(0);                  // Apparently not initialised
		else
			TRACE("RANROT PRNG returned to initial state!\n");
	}
#endif

	return retval;
}

#define RAND_GOOD_SIZE  17
static bool rand_good_init = false;
static unsigned long rand_good_val[RAND_GOOD_SIZE];

static void rand_good_fill()
{
	// Fill up the array with random numbers from rand2
	for (int ii = 0; ii < RAND_GOOD_SIZE; ++ii)
		rand_good_val[ii] = rand2();
	rand_good_init = true;
}

void rand_good_seed(unsigned long seed)
{
	rand1_seed(seed);
	rand2_seed(seed);
	rand_good_fill();
}

unsigned long rand_good()
{
	if (!rand_good_init)
		rand_good_fill();

//    return __int64(rand1())<<32 | __int64(rand2());
	size_t ind = rand1()%RAND_GOOD_SIZE;        // Use rand1 to work out the index
	unsigned long retval = rand_good_val[ind];  // Save the current value there
	rand_good_val[ind] = rand2();               // Get the next value
	return retval;
}

//-----------------------------------------------------------------------------
// Memory

// Compare two blocks of memory to find the first different byte.  (This is 
// similar to memcmp but returns the number of bytes the same.)
// Parameters:
//   pp, qq = pointers to the blocks of memory to compare
//   len = max number of bytes to compare
// Returns:
//   the number of bytes up to the difference OR
//   -1 if all bytes are the same
// Notes:
//   The code is optimized to do 8-byte compares for a 64-bit processor.
//   Best performance is when both pp and qq have the same 8-byte alignment.
//   However, even if pp and qq have different alignements better performance 
//   is obtained if 8-byte reads are done from one of the buffers - this is the 
//   reason for skipping up to 7 bytes at the start of the comparison.
//   Under MS C++ version 15 (VS2008) this gives comparable speed to memcmp()
//   when both buffers start on a 4-byte boundary but much better performance
//   in all other situations.  (The other problem is that memcmp does not say 
//   where in the buffers the difference was found.)
//   This code would be even faster compiled for a 64-bit processor since a
//   comparison between two __int64 values would be a single instruction, etc.
int next_diff(const void * buf1, const void * buf2, size_t len)
{
	const unsigned char * pp = (const unsigned char *)buf1;
	const unsigned char * qq = (const unsigned char *)buf2;

	// Check up to 7 initial bytes (until pp and/or qq is aligned on a 8-byte boundary)
	while (((int)pp % 8) != 0 && len > 0)
	{
		if (*pp != *qq)
			return pp - (const unsigned char *)buf1;
		++pp, ++qq;
		len--;
	}

	// Check most of the buffers 8 bytes at a time (best if qq is also 8 byte aligned)
	div_t len8 = div(len, 8);
	const unsigned __int64 * pend = (const unsigned __int64 *)pp + len8.quot;

	while ((const unsigned __int64 *)pp < pend)
	{
		if (*((const unsigned __int64 *)pp) != *((const unsigned __int64 *)qq))
		{
			// This 8-byte value is different - work out which byte it was
			while (*pp == *qq)
				++pp, ++qq;

			return pp - (const unsigned char *)buf1;
		}

		pp += 8;
		qq += 8;
	}

	// Check up to 7 bytes at the end
	while (len8.rem--)
	{
		if (*pp != *qq)
			return pp - (const unsigned char *)buf1;
		++pp, ++qq;
	}

	return -1;
}


//-----------------------------------------------------------------------------
// CRCs

#ifdef BOOST_CRC  // Use Boost for CRC 16
void * crc_16_init()
{
	return new boost::crc_16_type;
}

void crc_16_update(void *hh, const void *buf, size_t len)
		{
	boost::crc_16_type *pcrc = (boost::crc_16_type *)hh;
	pcrc->process_bytes(buf, len);
		}

unsigned short crc_16_final(void *hh)
{
	boost::crc_16_type *pcrc = (boost::crc_16_type *)hh;
	unsigned short retval = pcrc->checksum();
	delete pcrc;
	return retval;
}

unsigned short crc_16(const void *buf, size_t len)
{
	void * hh = crc_16_init();
	crc_16_update(hh, buf, len);
	return crc_16_final(hh);
}
#endif

#ifdef BOOST_CRC  // Use Boost for CRC32

void * crc_32_init()
{
	return new boost::crc_32_type;
}

void crc_32_update(void *hh, const void *buf, size_t len)
{
	boost::crc_32_type *pcrc = (boost::crc_32_type *)hh;
	pcrc->process_bytes(buf, len);
}

DWORD crc_32_final(void *hh)
{
	boost::crc_32_type *pcrc = (boost::crc_32_type *)hh;
	DWORD retval = pcrc->checksum();
	delete pcrc;
	return retval;
}

#else  // BOOST_CRC

static DWORD crc_32_tab[] = { /* CRC polynomial 0xedb88320 */
0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

#define UPDC32(octet,crc) (crc_32_tab[((crc) ^ ((BYTE)octet)) & 0xff] ^ ((crc) >> 8))

void * crc_32_init()
{
	DWORD * hh = new DWORD;
    *hh = 0xffffFFFF;
	return hh;
}

void crc_32_update(void *hh, const void *buf, size_t len)
{
    const char *pp = (const char *)buf;
    for ( ; len; --len, ++pp)
        *(DWORD *)hh = UPDC32(*pp, *(DWORD *)hh);
}

DWORD crc_32_final(void *hh)
{
	DWORD retval = ~(*(DWORD *)hh);
	delete (DWORD *)hh;
    return retval;
}

#endif  // BOOST_CRC

DWORD crc_32(const void *buf, size_t len)
{
	void * hh = crc_32_init();
	crc_32_update(hh, buf, len);
	return crc_32_final(hh);
}


#ifdef BOOST_CRC  // Use Boost for CRC CCITT

void * crc_ccitt_init()
{
	return new boost::crc_ccitt_type;
}

void crc_ccitt_update(void *hh, const void *buf, size_t len)
{
	boost::crc_ccitt_type *pcrc = (boost::crc_ccitt_type *)hh;
	pcrc->process_bytes(buf, len);
}

unsigned short crc_ccitt_final(void *hh)
{
	boost::crc_ccitt_type *pcrc = (boost::crc_ccitt_type *)hh;
	unsigned short retval = pcrc->checksum();
	delete pcrc;
	return retval;
}

#else  // BOOST_CRC

	/* the CRC polynomial. This is [not exactly] used by CCITT.
	 * If you change P, you must change crctab[]'s initial value to what is
	 * printed by initcrctab()
	 */
// #define   P    0x1021 /* only used for generating the table */

static WORD crctab[256] = { /* as calculated by initcrctab() */
	0x0000,  0x1021,  0x2042,  0x3063,  0x4084,  0x50a5,  0x60c6,  0x70e7,
	0x8108,  0x9129,  0xa14a,  0xb16b,  0xc18c,  0xd1ad,  0xe1ce,  0xf1ef,
	0x1231,  0x0210,  0x3273,  0x2252,  0x52b5,  0x4294,  0x72f7,  0x62d6,
	0x9339,  0x8318,  0xb37b,  0xa35a,  0xd3bd,  0xc39c,  0xf3ff,  0xe3de,
	0x2462,  0x3443,  0x0420,  0x1401,  0x64e6,  0x74c7,  0x44a4,  0x5485,
	0xa56a,  0xb54b,  0x8528,  0x9509,  0xe5ee,  0xf5cf,  0xc5ac,  0xd58d,
	0x3653,  0x2672,  0x1611,  0x0630,  0x76d7,  0x66f6,  0x5695,  0x46b4,
	0xb75b,  0xa77a,  0x9719,  0x8738,  0xf7df,  0xe7fe,  0xd79d,  0xc7bc,
	0x48c4,  0x58e5,  0x6886,  0x78a7,  0x0840,  0x1861,  0x2802,  0x3823,
	0xc9cc,  0xd9ed,  0xe98e,  0xf9af,  0x8948,  0x9969,  0xa90a,  0xb92b,
	0x5af5,  0x4ad4,  0x7ab7,  0x6a96,  0x1a71,  0x0a50,  0x3a33,  0x2a12,
	0xdbfd,  0xcbdc,  0xfbbf,  0xeb9e,  0x9b79,  0x8b58,  0xbb3b,  0xab1a,
	0x6ca6,  0x7c87,  0x4ce4,  0x5cc5,  0x2c22,  0x3c03,  0x0c60,  0x1c41,
	0xedae,  0xfd8f,  0xcdec,  0xddcd,  0xad2a,  0xbd0b,  0x8d68,  0x9d49,
	0x7e97,  0x6eb6,  0x5ed5,  0x4ef4,  0x3e13,  0x2e32,  0x1e51,  0x0e70,
	0xff9f,  0xefbe,  0xdfdd,  0xcffc,  0xbf1b,  0xaf3a,  0x9f59,  0x8f78,
	0x9188,  0x81a9,  0xb1ca,  0xa1eb,  0xd10c,  0xc12d,  0xf14e,  0xe16f,
	0x1080,  0x00a1,  0x30c2,  0x20e3,  0x5004,  0x4025,  0x7046,  0x6067,
	0x83b9,  0x9398,  0xa3fb,  0xb3da,  0xc33d,  0xd31c,  0xe37f,  0xf35e,
	0x02b1,  0x1290,  0x22f3,  0x32d2,  0x4235,  0x5214,  0x6277,  0x7256,
	0xb5ea,  0xa5cb,  0x95a8,  0x8589,  0xf56e,  0xe54f,  0xd52c,  0xc50d,
	0x34e2,  0x24c3,  0x14a0,  0x0481,  0x7466,  0x6447,  0x5424,  0x4405,
	0xa7db,  0xb7fa,  0x8799,  0x97b8,  0xe75f,  0xf77e,  0xc71d,  0xd73c,
	0x26d3,  0x36f2,  0x0691,  0x16b0,  0x6657,  0x7676,  0x4615,  0x5634,
	0xd94c,  0xc96d,  0xf90e,  0xe92f,  0x99c8,  0x89e9,  0xb98a,  0xa9ab,
	0x5844,  0x4865,  0x7806,  0x6827,  0x18c0,  0x08e1,  0x3882,  0x28a3,
	0xcb7d,  0xdb5c,  0xeb3f,  0xfb1e,  0x8bf9,  0x9bd8,  0xabbb,  0xbb9a,
	0x4a75,  0x5a54,  0x6a37,  0x7a16,  0x0af1,  0x1ad0,  0x2ab3,  0x3a92,
	0xfd2e,  0xed0f,  0xdd6c,  0xcd4d,  0xbdaa,  0xad8b,  0x9de8,  0x8dc9,
	0x7c26,  0x6c07,  0x5c64,  0x4c45,  0x3ca2,  0x2c83,  0x1ce0,  0x0cc1,
	0xef1f,  0xff3e,  0xcf5d,  0xdf7c,  0xaf9b,  0xbfba,  0x8fd9,  0x9ff8,
	0x6e17,  0x7e36,  0x4e55,  0x5e74,  0x2e93,  0x3eb2,  0x0ed1,  0x1ef0
	};

//static unsigned short crc_ccitt_value;

void * crc_ccitt_init()
{
	unsigned short * hh = new unsigned short;
	*hh = -1;
	return hh;
}

void crc_ccitt_update(void * hh, const void *buf, size_t len)
{
	const unsigned char *cp = (const unsigned char *)buf;

	while (len--)
		*(unsigned short *)hh = (*(unsigned short *)hh<<8) ^ crctab[(*(unsigned short *)hh>>(16-8)) ^ *cp++];
}

unsigned short crc_ccitt_final(void * hh)
{
    unsigned short retval = *(unsigned short *)hh;
	delete (unsigned short *)hh;
	return retval;
}

#endif  // BOOST_CRC

unsigned short crc_ccitt(const void *buf, size_t len)
{
	void * hh = crc_ccitt_init();
	crc_ccitt_update(hh, buf, len);
	return crc_ccitt_final(hh);
}

#ifdef BOOST_CRC  // Use Boost for XMODEM CRC
void * crc_xmodem_init()
{
	return new boost::crc_xmodem_type;
}

void crc_xmodem_update(void *hh, const void *buf, size_t len)
{
	boost::crc_xmodem_type *pcrc = (boost::crc_xmodem_type *)hh;
	pcrc->process_bytes(buf, len);
}

unsigned short crc_xmodem_final(void *hh)
{
	boost::crc_xmodem_type *pcrc = (boost::crc_xmodem_type *)hh;
	unsigned short retval = pcrc->checksum();
	delete pcrc;
	return retval;
}
unsigned short crc_xmodem(const void *buf, size_t len)
{
	void * hh = crc_xmodem_init();
	crc_xmodem_update(hh, buf, len);
	return crc_xmodem_final(hh);
}
#endif

// Apparently the following is the common but incorrect implementation
#define           poly     0x1021          /* crc-ccitt mask */ 

void * crc_ccitt_b_init()
{
	unsigned short * hh = new unsigned short;
	*hh = -1;
	return hh;
}

void crc_ccitt_b_update(void * hh, const void *buf, size_t len)
{
    const unsigned char *cp = (const unsigned char *)buf;
    const unsigned char *end = cp + len;

    unsigned short ii, vv, xor_flag; 

    for ( ; cp < end; ++cp)
    {
        /* 
        Align test bit with leftmost bit of the message byte. 
        */ 
        vv = 0x80; 

        for (ii = 0; ii < 8; ++ii) 
        { 
            if (*(unsigned short *)hh & 0x8000) 
                xor_flag= 1; 
            else 
                xor_flag= 0; 
            *(unsigned short *)hh = *(unsigned short *)hh << 1; 

            if (*cp & vv) 
            { 
                /* 
                Append next bit of message to end of CRC if it is not zero. 
                The zero bit placed there by the shift above need not be 
                changed if the next bit of the message is zero. 
                */ 
                *(unsigned short *)hh = *(unsigned short *)hh + 1; 
            } 

            if (xor_flag) 
                *(unsigned short *)hh = *(unsigned short *)hh ^ poly; 

            /* 
            Align test bit with next bit of the message byte. 
            */ 
            vv = vv >> 1; 
        } 
    }
}

unsigned short crc_ccitt_b_final(void *hh)
{
    unsigned short ii, xor_flag; 

    // Augment message
    for (ii = 0; ii < 16; ++ii) 
    { 
        if (*(unsigned short *)hh & 0x8000) 
            xor_flag= 1; 
        else 
            xor_flag= 0; 
        *(unsigned short *)hh = *(unsigned short *)hh << 1; 

        if (xor_flag) 
            *(unsigned short *)hh = *(unsigned short *)hh ^ poly; 
    }

    unsigned short retval = *(unsigned short *)hh;
	delete (unsigned short *)hh;
	return retval;
}

unsigned short crc_ccitt_b(const void *buf, size_t len)
{
	void * hh = crc_ccitt_b_init();
	crc_ccitt_b_update(hh, buf, len);
	return crc_ccitt_b_final(hh);
}


//-----------------------------------------------------------------------------
// Activation code stuff

// The following are for storing binary data as alphanumeric characters

static const char letter[33] = "ABCDEFGHJKLMNPQRSTUVWXYZ345679IO";

// Returns the valid letter or 0 if its not allowed
char letter_valid(char cc)
{
	if (cc == '0') cc = 'O';            // Zero -> O
	else if (cc == '1') cc = 'I';       // One -> I
	else cc = toupper(cc);              // Make sure it's uppercase

	if (strchr(letter, cc) != NULL)
		return cc;
	else
		return '\0';
}

// Encode binary data in buf, of length len into result.
// Note that the buffer pointed to by result must be big enough to hold the result.
void letter_encode(const void *buf, size_t len, char *result)
{
	const unsigned char *pp = (const unsigned char *)buf;
	const unsigned char *end = pp + len;
	char *pout = result;
	int shift, residue;

	for (shift = 3, residue = 0; pp < end; ++pout)
	{
		*pout = letter[(residue|(*pp>>shift))&0x1F];
		residue = *pp /* & ((1<<shift)-1))*/ << (5-shift);
		shift -= 5;
		if (shift < 0)
		{
			++pp;
			shift += 8;
		}
		else
			residue = 0;
	}
	if (shift != 3)
		*pout++ = letter[residue&0x1F];
	*pout = '\0';
}

// Given a string of letters from the letter array above (ie. a string produced
// by letter_encode) it generates the unencoded bytes.  It returns the number
// of bytes produced or -1 if an invalid character was found in the string.
int letter_decode(const char *str, size_t str_len, void *result)
{
	unsigned char *pout = (unsigned char *)result;
	const char *end = str + str_len;
	unsigned int value = 0;
	int bits_left;

	for (value = bits_left = 0; str < end; ++str)
	{
		char cc = letter_valid(*str);

		if (cc == '\0')
			return -1;

		ASSERT(strchr(letter, cc) != NULL);
		value = (value<<5) | (strchr(letter, cc) - letter);

		bits_left += 5;
		if (bits_left > 7)
		{
			*pout++ = value >> (bits_left-8);
			bits_left -= 8;
		}
	}
	return pout - (unsigned char *)result;
}

//-----------------------------------------------------------------------------
// Encryption

// The following is for Blow Fish encryption and decryption

#define size_ 16
static unsigned long P_[size_ + 2];
static unsigned long S_[4][256];

#define S(x,i) (S_[i][(x>>((3-i)*8))&0xFF])
#define RR(a,b,n) (a ^= (((S(b,0) + S(b,1)) ^ S(b,2)) + S(b,3)) ^ P_[n])

static void encipher(unsigned long *pl, unsigned long *pr);
static void decipher(unsigned long *pl, unsigned long *pr);

static const unsigned long def_P_[size_ + 2] =
{
	0x243f6a88, 0x85a308d3, 0x13198a2e, 0x03707344,
	0xa4093822, 0x299f31d0, 0x082efa98, 0xec4e6c89,
	0x452821e6, 0x38d01377, 0xbe5466cf, 0x34e90c6c,
	0xc0ac29b7, 0xc97c50dd, 0x3f84d5b5, 0xb5470917,
	0x9216d5d9, 0x8979fb1b,
};

static const unsigned long def_S_[4][256] =
{
	0xd1310ba6, 0x98dfb5ac, 0x2ffd72db, 0xd01adfb7,
	0xb8e1afed, 0x6a267e96, 0xba7c9045, 0xf12c7f99,
	0x24a19947, 0xb3916cf7, 0x0801f2e2, 0x858efc16,
	0x636920d8, 0x71574e69, 0xa458fea3, 0xf4933d7e,
	0x0d95748f, 0x728eb658, 0x718bcd58, 0x82154aee,
	0x7b54a41d, 0xc25a59b5, 0x9c30d539, 0x2af26013,
	0xc5d1b023, 0x286085f0, 0xca417918, 0xb8db38ef,
	0x8e79dcb0, 0x603a180e, 0x6c9e0e8b, 0xb01e8a3e,
	0xd71577c1, 0xbd314b27, 0x78af2fda, 0x55605c60,
	0xe65525f3, 0xaa55ab94, 0x57489862, 0x63e81440,
	0x55ca396a, 0x2aab10b6, 0xb4cc5c34, 0x1141e8ce,
	0xa15486af, 0x7c72e993, 0xb3ee1411, 0x636fbc2a,
	0x2ba9c55d, 0x741831f6, 0xce5c3e16, 0x9b87931e,
	0xafd6ba33, 0x6c24cf5c, 0x7a325381, 0x28958677,
	0x3b8f4898, 0x6b4bb9af, 0xc4bfe81b, 0x66282193,
	0x61d809cc, 0xfb21a991, 0x487cac60, 0x5dec8032,
	0xef845d5d, 0xe98575b1, 0xdc262302, 0xeb651b88,
	0x23893e81, 0xd396acc5, 0x0f6d6ff3, 0x83f44239,
	0x2e0b4482, 0xa4842004, 0x69c8f04a, 0x9e1f9b5e,
	0x21c66842, 0xf6e96c9a, 0x670c9c61, 0xabd388f0,
	0x6a51a0d2, 0xd8542f68, 0x960fa728, 0xab5133a3,
	0x6eef0b6c, 0x137a3be4, 0xba3bf050, 0x7efb2a98,
	0xa1f1651d, 0x39af0176, 0x66ca593e, 0x82430e88,
	0x8cee8619, 0x456f9fb4, 0x7d84a5c3, 0x3b8b5ebe,
	0xe06f75d8, 0x85c12073, 0x401a449f, 0x56c16aa6,
	0x4ed3aa62, 0x363f7706, 0x1bfedf72, 0x429b023d,
	0x37d0d724, 0xd00a1248, 0xdb0fead3, 0x49f1c09b,
	0x075372c9, 0x80991b7b, 0x25d479d8, 0xf6e8def7,
	0xe3fe501a, 0xb6794c3b, 0x976ce0bd, 0x04c006ba,
	0xc1a94fb6, 0x409f60c4, 0x5e5c9ec2, 0x196a2463,
	0x68fb6faf, 0x3e6c53b5, 0x1339b2eb, 0x3b52ec6f,
	0x6dfc511f, 0x9b30952c, 0xcc814544, 0xaf5ebd09,
	0xbee3d004, 0xde334afd, 0x660f2807, 0x192e4bb3,
	0xc0cba857, 0x45c8740f, 0xd20b5f39, 0xb9d3fbdb,
	0x5579c0bd, 0x1a60320a, 0xd6a100c6, 0x402c7279,
	0x679f25fe, 0xfb1fa3cc, 0x8ea5e9f8, 0xdb3222f8,
	0x3c7516df, 0xfd616b15, 0x2f501ec8, 0xad0552ab,
	0x323db5fa, 0xfd238760, 0x53317b48, 0x3e00df82,
	0x9e5c57bb, 0xca6f8ca0, 0x1a87562e, 0xdf1769db,
	0xd542a8f6, 0x287effc3, 0xac6732c6, 0x8c4f5573,
	0x695b27b0, 0xbbca58c8, 0xe1ffa35d, 0xb8f011a0,
	0x10fa3d98, 0xfd2183b8, 0x4afcb56c, 0x2dd1d35b,
	0x9a53e479, 0xb6f84565, 0xd28e49bc, 0x4bfb9790,
	0xe1ddf2da, 0xa4cb7e33, 0x62fb1341, 0xcee4c6e8,
	0xef20cada, 0x36774c01, 0xd07e9efe, 0x2bf11fb4,
	0x95dbda4d, 0xae909198, 0xeaad8e71, 0x6b93d5a0,
	0xd08ed1d0, 0xafc725e0, 0x8e3c5b2f, 0x8e7594b7,
	0x8ff6e2fb, 0xf2122b64, 0x8888b812, 0x900df01c,
	0x4fad5ea0, 0x688fc31c, 0xd1cff191, 0xb3a8c1ad,
	0x2f2f2218, 0xbe0e1777, 0xea752dfe, 0x8b021fa1,
	0xe5a0cc0f, 0xb56f74e8, 0x18acf3d6, 0xce89e299,
	0xb4a84fe0, 0xfd13e0b7, 0x7cc43b81, 0xd2ada8d9,
	0x165fa266, 0x80957705, 0x93cc7314, 0x211a1477,
	0xe6ad2065, 0x77b5fa86, 0xc75442f5, 0xfb9d35cf,
	0xebcdaf0c, 0x7b3e89a0, 0xd6411bd3, 0xae1e7e49,
	0x00250e2d, 0x2071b35e, 0x226800bb, 0x57b8e0af,
	0x2464369b, 0xf009b91e, 0x5563911d, 0x59dfa6aa,
	0x78c14389, 0xd95a537f, 0x207d5ba2, 0x02e5b9c5,
	0x83260376, 0x6295cfa9, 0x11c81968, 0x4e734a41,
	0xb3472dca, 0x7b14a94a, 0x1b510052, 0x9a532915,
	0xd60f573f, 0xbc9bc6e4, 0x2b60a476, 0x81e67400,
	0x08ba6fb5, 0x571be91f, 0xf296ec6b, 0x2a0dd915,
	0xb6636521, 0xe7b9f9b6, 0xff34052e, 0xc5855664,
	0x53b02d5d, 0xa99f8fa1, 0x08ba4799, 0x6e85076a,
	0x4b7a70e9, 0xb5b32944, 0xdb75092e, 0xc4192623,
	0xad6ea6b0, 0x49a7df7d, 0x9cee60b8, 0x8fedb266,
	0xecaa8c71, 0x699a17ff, 0x5664526c, 0xc2b19ee1,
	0x193602a5, 0x75094c29, 0xa0591340, 0xe4183a3e,
	0x3f54989a, 0x5b429d65, 0x6b8fe4d6, 0x99f73fd6,
	0xa1d29c07, 0xefe830f5, 0x4d2d38e6, 0xf0255dc1,
	0x4cdd2086, 0x8470eb26, 0x6382e9c6, 0x021ecc5e,
	0x09686b3f, 0x3ebaefc9, 0x3c971814, 0x6b6a70a1,
	0x687f3584, 0x52a0e286, 0xb79c5305, 0xaa500737,
	0x3e07841c, 0x7fdeae5c, 0x8e7d44ec, 0x5716f2b8,
	0xb03ada37, 0xf0500c0d, 0xf01c1f04, 0x0200b3ff,
	0xae0cf51a, 0x3cb574b2, 0x25837a58, 0xdc0921bd,
	0xd19113f9, 0x7ca92ff6, 0x94324773, 0x22f54701,
	0x3ae5e581, 0x37c2dadc, 0xc8b57634, 0x9af3dda7,
	0xa9446146, 0x0fd0030e, 0xecc8c73e, 0xa4751e41,
	0xe238cd99, 0x3bea0e2f, 0x3280bba1, 0x183eb331,
	0x4e548b38, 0x4f6db908, 0x6f420d03, 0xf60a04bf,
	0x2cb81290, 0x24977c79, 0x5679b072, 0xbcaf89af,
	0xde9a771f, 0xd9930810, 0xb38bae12, 0xdccf3f2e,
	0x5512721f, 0x2e6b7124, 0x501adde6, 0x9f84cd87,
	0x7a584718, 0x7408da17, 0xbc9f9abc, 0xe94b7d8c,
	0xec7aec3a, 0xdb851dfa, 0x63094366, 0xc464c3d2,
	0xef1c1847, 0x3215d908, 0xdd433b37, 0x24c2ba16,
	0x12a14d43, 0x2a65c451, 0x50940002, 0x133ae4dd,
	0x71dff89e, 0x10314e55, 0x81ac77d6, 0x5f11199b,
	0x043556f1, 0xd7a3c76b, 0x3c11183b, 0x5924a509,
	0xf28fe6ed, 0x97f1fbfa, 0x9ebabf2c, 0x1e153c6e,
	0x86e34570, 0xeae96fb1, 0x860e5e0a, 0x5a3e2ab3,
	0x771fe71c, 0x4e3d06fa, 0x2965dcb9, 0x99e71d0f,
	0x803e89d6, 0x5266c825, 0x2e4cc978, 0x9c10b36a,
	0xc6150eba, 0x94e2ea78, 0xa5fc3c53, 0x1e0a2df4,
	0xf2f74ea7, 0x361d2b3d, 0x1939260f, 0x19c27960,
	0x5223a708, 0xf71312b6, 0xebadfe6e, 0xeac31f66,
	0xe3bc4595, 0xa67bc883, 0xb17f37d1, 0x018cff28,
	0xc332ddef, 0xbe6c5aa5, 0x65582185, 0x68ab9802,
	0xeecea50f, 0xdb2f953b, 0x2aef7dad, 0x5b6e2f84,
	0x1521b628, 0x29076170, 0xecdd4775, 0x619f1510,
	0x13cca830, 0xeb61bd96, 0x0334fe1e, 0xaa0363cf,
	0xb5735c90, 0x4c70a239, 0xd59e9e0b, 0xcbaade14,
	0xeecc86bc, 0x60622ca7, 0x9cab5cab, 0xb2f3846e,
	0x648b1eaf, 0x19bdf0ca, 0xa02369b9, 0x655abb50,
	0x40685a32, 0x3c2ab4b3, 0x319ee9d5, 0xc021b8f7,
	0x9b540b19, 0x875fa099, 0x95f7997e, 0x623d7da8,
	0xf837889a, 0x97e32d77, 0x11ed935f, 0x16681281,
	0x0e358829, 0xc7e61fd6, 0x96dedfa1, 0x7858ba99,
	0x57f584a5, 0x1b227263, 0x9b83c3ff, 0x1ac24696,
	0xcdb30aeb, 0x532e3054, 0x8fd948e4, 0x6dbc3128,
	0x58ebf2ef, 0x34c6ffea, 0xfe28ed61, 0xee7c3c73,
	0x5d4a14d9, 0xe864b7e3, 0x42105d14, 0x203e13e0,
	0x45eee2b6, 0xa3aaabea, 0xdb6c4f15, 0xfacb4fd0,
	0xc742f442, 0xef6abbb5, 0x654f3b1d, 0x41cd2105,
	0xd81e799e, 0x86854dc7, 0xe44b476a, 0x3d816250,
	0xcf62a1f2, 0x5b8d2646, 0xfc8883a0, 0xc1c7b6a3,
	0x7f1524c3, 0x69cb7492, 0x47848a0b, 0x5692b285,
	0x095bbf00, 0xad19489d, 0x1462b174, 0x23820e00,
	0x58428d2a, 0x0c55f5ea, 0x1dadf43e, 0x233f7061,
	0x3372f092, 0x8d937e41, 0xd65fecf1, 0x6c223bdb,
	0x7cde3759, 0xcbee7460, 0x4085f2a7, 0xce77326e,
	0xa6078084, 0x19f8509e, 0xe8efd855, 0x61d99735,
	0xa969a7aa, 0xc50c06c2, 0x5a04abfc, 0x800bcadc,
	0x9e447a2e, 0xc3453484, 0xfdd56705, 0x0e1e9ec9,
	0xdb73dbd3, 0x105588cd, 0x675fda79, 0xe3674340,
	0xc5c43465, 0x713e38d8, 0x3d28f89e, 0xf16dff20,
	0x153e21e7, 0x8fb03d4a, 0xe6e39f2b, 0xdb83adf7,
	0xe93d5a68, 0x948140f7, 0xf64c261c, 0x94692934,
	0x411520f7, 0x7602d4f7, 0xbcf46b2e, 0xd4a20068,
	0xd4082471, 0x3320f46a, 0x43b7d4b7, 0x500061af,
	0x1e39f62e, 0x97244546, 0x14214f74, 0xbf8b8840,
	0x4d95fc1d, 0x96b591af, 0x70f4ddd3, 0x66a02f45,
	0xbfbc09ec, 0x03bd9785, 0x7fac6dd0, 0x31cb8504,
	0x96eb27b3, 0x55fd3941, 0xda2547e6, 0xabca0a9a,
	0x28507825, 0x530429f4, 0x0a2c86da, 0xe9b66dfb,
	0x68dc1462, 0xd7486900, 0x680ec0a4, 0x27a18dee,
	0x4f3ffea2, 0xe887ad8c, 0xb58ce006, 0x7af4d6b6,
	0xaace1e7c, 0xd3375fec, 0xce78a399, 0x406b2a42,
	0x20fe9e35, 0xd9f385b9, 0xee39d7ab, 0x3b124e8b,
	0x1dc9faf7, 0x4b6d1856, 0x26a36631, 0xeae397b2,
	0x3a6efa74, 0xdd5b4332, 0x6841e7f7, 0xca7820fb,
	0xfb0af54e, 0xd8feb397, 0x454056ac, 0xba489527,
	0x55533a3a, 0x20838d87, 0xfe6ba9b7, 0xd096954b,
	0x55a867bc, 0xa1159a58, 0xcca92963, 0x99e1db33,
	0xa62a4a56, 0x3f3125f9, 0x5ef47e1c, 0x9029317c,
	0xfdf8e802, 0x04272f70, 0x80bb155c, 0x05282ce3,
	0x95c11548, 0xe4c66d22, 0x48c1133f, 0xc70f86dc,
	0x07f9c9ee, 0x41041f0f, 0x404779a4, 0x5d886e17,
	0x325f51eb, 0xd59bc0d1, 0xf2bcc18f, 0x41113564,
	0x257b7834, 0x602a9c60, 0xdff8e8a3, 0x1f636c1b,
	0x0e12b4c2, 0x02e1329e, 0xaf664fd1, 0xcad18115,
	0x6b2395e0, 0x333e92e1, 0x3b240b62, 0xeebeb922,
	0x85b2a20e, 0xe6ba0d99, 0xde720c8c, 0x2da2f728,
	0xd0127845, 0x95b794fd, 0x647d0862, 0xe7ccf5f0,
	0x5449a36f, 0x877d48fa, 0xc39dfd27, 0xf33e8d1e,
	0x0a476341, 0x992eff74, 0x3a6f6eab, 0xf4f8fd37,
	0xa812dc60, 0xa1ebddf8, 0x991be14c, 0xdb6e6b0d,
	0xc67b5510, 0x6d672c37, 0x2765d43b, 0xdcd0e804,
	0xf1290dc7, 0xcc00ffa3, 0xb5390f92, 0x690fed0b,
	0x667b9ffb, 0xcedb7d9c, 0xa091cf0b, 0xd9155ea3,
	0xbb132f88, 0x515bad24, 0x7b9479bf, 0x763bd6eb,
	0x37392eb3, 0xcc115979, 0x8026e297, 0xf42e312d,
	0x6842ada7, 0xc66a2b3b, 0x12754ccc, 0x782ef11c,
	0x6a124237, 0xb79251e7, 0x06a1bbe6, 0x4bfb6350,
	0x1a6b1018, 0x11caedfa, 0x3d25bdd8, 0xe2e1c3c9,
	0x44421659, 0x0a121386, 0xd90cec6e, 0xd5abea2a,
	0x64af674e, 0xda86a85f, 0xbebfe988, 0x64e4c3fe,
	0x9dbc8057, 0xf0f7c086, 0x60787bf8, 0x6003604d,
	0xd1fd8346, 0xf6381fb0, 0x7745ae04, 0xd736fccc,
	0x83426b33, 0xf01eab71, 0xb0804187, 0x3c005e5f,
	0x77a057be, 0xbde8ae24, 0x55464299, 0xbf582e61,
	0x4e58f48f, 0xf2ddfda2, 0xf474ef38, 0x8789bdc2,
	0x5366f9c3, 0xc8b38e74, 0xb475f255, 0x46fcd9b9,
	0x7aeb2661, 0x8b1ddf84, 0x846a0e79, 0x915f95e2,
	0x466e598e, 0x20b45770, 0x8cd55591, 0xc902de4c,
	0xb90bace1, 0xbb8205d0, 0x11a86248, 0x7574a99e,
	0xb77f19b6, 0xe0a9dc09, 0x662d09a1, 0xc4324633,
	0xe85a1f02, 0x09f0be8c, 0x4a99a025, 0x1d6efe10,
	0x1ab93d1d, 0x0ba5a4df, 0xa186f20f, 0x2868f169,
	0xdcb7da83, 0x573906fe, 0xa1e2ce9b, 0x4fcd7f52,
	0x50115e01, 0xa70683fa, 0xa002b5c4, 0x0de6d027,
	0x9af88c27, 0x773f8641, 0xc3604c06, 0x61a806b5,
	0xf0177a28, 0xc0f586e0, 0x006058aa, 0x30dc7d62,
	0x11e69ed7, 0x2338ea63, 0x53c2dd94, 0xc2c21634,
	0xbbcbee56, 0x90bcb6de, 0xebfc7da1, 0xce591d76,
	0x6f05e409, 0x4b7c0188, 0x39720a3d, 0x7c927c24,
	0x86e3725f, 0x724d9db9, 0x1ac15bb4, 0xd39eb8fc,
	0xed545578, 0x08fca5b5, 0xd83d7cd3, 0x4dad0fc4,
	0x1e50ef5e, 0xb161e6f8, 0xa28514d9, 0x6c51133c,
	0x6fd5c7e7, 0x56e14ec4, 0x362abfce, 0xddc6c837,
	0xd79a3234, 0x92638212, 0x670efa8e, 0x406000e0,
	0x3a39ce37, 0xd3faf5cf, 0xabc27737, 0x5ac52d1b,
	0x5cb0679e, 0x4fa33742, 0xd3822740, 0x99bc9bbe,
	0xd5118e9d, 0xbf0f7315, 0xd62d1c7e, 0xc700c47b,
	0xb78c1b6b, 0x21a19045, 0xb26eb1be, 0x6a366eb4,
	0x5748ab2f, 0xbc946e79, 0xc6a376d2, 0x6549c2c8,
	0x530ff8ee, 0x468dde7d, 0xd5730a1d, 0x4cd04dc6,
	0x2939bbdb, 0xa9ba4650, 0xac9526e8, 0xbe5ee304,
	0xa1fad5f0, 0x6a2d519a, 0x63ef8ce2, 0x9a86ee22,
	0xc089c2b8, 0x43242ef6, 0xa51e03aa, 0x9cf2d0a4,
	0x83c061ba, 0x9be96a4d, 0x8fe51550, 0xba645bd6,
	0x2826a2f9, 0xa73a3ae1, 0x4ba99586, 0xef5562e9,
	0xc72fefd3, 0xf752f7da, 0x3f046f69, 0x77fa0a59,
	0x80e4a915, 0x87b08601, 0x9b09e6ad, 0x3b3ee593,
	0xe990fd5a, 0x9e34d797, 0x2cf0b7d9, 0x022b8b51,
	0x96d5ac3a, 0x017da67d, 0xd1cf3ed6, 0x7c7d2d28,
	0x1f9f25cf, 0xadf2b89b, 0x5ad6b472, 0x5a88f54c,
	0xe029ac71, 0xe019a5e6, 0x47b0acfd, 0xed93fa9b,
	0xe8d3c48d, 0x283b57cc, 0xf8d56629, 0x79132e28,
	0x785f0191, 0xed756055, 0xf7960e44, 0xe3d35e8c,
	0x15056dd4, 0x88f46dba, 0x03a16125, 0x0564f0bd,
	0xc3eb9e15, 0x3c9057a2, 0x97271aec, 0xa93a072a,
	0x1b3f6d9b, 0x1e6321f5, 0xf59c66fb, 0x26dcf319,
	0x7533d928, 0xb155fdf5, 0x03563482, 0x8aba3cbb,
	0x28517711, 0xc20ad9f8, 0xabcc5167, 0xccad925f,
	0x4de81751, 0x3830dc8e, 0x379d5862, 0x9320f991,
	0xea7a90c2, 0xfb3e7bce, 0x5121ce64, 0x774fbe32,
	0xa8b6e37e, 0xc3293d46, 0x48de5369, 0x6413e680,
	0xa2ae0810, 0xdd6db224, 0x69852dfd, 0x09072166,
	0xb39a460a, 0x6445c0dd, 0x586cdecf, 0x1c20c8ae,
	0x5bbef7dd, 0x1b588d40, 0xccd2017f, 0x6bb4e3bb,
	0xdda26a7e, 0x3a59ff45, 0x3e350a44, 0xbcb4cdd5,
	0x72eacea8, 0xfa6484bb, 0x8d6612ae, 0xbf3c6f47,
	0xd29be463, 0x542f5d9e, 0xaec2771b, 0xf64e6370,
	0x740e0d8d, 0xe75b1357, 0xf8721671, 0xaf537d5d,
	0x4040cb08, 0x4eb4e2cc, 0x34d2466a, 0x0115af84,
	0xe1b00428, 0x95983a1d, 0x06b89fb4, 0xce6ea048,
	0x6f3f3b82, 0x3520ab82, 0x011a1d4b, 0x277227f8,
	0x611560b1, 0xe7933fdc, 0xbb3a792b, 0x344525bd,
	0xa08839e1, 0x51ce794b, 0x2f32c9b7, 0xa01fbac9,
	0xe01cc87e, 0xbcc7d1f6, 0xcf0111c3, 0xa1e8aac7,
	0x1a908749, 0xd44fbd9a, 0xd0dadecb, 0xd50ada38,
	0x0339c32a, 0xc6913667, 0x8df9317c, 0xe0b12b4f,
	0xf79e59b7, 0x43f5bb3a, 0xf2d519ff, 0x27d9459c,
	0xbf97222c, 0x15e6fc2a, 0x0f91fc71, 0x9b941525,
	0xfae59361, 0xceb69ceb, 0xc2a86459, 0x12baa8d1,
	0xb6c1075e, 0xe3056a0c, 0x10d25065, 0xcb03a442,
	0xe0ec6e0e, 0x1698db3b, 0x4c98a0be, 0x3278e964,
	0x9f1f9532, 0xe0d392df, 0xd3a0342b, 0x8971f21e,
	0x1b0a7441, 0x4ba3348c, 0xc5be7120, 0xc37632d8,
	0xdf359f8d, 0x9b992f2e, 0xe60b6f47, 0x0fe3f11d,
	0xe54cda54, 0x1edad891, 0xce6279cf, 0xcd3e7e6f,
	0x1618b166, 0xfd2c1d05, 0x848fd2c5, 0xf6fb2299,
	0xf523f357, 0xa6327623, 0x93a83531, 0x56cccd02,
	0xacf08162, 0x5a75ebb5, 0x6e163697, 0x88d273cc,
	0xde966292, 0x81b949d0, 0x4c50901b, 0x71c65614,
	0xe6c6c7bd, 0x327a140a, 0x45e1d006, 0xc3f27b9a,
	0xc9aa53fd, 0x62a80f00, 0xbb25bfe2, 0x35bdd2f6,
	0x71126905, 0xb2040222, 0xb6cbcf7c, 0xcd769c2b,
	0x53113ec0, 0x1640e3d3, 0x38abbd60, 0x2547adf0,
	0xba38209c, 0xf746ce76, 0x77afa1c5, 0x20756060,
	0x85cbfe4e, 0x8ae88dd8, 0x7aaaf9b0, 0x4cf9aa7e,
	0x1948c25c, 0x02fb8a8c, 0x01c36ae4, 0xd6ebe1f9,
	0x90d4f869, 0xa65cdea0, 0x3f09252d, 0xc208e69f,
	0xb74e6132, 0xce77e25b, 0x578fdfe3, 0x3ac372e6,
};

void set_key(const char *pp, size_t len)
{
	const unsigned char *key = (const unsigned char *)pp;
	int ii, jj;                         /* loop variables */
	unsigned long left, right;

	for (ii = 0; ii < size_ + 2; ++ii)
		P_[ii] = def_P_[ii];

	for (ii = 0; ii < 4 ; ++ii)
		for (jj = 0; jj < 256 ; ++jj)
			S_[ii][jj] = def_S_[ii][jj];

	for (ii = 0, jj = 0; ii < size_ + 2; ++ii, jj = (jj+4)%len)
		P_[ii] = P_[ii] ^ 
			((key[jj]<<24) | (key[(jj+1)%len]<<16) |
			 (key[(jj+2)%len]<<8) | key[(jj+3)%len]);

	left = right = 0;
	for (ii = 0; ii < size_ + 2; ii += 2)
	{
		encipher(&left, &right);

		P_[ii] = left;
		P_[ii+1] = right;
	}

	for (ii = 0; ii < 4 ; ++ii)
		for (jj = 0; jj < 256; jj += 2)
		{
			encipher(&left, &right);

			S_[ii][jj] = left;
			S_[ii][jj+1] = right;
		}
}

void encrypt(void *buffer, size_t len)
{
	unsigned char *pp, *end;

	if (len % 8 != 0)
	{
		ASSERT(0);
		len = len - len%8;
	}

	for (pp = (unsigned char *)buffer, end = pp+len; pp < end; pp += 8)
		encipher((unsigned long *)pp, (unsigned long *)(pp+4));
}

static void encipher(unsigned long *pl, unsigned long *pr)
{
	unsigned long left = *pl;
	unsigned long right = *pr;

	left ^= P_[0];
	RR(right, left, 1);  RR(left, right, 2);
	RR(right, left, 3);  RR(left, right, 4);
	RR(right, left, 5);  RR(left, right, 6);
	RR(right, left, 7);  RR(left, right, 8);
	RR(right, left, 9);  RR(left, right, 10);
	RR(right, left, 11); RR(left, right, 12);
	RR(right, left, 13); RR(left, right, 14);
	RR(right, left, 15); RR(left, right, 16);
	right ^= P_[17];

	*pl = right;
	*pr = left;
}

void decrypt(void *buffer, size_t len)
{
	unsigned char *pp, *end;

	if (len % 8 != 0)
	{
		ASSERT(0);
		len = len - len%8;
	}

	for (pp = (unsigned char *)buffer, end = pp+len; pp < end; pp += 8)
		decipher((unsigned long *)pp, (unsigned long *)(pp+4));
}

static void decipher(unsigned long *pl, unsigned long *pr)
{
	unsigned long left = *pl;
	unsigned long right = *pr;

	left ^= P_[17];
	RR(right, left, 16);  RR(left, right, 15);
	RR(right, left, 14);  RR(left, right, 13);
	RR(right, left, 12);  RR(left, right, 11);
	RR(right, left, 10);  RR(left, right, 9);
	RR(right, left, 8);   RR(left, right, 7);
	RR(right, left, 6);   RR(left, right, 5);
	RR(right, left, 4);   RR(left, right, 3);
	RR(right, left, 2);   RR(left, right, 1);
	right ^= P_[0];

	*pl = right;
	*pr = left;
}
