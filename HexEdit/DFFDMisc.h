// DFFDMisc.h - various global functions used by templates and tree view
//
// Copyright (c) 2015 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
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
CMenu *make_var_menu_tree(const CXmlTree::CElt &current_elt, bool this_too=false, bool file_too=false);
bool valid_id(LPCTSTR name);
bool get_enum(LPCTSTR pp,std::vector<enum_entry> &retval);
std::vector<enum_entry>::const_iterator find_enum(__int64 val, const std::vector<enum_entry> &vv);

#endif
