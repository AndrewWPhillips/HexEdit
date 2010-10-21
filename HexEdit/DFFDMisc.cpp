// DFFDMisc.cpp
//
// Copyright (c) 2004-2010 by Andrew W. Phillips.
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
#include <MultiMon.h>

#include "HexEdit.h"
#include "DFFDMisc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPoint update_posn(CWnd *pw)
{
	CRect rr;
	pw->GetWindowRect(&rr);
	return CPoint(rr.left, rr.top);
}

// Work out where to place child DFFD dialog so it is next to or below its parent dialog, but
// not outside the "container".  The container is the rect of the monitor where the parent is
// (in mult monitor environment) or else the whole screen (whole of single monitor).
CPoint get_posn(CWnd *pw, CPoint &orig)
{
	ASSERT(pw != NULL && pw->GetParent() != NULL);
	CPoint retval;
	CRect rect;                         // Rect of this dialog
	pw->GetWindowRect(&rect);
	CRect cont_rect;                    // Rect of "container"

// xxx keep this change?
//    AfxGetMainWnd()->GetWindowRect(&cont_rect);     // Try main window first as container
//
//    CRect rr;                           // Temp rect
//    if (!rr.IntersectRect(&rect, &cont_rect))
	{
		// Use rect of containing monitor as "container"
		if (theApp.mult_monitor_)
		{
			HMONITOR hh = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi;
			mi.cbSize = sizeof(mi);
			if (hh != 0 && GetMonitorInfo(hh, &mi))
				cont_rect = mi.rcWork;  // work area of nearest monitor
			else
			{
				// Shouldn't happen but if it does use the whole virtual screen
				ASSERT(0);
				cont_rect = CRect(::GetSystemMetrics(SM_XVIRTUALSCREEN),
					::GetSystemMetrics(SM_YVIRTUALSCREEN),
					::GetSystemMetrics(SM_XVIRTUALSCREEN) + ::GetSystemMetrics(SM_CXVIRTUALSCREEN),
					::GetSystemMetrics(SM_YVIRTUALSCREEN) + ::GetSystemMetrics(SM_CYVIRTUALSCREEN));
			}
		}
		else if (!::SystemParametersInfo(SPI_GETWORKAREA, 0, &cont_rect, 0))
		{
			// I don't know if this will ever happen since the Windows documentation
			// is pathetic and does not say when or why SystemParametersInfo might fail.
			cont_rect = CRect(0, 0, ::GetSystemMetrics(SM_CXFULLSCREEN),
								   ::GetSystemMetrics(SM_CYFULLSCREEN));
		}
	}

	if (rect.right + rect.Width() - rect.Width()*2/9 < cont_rect.right)
	{
		// There's enough room on the right to put the new dialog window
		retval.x = rect.right - rect.Width()*2/9;
		retval.y = rect.top;
	}
	else if (rect.bottom + rect.Height() < cont_rect.bottom)
	{
		// There's enough room to add it below
		retval.x = orig.x;
		retval.y = rect.bottom;
	}
	else
	{
		// put it just below the first window
		orig.y += 20;
		retval.x = orig.x;
		retval.y = orig.y;
	}
	return retval;
}

CString get_name(const CXmlTree::CElt &ee)
{
	ASSERT(!ee.IsEmpty());
	CString elt_type = ee.GetName();

	if (elt_type == "define_struct")
	{
		return ee.GetAttr("type_name") + CString(" [STRUCT DEFN]");
	}
	else if (elt_type == "struct")
	{
		return ee.GetAttr("name") + CString(" [STRUCT]");
	}
	else if (elt_type == "use_struct")
	{
		return ee.GetAttr("name") + CString(" - using ") + ee.GetAttr("type_name");
	}
	else if (elt_type == "for")
	{
		CString name = ee.GetAttr("name");

		if (name.IsEmpty())
			return CString("[FOR] - ") + get_name(ee.GetFirstChild());
		else
			return name + CString(" [FOR]");
	}
	else if (elt_type == "if")
	{
		CString name1 = get_name(ee.GetFirstChild());
		CString name2;
		if (ee.GetNumChildren() > 1)
		{
			// Get name for else part
			ASSERT(ee.GetNumChildren() == 3 && ee.GetChild(1).GetName() == "else");
			name2 = get_name(ee.GetChild(2));
		}
		if (name1.IsEmpty())
			name1 = name2;              // no name for IF part so use else part
		else if (!name2.IsEmpty() && name1 != name2)
			name1 += "/" + name2;       // Combine both names
		return CString("[IF] - ") + name1;
	}
	else if (elt_type == "switch")
	{
		return CString("[CASE] ") + ee.GetAttr("test");
	}
	else if (elt_type == "jump")
	{
		return CString("[JUMP] - ") + get_name(ee.GetFirstChild());
	}
	else if (elt_type == "eval")
	{
		return CString("[EVAL] ") + ee.GetAttr("expr");
	}
	else if (elt_type == "data")
	{
		CString name = ee.GetAttr("name");
		if (name.IsEmpty())
			return CString("[DATA]");
		else
			return name;
	}
	else
		ASSERT(0);

	return CString("ERROR");
}

