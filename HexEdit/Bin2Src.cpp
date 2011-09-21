// Bin2Src.cpp: implementation of the Bin2Src class.
//
// This class is used to convert binary data into various text formats.
// It was originally designed to convert to C/C++ source code (strings or
// arrays of chars, ints or floats etc) but is now more general.
// In the future it might also support other programming languages.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Bin2Src.h"
#include "HexEditDoc.h"
#include "Misc.h"

// Set() sets conversion parameters
// l = what programming language we might be supporting (currently only none or C)
// t = the type of data, such as string, int or floating point
// s = the size of each unit of data in bytes
// b = radix for int types
// r = how much to put on each line of text (note that since r may not be divisible by s that the amount of data may vary from row to row)
// o = offset - amount the start of each row is offset from zero
// i = number of spaces to indent each row
// f = combination of bit flags with various options
void Bin2Src::Set(enum lang l, enum type t, int s, int b, int r, int o, int i, int f)
{
	lang_ = l;                             // Language: only C for now
	type_ = t;                             // type of data to format as
	size_ = s;                             // size of each data unit
	ASSERT(size_ == 1 || size_ == 2 || size_ == 4 || size_ == 6 || size_ == 8);
	if (type_ == STRING || type_ == CHAR)
		size_ = 1;
	radix_ = b;                            // base for ints
	rowsize_ = r;                          // how many input bytes per line of output text
	offset_ = o;                           // starting offset
	indent_ = i;                           // how many spaces to indent each line
	big_endian_ = (f & BIG_ENDIAN) != 0;   // only need for int and float data (maybe Unicode string/char later)
	unsigned_ = (f & UNSIGNED) != 0;       // only use for ints
	align_cols_ = (f & ALIGN) != 0;        // align columns of numbers
	hex_address_ = (f & HEX_ADDRESS) != 0; // display hex address?
	dec_address_ = (f & DEC_ADDRESS) != 0; // display dec address?
	line_num_ = (f & LINE_NUM) != 0;       // display line numbers?
	addr1_ = (f & ADDR1) != 0;             // addresses and line numbers start at one not zero?
};

// Given the address range of the binary bytes and the current settings (as supplied to
// Set() above) it returns the worst case number of bytes required to do the conversion.
// This value can be used to allocate a buffer to be passed to GetString() below.
int Bin2Src::GetLength(__int64 start, __int64 end) const
{
	__int64 blen = end - start;
	int addr_width = get_address_width(end);
	int unit_width = get_unit_width();     // get width required for each data unit (includes padding such as commas and spaces)

	addr_width += 2;                       // allow for line end (CR-LF)
	if (type_ == STRING)
		addr_width += 2;                   // allow for 2 string delimiters (") per line

	return int( (blen + 1)*unit_width/size_ +                 // the size of all the formatted data
		        (blen/rowsize_ + 2)*(indent_ + addr_width) +  // overhead for each line
		        2);                                           // add 2 for null byte and error check byte (0xCD)
}

