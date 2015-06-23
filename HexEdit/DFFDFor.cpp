// DFFDFor.cpp : implementation file
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
// CDFFDFor dialog


CDFFDFor::CDFFDFor(CXmlTree::CElt *pp, signed char parent_type, 
				   CWnd* pParent /*=NULL*/)
	: CDialog(CDFFDFor::IDD, pParent), saved_(pp->GetOwner())
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

	pos_.x = pos_.y = -30000;
	pcount_menu_ = NULL;
	pstop_menu_ = NULL;

	//{{AFX_DATA_INIT(CDFFDFor)
	comment_ = _T("");
	count_ = _T("");
	elt_name_ = _T("");
	name_ = _T("");
	stop_ = _T("");
	type_name_ = _T("");
	//}}AFX_DATA_INIT
}

CDFFDFor::~CDFFDFor()
{
	if (pcount_menu_ != NULL)
	{
		// Destroy the menu and free the memory
		pcount_menu_->DestroyMenu();
		delete pcount_menu_;
	}
	if (pstop_menu_ != NULL)
	{
		// Destroy the menu and free the memory
		pstop_menu_->DestroyMenu();
		delete pstop_menu_;
	}
}

void CDFFDFor::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDFFDFor)
	DDX_Control(pDX, IDC_DFFD_STOP, ctl_stop_);
	DDX_Control(pDX, IDC_DFFD_COUNT, ctl_count_);
	DDX_Control(pDX, IDC_DFFD_NAME, ctl_name_);
	DDX_Control(pDX, IDC_DFFD_REPLACE, ctl_replace_);
	DDX_Control(pDX, IDC_DFFD_EDIT, ctl_edit_);
	DDX_Control(pDX, ID_DFFD_PREV, ctl_prev_);
	DDX_Control(pDX, ID_DFFD_NEXT, ctl_next_);
	DDX_Text(pDX, IDC_DFFD_COMMENTS, comment_);
	DDX_Text(pDX, IDC_DFFD_COUNT, count_);
	DDX_Text(pDX, IDC_DFFD_ELT, elt_name_);
	DDX_Text(pDX, IDC_DFFD_NAME, name_);
	DDX_Text(pDX, IDC_DFFD_STOP, stop_);
	DDX_Text(pDX, IDC_DFFD_TYPE_NAME, type_name_);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_DFFD_COUNT_VAR, ctl_count_var_);
	DDX_Control(pDX, IDC_DFFD_STOP_VAR, ctl_stop_var_);
}


BEGIN_MESSAGE_MAP(CDFFDFor, CDialog)
	//{{AFX_MSG_MAP(CDFFDFor)
	ON_BN_CLICKED(ID_DFFD_NEXT, OnNext)
	ON_BN_CLICKED(ID_DFFD_PREV, OnPrev)
	ON_EN_CHANGE(IDC_DFFD_NAME, OnChange)
	ON_BN_CLICKED(IDC_DFFD_EDIT, OnEdit)
	ON_BN_CLICKED(IDC_DFFD_REPLACE, OnReplace)
	ON_EN_CHANGE(IDC_DFFD_COUNT, OnChange)
	ON_EN_CHANGE(IDC_DFFD_STOP, OnChange)
	ON_EN_CHANGE(IDC_DFFD_TYPE_NAME, OnChange)
	ON_EN_CHANGE(IDC_DFFD_COMMENTS, OnChange)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
	ON_WM_CONTEXTMENU()
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_BN_CLICKED(ID_DFFD_HELP, OnHelp)
	ON_BN_CLICKED(IDC_DFFD_COUNT_VAR, OnGetCountVar)
	ON_BN_CLICKED(IDC_DFFD_STOP_VAR, OnGetStopVar)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDFFDFor message handlers

