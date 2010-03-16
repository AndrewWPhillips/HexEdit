// boyer.cpp : implementation of the boyer class to do Boyer Moore searches
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

#include "stdafx.h"
#include <string.h>

#include "boyer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern unsigned char e2a_tab[256];

// Converts an EBCDIC char to upper case
static unsigned char e2u_tab[] =
{
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
        0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
        0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
        0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
        0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
        0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
        0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
        0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,

        0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,
        0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
        0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,
        0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
        0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,
        0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
        0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,
        0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,

        0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,
        0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
        0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,
        0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
        0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,
        0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
        0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,
        0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,

        0xc0,0x81,0x82,0x83,0x84,0x85,0x86,0x87,
        0x88,0x89,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
        0xd0,0x91,0x92,0x93,0x94,0x95,0x96,0x97,
        0x98,0x99,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
        0xe0,0xe1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,
        0xa8,0xa9,0xea,0xeb,0xec,0xed,0xee,0xef,
        0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,
        0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff,
};

static unsigned char e2l_tab[] =
{
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
        0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
        0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
        0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
        0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
        0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
        0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
        0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,

        0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,
        0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
        0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,
        0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
        0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,
        0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
        0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,
        0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,

        0x80,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,
        0xc8,0xc9,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
        0x90,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,
        0xd8,0xd9,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
        0xa0,0xa1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,
        0xe8,0xe9,0xaa,0xab,0xac,0xad,0xae,0xaf,
        0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,
        0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,

        0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,
        0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
        0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,
        0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
        0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,
        0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
        0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,
        0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff,
};

static unsigned char bit_count[] =
{
    0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
};

// Normal constructor
boyer::boyer(const unsigned char *pat, size_t patlen, const unsigned char *mask)
{
    size_t ii;

    // Keep a copy of the pattern
    pattern_ = new unsigned char[patlen];
    memcpy(pattern_, pat, patlen);
    if (mask != NULL)
    {
        mask_ = new unsigned char[patlen];
        memcpy(mask_, mask, patlen);
    }
    else
        mask_ = NULL;

    for (ii = 0; ii < 256; ++ii)
        fskip_[ii] = bskip_[ii] = patlen;
    for (ii = 0; ii < patlen; ++ii)
        fskip_[pat[ii]] = patlen - (ii + 1);
    for (ii = patlen; ii > 0; ii--)
        bskip_[pat[ii-1]] = ii - 1;
    pattern_len_ = patlen;

#ifdef _DEBUG
    ASSERT(sizeof(bit_count) == 256);

    // Check the values in out bit_count table
	for (int kk = 0; kk < 256; ++kk)
    {
        int count=0;
        for (int jj = 0; jj < 8; ++jj) if ((kk>>jj)&0x01) ++count;
        ASSERT(bit_count[kk] == count);
	}
#endif
}

// Copy constructor
boyer::boyer(const boyer &from)
{
    pattern_len_ = from.pattern_len_;
    pattern_ = new unsigned char[pattern_len_];
    memcpy(pattern_, from.pattern_, pattern_len_);
    if (from.mask_ != NULL)
    {
        mask_ = new unsigned char[pattern_len_];
        memcpy(mask_, from.mask_, pattern_len_);
    }
    else
        mask_ = NULL;

    memcpy(fskip_, from.fskip_, sizeof(fskip_));
    memcpy(bskip_, from.bskip_, sizeof(bskip_));
}

// Copy assignment operator
boyer &boyer::operator=(const boyer &from)
{
    if (&from != this)
    {
        pattern_len_ = from.pattern_len_;
        pattern_ = new unsigned char[pattern_len_];
        memcpy(pattern_, from.pattern_, pattern_len_);
        if (from.mask_ != NULL)
        {
            mask_ = new unsigned char[pattern_len_];
            memcpy(mask_, from.mask_, pattern_len_);
        }
        else
            mask_ = NULL;

        memcpy(fskip_, from.fskip_, sizeof(fskip_));
        memcpy(bskip_, from.bskip_, sizeof(bskip_));
    }
    return *this;
}

