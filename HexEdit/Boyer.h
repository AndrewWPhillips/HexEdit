// boyer.h : interface of the boyer class for byte searches
//
// Copyright (c) 2015 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

/////////////////////////////////////////////////////////////////////////////

class boyer
{
public:
	// Construction
	boyer(const unsigned char *pat, size_t len, const unsigned char *mask);
	boyer(const boyer &);
	boyer &operator=(const boyer &);
	~boyer();

	// Attributes
	size_t length() { return pattern_len_; }
	const unsigned char *pattern() { return pattern_; }
	const unsigned char *mask() { return mask_; }

	// Operations
	unsigned char *findforw(unsigned char *pp, size_t len,
						BOOL icase, int tt,
						BOOL wholeword, BOOL alpha_before, BOOL alpha_after,
						int alignment, int offset, __int64 base_addr, __int64 address) const;
	unsigned char *findback(unsigned char *pp, size_t len,
							BOOL icase, int tt,
							BOOL wholeword, BOOL alpha_before, BOOL alpha_after,
							int alignment, int offset, __int64 base_addr, __int64 address) const;

private:
	unsigned char *mask_find(unsigned char *pp, size_t len,
							 BOOL icase, int tt,
							 BOOL wholeword, BOOL alpha_before, BOOL alpha_after,
							 int alignment, int offset, __int64 base_addr, __int64 address) const;
	unsigned char *mask_findback(unsigned char *pp, size_t len,
								 BOOL icase, int tt,
								 BOOL wholeword, BOOL alpha_before, BOOL alpha_after,
								 int alignment, int offset, __int64 base_addr, __int64 address) const;

	unsigned char *pattern_;	// Current search bytes
	unsigned char *mask_;		// Which bits are used (all if NULL)
	size_t pattern_len_;		// Length of search bytes
	size_t fskip_[256];			// Use internally in forward searches
	size_t bskip_[256];			// Use internally in backward searches
};
