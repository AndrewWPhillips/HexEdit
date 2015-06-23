// DFFDStruct.cpp : implementation file
//
// Copyright (c) 2015 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#include "stdafx.h"
#include "hexedit.h"
#include "HexEditDoc.h"
#include "DataFormatView.h"
#include "DFFDStruct.h"
#include "DFFDUseStruct.h"
#include "DFFDFor.h"
#include "DFFDIf.h"
#include "DFFDSwitch.h"
#include "DFFDJump.h"
#include "DFFDData.h"
#include "DFFDEval.h"
#include "DFFDMisc.h"
#include "resource.hm"      // Help IDs
#include "HelpID.hm"            // User defined help IDs

#include <HtmlHelp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDFFDStruct dialog


CDFFDStruct::CDFFDStruct(CXmlTree::CElt *pp, signed char parent_type,
						 CWnd* pParent /*=NULL*/)
	: CDialog(CDFFDStruct::IDD, pParent), saved_(pp->GetOwner())
{
	modified_ = false;
	first_ = false;                     // Default to first (see SetPosition())

	pelt_ = pp;
	parent_type_ = parent_type;

	saved_.SaveKids(pelt_);
	saved_mod_flag_ = pelt_->GetOwner()->IsModified();
	show_prev_next_ = !pParent->IsKindOf(RUNTIME_CLASS(CDataFormatView)) &&
					  (parent_type == CHexEditDoc::DF_FILE ||
					   parent_type == CHexEditDoc::DF_STRUCT ||
					   parent_type == CHexEditDoc::DF_DEFINE_STRUCT ||
					   parent_type == CHexEditDoc::DF_SWITCH ||
					   parent_type == CHexEditDoc::DF_UNION);

	pos_.x = pos_.y = -30000;  // later set with SetPosition

	//{{AFX_DATA_INIT(CDFFDStruct)
	comment_ = _T("");
	name_ = _T("");
	type_name_ = _T("");
	//}}AFX_DATA_INIT
	expr_ = _T("");
}


void CDFFDStruct::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDFFDStruct)
	DDX_Control(pDX, IDC_DFFD_DOWN, ctl_down_);
	DDX_Control(pDX, IDC_DFFD_UP, ctl_up_);
	DDX_Control(pDX, IDC_DFFD_NAME, ctl_name_);
	DDX_Control(pDX, IDC_DFFD_ELEMENTS, ctl_elements_);
	DDX_Control(pDX, IDC_DFFD_INSERT, ctl_insert_);
	DDX_Control(pDX, IDC_DFFD_EDIT, ctl_edit_);
	DDX_Control(pDX, IDC_DFFD_DELETE, ctl_delete_);
	DDX_Control(pDX, ID_DFFD_PREV, ctl_prev_);
	DDX_Control(pDX, ID_DFFD_NEXT, ctl_next_);
	DDX_Text(pDX, IDC_DFFD_COMMENTS, comment_);
	DDX_Text(pDX, IDC_DFFD_NAME, name_);
	DDX_Text(pDX, IDC_DFFD_TYPE_NAME, type_name_);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_DFFD_TYPE_NAME, ctl_type_name_);
	DDX_Text(pDX, IDC_DFFD_TREE_DISPLAY, expr_);
}


BEGIN_MESSAGE_MAP(CDFFDStruct, CDialog)
	//{{AFX_MSG_MAP(CDFFDStruct)
	ON_BN_CLICKED(ID_DFFD_NEXT, OnNext)
	ON_BN_CLICKED(ID_DFFD_PREV, OnPrev)
	ON_EN_CHANGE(IDC_DFFD_NAME, OnChange)
	ON_BN_CLICKED(IDC_DFFD_INSERT, OnInsert)
	ON_BN_CLICKED(IDC_DFFD_DELETE, OnDelete)
	ON_BN_CLICKED(IDC_DFFD_EDIT, OnEdit)
	ON_BN_CLICKED(IDC_DFFD_UP, OnUp)
	ON_BN_CLICKED(IDC_DFFD_DOWN, OnDown)
	ON_EN_CHANGE(IDC_DFFD_TYPE_NAME, OnChange)
	ON_EN_CHANGE(IDC_DFFD_COMMENTS, OnChange)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
	ON_CBN_DBLCLK(IDC_DFFD_ELEMENTS, OnCaseDClick)
	ON_WM_CONTEXTMENU()
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_BN_CLICKED(ID_DFFD_HELP, OnHelp)
	ON_EN_CHANGE(IDC_DFFD_TREE_DISPLAY, OnChange)
