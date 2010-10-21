// Scheme.h: interface for the CScheme class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SCHEME_H__2A9AA62D_2298_49D5_9626_1AFE5A6142FE__INCLUDED_)
#define AFX_SCHEME_H__2A9AA62D_2298_49D5_9626_1AFE5A6142FE__INCLUDED_

#include <sstream>
#include <vector>
#include "range_set.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CScheme  
{
public:

	#define ASCII_NAME "ASCII default"
	#define ANSI_NAME "ANSI default"
	#define OEM_NAME "IBM/OEM default"
	#define EBCDIC_NAME "EBCDIC default"
	#define PLAIN_NAME "Black & white"
	#define PRETTY_NAME "Rainbow"

	CScheme(const char *nn) : name_(nn)
	{
		can_delete_ = !(name_ == ASCII_NAME || name_ == ANSI_NAME ||
						name_ == OEM_NAME || name_ == EBCDIC_NAME);
		bg_col_ = mark_col_ = hi_col_ = bm_col_ = search_col_ = trk_col_ = comp_col_ = sector_col_ = -1;
		addr_bg_col_ = hex_addr_col_ = dec_addr_col_ = -1;  // Init here just in case lines below are removed
		mark_col_     = RGB(0, 224, 224);    // Cyan (-1 gives grey in default colour scheme)
		hex_addr_col_ = RGB(128, 0, 0);      // Dark red (-1 gives grey)
		dec_addr_col_ = RGB(0, 0, 128);      // Dark blue (-1 gives grey)
	}
	void AddRange(const CString &nn = "", COLORREF cc = 0, const char *rr = NULL)
	{
		range_name_.push_back(nn);
		range_col_.push_back(cc);
		range_val_.push_back(range_set<int>());

		if (rr != NULL)
		{
			std::istringstream str(rr);
			str >> range_val_.back();
		}
	}

	CString name_;                      // name of the scheme
	bool can_delete_;                   // is this a special or deleteable scheme
	COLORREF hi_col_, mark_col_, bm_col_, search_col_, trk_col_, comp_col_;
	COLORREF bg_col_, sector_col_, addr_bg_col_, dec_addr_col_, hex_addr_col_;

	std::vector<CString> range_name_;
	std::vector<COLORREF> range_col_;
	std::vector<range_set<int> > range_val_;
};

#endif // !defined(AFX_SCHEME_H__2A9AA62D_2298_49D5_9626_1AFE5A6142FE__INCLUDED_)