// Return the size of an element (if it only contains data and/or struct elements) checking
// the size of struct children if necessary.  If any children are DATA elements with calc'd
// size (or FOR or IF) then -1 is returned.
long get_size(const CXmlTree::CElt &ee, int first /*=0*/, int last /*=999999999*/)
{
	long retval = 0;
	CXmlTree::CElt child;
	int ii;
	int bits_used = 0;   // Bits used by all consec. bitfields so far (0 if previous elt not a bitfield)
	int last_size;       // Storage unit size of previous element if a bitfield (1,2,4, or 8) or 0
	bool last_down;      // Direction for previous bitfield (must be same dirn to be put in same stg unit)

	for (child = ee.GetChild(first), ii = first; !child.IsEmpty() && ii < last; ++child, ++ii)
	{
		CString elt_type = child.GetName();
		if (elt_type != "data" && bits_used > 0)
		{
			// End of bit-field stg unit
			ASSERT(last_size == 1 || last_size == 2 || last_size == 4 || last_size == 8);
			retval += last_size;
			bits_used = 0;
		}

		if (elt_type == "data")
		{
			CString ss = child.GetAttr("type");
			int data_bits = 0;   // bits used in this element (or zero if not a bit-field)
			int data_size;
			bool data_down;

			// Check for bit-field (type is "int")
			if (ss.CompareNoCase("int") == 0)
			{
				data_bits = atoi(child.GetAttr("bits"));
				if (data_bits > 0)
				{
					data_size = atoi(child.GetAttr("len"));
					data_down = child.GetAttr("direction") == "down";
				}
			}

			if (bits_used > 0 &&
				(data_bits == 0 ||                     // not bit-field
				 data_size != last_size ||             // diff size
				 data_down != last_down ||             // diff dirn
				 bits_used + data_bits > data_size*8)  // overflow stg unit
			   )
			{
				// Previous elt was end of the bit-field stg unit
				ASSERT(last_size == 1 || last_size == 2 || last_size == 4 || last_size == 8);
				retval += last_size;
				bits_used = 0;
			}

			if (data_bits > 0)
			{
				// Save info about current bit-field
				bits_used += data_bits;
				last_size = data_size;
				last_down = data_down;
			}
			else if (ss.CompareNoCase("char") == 0)
			{
				// Work out the size of a character (normally 1 unless Unicode)
				ss = child.GetAttr("format");
				if (ss.CompareNoCase("unicode") == 0)
					retval += 2;
				else
					retval += 1;
			}
			else if (ss.CompareNoCase("date") == 0)
			{
				// Work out the size of the date field depending on the format
				ss = child.GetAttr("format");
				if (ss.CompareNoCase("c") == 0)
					retval += 4;
				else if (ss.CompareNoCase("c51") == 0)
					retval += 4;
				else if (ss.CompareNoCase("c7") == 0)
					retval += 4;
				else if (ss.CompareNoCase("cmin") == 0)
					retval += 4;
				else if (ss.CompareNoCase("c64") == 0)
					retval += 8;
				else if (ss.CompareNoCase("ole") == 0)
					retval += 8;
				else if (ss.CompareNoCase("systemtime") == 0)
					retval += 16;
				else if (ss.CompareNoCase("filetime") == 0)
					retval += 8;
				else if (ss.CompareNoCase("msdos") == 0)
					retval += 4;
				else
				{
					ASSERT(0);
					return -1;
				}
			}
			else
			{
				// Get the size of the child by checking
				ss = child.GetAttr("len");
				char *endp;
				//ss.TrimLeft();
				//ss.TrimRight();

				// Note: we could use expression parser to handle constant expression (eg, "10+1") but is it necessary?
				long len = strtoul(ss, &endp, 10);
				// Make sure the string was not empty and there was nothing after the number (eg, not "5+n")
				if (endp > (const char *)ss && endp - (const char *)ss == ss.GetLength())
					retval +=len;
				else
					return -1L;             // Does not appear to be a simple number so assume it is an expression
			}
		}
		else if (elt_type == "struct" || elt_type == "binary_file_format")
		{
			// Get the size of child by looking at its children
			long child_size = get_size(child);
			if (child_size > -1)
				retval += child_size;
			else
				return -1L;             // Child size is indeterminate, therefore so is our size
		}
		else if (elt_type == "use_struct")
		{
			long tmp = -1;
			// Find define_struct and get its size
			CString ss = child.GetAttr("type_name");            // Find struct name to use
			CXmlTree::CElt ee = child.GetOwner()->GetRoot().GetFirstChild();
			for ( ; !ee.IsEmpty() && ee.GetName() == "define_struct"; ++ee)
				if (ss == ee.GetAttr("type_name"))
					tmp = get_size(ee);

			// Not found
			if (tmp == -1)
				return -1L;
			else
				retval += tmp;
		}
		else if (elt_type == "eval" || elt_type == "jump" || elt_type == "define_struct")
			;  // zero size so just do nothing to retval
		else if (elt_type == "for")
		{
			// Get the size of the FOR elt's (only) child and multiply by the number of array elts
			long child_size = get_size(child);
			if (child_size <= -1)
				return -1L;             // Child size is indeterminate, therefore so is our size

			// Handle FOR that contains a constant integer in count
			CString ss = child.GetAttr("count");
			char *endp;
			ss.TrimLeft();
			ss.TrimRight();

			// Note: we could use expression parser to handle constant expression (eg, "10+1") but is it necessary?
			long count = strtoul(ss, &endp, 10);
			// Make sure the string was not empty and there was nothing after the number (eg not "10*n")
			if (endp > (const char *)ss && endp - (const char *)ss == ss.GetLength())
				retval += count * child_size;
			else
				return -1L;             // Does not appear to be a simple number so assume it is an expression
		}
		else
			return -1L;  // xxx handle constant size IF/FOR
	}

	// Add last bit-field stg unit if there was one
	if (bits_used > 0)
	{
		ASSERT(last_size == 1 || last_size == 2 || last_size == 4 || last_size == 8);

		// Don't add stg unit if elt after the range (ie last) is part of the same unit as the previous elt
		bool add_last = true;

		if (last < ee.GetNumChildren())  // make sure we are not off the end
		{
			ASSERT(!child.IsEmpty());
			int data_bits;

			// Check if last (elt one past the end) is part of the previous bit-field stg unit
			if (child.GetName() == "data" &&                              // data field
				child.GetAttr("type").CompareNoCase("int") == 0 &&        // integer type
				(data_bits = atoi(child.GetAttr("bits"))) > 0 &&          // bitfield
				atoi(child.GetAttr("len")) == last_size &&                // same size as previous one
				(child.GetAttr("direction") == "down") == last_down &&    // same dirn as previous one
				bits_used + data_bits <= last_size*8)
			{
				add_last = false;  // part way through a stg unit so don't add it to the size
			}
		}
		if (add_last)
			retval += last_size;
	}

	return retval;
}