// GetString performs the actual conversion
// start, end = the range of addresses to be used from the document (passed to c'tor)
// buf = where the output text is placed
// buf_len = conversion will stop before the end of the buffer is reached
size_t Bin2Src::GetString(__int64 start, __int64 end, char *buf, int buf_len) const
{
	buf[buf_len - 1] = '\xCD';          // Add marker char at end of buffer so we can check if buffer was overflowed

	const char *spaces = "                                                                "
						 "                                                                ";
	const char *hex;                    // string containing hex digits (in current selected case)
	if (theApp.hex_ucase_)
		hex = "0123456789ABCDEF?";
	else
		hex = "0123456789abcdef?";

	size_t slen;                        // Number of characters added to output buffer by sprintf call
	char *pp = buf;                     // Current place in the buffer where we are up to
	int addr_width = get_address_width(end);  // Width required for address (in comments) - may be 
	int unit_width = get_unit_width();  // get width required for each data unit (includes padding such as commas and spaces)

	// Work out when to stop - if there is not enough room in the buffer for another row
	// The worst case is that we just started a new line so as well as one data element (unit_width) we
	// need to allow for a new line = indent_ + addr_width + 2 (CR/LF) + 2 (double quotes for string)
	char *pp_end = buf + buf_len - (unit_width + indent_ + addr_width + 5);
	ASSERT(pp_end > pp);

	// Add initial indent and address(es)
	ASSERT(indent_ < 128 && buf_len > indent_ + addr_width + 2);
	slen = sprintf(pp, "%.*s", indent_, spaces);
	pp += slen;

	const char *paddr = get_address(start);
	slen = strlen(paddr);
	ASSERT(slen == addr_width);
	strcpy(pp, paddr);
	pp += slen;

	if (lang_ == C && type_ == STRING)
		*pp++ = '"';

	__int64 curr;                      // Address of current byte being worked on
	__int64 line_end;                  // Address of byte at end of current line
	line_end = ((start + offset_)/rowsize_ + 1)*rowsize_ - offset_;

	// Work out how many bytes in each chunk
	for (curr = start; curr + size_ <= end && pp < pp_end; )
	{
		unsigned char inbuf[8];               // Largest data type input is currently 64 bit int/float (8 bytes)

		VERIFY(pdoc_->GetData(inbuf, size_, curr) == size_);
		switch (type_)
		{
		default:
			ASSERT(0);
			// fall through

		case STRING:
			if (inbuf[0] >= ' ' && inbuf[0] < 127)
			{
				// Backslash (\) and double-quote (") must be escaped and also do question
				// mark (?) to avoid accidentally creating a trigraph sequence (??=, etc)
				if (lang_ == C && strchr("\\\"\?", inbuf[0]))
					*pp++ = '\\';
				*pp++ = inbuf[0];
			}
			else if (lang_ == C)
			{
				// Control char or non-ASCII char - display as escape char or in hex
				const char *check = "\a\b\f\n\r\t\v";       // used in search for escape char
				const char *display = "abfnrtv0";
				const char *ps;

				// Note we output a nul byte as hex just in case it is followed by another
				// digit.  Since strchr includes the terminating nul byte in the search
				// we have to explicitly check for it.
				if (inbuf[0] != '\0' && (ps = strchr(check, inbuf[0])) != NULL)
				{
					// Ouput C/C++ escape sequence
					*pp++ = '\\';
					*pp++ = display[ps-check];
				}
				else
				{
					// Output using hex escape sequence
					*pp++ = '\\';
					*pp++ = 'x';
					*pp++ = hex[(inbuf[0]>>4)&0xF];
					*pp++ = hex[inbuf[0]&0xF];
					ASSERT(size_ == 1);

					// If not at end of line we have to watch that the following char is not a hex digit
					if (curr + size_ < line_end && curr + size_ < end)
					{
						// Not at EOL so get the next character into inbuf[1] (we get it again later into inbuf[0]
						VERIFY(pdoc_->GetData(inbuf+1, 1, curr + size_) == 1);
						if (isxdigit(inbuf[1]))
						{
							// Terminate the string and start a new one so that the following char
							// does not become concatenated with the 2 hex digits already output
							*pp++ = '"';
							*pp++ = ' ';
							*pp++ = '"';
						}
					}
				}
			}
			break;

		case CHAR:
			if (lang_ == C)
			{
				// Always output something for C even if just a hex escape seq.
				*pp++ = '\'';                                   // put in single quotes
				if (inbuf[0] >= ' ' && inbuf[0] < 127)
				{
					// Backslash (\) and apostrophe or single quote (') must be escaped
					if (strchr("\\'", inbuf[0]))
						*pp++ = '\\';
					*pp++ = inbuf[0];
				}
				else
				{
					// Control char or non-ASCII char - display as escape char or in hex
					const char *check = "\a\b\f\n\r\t\v\0";
					const char *display = "abfnrtv0";
					const char *ps;
					if ((ps = strchr(check, inbuf[0])) != NULL)
					{
						// Ouput C/C++ escape sequence
						*pp++ = '\\';
						*pp++ = display[ps-check];
					}
					else
					{
						// Output using hex escape sequence
						*pp++ = '\\';
						*pp++ = 'x';
						*pp++ = hex[(inbuf[0]>>4)&0xF];
						*pp++ = hex[inbuf[0]&0xF];
					}
				}
				*pp++ = '\'';                                   // trailing single quote
				*pp++ = ',';
				*pp++ = ' ';
			}
			else if (inbuf[0] >= ' ' && inbuf[0] < 127)
			{
				// Just output printable characters
				*pp++ = inbuf[0];
				*pp++ = ',';
				*pp++ = ' ';
			}
			break;

		case INT:
			// Note we only support radix of hex, decimal and octal for now
			// TODO: use mpz_get_str() to support any radix up to 36
			if (big_endian_)
				flip_bytes(inbuf, size_);

			{
				__int64 val = -1;
				switch (size_)
				{
				case 1:
					if (unsigned_)
						val = inbuf[0];
					else
						val = (signed char)inbuf[0];
					break;
				case 2:
					if (unsigned_)
						val = *(unsigned short *)inbuf;
					else
						val = *(short *)inbuf;
					break;
				case 4:
					if (unsigned_)
						val = *(unsigned long *)inbuf;
					else
						val = *(long *)inbuf;
					break;
				case 8:
					val = *(__int64 *)inbuf;
					break;
				default:
					ASSERT(0);
					// fall through
				}

				if (!align_cols_)
					unit_width = 4;

				if (radix_ == 8 && lang_ == C)
				{
					// Octal with leading zero
					slen = sprintf(pp, "0%0*I64o, ", unit_width-3, val);
				}
				else if (radix_ == 8)
				{
					// Straight octal
					slen = sprintf(pp, "%0*I64o, ", unit_width-2, val);
				}
				else if (radix_ == 16 && lang_ == C)
				{
					if (!align_cols_)
						unit_width = 6;     // gives min 2 hex digits (6 - 4)
					// Hex with leading 0x
					if (theApp.hex_ucase_)
						slen = sprintf(pp, "0x%0*I64X, ", unit_width-4, val);
					else
						slen = sprintf(pp, "0x%0*I64x, ", unit_width-4, val);
				}
				else if (radix_ == 16)
				{
					// Straight hex padded with leading zeroes
					if (theApp.hex_ucase_)
						slen = sprintf(pp, "%0*I64X, ", unit_width-2, val);
					else
						slen = sprintf(pp, "%0*I64x, ", unit_width-2, val);
				}
				else if (radix_ == 10 && unsigned_)
				{
					// Unsigned decimal
					slen = sprintf(pp, "%*I64u, ", unit_width-2, val);
				}
				else
				{
					slen = sprintf(pp, "%*I64d, ", unit_width-2, val);
				}
			}

			pp += slen;
			ASSERT(*(pp-1) == ' ');  // Checks that slen was correct and trailing space added
			break;

		case FLOAT:
			if (big_endian_)
				flip_bytes(inbuf, size_);

			double val = 0.0;
			switch (size_)
			{
			case 8:
				val = *(double *)inbuf;
				break;
			case 4:
				val = *(float *)inbuf;
				break;
			case 6:
				val = real48(inbuf);
				break;
			default:
				ASSERT(0);
				// fall through
			}
			slen = sprintf(pp, "%*.*g, ", align_cols_ ? unit_width-2 : 1, unit_width-9, val);

			pp += slen;
			ASSERT(*(pp-1) == ' ');
			break;
		}

		// Check if we need to start a new line
		if ((curr += size_) >= line_end || curr >= end)
		{
			// Terminate previous line
			if (lang_ == C && type_ == STRING)
				*pp++ = '"';                // terminate string on this line
			*pp++ = '\r';
			*pp++ = '\n';

			// If this is not the last line
			if (curr < end)
			{
				// Add any indenting
				slen = sprintf(pp, "%.*s", indent_, spaces);
				pp += slen;

				paddr = get_address(curr);
				slen = strlen(paddr);
				ASSERT(slen == addr_width);
				strcpy(pp, paddr);
				pp += slen;

				if (lang_ == C && type_ == STRING)
					*pp++ = '"';            // start new string on this new line
			}

			// Work out where this next line ends
			line_end += rowsize_;
		}
	}
	ASSERT(buf[buf_len - 1] == '\xCD');   // Check if buffer was overflowed
	*pp ='\0';
	return pp - buf;
}