END_MESSAGE_MAP()

void CDFFDStruct::load_data()
{
	// Get values from XML data node
	ASSERT(pelt_ != NULL && (pelt_->GetName() == "binary_file_format" || 
							 pelt_->GetName() == "struct" || 
							 pelt_->GetName() == "define_struct" || 
							 pelt_->GetName() == "union"));
	if (pelt_->GetName() != "define_struct")
		name_ = pelt_->GetAttr("name");

	// Fill in list of elements
	int item = -1;                      // Index of last item added
	for (CXmlTree::CElt ee(pelt_->GetFirstChild()); !ee.IsEmpty(); ++ee)
	{
		// Add this child to the CListBox
		item = ctl_elements_.AddString(get_name(ee));
		ASSERT(item > -1);
	}
	ctl_elements_.AddString(CString(""));
	ctl_elements_.SetCurSel(0);

	type_name_ = pelt_->GetAttr("type_name");
	comment_ = pelt_->GetAttr("comment");
	expr_ = pelt_->GetAttr("expr");
	UpdateData(FALSE);
}

bool CDFFDStruct::check_data()
{
	// Get values from the controls and validate them
	UpdateData();

	if (pelt_->GetName() == "define_struct")
	{
		// Make sure a type name is given
		if (type_name_.IsEmpty())
		{
			TaskMessageBox("Type Name Required", "A DEFINE STRUCT element requires a type name.  Please enter a type name for this element");
			ctl_type_name_.SetFocus();
			return false;
		}
		if (!valid_id(type_name_))
		{
			TaskMessageBox("Invalid type name", "Please use alphanumeric characters (or underscore) "
						   "for the type name and start the name with an alphabetic character.");
			ctl_type_name_.SetFocus();
			return false;
		}

		// Check that the name is not the same as any siblings
		CXmlTree::CElt ee(*pelt_);

		// Check define struct siblings above for same name
		for (ee--; !ee.IsEmpty() && ee.GetName() == "define_struct"; ee--)
		{
			if (ee.GetAttr("type_name") == type_name_)
			{
				TaskMessageBox("Name In Use", "This element has a sibling with the same type name.");
				ctl_type_name_.SetFocus();
				return false;
			}
		}

		ee = *pelt_;

		// Check define struct siblings below for same name
		for (++ee; !ee.IsEmpty() && ee.GetName() == "define_struct"; ++ee)
		{
			if (ee.GetAttr("type_name") == type_name_)
			{
				TaskMessageBox("Name In Use", "This element has a sibling with the same type name.");
				ctl_type_name_.SetFocus();
				return false;
			}
		}
	}
	else
	{
		// Make sure name is given unless the container is a FOR (whence sub-elements are accessed via array index)
		if (name_.IsEmpty() && parent_type_ != CHexEditDoc::DF_FORV && parent_type_ != CHexEditDoc::DF_FORF)
		{
			TaskMessageBox("No Name", "Please enter a name for this element");
			ctl_name_.SetFocus();
			return false;
		}

		if (!name_.IsEmpty() && !valid_id(name_))
		{
			TaskMessageBox("Invalid STRUCT Name", "Please use alphanumeric characters (or "
							"underscores) and begin the name with an alphabetic character.");
			ctl_name_.SetFocus();
			return false;
		}
		if (name_.CompareNoCase("true") == 0 ||
			name_.CompareNoCase("false") == 0 ||
			name_.CompareNoCase("end") == 0 ||
			expr_eval::func_token(name_) != expr_eval::TOK_NONE)
		{
			TaskMessageBox("Reserved Name", name_ + " is reserved for internal use. Please choose another name.");
			ctl_name_.SetFocus();
			return false;
		}

		// We can only have an empty name if parent is FOR in which case there are no siblings
		ASSERT(!name_.IsEmpty() || pelt_->GetParent().GetNumChildren() == 1);

		// Check that the name is not the same as any siblings
		CXmlTree::CElt ee;

		// First find the actual element whose siblings we want to check (an IF takes the name of its child)
		for (ee = *pelt_; ee.GetParent().GetName() == "if" || ee.GetParent().GetName() == "jump"; ee = ee.GetParent())
			; // empty loop body

		// Check siblings above for same name
		for (ee--; !ee.IsEmpty(); ee--)
		{
			CXmlTree::CElt ee2;

			for (ee2 = ee; !ee2.IsEmpty() && (ee2.GetName() == "if" || ee2.GetName() == "jump"); ee2 = ee2.GetFirstChild())
				; // empty loop body

			if (!ee2.IsEmpty() && ee2.GetAttr("name") == name_)
			{
				TaskMessageBox("Name in use", name_ + " has a sibling with the same name.\n\n"
							   "It is not be possible to differentiate between two elements "
							   "with the same name at the same level (eg, in expressions).");
				ctl_name_.SetFocus();
				return false;
			}
		}

		// First find the actual element whose siblings we want to check (an IF takes the name of its child)
		for (ee = *pelt_; ee.GetParent().GetName() == "if" || ee.GetParent().GetName() == "jump"; ee = ee.GetParent())
			; // empty loop body

		// Check siblings below for same name
		for (++ee; !ee.IsEmpty(); ++ee)
		{
			CXmlTree::CElt ee2;
			for (ee2 = ee; ee2.GetName() == "if" || ee2.GetName() == "jump"; ee2 = ee2.GetFirstChild())
				; // nothing here
			if (ee2.GetAttr("name") == name_)
			{
				TaskMessageBox("Name in use", name_ + " has a sibling with the same name.\n\n"
							   "It is not be possible to differentiate between two elements "
							   "with the same name at the same level (eg, in expressions).");
				ctl_name_.SetFocus();
				return false;
			}
		}
	}

	// Make sure there is at least one (not a struct-definition or eval) element
	int count = 0;
	for (CXmlTree::CElt ee(pelt_->GetFirstChild()); !ee.IsEmpty(); ++ee)
		if (ee.GetName() != "define_struct" && ee.GetName() != "eval")
			++count;
	if (count == 0)
	{
		// There is only an empty trailing entry
		TaskMessageBox("No Elements", "A STRUCT requires at least one sub-element.\n");
		ctl_elements_.SetFocus();
		return false;
	}

	return true;
}

