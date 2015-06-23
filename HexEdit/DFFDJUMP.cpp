// DFFDJUMP.cpp : implementation file
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
#include "DFFDJUMP.h"
#include "DFFDStruct.h"
#include "DFFDUseStruct.h"
#include "DFFDFor.h"
#include "DFFDIf.h"
#include "DFFDSwitch.h"
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
// CDFFDJump dialog


CDFFDJump::CDFFDJump(CXmlTree::CElt *pp, signed char parent_type,
				 CWnd* pParent /*=NULL*/)
	: CDialog(CDFFDJump::IDD, pParent), saved_(pp->GetOwner())
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
	poffset_menu_ = NULL;

	//{{AFX_DATA_INIT(CDFFDJump)
	comment_ = _T("");
	offset_ = _T("");
	elt_name_ = _T("");
	//}}AFX_DATA_INIT
	origin_ = 1;
}

CDFFDJump::~CDFFDJump()
{
	if (poffset_menu_ != NULL)
	{
		// Destroy the menu and free the memory
		poffset_menu_->DestroyMenu();
		delete poffset_menu_;
	}
}

void CDFFDJump::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDFFDJump)
	DDX_Control(pDX, IDC_DFFD_EDIT, ctl_edit_);
	DDX_Control(pDX, ID_DFFD_PREV, ctl_prev_);
	DDX_Control(pDX, ID_DFFD_NEXT, ctl_next_);
	DDX_Text(pDX, IDC_DFFD_COMMENTS, comment_);
	DDX_Text(pDX, IDC_DFFD_OFFSET, offset_);
	DDX_Text(pDX, IDC_DFFD_ELT, elt_name_);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_DFFD_REPLACE, ctl_replace_);
	DDX_Control(pDX, IDC_DFFD_OFFSET, ctl_offset_);
	DDX_Control(pDX, IDC_DFFD_OFFSET_VAR, ctl_offset_var_);
	DDX_CBIndex(pDX, IDC_DFFD_ORIGIN, origin_);
}

BEGIN_MESSAGE_MAP(CDFFDJump, CDialog)
	//{{AFX_MSG_MAP(CDFFDJump)
	ON_BN_CLICKED(ID_DFFD_NEXT, OnNext)
	ON_BN_CLICKED(ID_DFFD_PREV, OnPrev)
	ON_EN_CHANGE(IDC_DFFD_COMMENTS, OnChange)
	ON_BN_CLICKED(IDC_DFFD_EDIT, OnEdit)
	ON_BN_CLICKED(IDC_DFFD_REPLACE, OnReplace)
	ON_EN_CHANGE(IDC_DFFD_OFFSET, OnChange)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
	ON_WM_CONTEXTMENU()
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_BN_CLICKED(ID_DFFD_HELP, OnHelp)
	ON_BN_CLICKED(IDC_DFFD_OFFSET_VAR, OnGetOffsetVar)
	ON_CBN_SELCHANGE(IDC_DFFD_ORIGIN, OnChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDFFDJump message handlers

BOOL CDFFDJump::OnInitDialog()
{
	CDialog::OnInitDialog();
	SetIcon(AfxGetApp()->LoadIcon(IDI_JUMP), TRUE);

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

	// Setup menu button that allows replace of struct/for/if/data
	VERIFY(button_menu_.LoadMenu(IDR_DFFD));
	CString strTemp;
	ASSERT((button_menu_.GetMenuString(4, strTemp, MF_BYPOSITION), strTemp == "ButtonReplace"));

	// Disable insertion of jump within a jump
	button_menu_.GetSubMenu(4)->EnableMenuItem(ID_DFFD_INSERT_JUMP, MF_BYCOMMAND | MF_GRAYED);

	ctl_replace_.m_hMenu = button_menu_.GetSubMenu(4)->GetSafeHmenu();
//    ctl_replace_.SizeToContent();
	ctl_replace_.m_bOSMenu = TRUE;
	ctl_replace_.m_bStayPressed = TRUE;
	ctl_replace_.m_bRightArrow = TRUE;

	// Set up menu that allows the user to choose any valid variable name
	poffset_menu_ = make_var_menu_tree(*pelt_);
	ctl_offset_var_.m_hMenu = poffset_menu_->GetSafeHmenu();
	ctl_offset_var_.m_bOSMenu = TRUE;
	ctl_offset_var_.m_bStayPressed = TRUE;
	ctl_offset_var_.m_bRightArrow = TRUE;
	ctl_offset_.SetSel(0, -1);           // Select all so a var selection (see OnGetOffsetVar) replaces current contents
	return TRUE;
}

static DWORD id_pairs[] = { 
	IDC_DFFD_OFFSET, HIDC_DFFD_OFFSET,
	IDC_DFFD_OFFSET_VAR, HIDC_DFFD_OFFSET_VAR,
	IDC_DFFD_ORIGIN, HIDC_DFFD_ORIGIN,
	IDC_DFFD_ELT, HIDC_DFFD_ELT,
	IDC_DFFD_EDIT, HIDC_DFFD_EDIT,
	IDC_DFFD_REPLACE, HIDC_DFFD_REPLACE,
	IDC_DFFD_COMMENTS, HIDC_DFFD_COMMENTS,
	ID_DFFD_PREV, HID_DFFD_PREV,
	ID_DFFD_NEXT, HID_DFFD_NEXT,
	0,0 
};

BOOL CDFFDJump::OnHelpInfo(HELPINFO* pHelpInfo)
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs);
	return TRUE;
}

