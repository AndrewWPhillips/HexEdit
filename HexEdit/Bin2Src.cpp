// Bin2Src.cpp: implementation of the Bin2Src class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Bin2Src.h"
#include "HexEditDoc.h"
#include "Misc.h"

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
	hex_address_ = (f & HEX_ADDRESS) != 0; // display hea address in comments?
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
	char *pp_end = buf + buf_len - (indent_ + addr_width + unit_width + 2);
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

	if (type_ == STRING)
		*pp++ = '"';

	__int64 curr;                      // Address of current byte being worked on
	__int64 line_end;                  // Address of byte at end of current line
	line_end = ((start + offset_)/rowsize_ + 1)*rowsize_ - offset_;

	// Work out how many bytes in each chunk
	for (curr = start; curr + size_ <= end && pp < pp_end; )
	{
		unsigned char buf[8];               // Largest is 64 bit int/float (8 bytes)

		VERIFY(pdoc_->GetData(buf, size_, curr) == size_);
		switch (type_)
		{
		default:
			ASSERT(0);
			// fall through

		case STRING:
			if (buf[0] >= ' ' && buf[0] < 127)
			{
				// Backslash (\) and double-quote (") must be escaped and also do question
				// mark (?) to avoid accidentally creating a trigraph sequence (??=, etc)
				if (strchr("\\\"\?", buf[0]))
					*pp++ = '\\';
				*pp++ = buf[0];
			}
			else
			{
				// Control char or non-ASCII char - display as escape char or in hex
				const char *check = "\a\b\f\n\r\t\v";       // used in search for escape char
				const char *display = "abfnrtv0";
				const char *ps;

				// Note we output a nul byte as hex just in case it is followed by another
				// digit.  Since strchr includes the terminating nul byte in the search
				// we have to explicitly check for it.
				if (buf[0] != '\0' && (ps = strchr(check, buf[0])) != NULL)
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
					*pp++ = hex[(buf[0]>>4)&0xF];
					*pp++ = hex[buf[0]&0xF];
					ASSERT(size_ == 1);

					// If not at end of line we have to watch that the following char is not a hex digit
					if (curr + size_ < line_end && curr + size_ < end)
					{
						// Not at EOL so get the next character into buf[1]
						VERIFY(pdoc_->GetData(buf+1, 1, curr + size_) == 1);
						if (isxdigit(buf[1]))
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
			*pp++ = '\'';                                   // put in single quotes
			if (buf[0] >= ' ' && buf[0] < 127)
			{
				// Backslash (\) and apostrophe or single quote (') must be escaped
				if (strchr("\\'", buf[0]))
					*pp++ = '\\';
				*pp++ = buf[0];
			}
			else
			{
				// Control char or non-ASCII char - display as escape char or in hex
				const char *check = "\a\b\f\n\r\t\v\0";
				const char *display = "abfnrtv0";
				const char *ps;
				if ((ps = strchr(check, buf[0])) != NULL)
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
					*pp++ = hex[(buf[0]>>4)&0xF];
					*pp++ = hex[buf[0]&0xF];
				}
			}
			*pp++ = '\'';                                   // trailing single quote
			*pp++ = ',';
			*pp++ = ' ';
			break;

		case INT:
			// Note we only support radix of hex, decimal and octal for now
			// TODO: use mpz_get_str() to support any radix up to 36
			if (big_endian_)
				flip_bytes(buf, size_);
			switch (size_)
			{
			default:
				ASSERT(0);
				// fall through
			case 4:
				if (radix_ == 8)
				{
					// Octal
					slen = sprintf(pp, "0%0*o, ", unit_width-3, *(long *)buf);    // octal with leading zeroes
				}
				else if (radix_ == 16)
				{
					// Hex
					if (theApp.hex_ucase_)
						slen = sprintf(pp, "0x%0*X, ", unit_width-4, *(long *)buf); // 0x then leading zeroes
					else
						slen = sprintf(pp, "0x%0*x, ", unit_width-4, *(long *)buf);
				}
				else if (radix_ == 10 && unsigned_)
				{
					// Unsigned decimal
					slen = sprintf(pp, "%*u, ", unit_width-2, *(long *)buf);
				}
				else
				{
					slen = sprintf(pp, "%*d, ", unit_width-2, *(long *)buf);
				}
				break;

			case 1:
				if (radix_ == 8)
				{
					// Octal
					slen = sprintf(pp, "0%0*o, ", unit_width-3, buf[0]);
				}
				else if (radix_ == 16)
				{
					// Hex
					if (theApp.hex_ucase_)
						slen = sprintf(pp, "0x%0*X, ", unit_width-4, buf[0]);
					else
						slen = sprintf(pp, "0x%0*x, ", unit_width-4, buf[0]);
				}
				else if (radix_ == 10 && unsigned_)
				{
					// Unsigned decimal
					slen = sprintf(pp, "%*u, ", unit_width-2, buf[0]);
				}
				else
				{
					slen = sprintf(pp, "%*d, ", unit_width-2, (signed char)buf[0]);
				}
				break;

			case 2:
				if (radix_ == 8)
				{
					// Octal
					slen = sprintf(pp, "0%0*ho, ", unit_width-3, *(short *)buf);
				}
				else if (radix_ == 16)
				{
					// Hex
					if (theApp.hex_ucase_)
						slen = sprintf(pp, "0x%0*hX, ", unit_width-4, *(short *)buf); // xxx need to test this to make sure only 2 digits are shown (no sign ext)
					else
						slen = sprintf(pp, "0x%0*hx, ", unit_width-4, *(short *)buf);
				}
				else if (radix_ == 10 && unsigned_)
				{
					// Unsigned decimal
					slen = sprintf(pp, "%*hu, ", unit_width-2, *(short *)buf);
				}
				else
				{
					slen = sprintf(pp, "%*hd, ", unit_width-2, *(short *)buf);
				}
				break;

			case 8:
				if (radix_ == 8)
				{
					// Octal
					slen = sprintf(pp, "0%0*I64o, ", unit_width-3, *(__int64 *)buf);    // octal with leading zeroes
				}
				else if (radix_ == 16)
				{
					// Hex
					if (theApp.hex_ucase_)
						slen = sprintf(pp, "0x%0*I64X, ", unit_width-4, *(__int64 *)buf); // 0x then leading zeroes
					else
						slen = sprintf(pp, "0x%0*I64x, ", unit_width-4, *(__int64 *)buf);
				}
				else if (radix_ == 10 && unsigned_)
				{
					// Unsigned decimal
					slen = sprintf(pp, "%*I64u, ", unit_width-2, *(__int64 *)buf);
				}
				else
				{
					slen = sprintf(pp, "%*I64d, ", unit_width-2, *(__int64 *)buf);
				}
				break;
			}
			pp += slen;
			ASSERT(*(pp-1) == ' ');  // Checks that slen was correct and trailing space added
			break;

		case FLOAT:
			if (big_endian_)
				flip_bytes(buf, size_);
			switch (size_)
			{
			default:
				ASSERT(0);
				// fall through
			case 8:
				slen = sprintf(pp, "%*.*g, ", unit_width-2, unit_width-9, *(double *)buf);
				break;
			case 4:
				slen = sprintf(pp, "%*.*g, ", unit_width-2, unit_width-9, *(float *)buf);
				break;
			case 6:
				slen = sprintf(pp, "%*.*g, ", unit_width-2, unit_width-9, real48(buf));
				break;
			}
			pp += slen;
			ASSERT(*(pp-1) == ' ');
			break;
		}

		// Check if we need to start a new line
		if ((curr += size_) >= line_end || curr >= end)
		{
			// Terminate previous line
			if (type_ == STRING)
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

				if (type_ == STRING)
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
		extra = (radix_ == 8 ? 1 : 0) +         // allow for leading 0
			    (radix_ == 16 ? 2 : 0) +        // allow for leading 0x
			    (unsigned_ ? 0 : 1) +           // allow for sign
				2;                              // allow for trailing comma and space
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
		case C:
			retval += 5;  // 5 bytes for [/**/ ]
			break;
		}
	}

	return retval;
}

const char * Bin2Src::get_address(__int64 addr) const
{
	if (hex_width_ + dec_width_ + num_width_ == 0)
		return "";

	static char buf[80];
	char *pp = buf;
	size_t slen;

	switch (lang_)
	{
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
	case C:
		*pp++ = '*';
		*pp++ = '/';
		break;
	}
	*pp++ = ' ';
	*pp++ = '\0';
	return buf;
}