boyer::~boyer()
{
    delete[] pattern_;
    if (mask_ != NULL)
        delete[] mask_;
}

// - extra params: wholeword, alignment, mask
// pp = ptr to first byte of memory to search
// len = length of memory to search
// icase = ignore case when comparing
// tt = 3 means text is EBCDIC
// wholeword = only match equal if characters on either side are not alpha
// alignment = only match if first byte has this alignment (in file) - use 1 for no alignment check
// address = address of first byte within file - used for alignment check
unsigned char *boyer::findforw(unsigned char *pp, size_t len, BOOL icase, int tt,
                           BOOL wholeword, BOOL alpha_before, BOOL alpha_after,
                           int alignment, int offset, __int64 base_addr,  __int64 address) const
{
    // Search with a mask is handled completely differently
    if (mask_ != NULL)
        return mask_find(pp, len, icase, tt, wholeword, alpha_before, alpha_after, alignment, offset, base_addr, address);

    size_t spos = pattern_len_ - 1;     // Posn within searched bytes
    size_t patpos = pattern_len_ - 1;   // Posn within search pattern

    // Leave icase/ebcdic tests outside the loop for speed
    if (tt == 3 && icase)
    {
        // Case-insensitive EBCDIC search
        while (spos < len)
        {
            if (e2u_tab[pp[spos]] != e2u_tab[pattern_[patpos]])
            {
                // Match failed so skip forward to start another
                spos += max(pattern_len_ - patpos, 
                    min(fskip_[e2u_tab[pp[spos]]], fskip_[e2l_tab[pp[spos]]]));
                patpos = pattern_len_ - 1;
            }
            else
            {
                // This characters matches - is that all of them?
                if (patpos == 0)
                {
                    // All bytes match - now check for other requirements
                    BOOL passed = TRUE;

                    // Check whole word and alignment options
                    if (wholeword && spos == 0 && alpha_before)
                        passed = FALSE;                         // start of search buffer but alpha before buffer
                    else if (wholeword && spos + pattern_len_ == len && alpha_after)
                        passed = FALSE;                         // match at end of buffer but alpha after buffer
                    else if (wholeword && spos > 0 && isalnum(e2a_tab[pp[spos-1]]))
                        passed = FALSE;                         // alpha before match so whole word search fails
                    else if (wholeword && spos + pattern_len_ < len && isalnum(e2a_tab[pp[spos+pattern_len_]]))
                        passed = FALSE;                         // alpha after match so it's not a whole word
                    else if (alignment > 1 && ((address - base_addr + spos) < 0 || (address - base_addr + spos) % alignment != offset))
                        passed = FALSE;
                    if (!passed)
                    {
                        // Failed a test so keep going
                        spos += pattern_len_;
                        patpos = pattern_len_ - 1;
                        continue;
                    }

                    return pp + spos;
                }

                // Go backwards one to check it next
                spos--;
                patpos--;
            }
        }
    }
    else if (icase)
    {
        // Case insensitive search (ASCII or Unicode).
        // Note: This only works for the ASCII subset of Unicode
        // (where the high byte is zero and low byte == ASCII value).
        while (spos < len)
        {
            if (toupper(pp[spos]) != toupper(pattern_[patpos]))
            {
                // Match failed so skip forward to start another
                spos += max(pattern_len_ - patpos, 
                    min(fskip_[toupper(pp[spos])], fskip_[tolower(pp[spos])]));
                patpos = pattern_len_ - 1;
            }
            else
            {
                // This characters matches - is that all of them?
                if (patpos == 0)
                {
                    // All bytes match - now check for other requirements
                    BOOL passed = TRUE;

                    // Check whole word and alignment options
                    if (wholeword && spos == 0 && alpha_before)
                        passed = FALSE;                         // start of search buffer but alpha before buffer
                    else if (wholeword && spos + pattern_len_ == len && alpha_after)
                        passed = FALSE;                         // match at end of buffer but alpha after buffer
                    else if (wholeword && spos > 0 && isalnum(pp[spos-1]))
                        passed = FALSE;
                    else if (wholeword && spos + pattern_len_ < len && isalnum(pp[spos+pattern_len_]))
                        passed = FALSE;
                    else if (alignment > 1 && ((address - base_addr + spos) < 0 || (address - base_addr + spos) % alignment != offset))
                        passed = FALSE;
                    if (!passed)
                    {
                        // Failed a test so keep going
                        spos += pattern_len_;
                        patpos = pattern_len_ - 1;
                        continue;
                    }

                    return pp + spos;
                }

                // Go backwards one for to check it next
                spos--;
                patpos--;
            }
        }
    }
    else
    {
        // While we haven't reached the end of the buffer to search
        while (spos < len)
        {
            if (pp[spos] != pattern_[patpos])
            {
                // Match failed so skip forward to start another
                spos += max(pattern_len_ - patpos, fskip_[pp[spos]]);
                patpos = pattern_len_ - 1;
            }
            else
            {
                // This characters matches - is that all of them?
                if (patpos == 0)
                {
                    // All bytes match - now check for other requirements
                    BOOL passed = TRUE;

                    // Check whole word and alignment options
                    if (wholeword && spos == 0 && alpha_before)
                        passed = FALSE;                         // start of search buffer but alpha before buffer
                    else if (wholeword && spos + pattern_len_ == len && alpha_after)
                        passed = FALSE;                         // match at end of buffer but alpha after buffer
                    else if (tt==3 && wholeword && spos > 0 && isalnum(e2a_tab[pp[spos-1]]))
                        passed = FALSE;
                    else if (tt==3 && wholeword && spos + pattern_len_ < len && isalnum(e2a_tab[pp[spos+pattern_len_]]))
                        passed = FALSE;
                    else if (tt!=3 && wholeword && spos > 0 && isalnum(pp[spos-1]))
                        passed = FALSE;
                    else if (tt!=3 && wholeword && spos + pattern_len_ < len && isalnum(pp[spos+pattern_len_]))
                        passed = FALSE;
                    else if (alignment > 1 && ((address - base_addr + spos) < 0 || (address - base_addr + spos) % alignment != offset))
                        passed = FALSE;
                    if (!passed)
                    {
                        // Failed a test so keep going
                        spos += pattern_len_;
                        patpos = pattern_len_ - 1;
                        continue;
                    }

                    return pp + spos;
                }

                // Go backwards one to check it next
                spos--;
                patpos--;
            }
        }
    }
    return NULL;                // Pattern not found
}