int Bin2Src::get_unit_width() const
{
	int extra;
	switch (type_)
	{
	case STRING:
		return 4;   // worst case \xFF  [backslash, x, 2 hex digits]
		
	case CHAR:
		return 8;  // worst case '\xFF', [includes 2 apostrophes, comma, space

	case INT:
		extra = (lang_ == C && radix_ == 8  ? 1 : 0) +    // allow for leading 0
			    (lang_ == C && radix_ == 16 ? 2 : 0) +    // allow for leading 0x
			    (unsigned_                  ? 0 : 1) +    // allow for leading sign
				2;                                        // allow for trailing comma and space
		switch (size_)
		{
		case 1:
			return SigDigits(0xFF, radix_) + extra;
		case 2:
			return SigDigits(0xFFFF, radix_) + extra;
		case 4:
			return SigDigits(0xffffFFFF, radix_) + extra;
		case 8:
			return SigDigits(0xffffFFFFffffFFFFUi64, radix_) + extra;
		}
		break;

	case FLOAT:
		switch (size_)
		{
		case 4:
			return 16;
		case 6:
			return 21;
		case 8:
			return 24;
		}
		break;
	}
	
	ASSERT(0);      // we should not get here but if we use worst case of 8 output characters per byte
	return size_*8;
}

int Bin2Src::get_address_width(__int64 addr) const
{
	hex_width_ = hex_address_ ? SigDigits(addr, 16) + 1 : 0;
	dec_width_ = dec_address_ ? SigDigits(addr)     + 1 : 0;
	num_width_ = line_num_    ? SigDigits((addr + offset_)/rowsize_) + 1 : 0;

	int retval = hex_width_ + dec_width_ + num_width_;
	if (retval > 0)
	{
		switch (lang_)
		{
		default:
			ASSERT(0);
			// fall through
		case NOLANG:
			retval += 1;  // just a space
			break;
		case C:
			retval += 5;  // 5 bytes for [/**/ ]
			break;
		}
	}

	return retval;
}