BOOL CDFFDFor::OnInitDialog()
{
	CDialog::OnInitDialog();
	SetIcon(AfxGetApp()->LoadIcon(IDI_FOR), TRUE);

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

	// Setup menu button that allows insert/replace of struct/for/if/jump/data
	VERIFY(button_menu_.LoadMenu(IDR_DFFD));
	CString strTemp;
	ASSERT((button_menu_.GetMenuString(4, strTemp, MF_BYPOSITION), strTemp == "ButtonReplace"));
	ctl_replace_.m_hMenu = button_menu_.GetSubMenu(4)->GetSafeHmenu();
//    ctl_replace_.SizeToContent();
	ctl_replace_.m_bOSMenu = TRUE;
	ctl_replace_.m_bStayPressed = TRUE;
	ctl_replace_.m_bRightArrow = TRUE;

	// Set up menu that allows the user to choose any valid variable name
	pcount_menu_ = make_var_menu_tree(*pelt_);
	ctl_count_var_.m_hMenu = pcount_menu_->GetSafeHmenu();
	ctl_count_var_.m_bOSMenu = TRUE;
	ctl_count_var_.m_bStayPressed = TRUE;
	ctl_count_var_.m_bRightArrow = TRUE;
	ctl_count_.SetSel(0, -1);           // Select all so a var selection (see OnGetVar) replaces current contents

	pstop_menu_ = make_var_menu_tree(*pelt_, true);
	ctl_stop_var_.m_hMenu = pstop_menu_->GetSafeHmenu();
	ctl_stop_var_.m_bOSMenu = TRUE;
	ctl_stop_var_.m_bStayPressed = TRUE;
	ctl_stop_var_.m_bRightArrow = TRUE;
	ctl_stop_.SetSel(0, -1);            // Select all so a var selection (see OnGetVar) replaces current contents

	return TRUE;
}

static DWORD id_pairs[] = { 
	IDC_DFFD_NAME, HIDC_DFFD_NAME,
	IDC_DFFD_COUNT, HIDC_DFFD_COUNT,
	IDC_DFFD_COUNT_VAR, HIDC_DFFD_COUNT_VAR,
	IDC_DFFD_STOP, HIDC_DFFD_STOP,
	IDC_DFFD_STOP_VAR, HIDC_DFFD_STOP_VAR,
	IDC_DFFD_ELT, HIDC_DFFD_ELT,
	IDC_DFFD_EDIT, HIDC_DFFD_EDIT,
	IDC_DFFD_REPLACE, HIDC_DFFD_REPLACE,
	IDC_DFFD_COMMENTS, HIDC_DFFD_COMMENTS,
	IDC_DFFD_TYPE_NAME, HIDC_DFFD_TYPE_NAME,
	ID_DFFD_PREV, HID_DFFD_PREV,
	ID_DFFD_NEXT, HID_DFFD_NEXT,
	0,0 
};

BOOL CDFFDFor::OnHelpInfo(HELPINFO* pHelpInfo)
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs);
	return TRUE;
}

void CDFFDFor::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.HtmlHelpContextMenu(pWnd, id_pairs);
}

void CDFFDFor::OnHelp()
{
	if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_DFFD_FOR))
		AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

void CDFFDFor::OnCancel()
{
	pelt_->DeleteAllChildren();
	saved_.InsertKids(pelt_);
	pelt_->GetOwner()->SetModified(saved_mod_flag_);

	CDialog::OnCancel();
}

void CDFFDFor::OnOK()
{
	if (modified_)
	{
		if (!check_data())
			return;

		save_data();
	}

	CDialog::OnOK();
}

void CDFFDFor::OnNext()
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

void CDFFDFor::OnPrev()
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

LRESULT CDFFDFor::OnKickIdle(WPARAM, LPARAM lCount)
{
	if (valid_element_)
	{
		ctl_edit_.EnableWindow(TRUE);
		CString ss;
		ctl_replace_.GetWindowText(ss);
		if (ss != "Replace with")
			ctl_replace_.SetWindowText("Replace with");
	}
	else
	{
		ctl_edit_.EnableWindow(FALSE);
		CString ss;
		ctl_replace_.GetWindowText(ss);
		if (ss != "Insert")
			ctl_replace_.SetWindowText("Insert");
	}

	return FALSE;
}