unsigned char *boyer::findback(unsigned char *pp, size_t len, BOOL icase, int tt,
                               BOOL wholeword, BOOL alpha_before, BOOL alpha_after,
                               int alignment, int offset, __int64 base_addr, __int64 address) const
{
    // Search with a mask is handled completely differently
    if (mask_ != NULL)
        return mask_findback(pp, len, icase, tt, wholeword, alpha_before, alpha_after, alignment, offset, base_addr, address);

    long spos = len - pattern_len_;     // Current position within search bytes
    size_t patpos = 0;                  // Current position within pattern

    // Leave icase/ebcdic tests outside the loop for speed
    if (tt == 3 && icase)
    {
        // Case insensitive EBCDIC search
        while (spos >= 0)
        {
            if (e2u_tab[pp[spos]] != e2u_tab[pattern_[patpos]])
            {
                // Match failed so skip back to start another comparison
                spos -= max(patpos + 1, 
                    min(bskip_[e2u_tab[pp[spos]]], bskip_[e2l_tab[pp[spos]]]));
                patpos = 0;
            }
            else
            {
                // One more byte matches - check if that's the last
                if (patpos == pattern_len_ - 1)
                {
                    // All bytes match - now check for other requirements
                    BOOL passed = TRUE;

                    // Check whole word and alignment options
                    if (wholeword && spos - pattern_len_ + 1 == 0 && alpha_before)
                        passed = FALSE;
                    else if (wholeword && spos + 1 == long(len) && alpha_after)
                        passed = FALSE;
                    else if (wholeword && spos - pattern_len_ + 1 > 0 && isalnum(e2a_tab[pp[spos - pattern_len_]]))
                        passed = FALSE;
                    else if (wholeword && spos + 1 < long(len) && isalnum(e2a_tab[pp[spos+1]]))
                        passed = FALSE;
                    else if (alignment > 1 && ((address - base_addr + spos - pattern_len_ + 1) < 0 || (address - base_addr + spos - pattern_len_ + 1) % alignment != offset))
                        passed = FALSE;
                    if (!passed)
                    {
                        // Failed a test so keep going
                        spos -= pattern_len_;
                        patpos = 0;
                        continue;
                    }

                    return pp + spos - pattern_len_ + 1;
                }

                ++spos;
                ++patpos;
            }
        }
    }
    else if (icase)
    {
        // Case insensitive search (ASCII or Unicode).
        // Note: This only works for the ASCII subset of Unicode
        // (where the high byte is zero and low byte == ASCII value).
        while (spos >= 0)
        {
            if (toupper(pp[spos]) != toupper(pattern_[patpos]))
            {
                // Match failed so skip back to start another comparison
                spos -= max(patpos + 1, 
                    min(bskip_[toupper(pp[spos])], bskip_[tolower(pp[spos])]));
                patpos = 0;
            }
            else
            {
                // One more byte matches - check if that's the last
                if (patpos == pattern_len_ - 1)
                {
                    // All bytes match - now check for other requirements
                    BOOL passed = TRUE;

                    // Check whole word and alignment options
                    if (wholeword && spos - pattern_len_ + 1 == 0 && alpha_before)
                        passed = FALSE;
                    else if (wholeword && spos + 1 == long(len) && alpha_after)
                        passed = FALSE;
                    else if (wholeword && spos - pattern_len_ + 1 > 0 && isalnum(pp[spos - pattern_len_]))
                        passed = FALSE;
                    else if (wholeword && spos + 1 < long(len) && isalnum(pp[spos+1]))
                        passed = FALSE;
                    else if (alignment > 1 && ((address - base_addr + spos - pattern_len_ + 1) < 0 || (address - base_addr + spos - pattern_len_ + 1) % alignment != offset))
                        passed = FALSE;
                    if (!passed)
                    {
                        // Failed a test so keep going
                        spos -= pattern_len_;
                        patpos = 0;
                        continue;
                    }

                    return pp + spos - pattern_len_ + 1;
                }

                ++spos;
                ++patpos;
            }
        }
    }
    else
    {
        // While we haven't reached the start of the buffer to be searched
        while (spos >= 0)
        {
            if (pp[spos] != pattern_[patpos])
            {
                // Match failed so skip back to start another comparison
                spos -= max(patpos + 1, bskip_[pp[spos]]);
                patpos = 0;
            }
            else
            {
                // One more byte matches - check if that's the last
                if (patpos == pattern_len_ - 1)
                {
                    // All bytes match - now check for other requirements
                    BOOL passed = TRUE;

                    // Check whole word and alignment options
                    if (wholeword && spos - pattern_len_ + 1 == 0 && alpha_before)
                        passed = FALSE;
                    else if (wholeword && spos + 1 == long(len) && alpha_after)
                        passed = FALSE;
                    else if (tt==3 && wholeword && spos - pattern_len_ + 1 > 0 && isalnum(e2a_tab[pp[spos - pattern_len_]]))
                        passed = FALSE;
                    else if (tt==3 && wholeword && spos + 1 < long(len) && isalnum(e2a_tab[pp[spos+1]]))
                        passed = FALSE;
                    else if (tt!=3 && wholeword && spos - pattern_len_ + 1 > 0 && isalnum(pp[spos - pattern_len_]))
                        passed = FALSE;
                    else if (tt!=3 && wholeword && spos + 1 < long(len) && isalnum(pp[spos+1]))
                        passed = FALSE;
                    else if (alignment > 1 && ((address - base_addr + spos - pattern_len_ + 1) < 0 || (address - base_addr + spos - pattern_len_ + 1) % alignment != offset))
                        passed = FALSE;
                    if (!passed)
                    {
                        // Failed a test so keep going
                        spos -= pattern_len_;
                        patpos = 0;
                        continue;
                    }

                    return pp + spos - pattern_len_ + 1;
                }

                ++spos;
                ++patpos;
            }
        }
    }
    return NULL;                // Pattern not matched
}