void CDFFDJump::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.HtmlHelpContextMenu(pWnd, id_pairs);
}

void CDFFDJump::OnHelp()
{
	// Display help for this page
	if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_DFFD_JUMP))
		AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

void CDFFDJump::OnCancel()
{
	pelt_->DeleteAllChildren();
	saved_.InsertKids(pelt_);
	pelt_->GetOwner()->SetModified(saved_mod_flag_);

	CDialog::OnCancel();
}

void CDFFDJump::OnOK()
{
	if (modified_)
	{
		if (!check_data())
			return;

		save_data();
	}

	CDialog::OnOK();
}

void CDFFDJump::OnNext()
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

void CDFFDJump::OnPrev()
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

LRESULT CDFFDJump::OnKickIdle(WPARAM, LPARAM lCount)
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

void CDFFDJump::OnChange()
{
	modified_ = true;
}

void CDFFDJump::OnEdit()
{
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
		CDFFDStruct dlg(&ee, CHexEditDoc::DF_JUMP, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			modified_ = true;
	}
	else if (elt_type == "use_struct")
	{
		// Edit the struct element node
		CDFFDUseStruct dlg(&ee, CHexEditDoc::DF_JUMP, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			modified_ = true;
	}
	else if (elt_type == "for")
	{
		// Edit the for element node
		CDFFDFor dlg(&ee, CHexEditDoc::DF_JUMP, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			modified_ = true;
	}
	else if (elt_type == "if")
	{
		// Edit the if element node
		CDFFDIf dlg(&ee, CHexEditDoc::DF_JUMP, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			modified_ = true;
	}
	else if (elt_type == "switch")
	{
		// Edit the switch element node
		CDFFDSwitch dlg(&ee, CHexEditDoc::DF_JUMP, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			modified_ = true;
	}
	else if (elt_type == "data")
	{
		// Edit the data element node
		CDFFDData dlg(&ee, CHexEditDoc::DF_JUMP, this);
		dlg.SetPosition(pt, orig_);
		dlg_ret = dlg.DoModal();
		if (dlg_ret != IDCANCEL && dlg.IsModified())
			modified_ = true;
	}
	else
		ASSERT(0);

	UpdateData(TRUE);
	elt_name_ = get_name(ee);
	UpdateData(FALSE);
}

void CDFFDJump::OnReplace()
{
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
			mess.Format("This will replace the contents of %s "
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
		ee = pelt_->InsertNewChild("struct");

		{
			CDFFDStruct dlg(&ee, CHexEditDoc::DF_JUMP, this);
			dlg.SetModified();          // Force validation as it is incomplete
			dlg.SetPosition(pt,orig_);
			dlg_ret = dlg.DoModal();
		}
		break;
	case ID_DFFD_INSERT_USE_STRUCT:
		ee = pelt_->InsertNewChild("use_struct", NULL);

		{
			CDFFDUseStruct dlg(&ee, CHexEditDoc::DF_JUMP, this);
			dlg.SetModified();          // Force validation as it is incomplete
			dlg.SetPosition(pt,orig_);
			dlg_ret = dlg.DoModal();
		}
		break;
	case ID_DFFD_INSERT_FOR:
		ee = pelt_->InsertNewChild("for");

		{
			CDFFDFor dlg(&ee, CHexEditDoc::DF_JUMP, this);
			dlg.SetModified();          // Force validation as it is incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
		}
		break;
	case ID_DFFD_INSERT_IF:
		ee = pelt_->InsertNewChild("if");
		ee.SetAttr("test", "true");

		{
			CDFFDIf dlg(&ee, CHexEditDoc::DF_JUMP, this);
			dlg.SetModified();          // Force validation as it is incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
		}
		break;
	case ID_DFFD_INSERT_SWITCH:
		ee = pelt_->InsertNewChild("switch");

		{
			CDFFDSwitch dlg(&ee, CHexEditDoc::DF_JUMP, this);
			dlg.SetModified();          // Force validation as it is incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
		}
		break;
	case ID_DFFD_INSERT_DATA:
		ee = pelt_->InsertNewChild("data");
		ee.SetAttr("type", "none");

		{
			CDFFDData dlg(&ee, CHexEditDoc::DF_JUMP, this);
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
			ASSERT(valid_element_);
			pelt_->InsertChild(curr_elt, NULL);
		}
		return;
	}

	modified_ = true;
	valid_element_ = true;

	UpdateData(TRUE);
	elt_name_ = get_name(ee);
	UpdateData(FALSE);
}

void CDFFDJump::OnGetOffsetVar()
{
	if (ctl_offset_var_.m_nMenuResult != 0)
	{
		CString strVar = get_menu_text(poffset_menu_, ctl_offset_var_.m_nMenuResult);
		ASSERT(!strVar.IsEmpty());      // The selected menu item should be found in the menu tree

		// Put the var name into the "offset" control
		for (int ii = 0; ii < strVar.GetLength (); ii++)
			ctl_offset_.SendMessage(WM_CHAR, (TCHAR)strVar[ii]);
		ctl_offset_.SetFocus();
		modified_ = true;
	}
}

void CDFFDJump::load_data()
{
	CXmlTree::CElt ee(pelt_->GetFirstChild());
	elt_name_.Empty();                  // Clear the name of the contained element

	if (valid_element_ = !ee.IsEmpty())
		elt_name_ = get_name(ee);

	offset_ = pelt_->GetAttr("offset");
	comment_ = pelt_->GetAttr("comment");
	CString ss = pelt_->GetAttr("origin");
	if (ss.CompareNoCase("start") == 0)
		origin_ = 0;  // start of file
	else if (ss.CompareNoCase("end") == 0)
		origin_ = 2;  // end of file
	else
		origin_ = 1;  // current pos in file

	UpdateData(FALSE);
}

bool CDFFDJump::check_data()
{
	// Get values from the controls and validate them
	UpdateData();

	if (!valid_element_)
	{
		TaskMessageBox("JUMP Element Required", "A JUMP element requires exactly one sub-element.  Please add the element of the JUMP.\n");
		ctl_replace_.SetFocus();
		return false;
	}

	// xxx TBD check that the address offset is a valid intger expression

	return true;
}

void CDFFDJump::save_data()
{
	// Save data to XML element node
	pelt_->SetAttr("offset", offset_);
	pelt_->SetAttr("comment", comment_);
	switch (origin_)
	{
	case 0:
		pelt_->SetAttr("origin", "start");
		break;
	default:
		ASSERT(0);
		// fall through
	case 1:
		pelt_->SetAttr("origin", "current");
		break;
	case 2:
		pelt_->SetAttr("origin", "end");
		break;
	}
}
