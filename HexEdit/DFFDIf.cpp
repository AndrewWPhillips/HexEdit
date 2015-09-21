// DFFDIf.cpp : implementation file
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
#include "DFFDEval.h"
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
// CDFFDIf dialog


CDFFDIf::CDFFDIf(CXmlTree::CElt *pp, signed char parent_type,
				 CWnd* pParent /*=NULL*/)
	: CDialog(CDFFDIf::IDD, pParent), saved_(pp->GetOwner())
{
	modified_ = false;
	first_ = false;                     // Default to first (see SetPosition())

	pelt_ = pp;
	parent_type_ = parent_type;

	// Save info in case Cancel button pressed
	saved_.SaveKids(pelt_);
	saved_mod_flag_ = pelt_->GetOwner()->IsModified();

	show_prev_next_ = !pParent->IsKindOf(RUNTIME_CLASS(CDataFormatView)) &&
					  (parent_type == CHexEditDoc::DF_FILE ||
					   parent_type == CHexEditDoc::DF_STRUCT ||
					   parent_type == CHexEditDoc::DF_DEFINE_STRUCT ||
					   parent_type == CHexEditDoc::DF_SWITCH ||
					   parent_type == CHexEditDoc::DF_UNION);

	pos_.x = pos_.y = -30000;
	pcondition_menu_ = NULL;

	//{{AFX_DATA_INIT(CDFFDIf)
	comment_ = _T("");
	condition_ = _T("");
	//}}AFX_DATA_INIT
	if_elt_name_ = _T("");
	else_elt_name_ = _T("");
	else_ = FALSE;
}

CDFFDIf::~CDFFDIf()
{
	if (pcondition_menu_ != NULL)
	{
		// Destroy the menu and free the memory
		pcondition_menu_->DestroyMenu();
		delete pcondition_menu_;
	}
}

void CDFFDIf::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDFFDIf)
	DDX_Control(pDX, ID_DFFD_PREV, ctl_prev_);
	DDX_Control(pDX, ID_DFFD_NEXT, ctl_next_);
	DDX_Text(pDX, IDC_DFFD_COMMENTS, comment_);
	DDX_Text(pDX, IDC_DFFD_CONDITION, condition_);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_DFFD_CONDITION, ctl_condition_);
	DDX_Control(pDX, IDC_DFFD_CONDITION_VAR, ctl_condition_var_);

	DDX_Text(pDX, IDC_DFFD_IF_ELT, if_elt_name_);
	DDX_Control(pDX, IDC_DFFD_IF_EDIT, ctl_if_edit_);
	DDX_Control(pDX, IDC_DFFD_IF_REPLACE, ctl_if_replace_);

	DDX_Control(pDX, IDC_DFFD_ELSE, ctl_else_);
	DDX_Check(pDX, IDC_DFFD_ELSE, else_);
	DDX_Control(pDX, IDC_DFFD_ELSE_ELT, ctl_else_elt_name_);
	DDX_Text(pDX, IDC_DFFD_ELSE_ELT, else_elt_name_);
	DDX_Control(pDX, IDC_DFFD_ELSE_EDIT, ctl_else_edit_);
	DDX_Control(pDX, IDC_DFFD_ELSE_REPLACE, ctl_else_replace_);
}

BEGIN_MESSAGE_MAP(CDFFDIf, CDialog)
	//{{AFX_MSG_MAP(CDFFDIf)
	ON_BN_CLICKED(ID_DFFD_NEXT, OnNext)
	ON_BN_CLICKED(ID_DFFD_PREV, OnPrev)
	ON_EN_CHANGE(IDC_DFFD_COMMENTS, OnChange)
	ON_EN_CHANGE(IDC_DFFD_CONDITION, OnChange)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
	ON_WM_CONTEXTMENU()
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_BN_CLICKED(ID_DFFD_HELP, OnHelp)
	ON_BN_CLICKED(IDC_DFFD_ELSE, OnElse)
	ON_BN_CLICKED(IDC_DFFD_IF_EDIT, OnIfEdit)
	ON_BN_CLICKED(IDC_DFFD_IF_REPLACE, OnIfReplace)
	ON_BN_CLICKED(IDC_DFFD_ELSE_EDIT, OnElseEdit)
	ON_BN_CLICKED(IDC_DFFD_ELSE_REPLACE, OnElseReplace)
	ON_BN_CLICKED(IDC_DFFD_CONDITION_VAR, OnGetConditionVar)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDFFDIf message handlers

