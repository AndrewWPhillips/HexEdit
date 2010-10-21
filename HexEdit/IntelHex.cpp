// IntelHex.cpp - implements CReadIntelHex and CWriteIntelHex classes that
// can be used to read and write Intel hex format files.
//
// Copyright (c) 2003-2010 by Andrew W. Phillips.
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

#include "stdafx.h"
#include "hexedit.h"
#include "misc.h"
#include "IntelHex.h"

CWriteIntelHex::CWriteIntelHex(const char *filename,
							 unsigned long base_addr /*= 0L*/,
							 size_t reclen /*= 32*/)
{
	addr_ = base_addr;                      // Init addr_ to base address
	reclen_ = reclen;
	recs_out_ = 0;                          // Init output record count

	CFileException fe;                      // Stores file exception info

	// Open the file
	if (!file_.Open(filename,
		CFile::modeCreate|CFile::modeWrite|CFile::shareExclusive|CFile::typeText,
					  &fe))
	{
		error_ = ::FileErrorMessage(&fe, CFile::modeWrite);
		return;
	}
}

CWriteIntelHex::~CWriteIntelHex()
{
	put_rec(1, 0, "", 0);       // Ouput the EOF (01) record
	try
	{
		file_.Close();
	}
	catch (CFileException *pfe)
	{
		error_ = ::FileErrorMessage(pfe, CFile::modeWrite);
		pfe->Delete();
	}
}

void CWriteIntelHex::Put(const void *data, size_t len)
{
	char *pp = (char *)data;

	while (len > 0)
	{
		if (len > reclen_)
		{
			put_rec(0, addr_, pp, reclen_);
			addr_ += reclen_;
			pp += reclen_;
			len -= reclen_;
		}
		else
		{
			put_rec(0, addr_, pp, size_t(len));
			addr_ += len;
			len = 0;
		}
	}
}

// Output a record, where stype is 0 for data or 1 for EOF record, addr is the 16-bit address
// data points to the data bytes and len is the number of data bytes.
// It works out and stores the byte count and checksum.
void CWriteIntelHex::put_rec(int stype, unsigned long addr, void *data, size_t len)
{
	ASSERT(stype == 0 || stype == 1);  // We only know how to do data and EOF records
	ASSERT(len < 256);

	char buffer[520];
	int checksum = 0;
	char *pp = buffer;

	*pp++ = ':';
	checksum += put_hex(pp, len, 1);
	pp += 2;
	checksum += put_hex(pp, addr, 2);
	pp += 4;
	checksum += put_hex(pp, stype, 1);
	pp += 2;

	for (size_t ii = 0; ii < len; ++ii)
	{
		checksum += put_hex(pp, *((char*)data+ii), 1);
		pp += 2;
	}
	put_hex(pp, (256-checksum)&0xFF, 1);
	pp += 2;
	*pp++ = '\n';
	*pp = '\0';

	if (stype == 0)
		++recs_out_;

	try
	{
		file_.WriteString(buffer);
	}
	catch (CFileException *pfe)
	{
		error_ = ::FileErrorMessage(pfe, CFile::modeWrite);
		pfe->Delete();
	}
}

// Output from 1 to 4 bytes converting into (from 2 to 8) hex digits
// Also calculates the checksum of digits output and returns it.
// Note that 'bytes' is the number of data bytes 1 byte=2 hex digits
int CWriteIntelHex::put_hex(char *pstart, unsigned long val, int bytes)
{
	static char hex_digit[18] = "0123456789ABCDEF?";

	unsigned long tt = val;

	for (char *pp = pstart + bytes*2; pp > pstart; pp--)
	{
		*(pp-1) = hex_digit[tt&0x0F];
		tt >>= 4;
	}

	// Work out sum of bytes
	int retval = 0;
	tt = val;
	while (bytes--)
	{
		retval += tt&0xFF;
		tt >>= 8;
	}
	return retval;
}

////////////////////////////////////////////////////////////////////////////////

CReadIntelHex::CReadIntelHex(const char *filename, BOOL allow_discon /*= FALSE*/)
{
	addr_ = -1;                         // Signal that we haven't actually read anything yet
	line_no_ = 0;
	recs_in_ = 0;
	allow_discon_ = allow_discon;
	CFileException fe;                      // Stores file exception info

	// Open the file
	if (!file_.Open(filename,
		CFile::modeRead|CFile::shareDenyWrite|CFile::typeText,
					  &fe))
	{
		error_ = ::FileErrorMessage(&fe, CFile::modeRead);
		return;
	}
}

