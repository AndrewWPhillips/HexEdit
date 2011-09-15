// Bin2Src.h: interface for the Bin2Src class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(BIN2SRC_H_)
#define BIN2SRC_H_

#if _MSC_VER > 1000
#pragma once
#endif

// This allows us to use short meaningful enum names without worrying about nasty CPP surprises
#if defined(STRING) || defined(CHAR) || defined(INT) || defined(FLOAT) || defined(NONE) || defined(UNKNOWN) || defined(BIG_ENDIAN) || defined(UNSIGNED)
#error enum const is #defined
#endif

class CHexEditDoc;

class Bin2Src  
{
public:
	enum lang { C, };
	enum type { UNKNOWN, STRING, CHAR, INT, FLOAT, };
	enum flags { NONE = 0, BIG_ENDIAN = 1, UNSIGNED = 2,
		         HEX_ADDRESS = 8, DEC_ADDRESS = 16, LINE_NUM = 32, ADDR1 = 64, };

	Bin2Src(CHexEditDoc *pdoc) : pdoc_(pdoc), lang_(C), type_(UNKNOWN) { }
	void Set(enum lang l, enum type t = INT, int s = 4, int b = 10, int r = 16, int o = 0, int i = 0, int f = NONE);

	int GetLength(__int64 start, __int64 end) const;
	size_t GetString(__int64 start, __int64 end, char *buf, int buf_len) const;

private:
	CHexEditDoc *pdoc_;                  // Document from which to get the data to convert

	enum lang lang_;                     // Format based on this language
	enum type type_;                     // Type of data
	int size_;                           // Size of data elements where appropriate (eg FLOAT can be 4 or 8)
	int radix_;                          // Only used for ints
	int rowsize_, offset_;               // # of src bytes per output line, and offset of 1st line
	int indent_;                         // Number of spaces to indent each line
	bool big_endian_, unsigned_;         // Various options that affect the formatting
	bool hex_address_, dec_address_, line_num_, addr1_; // If and how to show addresses are added (in comments)

	int get_address_width(__int64) const;
	int get_unit_width() const;
	mutable int hex_width_, dec_width_, num_width_; // cached widths of address parts
	const char * get_address(__int64 addr) const;   // get address/line number string enclosed in comments
};

#endif // !defined(BIN2SRC_H_)