BOOL CDFFDIf::OnInitDialog()
{
	CDialog::OnInitDialog();
	SetIcon(AfxGetApp()->LoadIcon(IDI_IF), TRUE);

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
	ctl_if_replace_.m_hMenu = button_menu_.GetSubMenu(3)->GetSafeHmenu();
//    ctl_if_replace_.SizeToContent();
	ctl_if_replace_.m_bOSMenu = TRUE;
	ctl_if_replace_.m_bStayPressed = TRUE;
	ctl_if_replace_.m_bRightArrow = TRUE;
	ctl_else_replace_.m_hMenu = button_menu_.GetSubMenu(3)->GetSafeHmenu();
//    ctl_else_replace_.SizeToContent();
	ctl_else_replace_.m_bOSMenu = TRUE;
	ctl_else_replace_.m_bStayPressed = TRUE;
	ctl_else_replace_.m_bRightArrow = TRUE;

	// Set up menu that allows the user to choose any valid variable name
	pcondition_menu_ = make_var_menu_tree(*pelt_);
	ctl_condition_var_.m_hMenu = pcondition_menu_->GetSafeHmenu();
	ctl_condition_var_.m_bOSMenu = TRUE;
	ctl_condition_var_.m_bStayPressed = TRUE;
	ctl_condition_var_.m_bRightArrow = TRUE;
	ctl_condition_.SetSel(0, -1);           // Select all so a var selection (see OnGetConditionVar) replaces current contents
	return TRUE;
}

static DWORD id_pairs[] = {
//        IDC_DFFD_NAME, HIDC_DFFD_NAME,
	IDC_DFFD_CONDITION, HIDC_DFFD_CONDITION,
	IDC_DFFD_CONDITION_VAR, HIDC_DFFD_CONDITION_VAR,
	IDC_DFFD_IF_ELT, HIDC_DFFD_IF_ELT,
	IDC_DFFD_IF_EDIT, HIDC_DFFD_IF_EDIT,
	IDC_DFFD_IF_REPLACE, HIDC_DFFD_IF_REPLACE,
	IDC_DFFD_ELSE, HIDC_DFFD_ELSE,
	IDC_DFFD_ELSE_ELT, HIDC_DFFD_ELSE_ELT,
	IDC_DFFD_ELSE_EDIT, HIDC_DFFD_ELSE_EDIT,
	IDC_DFFD_ELSE_REPLACE, HIDC_DFFD_ELSE_REPLACE,
	IDC_DFFD_COMMENTS, HIDC_DFFD_COMMENTS,
	ID_DFFD_PREV, HID_DFFD_PREV,
	ID_DFFD_NEXT, HID_DFFD_NEXT,
	0,0
};

BOOL CDFFDIf::OnHelpInfo(HELPINFO* pHelpInfo)
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs);
	return TRUE;
}

void CDFFDIf::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.HtmlHelpContextMenu(pWnd, id_pairs);
}

void CDFFDIf::OnHelp()
{
	// Display help for this page
	if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_DFFD_IF))
		AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

void CDFFDIf::OnCancel()
{
	pelt_->DeleteAllChildren();
	saved_.InsertKids(pelt_);
	pelt_->GetOwner()->SetModified(saved_mod_flag_);

	CDialog::OnCancel();
}

void CDFFDIf::OnOK()
{
	if (modified_)
	{
		if (!check_data())
			return;

		save_data();
	}

	CDialog::OnOK();
}

void CDFFDIf::OnNext()
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

void CDFFDIf::OnPrev()
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