// The following are used for building menu trees for selecting file variables.  Given a node in the tree
// (item_no) we can generate a tree of menus that allows selection of any valid variable, first from
// the sub-nodes and then checking siblings of each ancestor node too.

std::vector<CString> prev_defined;      // This keep track of already seen "define_struct" elts to avoid inf. recursion

// Static functions used by make_var_menu_tree (below)
static void add_menu_items(CMenu *pmenu, int &item_no, const CXmlTree::CElt &current_elt, CString name)
{
	CString ss = current_elt.GetName();    // type of this node

	// Add element name unless parent is FOR (or we are IF/SWITCH/JUMP which use child name)
	if (ss != "if" && ss != "switch" && ss != "jump" && name.Right(1) != "]")
		name += current_elt.GetAttr("name");

	if (ss == "data")
	{
		pmenu->AppendMenu(MF_ENABLED, item_no++, name);
	}
	else if (ss == "for")
	{
		// Add array access operator [] and add item(s) for sub-element
		ASSERT(current_elt.GetNumChildren() <= 1);                              // May be zero for new elt
		if (current_elt.GetNumChildren() > 0)
			add_menu_items(pmenu, item_no, current_elt.GetFirstChild(), name+"[ ]");
	}
	else if (ss == "struct")
	{
		// Add items for all sub-elements of the struct
		for (CXmlTree::CElt ee = current_elt.GetFirstChild(); !ee.IsEmpty(); ++ee)
		{
			add_menu_items(pmenu, item_no, ee, name+".");
		}
	}
	else if (ss == "use_struct")
	{
		CString ss = current_elt.GetAttr("type_name");            // Find struct name to use
		CXmlTree::CElt ee = current_elt.GetOwner()->GetRoot().GetFirstChild();
		for ( ; !ee.IsEmpty() && ee.GetName() == "define_struct"; ++ee)
			if (ss == ee.GetAttr("type_name"))
			{
				// Check if recursive call to same defined struct
				for (std::vector<CString>::const_iterator ps = prev_defined.begin(); ps != prev_defined.end(); ++ps)
				{
					if (*ps == ss)
						return;    // avoid recursion into same define_struct
				}
				prev_defined.push_back(ss);

				// Add items for all sub-elements of the struct
				for (CXmlTree::CElt ee2 = ee.GetFirstChild(); !ee2.IsEmpty(); ++ee2)
				{
					add_menu_items(pmenu, item_no, ee2, name+".");
				}
				break;    // Found the (hopefully) only one with the correct name
			}
	}
	else if (ss == "if")
	{
		if (current_elt.GetNumChildren() > 0)
			add_menu_items(pmenu, item_no, current_elt.GetFirstChild(), name);
		if (current_elt.GetNumChildren() > 1)
		{
			// Add a separate item for the else part
			ASSERT(current_elt.GetNumChildren() == 3 && current_elt.GetChild(1).GetName() == "else");
			add_menu_items(pmenu, item_no, current_elt.GetChild(2), name);
		}
	}
	else if (ss == "switch")
	{
		// Add names of all case sub-elements
		CStringList found;      // just used to eliminate duplicate data elts
		for (CXmlTree::CElt ee = current_elt.GetFirstChild(); !ee.IsEmpty(); ++ee)
		{
			ASSERT(ee.GetName() == "case");
			if (ee.GetFirstChild().GetName() == "data")
			{
				CString ss = ee.GetFirstChild().GetAttr("name");
				if (!ss.IsEmpty())
				{
					if (found.Find(ss) != NULL)
						continue;           // ignore data elements with the same name
					else
						found.AddTail(ss);  // store this one for later checks
				}
			}
			add_menu_items(pmenu, item_no, ee.GetFirstChild(), name);
		}
	}
	else if (ss == "jump")
	{
		if (current_elt.GetNumChildren() > 0)
			add_menu_items(pmenu, item_no, current_elt.GetFirstChild(), name);
	}
	else if (ss == "define_struct" || ss == "eval")
	{
		// Do nothing here
	}
	else
		ASSERT(0);
}