void CDFFDStruct::save_data()
{
	// Save data to XML element node
	if (pelt_->GetName() != "define_struct")
		pelt_->SetAttr("name", name_);

	pelt_->SetAttr("type_name", type_name_);
	pelt_->SetAttr("comment", comment_);
	pelt_->SetAttr("expr", expr_);
}

// The parameter says to delete the current element if the dialog is cancelled (but it will not
// be deleted if the prev/next button has been used even if we are back on the same sub-element).
void CDFFDStruct::do_edit(bool delete_on_cancel /*=false*/)
{
	// Find out which child we are editing
	int item = ctl_elements_.GetCurSel();
	ASSERT(item > -1 && item < ctl_elements_.GetCount()-1);
	if (item < 0 || item >= ctl_elements_.GetCount()-1) return;

	bool continue_edit = true;        // For loop that allows the user to cycle through editing siblings
	bool changes_made = false;
	CXmlTree::CElt current_elt = pelt_->GetChild(long(item));

	// Work out where to put the dialog
	pos_ = update_posn(this);
	if (first_) orig_ = pos_;
	CPoint pt = get_posn(this, orig_);

	int dlg_ret = IDCANCEL;
	while (continue_edit)
	{
		CString elt_type = current_elt.GetName();

		if (elt_type == "struct" || elt_type == "define_struct")
		{
			// Edit the struct element node
			CDFFDStruct dlg(&current_elt, CHexEditDoc::DF_STRUCT, this);
			if (delete_on_cancel)
				dlg.SetModified();      // Force validation as it is new & incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
			dlg.GetPosition(pt);
			if (dlg_ret != IDCANCEL && dlg.IsModified())
				changes_made = true;
		}
		else if (elt_type == "use_struct")
		{
			// Edit the struct element node
			CDFFDUseStruct dlg(&current_elt, CHexEditDoc::DF_STRUCT, this);
			if (delete_on_cancel)
				dlg.SetModified();      // Force validation as it is new & incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
			dlg.GetPosition(pt);
			if (dlg_ret != IDCANCEL && dlg.IsModified())
				changes_made = true;
		}
		else if (elt_type == "for")
		{
			// Edit the for element node
			CDFFDFor dlg(&current_elt, CHexEditDoc::DF_STRUCT, this);
			if (delete_on_cancel)
				dlg.SetModified();      // Force validation as it is new & incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
			dlg.GetPosition(pt);
			if (dlg_ret != IDCANCEL && dlg.IsModified())
				changes_made = true;
		}
		else if (elt_type == "if")
		{
			// Edit the if element node
			CDFFDIf dlg(&current_elt, CHexEditDoc::DF_STRUCT, this);
			if (delete_on_cancel)
				dlg.SetModified();      // Force validation as it is new & incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
			dlg.GetPosition(pt);
			if (dlg_ret != IDCANCEL && dlg.IsModified())
				changes_made = true;
		}
		else if (elt_type == "switch")
		{
			// Edit the switch element node
			CDFFDSwitch dlg(&current_elt, CHexEditDoc::DF_STRUCT, this);
			if (delete_on_cancel)
				dlg.SetModified();      // Force validation as it is new & incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
			dlg.GetPosition(pt);
			if (dlg_ret != IDCANCEL && dlg.IsModified())
				changes_made = true;
		}
		else if (elt_type == "jump")
		{
			// Edit the jump element node
			CDFFDJump dlg(&current_elt, CHexEditDoc::DF_STRUCT, this);
			if (delete_on_cancel)
				dlg.SetModified();      // Force validation as it is new & incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
			dlg.GetPosition(pt);
			if (dlg_ret != IDCANCEL && dlg.IsModified())
				changes_made = true;
		}
		else if (elt_type == "eval")
		{
			// Edit the eval element node
			CDFFDEval dlg(&current_elt, CHexEditDoc::DF_STRUCT, this);
			if (delete_on_cancel)
				dlg.SetModified();      // Force validation as it is new & incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
			dlg.GetPosition(pt);
			if (dlg_ret != IDCANCEL && dlg.IsModified())
				changes_made = true;
		}
		else if (elt_type == "data")
		{
			// Edit the data element node
			CDFFDData dlg(&current_elt, CHexEditDoc::DF_STRUCT, this);
			if (delete_on_cancel)
				dlg.SetModified();      // Force validation as it is new & incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
			dlg.GetPosition(pt);
			if (dlg_ret != IDCANCEL && dlg.IsModified())
				changes_made = true;
		}

		// Update the name in the list in case it has been modified
		if (changes_made)
		{
			int item = ctl_elements_.GetCurSel();
			ctl_elements_.InsertString(item, get_name(current_elt));
			ctl_elements_.DeleteString(item+1);
			ctl_elements_.SetCurSel(item);
			modified_ = true;
		}

		// If the user pressed the next or previous button start editing the sibling
		if (dlg_ret == ID_DFFD_PREV)
		{
			// Edit previous sibling
			ASSERT(ctl_elements_.GetCurSel() > 0);
			ctl_elements_.SetCurSel(ctl_elements_.GetCurSel()-1);
			current_elt--;
			ASSERT(!current_elt.IsEmpty());

			delete_on_cancel = false;
		}
		else if (dlg_ret == ID_DFFD_NEXT)
		{
			// Edit next sibling
			ASSERT(ctl_elements_.GetCurSel() != LB_ERR &&
				   ctl_elements_.GetCurSel() < ctl_elements_.GetCount()-2);
			ctl_elements_.SetCurSel(ctl_elements_.GetCurSel()+1);
			++current_elt;
			ASSERT(!current_elt.IsEmpty());

			delete_on_cancel = false;
		}
		else if (dlg_ret == ID_DFFD_CLONE)
		{
			// Create a clone of this element straight after it
			CXmlTree::CElt next_elt = current_elt;   ++next_elt;    // Get insert posn
			CXmlTree::CElt new_elt = pelt_->InsertClone(current_elt, next_elt.IsEmpty() ? NULL : &next_elt);
			CString ss = new_elt.GetAttr("name");
			if (!ss.IsEmpty())
			{
				ss = "copy_of_" + ss;
				new_elt.SetAttr("name", ss);
			}

			// Add new element into list box
			int item = ctl_elements_.GetCurSel();
			ASSERT(item != LB_ERR && item < ctl_elements_.GetCount()-1);
			ctl_elements_.InsertString(item + 1, ::get_name(new_elt));
			ctl_elements_.SetCurSel(item + 1);

			// Now edit the new element (next sibling)
			++current_elt;
			ASSERT(!current_elt.IsEmpty());

			delete_on_cancel = false;
			modified_ = true;                   // We have added a new elt (even if user clicked cancel in data dlg for clone)
		}
		else if(dlg_ret == IDOK)
		{
			continue_edit = false;
			delete_on_cancel = false;
		}
		else
		{
			ASSERT(dlg_ret == IDCANCEL);
			continue_edit = false;
		}
	}

	if (delete_on_cancel)
	{
		ASSERT(dlg_ret == IDCANCEL);

		// Cancel hit when inserting element (and OK/prev/next buttons never used)
		pelt_->DeleteChild(current_elt);        // Remove inserted elt from the XML tree
		ctl_elements_.DeleteString(item);       // Remove from the list box
		ASSERT(item < ctl_elements_.GetCount());
		ctl_elements_.SetCurSel(item);
	}
	else
	{
		ASSERT(item+1 < ctl_elements_.GetCount());
//        ctl_elements_.SetCurSel(item+1);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDFFDStruct message handlers

BOOL CDFFDStruct::OnInitDialog()
{
	CDialog::OnInitDialog();
	SetIcon(AfxGetApp()->LoadIcon(IDI_STRUCT), TRUE);

	if (pelt_->GetName() == "define_struct")
	{
		ASSERT(GetDlgItem(IDC_DFFD_NAME_DESC) != NULL);
		ASSERT(GetDlgItem(IDC_DFFD_TYPE_NAME_DESC) != NULL);

		SetWindowText(_T("STRUCT DEFINITION"));
		ctl_name_.ShowWindow(SW_HIDE);
		GetDlgItem(IDC_DFFD_NAME_DESC)->ShowWindow(SW_HIDE);

		// Move type name control to top (where name was)
		CRect rct;
		ctl_name_.GetWindowRect(&rct);
		ScreenToClient(&rct);
		ctl_type_name_.MoveWindow(&rct);
		GetDlgItem(IDC_DFFD_NAME_DESC)->GetWindowRect(&rct);
		ScreenToClient(&rct);
		GetDlgItem(IDC_DFFD_TYPE_NAME_DESC)->MoveWindow(&rct);
	}

	VERIFY(icon_del_ = AfxGetApp()->LoadIcon(IDI_DEL));
	ctl_delete_.SetIcon(icon_del_);
	VERIFY(icon_up_ = AfxGetApp()->LoadIcon(IDI_UP));
	ctl_up_.SetIcon(icon_up_);
	VERIFY(icon_down_ = AfxGetApp()->LoadIcon(IDI_DOWN));
	ctl_down_.SetIcon(icon_down_);

	// If window position specified
	if (pos_.y != -30000)
	{
		CRect rr;               // Rectangle where we will put the dialog
		GetWindowRect(&rr);

		// Move to where it was when it was last closed
		rr.OffsetRect(pos_.x - rr.left, pos_.y - rr.top);
		(void)::NeedsFix(rr);  // Make sure its visible

		MoveWindow(&rr);
	}

	if (!show_prev_next_)
	{
		// Hide << and >> (previous/next) buttons
		ctl_prev_.ShowWindow(SW_HIDE);
		ctl_next_.ShowWindow(SW_HIDE);
	}
	else
	{
		CXmlTree::CElt ee;
		if (parent_type_ == CHexEditDoc::DF_SWITCH)
			ee = pelt_->GetParent();
		else
			ee = *pelt_;
		ee--;
		ctl_prev_.EnableWindow(!ee.IsEmpty());

		if (parent_type_ == CHexEditDoc::DF_SWITCH)
			ee = pelt_->GetParent();
		else
			ee = *pelt_;
		++ee;
		ctl_next_.EnableWindow(!ee.IsEmpty());
	}

	load_data();

	// Setup menu button that allows insert/replace of struct/for/if/jump/eval/data
	VERIFY(button_menu_.LoadMenu(IDR_DFFD));
	CString strTemp;
	ASSERT((button_menu_.GetMenuString(3, strTemp, MF_BYPOSITION), strTemp == "ButtonInsert"));
	ASSERT((button_menu_.GetMenuString(10, strTemp, MF_BYPOSITION), strTemp == "RootInsert"));

//  button_menu_.GetSubMenu(3)->EnableMenuItem(ID_DFFD_INSERT_DEFINE_STRUCT, MF_BYCOMMAND | MF_GRAYED);

	if (pelt_->GetName() == "binary_file_format")
		ctl_insert_.m_hMenu = button_menu_.GetSubMenu(10)->GetSafeHmenu();
	else
		ctl_insert_.m_hMenu = button_menu_.GetSubMenu(3)->GetSafeHmenu();
//    ctl_insert_.SizeToContent();
	ctl_insert_.m_bOSMenu = TRUE;
	ctl_insert_.m_bStayPressed = TRUE;
	ctl_insert_.m_bRightArrow = TRUE;

	return TRUE;
}

static DWORD id_pairs[] = { 
	IDC_DFFD_NAME, HIDC_DFFD_NAME,
	IDC_DFFD_COMMENTS, HIDC_DFFD_COMMENTS,
	IDC_DFFD_TYPE_NAME, HIDC_DFFD_TYPE_NAME,
	IDC_DFFD_TREE_DISPLAY, HIDC_DFFD_TREE_DISPLAY,
	IDC_DFFD_ELEMENTS, HIDC_DFFD_ELEMENTS,
	IDC_DFFD_EDIT, HIDC_DFFD_EDIT,
	IDC_DFFD_INSERT, HIDC_DFFD_INSERT,
	IDC_DFFD_DELETE, HIDC_DFFD_DELETE,
	IDC_DFFD_UP, HIDC_DFFD_UP,
	IDC_DFFD_DOWN, HIDC_DFFD_DOWN,
	IDC_ELT_SIZE, HIDC_ELT_SIZE,
	IDC_ELT_OFFSET, HIDC_ELT_OFFSET,
	ID_DFFD_PREV, HID_DFFD_PREV,
	ID_DFFD_NEXT, HID_DFFD_NEXT,
	0,0 
};

BOOL CDFFDStruct::OnHelpInfo(HELPINFO* pHelpInfo)
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs);
	return TRUE;
}