// This does not use a Boyer Moore search but simply searches for one character of the
// search text and then sees if the rest of the string matches.  This will be slower
// but in the future we may be able to pass stats on number of different bytes
// in the file to at least look for the least common byte.
unsigned char *boyer::mask_find(unsigned char *pp, size_t len, BOOL icase, int tt,
                                BOOL wholeword, BOOL alpha_before, BOOL alpha_after,
                                int alignment, int offset, __int64 base_addr, __int64 address) const
{
#ifdef _DEBUG
    // If case-insensitive mask can only have all bits on or off
    if (icase) { for (size_t  ii=0; ii < pattern_len_; ++ii) ASSERT(mask_[ii] == 0 || mask_[ii] == 0xFF); }
#endif

    int best_pos = -1;          // Index into pattern_ of byte that we will search for
    int best_bits = 0;          // Number of bits in mask_[bestpos]
    BOOL best_alpha = TRUE;     // Is the best found so far an alpha (and icase is on)
    size_t ii;

    for (ii = 0; ii < pattern_len_; ++ii)
    {
        if (mask_[ii] == 0xFF &&
            (!icase || (tt==3 && !isalpha(e2a_tab[pattern_[ii]])) || (tt != 3 && !isalpha(pattern_[ii]))) )
        {
            best_pos = ii;
            best_bits = 8;
            best_alpha = FALSE;
            break;              // Best possible character to search for so exit loop
        }
        else if (bit_count[mask_[ii]] > best_bits &&
            (!icase || (tt==3 && !isalpha(e2a_tab[pattern_[ii]])) || (tt != 3 && !isalpha(pattern_[ii]))) )
        {
            best_pos = ii;
            best_bits = bit_count[mask_[ii]];
            best_alpha = FALSE;
        }
        else if (best_alpha && bit_count[mask_[ii]] > 0)
        {
            best_pos = ii;
        }
    }

    ASSERT(best_pos >= -1 && best_pos < int(pattern_len_));

    // Now do the search
    for (unsigned char *pfound = pp + best_pos; ; ++pfound)
    {
        if (best_pos == -1)
        {
            ; // will always match 1st character so do nothin here
        }
        else if (best_bits == 8)
        {
            ASSERT(!icase || !best_alpha);

            // Search for byte
            pfound = (unsigned char *)memchr(pfound, pattern_[best_pos], len - (pfound-pp));
        }
        else if (!icase || !best_alpha)
        {
            ASSERT(best_bits > 0);

            // Perform masked search
            unsigned char *qq, *qend = pp + len;
            for (qq = pfound; qq < qend; ++qq)
            {
                if ((*qq & mask_[best_pos]) == (pattern_[best_pos] & mask_[best_pos]))
                    break;
            }
            if (qq == qend)
                pfound = NULL;
            else
                pfound = qq;
        }
        else if (tt == 3)
        {
            ASSERT(icase && best_alpha);

            // Search for both cases
            unsigned char *p1 = (unsigned char *)memchr(pfound, e2l_tab[pattern_[best_pos]], len - (pfound-pp));
            unsigned char *p2 = (unsigned char *)memchr(pfound, e2u_tab[pattern_[best_pos]], len - (pfound-pp));

            if (p1 == NULL)
                pfound = p2;
            else if (p2 == NULL)
                pfound = p1;
            else if (p2 < p1)
                pfound = p2;
            else
                pfound = p1;
        }
        else
        {
            ASSERT(icase && best_alpha && tt != 3);

            // Search for both cases
            unsigned char *p1 = (unsigned char *)memchr(pfound, tolower(pattern_[best_pos]), len - (pfound-pp));
            unsigned char *p2 = (unsigned char *)memchr(pfound, toupper(pattern_[best_pos]), len - (pfound-pp));

            if (p1 == NULL)
                pfound = p2;
            else if (p2 == NULL)
                pfound = p1;
            else if (p2 < p1)
                pfound = p2;
            else
                pfound = p1;
        }

        // If not found return
        if (pfound == NULL || (pfound - pp) + pattern_len_ - best_pos > len)
            return NULL;

        BOOL passed = TRUE;     // Signals if the match failed for any reason

        // Check that the pattern/mask actually matches
        unsigned char *base = pfound - best_pos;
        ASSERT(base >= pp);

        for (ii = 0; ii < pattern_len_; ++ii)
        {
            if (mask_[ii] == 0)
            {
                ;
            }
            else if (mask_[ii] != 0xFF)
            {
                // Do masked compare
                if ((base[ii] & mask_[ii]) != (pattern_[ii] & mask_[ii]))
                    break;
            }
            else if (!icase)
            {
                ASSERT(mask_[ii] == 0xFF);

                // Do simple compare
                if (base[ii] != pattern_[ii])
                    break;
            }
            else if (tt != 3)
            {
                ASSERT(mask_[ii] == 0xFF && icase);

                // Do case-insensitive compare
                if (toupper(base[ii]) != toupper(pattern_[ii]))
                    break;
            }
            else
            {
                ASSERT(mask_[ii] == 0xFF && icase && tt == 3);

                // Do case-insensitive EBCDIC compare
                if (e2u_tab[base[ii]] != e2u_tab[pattern_[ii]])
                    break;
            }
        }

        if (ii < pattern_len_)
            passed = FALSE;     // Pattern did not match
        else if (wholeword && base == pp && alpha_before)
            passed = FALSE;
        else if (wholeword && base + pattern_len_ == pp + len && alpha_after)
            passed = FALSE;
        else if (wholeword && tt == 3 && base > pp && isalnum(e2a_tab[base[-1]]))
            passed = FALSE;
        else if (wholeword && tt == 3 && base + pattern_len_ < pp + len && isalnum(e2a_tab[base[pattern_len_]]))
            passed = FALSE;
        else if (wholeword && tt != 3 && base > pp && isalnum(base[-1]))
            passed = FALSE;
        else if (wholeword && tt != 3 && base + pattern_len_ < pp + len && isalnum(base[pattern_len_]))
            passed = FALSE;
        else if (alignment > 1 && ((address - base_addr + base - pp) < 0 || (address - base_addr + base - pp) % alignment != offset))
            passed = FALSE;

        if (passed)
            return base;
        // else continue the search
    }
    ASSERT(0);
    return NULL;
}