void CDFFDFor::OnChange()
{
	modified_ = true;
}

// Edit current child element (Edit button)
void CDFFDFor::OnEdit()
{
	bool new_change = false;            // Were changes made during the edit?
	CXmlTree::CElt ee(pelt_->GetFirstChild());
	ASSERT(valid_element_ && !ee.IsEmpty());

	// Work out where to put the dialog
	pos_ = update_posn(this);
	if (first_) orig_ = pos_;
	CPoint pt = get_posn(this, orig_);

	// Edit the subelement according to its type
	int dlg_ret;                        // Save value returned from dialog in case we need it

	CString elt_type = ee.GetName();
	if (elt_type == "struct")
	{
		// Edit the struct element node
		CDFFDStruct dlg(&ee, CHexEditDoc::DF_FORV, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			new_change = true;
	}
	else if (elt_type == "use_struct")
	{
		// Edit the struct element node
		CDFFDUseStruct dlg(&ee, CHexEditDoc::DF_FORV, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			new_change = true;
	}
	else if (elt_type == "for")
	{
		// Edit the for element node
		CDFFDFor dlg(&ee, CHexEditDoc::DF_FORV, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			new_change = true;
	}
	else if (elt_type == "if")
	{
		// Edit the if element node
		CDFFDIf dlg(&ee, CHexEditDoc::DF_FORV, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			new_change = true;
	}
	else if (elt_type == "switch")
	{
		// Edit the switch element node
		CDFFDSwitch dlg(&ee, CHexEditDoc::DF_FORV, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			new_change = true;
	}
	else if (elt_type == "jump")
	{
		// Edit the jump element node
		CDFFDJump dlg(&ee, CHexEditDoc::DF_FORV, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			new_change = true;
	}
	else if (elt_type == "data")
	{
		// Edit the data element node
		CDFFDData dlg(&ee, CHexEditDoc::DF_FORV, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			new_change = true;
	}
	else
		ASSERT(0);

	if (new_change)
	{
		modified_ = true;

		// Update the displayed name of the sub-element
		UpdateData(TRUE);
		elt_name_ = get_name(ee);
		UpdateData(FALSE);

		// Fix the "stop" variable menu since the "this" subelements may have changed
		pstop_menu_->DestroyMenu();
		delete pstop_menu_;
		pstop_menu_ = make_var_menu_tree(*pelt_, true);
		ctl_stop_var_.m_hMenu = pstop_menu_->GetSafeHmenu();
	}
}

void CDFFDFor::OnReplace()
{
	if (ctl_replace_.m_nMenuResult == 0)
		return;

	// See if we have a subelement to replace (otherwise we just insert)
	CXmlTree::CElt curr_elt(pelt_->GetFirstChild());
	if (!curr_elt.IsEmpty())
	{
		ASSERT(valid_element_);
		CString sub_name = get_name(curr_elt);

		CString mess;
		if (curr_elt.GetFirstChild().IsEmpty())
		{
			mess.Format("This will replace the contents of %s.  "
						"The existing contents will be deleted.\n\n"
						"Are you sure you want to do this?", sub_name);
		}
		else
		{
			mess.Format("This will replace the contents of %s\n"
						"and all elements below it.  "
						"The existing contents will be deleted.\n\n"
						"Are you sure you want to do this?", sub_name);
		}
		if (TaskMessageBox("Replacing Element", mess, MB_YESNO) != IDYES)
			return;

		pelt_->DeleteChild(curr_elt);         // Remove the replaced node from the tree
	}

	// Work out where to put the dialog
	pos_ = update_posn(this);
	if (first_) orig_ = pos_;
	CPoint pt = get_posn(this, orig_);
	int dlg_ret;
	CXmlTree::CElt ee;

	switch (ctl_replace_.m_nMenuResult)
	{
	case ID_DFFD_INSERT_STRUCT:
		ee = pelt_->InsertNewChild("struct", NULL);

		{
			CDFFDStruct dlg(&ee, CHexEditDoc::DF_FORV, this);
			dlg.SetModified();          // Force validation as it is incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
		}
		break;
	case ID_DFFD_INSERT_USE_STRUCT:
		ee = pelt_->InsertNewChild("use_struct", NULL);

		{
			CDFFDUseStruct dlg(&ee, CHexEditDoc::DF_FORV, this);
			dlg.SetModified();          // Force validation as it is incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
		}
		break;
	case ID_DFFD_INSERT_FOR:
		ee = pelt_->InsertNewChild("for", NULL);

		{
			CDFFDFor dlg(&ee, CHexEditDoc::DF_FORV, this);
			dlg.SetModified();          // Force validation as it is incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
		}
		break;
	case ID_DFFD_INSERT_IF:
		ee = pelt_->InsertNewChild("if", NULL);
		ee.SetAttr("test", "true");

		{
			CDFFDIf dlg(&ee, CHexEditDoc::DF_FORV, this);
			dlg.SetModified();          // Force validation as it is incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
		}
		break;
	case ID_DFFD_INSERT_SWITCH:
		ee = pelt_->InsertNewChild("switch", NULL);

		{
			CDFFDSwitch dlg(&ee, CHexEditDoc::DF_FORV, this);
			dlg.SetModified();          // Force validation as it is incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
		}
		break;
	case ID_DFFD_INSERT_JUMP:
		ee = pelt_->InsertNewChild("jump", NULL);
		ee.SetAttr("offset", "0");

		{
			CDFFDJump dlg(&ee, CHexEditDoc::DF_FORV, this);
			dlg.SetModified();          // Force validation as it is incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
		}
		break;
	case ID_DFFD_INSERT_DATA:
		ee = pelt_->InsertNewChild("data", NULL);
		ee.SetAttr("type", "none");

		{
			CDFFDData dlg(&ee, CHexEditDoc::DF_FORV, this);
			dlg.SetModified();          // Force validation as it is incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
		}
		break;
	default:
		ASSERT(0);
	}

	if (dlg_ret == IDCANCEL)
	{
		// Restore elts again (remove new element, add back old elt)
		(void)pelt_->DeleteChild(ee);
		if (!curr_elt.IsEmpty())
		{
			ASSERT(valid_element_);
			pelt_->InsertChild(curr_elt, NULL);
		}
		return;
	}

	modified_ = true;
	valid_element_ = true;              // Keep not that we now have a sub-element

	// Update the displayed name of the sub-element
	UpdateData(TRUE);
	elt_name_ = get_name(ee);
	UpdateData(FALSE);

	// Fix the "stop" variable menu since the "this" subelements may have changed
	pstop_menu_->DestroyMenu();
	delete pstop_menu_;
	pstop_menu_ = make_var_menu_tree(*pelt_, true);
	ctl_stop_var_.m_hMenu = pstop_menu_->GetSafeHmenu();
}

void CDFFDFor::OnGetCountVar()
{
	if (ctl_count_var_.m_nMenuResult != 0)
	{
		CString strVar = get_menu_text(pcount_menu_, ctl_count_var_.m_nMenuResult);
		ASSERT(!strVar.IsEmpty());      // The selected menu item should be found in the menu tree

		// Put the var name into the "count" control
//        CString ss;
//        int start, end;
//        ctl_count_.GetWindowText(ss);
//        ctl_count_.GetSel(start, end);
//        ASSERT(start <= end);
//        ss.Delete(start, end-start);
//        ss.Insert(start, strVar);
//        end = start + strVar.GetLength();
//        ctl_count_.SetWindowText(ss);
//        ctl_count_.SetSel(start, end);
		for (int ii = 0; ii < strVar.GetLength (); ii++)
			ctl_count_.SendMessage(WM_CHAR, (TCHAR)strVar[ii]);
		ctl_count_.SetFocus();
		modified_ = true;
	}
}

void CDFFDFor::OnGetStopVar()
{
	if (ctl_stop_var_.m_nMenuResult != 0)
	{
		CString strVar = get_menu_text(pstop_menu_, ctl_stop_var_.m_nMenuResult);
		ASSERT(!strVar.IsEmpty());      // The selected menu item should be found in the menu tree

		// Put the var name into the "stop" control
		for (int ii = 0; ii < strVar.GetLength (); ii++)
			ctl_stop_.SendMessage(WM_CHAR, (TCHAR)strVar[ii]);
		ctl_stop_.SetFocus();
		modified_ = true;
	}
}

void CDFFDFor::load_data()
{
	// Get values from XML data node
	ASSERT(pelt_ != NULL && pelt_->GetName() == "for");
	name_ = pelt_->GetAttr("name");

	count_ = pelt_->GetAttr("count");
	stop_ = pelt_->GetAttr("stop_test");

	CXmlTree::CElt ee(pelt_->GetFirstChild());
	elt_name_.Empty();                   // Clear the name of the contained element

	// Check if we have a sub-element (without which a FOR is not valid)
	if (valid_element_ = !ee.IsEmpty())
		elt_name_ = get_name(ee);       // Show sub-element name

	type_name_ = pelt_->GetAttr("type_name");
	comment_ = pelt_->GetAttr("comment");
	UpdateData(FALSE);
}

bool CDFFDFor::check_data()
{
	// Get values from the controls and validate them
	UpdateData();

	if (!valid_element_)
	{
		TaskMessageBox("No For Element", "A FOR element must contain exactly one sub-element.  "
			"This may simply be a DATA element or complex element such as a FOR or STRUCT.\n\n"
			"Please add the contained element to continue.");
		ctl_replace_.SetFocus();
		return false;
	}

	// Make sure name is given unless the container is a FOR (whence sub-elements are accessed via array index)
	if (name_.IsEmpty() && parent_type_ != CHexEditDoc::DF_FORV && parent_type_ != CHexEditDoc::DF_FORF)
	{
		TaskMessageBox("No Name", "A FOR element must have a name (unless it is nested within another FOR) so that "
			"the FOR element can be accessed in expressions.\n\nPlease enter a name for this element.");
		ctl_name_.SetFocus();
		return false;
	}

	if (!name_.IsEmpty() && !valid_id(name_))
	{
		TaskMessageBox("Invalid FOR Name", "Please use alphanumeric characters (or "
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
	for (ee = *pelt_; ee.GetParent().GetName() == "if"; ee = ee.GetParent())
		; // empty loop body

	// Check that older siblings don't have the same name
	for (ee--; !ee.IsEmpty(); ee--)
	{
		CXmlTree::CElt ee2;
		for (ee2 = ee; !ee2.IsEmpty() && ee2.GetName() == "if"; ee2 = ee2.GetFirstChild())
			; // nothing here
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
	for (ee = *pelt_; ee.GetParent().GetName() == "if"; ee = ee.GetParent())
		; // empty loop body

	// Check that younger siblings don't have the same name
	for (++ee; !ee.IsEmpty(); ++ee)
	{
		CXmlTree::CElt ee2;
		for (ee2 = ee; !ee2.IsEmpty() && ee2.GetName() == "if"; ee2 = ee2.GetFirstChild())
			; // nothing here
		if (!ee2.IsEmpty() && ee2.GetAttr("name") == name_)
		{
			TaskMessageBox("Name in use", name_ + " has a sibling with the same name.\n\n"
				           "It is not be possible to differentiate between two elements "
			               "with the same name at the same level (eg, in expressions).");
			ctl_name_.SetFocus();
			return false;
		}
	}

	// xxx TBD check that stop_ and count_ are empty or valid expressions

	return true;
}

void CDFFDFor::save_data()
{
	// Save data to XML element node
	pelt_->SetAttr("name", name_);

	pelt_->SetAttr("count", count_);
	pelt_->SetAttr("stop_test", stop_);

	pelt_->SetAttr("type_name", type_name_);
	pelt_->SetAttr("comment", comment_);
}