// Create an address as text to be put at the start of the line of text if one or
// more of the flags HEX_ADDRESS, DEC_ADDRESS, or LINE_NUM are specified.
// NOTE: Returns a pointer to a static buffer so don't call more than once in the
// same expression and don't store the returned pointer.
const char * Bin2Src::get_address(__int64 addr) const
{
	if (hex_width_ + dec_width_ + num_width_ == 0)
		return "";

	static char buf[80];
	char *pp = buf;
	size_t slen;

	switch (lang_)
	{
	case NOLANG:
		break; // nothing required
	case C:
		*pp++ = '/';
		*pp++ = '*';
		break;
	default:
		ASSERT(0);
		return "";
	}
	if (hex_address_)
	{
		slen = sprintf(pp, theApp.hex_ucase_ ? "%0*I64X:" : "%0*I64x:", hex_width_-1, addr + (addr1_ ? 1 : 0));
		pp += slen;
	}
	if (dec_address_)
	{
		slen = sprintf(pp, "%*I64d:", dec_width_-1, addr + (addr1_ ? 1 : 0));
		pp += slen;
	}
	if (line_num_)
	{
		slen = sprintf(pp, "%*I64d:", num_width_-1, (addr + offset_)/rowsize_ + (addr1_ ? 1 : 0));
		pp += slen;
	}

	switch (lang_)
	{
	case NOLANG:
		break; // nothing required
	case C:
		*pp++ = '*';
		*pp++ = '/';
		break;
	}
	*pp++ = ' ';
	*pp++ = '\0';
	return buf;
}