unsigned char *boyer::mask_findback(unsigned char *pp, size_t len, BOOL icase, int tt,
                                    BOOL wholeword, BOOL alpha_before, BOOL alpha_after,
                                    int alignment, int offset, __int64 base_addr, __int64 address) const
{
#ifdef _DEBUG
    // If case-insensitive then mask can only have all bits on or off
    if (icase) { for (size_t  ii=0; ii < pattern_len_; ++ii) ASSERT(mask_[ii] == 0 || mask_[ii] == 0xFF); }
#endif

    int best_pos = -1;          // Index into pattern_ of byte that we will search for
    int best_bits = 0;          // Number of bits in mask_[bestpos]
    BOOL best_alpha = TRUE;     // Is the best found so far an alpha (and icase is on)
    size_t ii;

    for (ii = 0; ii < pattern_len_; ++ii)
    {
        if (mask_[ii] == 0xFF &&
            (!icase || (tt==3 && !isalpha(e2a_tab[pattern_[ii]])) || (tt != 3 && !isalpha(pattern_[ii]))) )
        {
            best_pos = ii;
            best_bits = 8;
            best_alpha = FALSE;
            break;              // Best possible character to search for so exit loop
        }
        else if (bit_count[mask_[ii]] > best_bits &&
            (!icase || (tt==3 && !isalpha(e2a_tab[pattern_[ii]])) || (tt != 3 && !isalpha(pattern_[ii]))) )
        {
            best_pos = ii;
            best_bits = bit_count[mask_[ii]];
            best_alpha = FALSE;
        }
        else if (best_alpha && bit_count[mask_[ii]] > 0)
        {
            best_pos = ii;
        }
    }

    ASSERT(best_pos >= 0 && best_pos < int(pattern_len_));

    // Now do the search
    for (unsigned char *pfound = pp + len - pattern_len_ + best_pos; ; pfound--)
    {
        if (best_pos == -1)
        {
            ; // will always match 1st character so do nothing here
        }
        else if (best_bits == 8)
        {
            ASSERT(!icase || !best_alpha);

            // Search for byte from end
//            pfound = (unsigned char *)memrchr(pfound, pattern_[best_pos], len - (pfound-pp));
            unsigned char *qq;
            for (qq = pfound; qq >= pp; qq--)
            {
                if (*qq == pattern_[best_pos])
                    break;
            }
            if (qq < pp)
                pfound = NULL;
            else
                pfound = qq;
        }
        else if (!icase || !best_alpha)
        {
            ASSERT(best_bits > 0);

            // Perform masked search
            unsigned char *qq;
            for (qq = pfound; qq >= pp; qq--)
            {
                if ((*qq & mask_[best_pos]) == (pattern_[best_pos] & mask_[best_pos]))
                    break;
            }
            if (qq < pp)
                pfound = NULL;
            else
                pfound = qq;
        }
        else if (tt == 3)
        {
            ASSERT(icase && best_alpha);

            // Search EBCDIC ignoring case
            unsigned char *qq;
            for (qq = pfound; qq >= pp; qq--)
            {
                if (e2u_tab[*qq] == e2u_tab[pattern_[best_pos]])
                    break;
            }
            if (qq < pp + best_pos)
                pfound = NULL;
            else
                pfound = qq;
        }
        else
        {
            ASSERT(icase && best_alpha && tt != 3);

            // Search ASCII ignoring case
            unsigned char *qq;
            for (qq = pfound; qq >= pp; qq--)
            {
                if (toupper(*qq) == toupper(pattern_[best_pos]))
                    break;
            }
            if (qq < pp + best_pos)
                pfound = NULL;
            else
                pfound = qq;
        }

        // If not found return
        if (pfound == NULL)
            return NULL;

        BOOL passed = TRUE;     // Signals if the match failed for any reason

        // Check that the pattern/mask actually matches
        unsigned char *base = pfound - best_pos;
        ASSERT(base >= pp);

        for (ii = 0; ii < pattern_len_; ++ii)
        {
            if (mask_[ii] == 0)
            {
                ;
            }
            else if (mask_[ii] != 0xFF)
            {
                // Do masked compare
                if ((base[ii] & mask_[ii]) != (pattern_[ii] & mask_[ii]))
                    break;
            }
            else if (!icase)
            {
                ASSERT(mask_[ii] == 0xFF);

                // Do simple compare
                if (base[ii] != pattern_[ii])
                    break;
            }
            else if (tt != 3)
            {
                ASSERT(mask_[ii] == 0xFF && icase);

                // Do case-insensitive compare
                if (toupper(base[ii]) != toupper(pattern_[ii]))
                    break;
            }
            else
            {
                ASSERT(mask_[ii] == 0xFF && icase && tt == 3);

                // Do case-insensitive EBCDIC compare
                if (e2u_tab[base[ii]] != e2u_tab[pattern_[ii]])
                    break;
            }
        }

        if (ii < pattern_len_)
            passed = FALSE;     // Pattern did not match
        else if (wholeword && base == pp && alpha_before)
            passed = FALSE;
        else if (wholeword && base + pattern_len_ == pp + len && alpha_after)
            passed = FALSE;
        else if (wholeword && tt == 3 && base > pp && isalnum(e2a_tab[base[-1]]))
            passed = FALSE;
        else if (wholeword && tt == 3 && base + pattern_len_ < pp + len && isalnum(e2a_tab[base[pattern_len_]]))
            passed = FALSE;
        else if (wholeword && tt != 3 && base > pp && isalnum(base[-1]))
            passed = FALSE;
        else if (wholeword && tt != 3 && base + pattern_len_ < pp + len && isalnum(base[pattern_len_]))
            passed = FALSE;
        else if (alignment > 1 && ((address - base_addr + base - pp) < 0 || (address - base_addr + base - pp) % alignment != offset))
            passed = FALSE;

        if (passed)
            return base;
        // else continue the search
    }
    ASSERT(0);
    return NULL;
}