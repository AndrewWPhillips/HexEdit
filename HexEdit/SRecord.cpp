// srecord.cpp - implements CReadSRecord and CWriteSRecord classes that
// handle files containing Motorola S-Records
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
#include "srecord.h"

// Sizes of address fields for S-Records (S0 to S9)
static size_t addr_size[10] = { 2, 2, 3, 4, -1, 2, -1, 4, 3, 2 };

CWriteSRecord::CWriteSRecord(const char *filename,
							 unsigned long base_addr /*= 0L*/,
							 int stype /*= 3*/,
							 size_t reclen /*= 32*/)
{
	stype_ = stype;                         // Keep type of recs to output
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

	// Write S0 record
	put_rec(0, 0L, "HDR", 3);
}


CWriteSRecord::~CWriteSRecord()
{
	put_rec(5, recs_out_, "", 0);       // For S5 the number of records read is stored in address field
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

void CWriteSRecord::Put(const void *data, size_t len, unsigned long address /* = UINT_MAX */)
{
	if (address == UINT_MAX)
		address = addr_;
	addr_ = address + len;   // Save next address after these record(s)

	char *pp = (char *)data;

	while (len > 0)
	{
		if (len > reclen_)
		{
			put_rec(stype_, address, pp, reclen_);
			address += reclen_;
			pp += reclen_;
			len -= reclen_;
		}
		else
		{
			put_rec(stype_, address, pp, size_t(len));
			len = 0;
		}
	}
}

// Output an S-Record, where stype is the S record type (0 to 9), addr is the 16/24/32-bit address
// data points to the data bytes and len is the number of data bytes.  It calculates the address
// size based on the record type and also works out and stores the byte count and checksum.
void CWriteSRecord::put_rec(int stype, unsigned long addr, void *data, size_t len)
{
	ASSERT(stype >= 0 && stype <= 9);

	char buffer[520];
	int checksum;
	int byte_count = addr_size[stype] + len + 1;    // size of address, data, checksum
	char *pp = buffer;

	*pp++ = 'S';
	*pp++ = stype + '0';
	checksum = put_hex(pp, byte_count, 1);
	pp += 2;
	checksum += put_hex(pp, addr, addr_size[stype]);
	pp += addr_size[stype]*2;

	for (size_t ii = 0; ii < len; ++ii)
	{
		checksum += put_hex(pp, *((char*)data+ii), 1);
		pp += 2;
	}
	put_hex(pp, 255-(checksum&0xFF), 1);
	pp += 2;
	*pp++ = '\n';
	*pp = '\0';

	if (stype >= 1 && stype <= 3)
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
int CWriteSRecord::put_hex(char *pstart, unsigned long val, int bytes)
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

CReadSRecord::CReadSRecord(const char *filename, BOOL allow_discon /*= FALSE*/)
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

#if 0  // Some idiots do not put out an S0 record
	// Get S0 record
	int stype;
	size_t len;
	unsigned long address;

	while ((stype = get_rec(NULL, 0, len, address)) != 0)
	{
		if (!error_.IsEmpty())
			break;
		else if (stype == -1)
		{
			error_ = "No S0 record found";
			break;
		}
	}
#endif
}

// Returns the length of the data read or zero on error or EOF
// data is a pointer to where the bytes should be stored
// max is the size of the buffer - no more than that many bytes are returned
// address is the specified address from the import file
size_t CReadSRecord::Get(void *data, size_t max_len, unsigned long &address)
{
	int stype;                          // Record type read, 0, 1, 5 etc
	size_t len;
//    unsigned long address;

	// Get next S1, S2 or S3 record
	while ((stype = get_rec(data, max_len, len, address)) != 1 && stype != 2 && stype != 3 &&
			stype != 5 && stype != -1)
	{
		if (!error_.IsEmpty())
			return 0;
	}

	if (allow_discon_ && stype >= 1 && stype <= 3)
	{
		ASSERT(addr_ == -1);
		return len;
	}
	else if (stype >= 1 && stype <= 3)
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
	else if (stype == 5)
	{
		// Check that number of records read matches S5 record
	   if (recs_in_ == 0)
			error_ = "No S1/S2/S3 records founds";
	   else if (address != (recs_in_&0xFFFF))
			error_.Format("ERROR: Mismatch in number of records (S5 record) at line %ld", long(line_no_));
	}
	else if (error_.IsEmpty())
		error_ = "WARNING: No S5 record found";

	return 0;
}

// Reads a Motorola S record and checks it for validity (including checksum).
// The type of record found (0, 1, 5, etc) is returned, or -1 on EOF or on
// error (in which case error_ is set), or 99 for an unrecognised record type.
// For S1/S2/S3 records the data with the record is returned in 'data'.
// If there are more than max_len data bytes then an error is returned.

int CReadSRecord::get_rec(void *data, size_t max_len, size_t &len, unsigned long &address)
{
	char buffer[520];                   // Where read string is stored
	char *pp;                           // Ptr into read string
	size_t slen;                        // Length of read string
	int stype;                          // Type of S Record, 0, 1, 2, 3, 5, etc
	int byte_count;                     // How many bytes of data (incl addr and checksum)
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
	if (slen < 8 || pp[0] != 'S') return 99;

	// Get S record type and check it's valid
	stype = *++pp - '0';
	if (stype < 0 || stype > 9 || addr_size[stype] == -1) return 99;
	++pp;

	// Get length of data and the address
	byte_count = get_hex(pp, 1, checksum);
	len = byte_count - addr_size[stype] - 1;     // Number of actual data bytes
	if (len > max_len)
	{
		error_.Format("ERROR: Record too long at line %ld", long(line_no_));
		return -1;
	}

	// Make sure we have the indicated number of bytes
	if (byte_count*2 + 4 > (int)slen)
	{
		error_.Format("ERROR: Short S record at line %ld", long(line_no_));
		return -1;
	}
	pp += 2;
	address = get_hex(pp, addr_size[stype], checksum);
	pp += addr_size[stype]*2;

	// If S1, S2, S3 convert the hex digits to binary data
	if (stype < 5)
	{
		for (size_t ii = 0; ii < len; ++ii, pp+=2)
		{
			if (data != NULL)
				*((char *)data + ii) = (char)get_hex(pp, 1, checksum);
			else
				(void)get_hex(pp, 1, checksum);
		}
	}

	if (stype == 1 || stype == 2 || stype == 3)
		++recs_in_;                     // Keep track of number of S1/S2/S3 records read

	// Verify checksum
	int dummy = 0;
	if (get_hex(pp, 1, dummy) != 255-(checksum&0xFF))
	{
		error_.Format("ERROR: Checksum mismatch at line %ld", long(line_no_));
		return -1;
	}

	return stype;
}

// Output from 1 to 4 bytes converting into (from 2 to 8) hex digits
// Also calculates the checksum of digits output and returns it.
// Note that 'bytes' is the number of data bytes 1 byte=2 hex digits
unsigned long CReadSRecord::get_hex(char *pstart, int bytes, int &checksum)
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