// Returns the length of the data read or zero on error or EOF
size_t CReadIntelHex::Get(void *data, size_t max_len, unsigned long &address)
{
	int stype;
	size_t len;
//    unsigned long address;

	// Get next data (00) record
	while ((stype = get_rec(data, max_len, len, address)) != 0 && stype != 1 && stype != -1)
	{
		if (!error_.IsEmpty())
			return 0;
	}

	if (allow_discon_ && stype == 0)
	{
		ASSERT(addr_ == -1);
		return len;
	}
	else if (stype == 0)
	{
		// Check that address is as expected
		if (addr_ == -1)
		{
			ASSERT(recs_in_ == 1);
			addr_ = address;
		}
		else if (addr_ != address)
		{
			error_.Format("ERROR: Non-adjoining address at line %ld", long(line_no_));
			return 0;
		}
		addr_ += len;
		return len;
	}
	else if (stype == -1 && error_.IsEmpty())
		error_ = "WARNING: No Intel hex EOF record found";

	return 0;
}

// Reads an Intel hex record and checks it for validity (including checksum).
// The type of record found (0 to 5) is returned, or -1 on EOF or on error
// (in which case error_ is also set), or 99 for an unrecognised record type.
// For data (00) records the data with the record is returned in 'data'.
// If there are more than max_len data bytes then an error is returned.

int CReadIntelHex::get_rec(void *data, size_t max_len, size_t &len, unsigned long &address)
{
	char buffer[520];                   // Where read string is stored
	char *pp;                           // Ptr into read string
	size_t slen;                        // Length of read string
	int stype;                          // Type of Intel hex record (0-5)
	int checksum = 0;                   // Checksum of data, length, and address

	try
	{
		pp = file_.ReadString(buffer, sizeof(buffer)-1);
		++line_no_;
	}
	catch (CFileException *pfe)
	{
		error_ = ::FileErrorMessage(pfe, CFile::modeRead);
		pfe->Delete();
		return -1;
	}

	// Simple validity check
	if (pp == NULL) return -1;
	slen = strlen(pp);
	if (slen < 10 || pp[0] != ':') return 99;
	++pp;

	// Get the data length and make sure we have enough bytes
	len = get_hex(pp, 1, checksum);
	if (len > max_len)
	{
		error_.Format("ERROR: Record too long at line %ld", long(line_no_));
		return -1;
	}
	pp += 2;
	if ((len + 5)*2 + 1 > slen)
	{
		error_.Format("ERROR: Short record at line %ld", long(line_no_));
		return -1;
	}

	// Get address and record type
	address = get_hex(pp, 2, checksum);
	pp += 4;
	stype = get_hex(pp, 1, checksum);
	pp += 2;

	if (stype != 0 && stype != 1) return 99;


	// If data  convert the hex digits to binary data
	if (stype == 0)
	{
		for (size_t ii = 0; ii < len; ++ii, pp+=2)
		{
			if (data != NULL)
				*((char *)data + ii) = (char)get_hex(pp, 1, checksum);
			else
				(void)get_hex(pp, 1, checksum);
		}
		++recs_in_;                     // Keep track of number of data records read
	}

	// Verify checksum
	(void)get_hex(pp, 1, checksum);
	if ((checksum&0xFF) != 0)
	{
		error_.Format("ERROR: Checksum mismatch at line %ld", long(line_no_));
		return -1;
	}

	return stype;
}

// Output from 1 to 4 bytes converting into (from 2 to 8) hex digits
// Also calculates the checksum of digits output and returns it.
// Note that 'bytes' is the number of data bytes 1 byte=2 hex digits
unsigned long CReadIntelHex::get_hex(char *pstart, int bytes, int &checksum)
{
	char buf[16];

	memcpy(buf, pstart, bytes*2);
	buf[bytes*2] = '\0';

	unsigned long retval = strtoul(buf, NULL, 16);

	// Add bytes to checksum
	for (unsigned long ul = retval; bytes > 0; bytes--)
	{
		checksum += (ul & 0xff);
		ul >>= 8;
	}

	return retval;
}