void CDFFDStruct::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.HtmlHelpContextMenu(pWnd, id_pairs);
}

void CDFFDStruct::OnHelp()
{
	// Display help for this page
	DWORD_PTR hid = HIDD_DFFD_STRUCT;
	if (pelt_->GetName() == "define_struct")
		hid = HIDD_DFFD_STRUCT_DEFN;
	if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, hid))
		AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

void CDFFDStruct::OnCancel()
{
	pelt_->DeleteAllChildren();
	saved_.InsertKids(pelt_);
	pelt_->GetOwner()->SetModified(saved_mod_flag_);

	CDialog::OnCancel();
}

void CDFFDStruct::OnOK()
{
	if (modified_)
	{
		if (!check_data())
			return;

		save_data();
	}

	CDialog::OnOK();
}

void CDFFDStruct::OnNext()
{
	// Does the same as OnOK but returns ID_DFFD_NEXT to indicate
	// that the next sibling should now be edited
	if (modified_)
	{
		if (!check_data())
			return;

		save_data();
	}

	// Save position so sibling node's dialog can be put in the same place
	pos_ = update_posn(this);

	CDialog::EndDialog(ID_DFFD_NEXT);
}

void CDFFDStruct::OnPrev()
{
	// Does the same as OnOK but returns ID_DFFD_PREV to indicate
	// that the previous sibling should now be edited
	if (modified_)
	{
		if (!check_data())
			return;

		save_data();
	}

	// Save position so sibling node's dialog can be put in the same place
	pos_ = update_posn(this);

	CDialog::EndDialog(ID_DFFD_PREV);
}