LRESULT CDFFDIf::OnKickIdle(WPARAM, LPARAM lCount)
{
	if (valid_if_element_)
	{
		ctl_if_edit_.EnableWindow(TRUE);
		CString ss;
		ctl_if_replace_.GetWindowText(ss);
		if (ss != "Replace with")
			ctl_if_replace_.SetWindowText("Replace with");

		// Enable ELSE check box (only if we have a valid IF element)
		ctl_else_.EnableWindow(TRUE);
	}
	else
	{
		ctl_if_edit_.EnableWindow(FALSE);
		CString ss;
		ctl_if_replace_.GetWindowText(ss);
		if (ss != "Insert")
			ctl_if_replace_.SetWindowText("Insert");

		ctl_else_.EnableWindow(FALSE);
		else_ = 0;
	}

	if (else_)
	{
		if (valid_else_element_)
		{
			ctl_else_edit_.EnableWindow(TRUE);
			CString ss;
			ctl_else_replace_.GetWindowText(ss);
			if (ss != "Replace with")
				ctl_else_replace_.SetWindowText("Replace with");
		}
		else
		{
			ctl_else_edit_.EnableWindow(FALSE);
			CString ss;
			ctl_else_replace_.GetWindowText(ss);
			if (ss != "Insert")
				ctl_else_replace_.SetWindowText("Insert");
		}
		ctl_else_elt_name_.EnableWindow(TRUE);
		ctl_else_replace_.EnableWindow(TRUE);
	}
	else
	{
		// Disable all the else controls
		ctl_else_elt_name_.EnableWindow(FALSE);
		ctl_else_edit_.EnableWindow(FALSE);
		ctl_else_replace_.EnableWindow(FALSE);
	}

	return FALSE;
}

void CDFFDIf::OnChange()
{
	modified_ = true;
}

void CDFFDIf::OnElse()
{
	UpdateData();
	if (!else_ && valid_else_element_)
	{
		ASSERT(pelt_->GetNumChildren() == 3);
		CXmlTree::CElt ee(pelt_->GetChild(2));
		ASSERT(!ee.IsEmpty());
		CString sub_name = get_name(ee);

		CString mess;
		if (ee.GetFirstChild().IsEmpty())
		{
			mess.Format("This will delete the contents of %s.\n\n"
						"Are you sure you want to do this?", sub_name);
		}
		else
		{
			mess.Format("This will delete the contents of %s "
						"and all elements below it.\n\n"
						"Are you sure you want to do this?", sub_name);
		}
		if (TaskMessageBox("Deleting ELSE Element", mess, MB_YESNO) != IDYES)
		{
			else_ = TRUE;
			UpdateData(FALSE);
			return;
		}
		// Delete the else branch and the ELSE element too
		pelt_->DeleteChild(ee);
		ee = pelt_->GetChild(1);
		ASSERT(!ee.IsEmpty() && ee.GetName() == "else");
		pelt_->DeleteChild(ee);
		valid_else_element_ = false;
	}
	modified_ = true;
}