static CMenu *make_menu(const CXmlTree::CElt &current_elt, int &item_no)
{
	CMenu * retval = new CMenu;
	retval->CreatePopupMenu();
	ASSERT(prev_defined.size() == 0);   // Make sure the list is cleared

	// There is no point in doing siblings of IF/SWITCH since only one element actually
	// exists - ie, if we are evaluating an element then the siblings will not exist.
	if (current_elt.GetParent().GetName() == "if" || current_elt.GetParent().GetName() == "switch")
		return retval;

	// Check all "older" siblings
	CXmlTree::CElt ee = current_elt;
	for (ee--; !ee.IsEmpty(); ee--)
	{
		add_menu_items(retval, item_no, ee, "");
	}

	prev_defined.clear();
	return retval;
}

// Make a whole menu system that shows all the variables that can be accessed
// by an element in an expression.
// Returns a pointer to a CMenu that must be destroyed and freed, unless NULL
// is returned in which case no variables were found.
CMenu *make_var_menu_tree(const CXmlTree::CElt &current_elt, bool this_too /*=false*/, bool file_too /*=false*/)
{
	int item_no = 1;

	// Get sibling items
	CMenu *retval = make_menu(current_elt, item_no);

	// Add "this" if required
	if (this_too)
	{
		ASSERT(current_elt.GetName() != "if" && current_elt.GetName() != "switch");
		ASSERT(current_elt.GetNumChildren() <= 1);
		ASSERT(prev_defined.size() == 0);   // Make sure the list is cleared
		if (current_elt.GetNumChildren() > 0)
			add_menu_items(retval, item_no, current_elt.GetFirstChild(), _T("this"));
		else
			retval->AppendMenu(MF_ENABLED, item_no++, _T("this"));
		prev_defined.clear();
	}

	// Added in 3.5 - ability to use file values (cursor, mark, eof) in expressions
	if (file_too)
	{
		CMenu * submenu = new CMenu;
		submenu->CreatePopupMenu();

		submenu->AppendMenu(MF_ENABLED, item_no++, _T("cursor"));
		submenu->AppendMenu(MF_ENABLED, item_no++, _T("mark"));
		submenu->AppendMenu(MF_ENABLED, item_no++, _T("eof"));
		retval->AppendMenu(MF_POPUP, (UINT)submenu->m_hMenu, _T("From File"));
	}

	// Now check each ancestor in turn and add sub-menu (if not empty)
	CXmlTree::CElt ee;                  // Node of ancestor (starting with parent)
	int ii;                             // Keep track of which ancestor (1 = parent)
	for (ee = current_elt.GetParent(), ii = 1;
		 ee.GetName() != "binary_file_format";
		 ee = ee.GetParent(), ++ii )
	{
		CMenu *submenu = make_menu(ee, item_no);
		if (submenu->GetMenuItemCount() > 0)
		{
			// Add this menu as a submenu (popup)
			CString submenu_name;
			if (ii == 1)
				submenu_name = "Parent siblings";
			else if (ii == 2)
				submenu_name = "Grandparent siblings";
			else
				submenu_name.Format("Ancestor [%d] siblings", ii);

			retval->AppendMenu(MF_POPUP, (UINT)submenu->m_hMenu, submenu_name);
		}
		else
			submenu->DestroyMenu();
		delete submenu;
	}

	// If we added nothing just put a dummy (disabled) menu item in there
	if (retval->GetMenuItemCount() == 0)
		retval->AppendMenu(MF_GRAYED, -1, _T("No values available"));

	return retval;
}