LRESULT CDFFDStruct::OnKickIdle(WPARAM, LPARAM lCount)
{
	int bot = 0, top = ctl_elements_.GetCount()-1;
	int item = ctl_elements_.GetCurSel();
	if (item != -1)
	{
		CXmlTree::CElt ee = pelt_->GetChild(long(item));

		if (ee.IsEmpty())
			item = -1;
		else if (ee.GetName() == "define_struct")
		{
			top = item;
			while (!ee.IsEmpty() && ee.GetName() == "define_struct")
				++ee, ++top;
			ASSERT(top < ctl_elements_.GetCount());
		}
		else
		{
			bot = item + 1;
			while (!ee.IsEmpty() && ee.GetName() != "define_struct")
				ee--, bot--;
			ASSERT(bot >= 0);
		}
	}

	ctl_up_.EnableWindow(item != -1 && item > bot);
	ctl_down_.EnableWindow(item != -1 && item < top-1);
	ctl_edit_.EnableWindow(item != -1);
	ctl_delete_.EnableWindow(item != -1);

	long len;                           // Length or offset of struct or member
	CString ss;                         // Temp string for writing to read-only text box

	// Get size of current child
	if (item == -1 || (len = get_size(*pelt_, item, item+1)) < 0)
		ss = "?";
	else
		ss.Format("%ld", long(len));
	ASSERT(GetDlgItem(IDC_ELT_SIZE) != NULL);
	GetDlgItem(IDC_ELT_SIZE)->SetWindowText(ss);

	// Get offset of current child
	len = get_size(*pelt_, 0, item == -1 ? 999999999 : item);    // Size of all elts up to this one
	if (len < 0)
		ss = "?";
	else
		ss.Format("%ld", long(len));
	ASSERT(GetDlgItem(IDC_ELT_OFFSET) != NULL);
	GetDlgItem(IDC_ELT_OFFSET)->SetWindowText(ss);

	return FALSE;
}