void CDFFDIf::OnIfEdit()
{
	CXmlTree::CElt ee(pelt_->GetFirstChild());
	ASSERT(valid_if_element_ && !ee.IsEmpty());

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
		CDFFDStruct dlg(&ee, CHexEditDoc::DF_IF, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			modified_ = true;
	}
	else if (elt_type == "use_struct")
	{
		// Edit the struct element node
		CDFFDUseStruct dlg(&ee, CHexEditDoc::DF_IF, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			modified_ = true;
	}
	else if (elt_type == "for")
	{
		// Edit the for element node
		CDFFDFor dlg(&ee, CHexEditDoc::DF_IF, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			modified_ = true;
	}
	else if (elt_type == "if")
	{
		// Edit the if element node
		CDFFDIf dlg(&ee, CHexEditDoc::DF_IF, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			modified_ = true;
	}
	else if (elt_type == "switch")
	{
		// Edit the switch element node
		CDFFDSwitch dlg(&ee, CHexEditDoc::DF_IF, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			modified_ = true;
	}
	else if (elt_type == "jump")
	{
		// Edit the jump element node
		CDFFDJump dlg(&ee, CHexEditDoc::DF_IF, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			modified_ = true;
	}
	else if (elt_type == "eval")
	{
		CDFFDEval dlg(&ee, CHexEditDoc::DF_IF, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			modified_ = true;
	}
	else if (elt_type == "data")
	{
		// Edit the data element node
		CDFFDData dlg(&ee, CHexEditDoc::DF_IF, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			modified_ = true;
	}
	else
		ASSERT(0);

	UpdateData(TRUE);
	if_elt_name_ = get_name(ee);
	UpdateData(FALSE);
}

void CDFFDIf::OnIfReplace()
{
	// See if we have a subelement to replace (otherwise we just insert)
	CXmlTree::CElt curr_elt(pelt_->GetFirstChild());
	CXmlTree::CElt before_elt;          // Element to insert new one before (if ELSE present)
	CXmlTree::CElt *pbefore = NULL;     // Ptr to before element or NULL if no ELSE part

	if (!curr_elt.IsEmpty())
	{
		ASSERT(valid_if_element_);
		CString sub_name = get_name(curr_elt);

		CString mess;
		if (curr_elt.GetFirstChild().IsEmpty())
		{
			mess.Format("This will replace the contents of %s.\n\n"
						"Are you sure you want to do this?", sub_name);
		}
		else
		{
			mess.Format("This will replace the contents of %s "
						"and all elements below it.\n\n"
						"Are you sure you want to do this?", sub_name);
		}
		if (TaskMessageBox("Replacing IF Element", mess, MB_YESNO) != IDYES)
			return;

		pelt_->DeleteChild(curr_elt);         // Remove the replaced node from the tree

		// Check if there is an ELSE part
		before_elt = pelt_->GetFirstChild();
		if (!before_elt.IsEmpty())
			pbefore = &before_elt;
	}

	// Work out where to put the dialog
	pos_ = update_posn(this);
	if (first_) orig_ = pos_;
	CPoint pt = get_posn(this, orig_);
	int dlg_ret;
	CXmlTree::CElt ee;

	switch (ctl_if_replace_.m_nMenuResult)
	{
	case ID_DFFD_INSERT_STRUCT:
		//ee = CXmlTree::CElt("struct", pelt_->GetOwner());
		//pelt_->InsertChild(ee);
		ee = pelt_->InsertNewChild("struct", pbefore);

		{
			CDFFDStruct dlg(&ee, CHexEditDoc::DF_IF, this);
			dlg.SetModified();          // Force validation as it is incomplete
			dlg.SetPosition(pt,orig_);
			dlg_ret = dlg.DoModal();
		}
		break;
	case ID_DFFD_INSERT_USE_STRUCT:
		ee = pelt_->InsertNewChild("use_struct", pbefore);

		{
			CDFFDUseStruct dlg(&ee, CHexEditDoc::DF_IF, this);
			dlg.SetModified();          // Force validation as it is incomplete
			dlg.SetPosition(pt,orig_);
			dlg_ret = dlg.DoModal();
		}
		break;
	case ID_DFFD_INSERT_FOR:
		ee = pelt_->InsertNewChild("for", pbefore);

		{
			CDFFDFor dlg(&ee, CHexEditDoc::DF_IF, this);
			dlg.SetModified();          // Force validation as it is incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
		}
		break;
	case ID_DFFD_INSERT_IF:
		ee = pelt_->InsertNewChild("if", pbefore);
		ee.SetAttr("test", "true");

		{
			CDFFDIf dlg(&ee, CHexEditDoc::DF_IF, this);
			dlg.SetModified();          // Force validation as it is incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
		}
		break;
	case ID_DFFD_INSERT_SWITCH:
		ee = pelt_->InsertNewChild("switch", pbefore);
		//ee.SetAttr("test", "0");

		{
			CDFFDSwitch dlg(&ee, CHexEditDoc::DF_IF, this);
			dlg.SetModified();          // Force validation as it is incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
		}
		break;
	case ID_DFFD_INSERT_JUMP:
		ee = pelt_->InsertNewChild("jump", pbefore);
		ee.SetAttr("offset", "0");

		{
			CDFFDJump dlg(&ee, CHexEditDoc::DF_IF, this);
			dlg.SetModified();          // Force validation as it is incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
		}
		break;
	case ID_DFFD_INSERT_EVAL:
		ee = pelt_->InsertNewChild("eval", pbefore);
		ee.SetAttr("expr", "true");

		{
			CDFFDEval dlg(&ee, CHexEditDoc::DF_IF, this);
			dlg.SetModified();          // Force validation as it is incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
		}
		break;
	case ID_DFFD_INSERT_DATA:
		ee = pelt_->InsertNewChild("data", pbefore);
		ee.SetAttr("type", "none");

		{
			CDFFDData dlg(&ee, CHexEditDoc::DF_IF, this);
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
		pelt_->DeleteChild(ee);
		if (!curr_elt.IsEmpty())
		{
			ASSERT(valid_if_element_);
			pelt_->InsertChild(curr_elt, pbefore);
		}
		return;
	}

	modified_ = true;
	valid_if_element_ = true;

	UpdateData(TRUE);
	if_elt_name_ = get_name(ee);
	UpdateData(FALSE);
}

void CDFFDIf::OnElseEdit()
{
	CXmlTree::CElt ee(pelt_->GetFirstChild());
	ASSERT(!ee.IsEmpty());
	++ee;
	ASSERT(!ee.IsEmpty() && ee.GetName() == "else");
	++ee;
	ASSERT(valid_else_element_ && !ee.IsEmpty());

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
		CDFFDStruct dlg(&ee, CHexEditDoc::DF_IF, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			modified_ = true;
	}
	else if (elt_type == "use_struct")
	{
		// Edit the struct element node
		CDFFDUseStruct dlg(&ee, CHexEditDoc::DF_IF, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			modified_ = true;
	}
	else if (elt_type == "for")
	{
		// Edit the for element node
		CDFFDFor dlg(&ee, CHexEditDoc::DF_IF, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			modified_ = true;
	}
	else if (elt_type == "if")
	{
		// Edit the if element node
		CDFFDIf dlg(&ee, CHexEditDoc::DF_IF, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			modified_ = true;
	}
	else if (elt_type == "switch")
	{
		// Edit the switch element node
		CDFFDSwitch dlg(&ee, CHexEditDoc::DF_IF, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			modified_ = true;
	}
	else if (elt_type == "jump")
	{
		// Edit the jump element node
		CDFFDJump dlg(&ee, CHexEditDoc::DF_IF, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			modified_ = true;
	}
	else if (elt_type == "eval")
	{
		CDFFDEval dlg(&ee, CHexEditDoc::DF_IF, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			modified_ = true;
	}
	else if (elt_type == "data")
	{
		// Edit the data element node
		CDFFDData dlg(&ee, CHexEditDoc::DF_IF, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			modified_ = true;
	}
	else
		ASSERT(0);

	UpdateData(TRUE);
	else_elt_name_ = get_name(ee);
	UpdateData(FALSE);
}

// Insert sub-element (if !valid_else_element_) or replace current one
void CDFFDIf::OnElseReplace()
{
	ASSERT(else_ == 1);                 // Make sure check box is checked
	CXmlTree::CElt curr_elt(pelt_->GetFirstChild());
	CXmlTree::CElt else_elt;
	ASSERT(valid_if_element_ && !curr_elt.IsEmpty());        // We need a valid IF before adding ELSE

	// See if we have a ELSE to replace (otherwise we just insert)
	++curr_elt;                         // Move to "else" element
	if (!curr_elt.IsEmpty())
	{
		else_elt = curr_elt;            // The new node will be inserted after "else" element

		++curr_elt;                     // Move to else part (after "else" element)
		ASSERT(!curr_elt.IsEmpty() && valid_else_element_);
		CString sub_name = get_name(curr_elt);

		CString mess;
		if (curr_elt.GetFirstChild().IsEmpty())
		{
			mess.Format("This will replace the contents of %s.\n\n"
						"Are you sure you want to do this?", sub_name);
		}
		else
		{
			mess.Format("This will replace the contents of %s\n"
						"and all elements below it.\n\n"
						"Are you sure you want to do this?", sub_name);
		}
		if (TaskMessageBox("Replacing ELSE Element", mess, MB_YESNO) != IDYES)
			return;

		pelt_->DeleteChild(curr_elt);         // Remove the replaced node from the tree
	}
	else
	{
		// Create "else" element at end
		else_elt = pelt_->InsertNewChild("else");
	}

	// Work out where to put the dialog
	pos_ = update_posn(this);
	if (first_) orig_ = pos_;
	CPoint pt = get_posn(this, orig_);
	int dlg_ret;
	CXmlTree::CElt ee;

	switch (ctl_else_replace_.m_nMenuResult)
	{
	case ID_DFFD_INSERT_STRUCT:
		ee = pelt_->InsertNewChild("struct");

		{
			CDFFDStruct dlg(&ee, CHexEditDoc::DF_IF, this);
			dlg.SetModified();          // Force validation as it is incomplete
			dlg.SetPosition(pt,orig_);
			dlg_ret = dlg.DoModal();
		}
		break;
	case ID_DFFD_INSERT_USE_STRUCT:
		ee = pelt_->InsertNewChild("use_struct");

		{
			CDFFDUseStruct dlg(&ee, CHexEditDoc::DF_IF, this);
			dlg.SetModified();          // Force validation as it is incomplete
			dlg.SetPosition(pt,orig_);
			dlg_ret = dlg.DoModal();
		}
		break;
	case ID_DFFD_INSERT_FOR:
		ee = pelt_->InsertNewChild("for");

		{
			CDFFDFor dlg(&ee, CHexEditDoc::DF_IF, this);
			dlg.SetModified();          // Force validation as it is incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
		}
		break;
	case ID_DFFD_INSERT_IF:
		ee = pelt_->InsertNewChild("if");
		ee.SetAttr("test", "true");

		{
			CDFFDIf dlg(&ee, CHexEditDoc::DF_IF, this);
			dlg.SetModified();          // Force validation as it is incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
		}
		break;
	case ID_DFFD_INSERT_SWITCH:
		ee = pelt_->InsertNewChild("switch");
		//ee.SetAttr("test", "0");

		{
			CDFFDSwitch dlg(&ee, CHexEditDoc::DF_IF, this);
			dlg.SetModified();          // Force validation as it is incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
		}
		break;
	case ID_DFFD_INSERT_JUMP:
		ee = pelt_->InsertNewChild("jump");
		ee.SetAttr("offset", "0");

		{
			CDFFDJump dlg(&ee, CHexEditDoc::DF_IF, this);
			dlg.SetModified();          // Force validation as it is incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
		}
		break;
	case ID_DFFD_INSERT_EVAL:
		ee = pelt_->InsertNewChild("eval");
		ee.SetAttr("expr", "true");

		{
			CDFFDEval dlg(&ee, CHexEditDoc::DF_IF, this);
			dlg.SetModified();          // Force validation as it is incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
		}
		break;
	case ID_DFFD_INSERT_DATA:
		ee = pelt_->InsertNewChild("data");
		ee.SetAttr("type", "none");

		{
			CDFFDData dlg(&ee, CHexEditDoc::DF_IF, this);
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
		pelt_->DeleteChild(ee);
		if (!curr_elt.IsEmpty())
		{
			ASSERT(valid_else_element_);
			pelt_->InsertChild(curr_elt);
		}
		else
		{
			// Remove "else" element
			pelt_->DeleteChild(else_elt);
		}
		return;
	}

	modified_ = true;
	valid_else_element_ = true;

	UpdateData(TRUE);
	else_elt_name_ = get_name(ee);
	UpdateData(FALSE);
}

void CDFFDIf::OnGetConditionVar()
{
	if (ctl_condition_var_.m_nMenuResult != 0)
	{
		CString strVar = get_menu_text(pcondition_menu_, ctl_condition_var_.m_nMenuResult);
		ASSERT(!strVar.IsEmpty());      // The selected menu item should be found in the menu tree

		// Put the var name into the "condition" control
		for (int ii = 0; ii < strVar.GetLength (); ii++)
			ctl_condition_.SendMessage(WM_CHAR, (TCHAR)strVar[ii]);
		ctl_condition_.SetFocus();
		modified_ = true;
	}
}

void CDFFDIf::load_data()
{
	CXmlTree::CElt ee(pelt_->GetFirstChild());
	if_elt_name_.Empty();               // Clear the name of the contained elements
	else_elt_name_.Empty();

	valid_else_element_ = false;
	if (valid_if_element_ = !ee.IsEmpty())
	{
		// We have an IF part
		if_elt_name_ = get_name(ee);

		// Now check if there is an ELSE part
		++ee;
		if (!ee.IsEmpty())
		{
			ASSERT(ee.GetName() == "else");
			else_ = 1;

			++ee;
			if (valid_else_element_ = !ee.IsEmpty())
				else_elt_name_ = get_name(ee);
		}
	}

	condition_ = pelt_->GetAttr("test");
	comment_ = pelt_->GetAttr("comment");

	UpdateData(FALSE);
}

bool CDFFDIf::check_data()
{
	// Get values from the controls and validate them
	UpdateData();

	if (!valid_if_element_)
	{
		TaskMessageBox("IF Element Required", "An IF element requires exactly one sub-element.  Please add the conditional element.\n");
		ctl_if_replace_.SetFocus();
		return false;
	}

	if (else_ && !valid_else_element_)
	{
		TaskMessageBox("ELSE Element Required", "If enabled the ELSE part requires one sub-element.  Please add an a sub-element or disable the ELSE part.\n");
		ctl_else_replace_.SetFocus();
		return false;
	}

	// xxx TBD check that the condition is a valid boolean expression

	return true;
}

void CDFFDIf::save_data()
{
	// Save data to XML element node
	pelt_->SetAttr("test", condition_);
	pelt_->SetAttr("comment", comment_);
}