bool valid_id(LPCTSTR name)
{
	if (!isalpha(name[0]))
		return false;

	if (strlen(name) >
		strspn(name, "abcdefghijklmonpqrstuvwxyz"
					 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
					 "0123456789_$"))
		return false;

	return true;
}

// xxx TBD handle whitespace including CR/LF
// Takes a string in the form of an C/C++ enum content such as "{NONE, RED=2, GREEN}"
// and returns a vector of name/value (Eg NONE/0, RED/2, GREEN/3).
// If any errors are found in the string false is returned and the vector is empty.
bool get_enum(LPCTSTR pp, std::vector<enum_entry> &retval)
{
	retval.clear();

	// Make sure the enum string starts with the flag character {
	if (pp[0] != '{') return false;

	// Get rid of the trailing } so we just have the enum strings
	const char *pend = strchr(pp, '}');
	if (pend == NULL) return false;

	CString ss(pp+1, pend-pp-1);

	__int64 enum_val = 0;
	for (int ii = 0; ; ++ii)
	{
		CString entry;                  // One enum entry (comma separated)
		enum_entry ee;                  // Entry to add to the returned vector
		int eq_pos;                     // Posn of equals sign in the entry

		// Get a single enum entry and break from loop if no more
		if (!AfxExtractSubString(entry, ss, ii, ','))
			break;

		entry.TrimLeft();
		entry.TrimRight();
		if ((eq_pos = entry.Find('=')) == -1)
		{
			if (!valid_id(entry))
			{
				retval.clear();
				return false;
			}
			ee.name = entry;
		}
		else
		{
			// Get separate name and value
			ee.name = entry.Left(eq_pos);
			ee.name.TrimRight();
			if (!valid_id(ee.name))
			{
				retval.clear();
				return false;
			}
			enum_val = _atoi64(entry.Mid(eq_pos + 1));
		}

		ee.value = enum_val;
		retval.push_back(ee);
		//TRACE2("ENUM: %s=%ld\n", ee.name, long(enum_val));

		++enum_val;
	}

	return true;
}

// Find an entry in an enum vector.  Returns a ptr to it or end() if not found.
std::vector<enum_entry>::const_iterator find_enum(__int64 val, const std::vector<enum_entry> &vv)
{
	std::vector<enum_entry>::const_iterator retval;

	for (retval = vv.begin(); retval != vv.end(); ++retval)
	{
		if (retval->value == val)
			break;
	}
	
	return retval;
}