void CDFFDStruct::OnChange()
{
	modified_ = true;
}

void CDFFDStruct::OnCaseDClick()
{
	// Double-click edits the sub-item unless we're at the last (empty) item
	if (ctl_elements_.GetCurSel() < ctl_elements_.GetCount()-1)
		do_edit();
}

void CDFFDStruct::OnEdit()
{
	do_edit();
}

void CDFFDStruct::OnInsert()
{
	// Find out which child we are inserting before
	int item = ctl_elements_.GetCurSel();
	ASSERT(item < ctl_elements_.GetCount());

	// Check menu item selected to see what type of element to insert
	int elt_id = ctl_insert_.m_nMenuResult;

	CXmlTree::CElt ee;               // New element that is added
	CXmlTree::CElt before;           // Element to insert before
	const CXmlTree::CElt *pbefore = NULL;      // Ptr to element to insert before

	// Work out where to insert the new item
	if (elt_id == ID_DFFD_INSERT_DEFINE_STRUCT)
	{
		// xxx error if list is empty
		before = pelt_->GetFirstChild();    // Structure definitions must be before other elements
		pbefore = &before;
		item = 0;
	}
	else if (item == -1 || item == ctl_elements_.GetCount()-1)
	{
		pbefore = NULL;                     // Add to end
		item = ctl_elements_.GetCount()-1;
	}
	else
	{
		// Add at current position bu make sure it is after all structure definitions
		before = pelt_->GetChild(long(item));
		while (!before.IsEmpty() && before.GetName() == "define_struct")
		{
			++before;
			++item;
		}
		if (!before.IsEmpty())
			pbefore = &before;
		else
			pbefore = NULL;
	}

	switch (elt_id)
	{
	case ID_DFFD_INSERT_DEFINE_STRUCT:
		ee = pelt_->InsertNewChild("define_struct", pbefore);
		item = ctl_elements_.InsertString(item, "[STRUCT DEFN]");
		break;
	case ID_DFFD_INSERT_STRUCT:
		ee = pelt_->InsertNewChild("struct", pbefore);
		item = ctl_elements_.InsertString(item, "[STRUCT]");
		break;
	case ID_DFFD_INSERT_USE_STRUCT:
		ee = pelt_->InsertNewChild("use_struct", pbefore);
		item = ctl_elements_.InsertString(item, "[USING STRUCT]");
		break;
	case ID_DFFD_INSERT_FOR:
		ee = pelt_->InsertNewChild("for", pbefore);
		item = ctl_elements_.InsertString(item, "[FOR]");
		break;
	case ID_DFFD_INSERT_IF:
		ee = pelt_->InsertNewChild("if", pbefore);
		ee.SetAttr("test", "true");
		item = ctl_elements_.InsertString(item, "[IF]");
		break;
	case ID_DFFD_INSERT_SWITCH:
		ee = pelt_->InsertNewChild("switch", pbefore);
		//ee.SetAttr("test", "0");
		item = ctl_elements_.InsertString(item, "[CASE]");
		break;
	case ID_DFFD_INSERT_JUMP:
		ee = pelt_->InsertNewChild("jump", pbefore);
		ee.SetAttr("offset", "0");
		item = ctl_elements_.InsertString(item, "[JUMP]");
		break;
	case ID_DFFD_INSERT_EVAL:
		ee = pelt_->InsertNewChild("eval", pbefore);
		ee.SetAttr("expr", "true");
		item = ctl_elements_.InsertString(item, "[EXPR]");
		break;
	case ID_DFFD_INSERT_DATA:
		ee = pelt_->InsertNewChild("data", pbefore);
		ee.SetAttr("type", "none");
		item = ctl_elements_.InsertString(item, "[DATA]");
		break;
	default:
		ASSERT(0);
	}
	ctl_elements_.SetCurSel(item);

	do_edit(true);
}

