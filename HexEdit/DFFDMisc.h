// DFFDMisc.h - various global functions used by templates and tree view
//
// Copyright (c) 2004 by Andrew W. Phillips.
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

#ifndef DFFDMISC_INCLUDED_
#define DFFDMISC_INCLUDED_  1

// DFFDMisc.h
#include <vector>

#include "XMLTree.h"

struct enum_entry
{
    CString name;
    __int64 value;
};

CPoint	get_posn(CWnd *pw, CPoint &orig);
CPoint	update_posn(CWnd *pw);
CString	get_name(const CXmlTree::CElt &ee);
long	get_size(const CXmlTree::CElt &ee,int first=0,int last=999999999);
CMenu *make_var_menu_tree(const CXmlTree::CElt &current_elt, bool this_too=false);
bool valid_id(LPCTSTR name);
bool get_enum(LPCTSTR pp,std::vector<enum_entry> &retval);
std::vector<enum_entry>::const_iterator find_enum(__int64 val, const std::vector<enum_entry> &vv);

#endif