void CDFFDStruct::OnDelete()
{
	// Find out which child we are deleting
	int item = ctl_elements_.GetCurSel();
	ASSERT(item > -1 && item < ctl_elements_.GetCount()-1);
	if (item < 0 || item >= ctl_elements_.GetCount()-1) return;

	// Check with the user that they really want to delete
	if (TaskMessageBox("Delete STRUCT?", "The STRUCT and all sub-elements will be removed.\n\n"
	                   "Are you sure you want to delete this element?", MB_YESNO) != IDYES)
		return;

	// Delete the node from the XML document
	pelt_->DeleteChild(pelt_->GetChild(long(item)));

	// Delete the node from the list box
	ctl_elements_.DeleteString(item);
	ctl_elements_.SetCurSel(item);

	modified_ = true;
}

void CDFFDStruct::OnUp()
{
	// Find out which child to move
	int item = ctl_elements_.GetCurSel();
	ASSERT(item > 0 && item < ctl_elements_.GetCount()-1);
	if (item < 1 || item >= ctl_elements_.GetCount()-1) return;

	// Move the node
	CXmlTree::CElt ee = pelt_->GetChild(long(item-1));
	pelt_->InsertChild(pelt_->GetChild(long(item)), &ee);

	// Move in the list control too
	CString ss;
	ctl_elements_.GetText(item, ss);
	ctl_elements_.DeleteString(item);
	ctl_elements_.InsertString(item-1, ss);
	ctl_elements_.SetCurSel(item-1);

	modified_ = true;
}

void CDFFDStruct::OnDown()
{
	// Find out which child to move
	int item = ctl_elements_.GetCurSel();
	ASSERT(item > -1 && item < ctl_elements_.GetCount()-2);
	if (item < 0 || item >= ctl_elements_.GetCount()-2) return;

	// Move the node
	if (item == ctl_elements_.GetCount()-3)     // 2nd last item which is last that can move down
		pelt_->InsertChild(pelt_->GetChild(long(item)), NULL);
	else
	{
		CXmlTree::CElt ee = pelt_->GetChild(long(item+2));
		pelt_->InsertChild(pelt_->GetChild(long(item)), &ee);
	}

	// Move in the list control too
	CString ss;
	ctl_elements_.GetText(item, ss);
	ctl_elements_.DeleteString(item);
	ctl_elements_.InsertString(item+1, ss);
	ctl_elements_.SetCurSel(item+1);

	modified_ = true;
}
