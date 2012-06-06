// DFFDSwitch.cpp : implementation file
//
// Copyright (c) 2006-2010 by Andrew W. Phillips.
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

//- implement up/down

#include "hexedit.h"
#include "HexEditDoc.h"
#include "DataFormatView.h"
#include "DFFDSwitch.h"
#include "DFFDStruct.h"
#include "DFFDUseStruct.h"
#include "DFFDFor.h"
#include "DFFDIf.h"
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
// CDFFDSwitch dialog


CDFFDSwitch::CDFFDSwitch(CXmlTree::CElt *pp, signed char parent_type,
				 CWnd* pParent /*=NULL*/)
	: CDialog(CDFFDSwitch::IDD, pParent), saved_(pp->GetOwner())
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
	pexpr_menu_ = NULL;

	expr_ = _T("");
	comment_ = _T("");
}

CDFFDSwitch::~CDFFDSwitch()
{
	if (pexpr_menu_ != NULL)
	{
		// Destroy the menu and free the memory
		pexpr_menu_->DestroyMenu();
		delete pexpr_menu_;
	}
}

void CDFFDSwitch::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_DFFD_SWITCH_EXPR, expr_);
	DDX_Control(pDX, IDC_DFFD_SWITCH_EXPR, ctl_expr_);
	DDX_Control(pDX, IDC_DFFD_SWITCH_EXPR_VAR, ctl_expr_var_);

	DDX_Control(pDX, IDC_DFFD_CASES, ctl_cases_);
	DDX_Control(pDX, IDC_DFFD_CASE_RANGE, ctl_range_);
	DDX_Control(pDX, IDC_DFFD_EDIT, ctl_edit_);
	DDX_Control(pDX, IDC_DFFD_INSERT, ctl_insert_);
	DDX_Control(pDX, IDC_DFFD_DELETE, ctl_delete_);
	DDX_Control(pDX, IDC_DFFD_UP, ctl_up_);
	DDX_Control(pDX, IDC_DFFD_DOWN, ctl_down_);

	DDX_Text(pDX, IDC_DFFD_COMMENTS, comment_);
	DDX_Control(pDX, ID_DFFD_PREV, ctl_prev_);
	DDX_Control(pDX, ID_DFFD_NEXT, ctl_next_);
}

BEGIN_MESSAGE_MAP(CDFFDSwitch, CDialog)
	ON_WM_CONTEXTMENU()
	ON_WM_HELPINFO()
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_BN_CLICKED(ID_DFFD_HELP, OnHelp)

	ON_EN_CHANGE(IDC_DFFD_SWITCH_EXPR, OnChange)
	// ON_LBN_SELCHANGE(IDC_DFFD_CASES, OnSelchangeCase)  // use OnKickIdle to update controls based on current list item selected
	ON_CBN_DBLCLK(IDC_DFFD_CASES, OnCaseDClick)
	ON_EN_CHANGE(IDC_DFFD_CASE_RANGE, OnChangeRange)
	ON_EN_CHANGE(IDC_DFFD_COMMENTS, OnChange)

	ON_BN_CLICKED(IDC_DFFD_SWITCH_EXPR_VAR, OnGetExprVar)
	ON_BN_CLICKED(IDC_DFFD_EDIT, OnEdit)
	ON_BN_CLICKED(IDC_DFFD_INSERT, OnInsert)
	ON_BN_CLICKED(IDC_DFFD_DELETE, OnDelete)
	ON_BN_CLICKED(IDC_DFFD_UP, OnUp)                    // allow user to re-order cases since only the first encountered is "taken"
	ON_BN_CLICKED(IDC_DFFD_DOWN, OnDown)
	ON_BN_CLICKED(ID_DFFD_NEXT, OnNext)
	ON_BN_CLICKED(ID_DFFD_PREV, OnPrev)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDFFDSwitch message handlers

BOOL CDFFDSwitch::OnInitDialog()
{
	CDialog::OnInitDialog();
	SetIcon(AfxGetApp()->LoadIcon(IDI_SWITCH), TRUE);

	// Load button icons
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

	// Set up menu button (with arrow) that allows the user to choose any valid variable name
	pexpr_menu_ = make_var_menu_tree(*pelt_);
	ctl_expr_var_.m_hMenu = pexpr_menu_->GetSafeHmenu();
	ctl_expr_var_.m_bOSMenu = TRUE;
	ctl_expr_var_.m_bStayPressed = TRUE;
	ctl_expr_var_.m_bRightArrow = TRUE;
	ctl_expr_.SetSel(0, -1);           // Select all so a var selection (see OnGetExprVar) replaces current contents

	// Setup menu button that allows insert of struct/for/if/jump/eval/data
	VERIFY(button_menu_.LoadMenu(IDR_DFFD));
	CString strTemp;
	ASSERT((button_menu_.GetMenuString(3, strTemp, MF_BYPOSITION), strTemp == "ButtonInsert"));
	ctl_insert_.m_hMenu = button_menu_.GetSubMenu(3)->GetSafeHmenu();
//    ctl_insert_.SizeToContent();
	ctl_insert_.m_bOSMenu = TRUE;
	ctl_insert_.m_bStayPressed = TRUE;
	ctl_insert_.m_bRightArrow = TRUE;

	return TRUE;
}

static DWORD id_pairs[] = { 
	//IDC_DFFD_NAME, HIDC_DFFD_NAME,
	IDC_DFFD_SWITCH_EXPR, HIDC_DFFD_SWITCH_EXPR,
	IDC_DFFD_SWITCH_EXPR_VAR, HIDC_DFFD_SWITCH_EXPR_VAR,
	IDC_DFFD_CASES, HIDC_DFFD_CASES,
	IDC_DFFD_CASE_RANGE, HIDC_DFFD_CASE_RANGE,
	IDC_DFFD_EDIT, HIDC_DFFD_EDIT,
	IDC_DFFD_INSERT, HIDC_DFFD_INSERT,
	IDC_DFFD_DELETE, HIDC_DFFD_DELETE,
	IDC_DFFD_UP, HIDC_DFFD_UP,
	IDC_DFFD_DOWN, HIDC_DFFD_DOWN,
	IDC_DFFD_COMMENTS, HIDC_DFFD_COMMENTS,
	ID_DFFD_PREV, HID_DFFD_PREV,
	ID_DFFD_NEXT, HID_DFFD_NEXT,
	0,0 
};

BOOL CDFFDSwitch::OnHelpInfo(HELPINFO* pHelpInfo)
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs);
	return TRUE;
}

void CDFFDSwitch::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.HtmlHelpContextMenu(pWnd, id_pairs);
}

void CDFFDSwitch::OnHelp()
{
	// Display help for this page
	if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_DFFD_SWITCH))
		AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

void CDFFDSwitch::OnCancel()
{
	pelt_->DeleteAllChildren();
	saved_.InsertKids(pelt_);
	pelt_->GetOwner()->SetModified(saved_mod_flag_);

	CDialog::OnCancel();
}

void CDFFDSwitch::OnOK()
{
	if (modified_)
	{
		if (!check_data())
			return;

		save_data();
	}

	CDialog::OnOK();
}

void CDFFDSwitch::OnNext()
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

void CDFFDSwitch::OnPrev()
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

LRESULT CDFFDSwitch::OnKickIdle(WPARAM, LPARAM lCount)
{
	// Update controls based on currently selected listbox element
	// range text, edit/delete/up/down buttons
	int bot = 0, top = ctl_cases_.GetCount()-1;
	int item = ctl_cases_.GetCurSel();
	if (item == top) item = -1;                      // Top entry is blank only used for appending (inserting at end)

	ctl_range_.EnableWindow (item != -1);
	ctl_edit_.EnableWindow  (item != -1);
	ctl_delete_.EnableWindow(item != -1);           // Note last (blank) entry has been set to item = -1 (above)
	ctl_up_.EnableWindow  (item != -1 && item > bot);
	ctl_down_.EnableWindow(item != -1 && item < top-1);

	if (item != -1)
	{
		// Update range for this item
		CXmlTree::CElt ee = pelt_->GetChild(long(item));
		ASSERT(!ee.IsEmpty() && ee.GetName() == "case");
		CString ss, ss_new = ee.GetAttr("range");
		ctl_range_.GetWindowText(ss);
		if (ss != ss_new)
			ctl_range_.SetWindowText(ss_new);
	}

	return FALSE;
}

void CDFFDSwitch::OnChange()
{
	modified_ = true;
}

void CDFFDSwitch::OnChangeRange()
{
	// Find item for which the range was changed
	int item = ctl_cases_.GetCurSel();
	ASSERT(item != LB_ERR && item < ctl_cases_.GetCount() - 1);
	if (item < 0 || item >= ctl_cases_.GetCount()-1) return;

	// Save the new range as the "range" attribute of the current "case" node
	CString ss;
	ctl_range_.GetWindowText(ss);
	pelt_->GetChild(long(item)).SetAttr("range", ss);

	modified_ = true;
}

void CDFFDSwitch::OnCaseDClick()
{
	// Double-click edits the sub-item unless we're at the last (empty) item
	if (ctl_cases_.GetCurSel() < ctl_cases_.GetCount()-1)
		do_edit();
}

void CDFFDSwitch::OnEdit()
{
	do_edit();
}

void CDFFDSwitch::OnInsert()
{
	// Find out which child we are inserting before
	int item = ctl_cases_.GetCurSel();
	ASSERT(item < ctl_cases_.GetCount());

	// Check menu item selected to see what type of element to insert
	int elt_id = ctl_insert_.m_nMenuResult;

	CXmlTree::CElt ecase;            // New case element
	CXmlTree::CElt ee;               // New element that is added into case elt
	CXmlTree::CElt before;           // Element to insert before
	const CXmlTree::CElt *pbefore = NULL;      // Ptr to element to insert before

	// Work out where to insert the new item
	if (item == -1 || item >= ctl_cases_.GetCount()-1)
	{
		pbefore = NULL;                     // Add to end
		item = ctl_cases_.GetCount()-1;
	}
	else
	{
		// Add at current position but make sure it is after all structure definitions
		before = pelt_->GetChild(long(item));
		pbefore = &before;
	}
	ecase = pelt_->InsertNewChild("case", pbefore);

	switch (elt_id)
	{
	case ID_DFFD_INSERT_STRUCT:
		ee = ecase.InsertNewChild("struct");
		item = ctl_cases_.InsertString(item, "[STRUCT]");
		break;
	case ID_DFFD_INSERT_USE_STRUCT:
		ee = ecase.InsertNewChild("use_struct");
		item = ctl_cases_.InsertString(item, "[USING STRUCT]");
		break;
	case ID_DFFD_INSERT_FOR:
		ee = ecase.InsertNewChild("for");
		item = ctl_cases_.InsertString(item, "[FOR]");
		break;
	case ID_DFFD_INSERT_IF:
		ee = ecase.InsertNewChild("if");
		ee.SetAttr("test", "true");
		item = ctl_cases_.InsertString(item, "[IF]");
		break;
	case ID_DFFD_INSERT_SWITCH:
		ee = ecase.InsertNewChild("switch");
		//ee.SetAttr("test", "0");
		//(void)ee.InsertNewChild("case");
		item = ctl_cases_.InsertString(item, "[CASE]");
		break;
	case ID_DFFD_INSERT_JUMP:
		ee = ecase.InsertNewChild("jump");
		ee.SetAttr("offset", "0");
		item = ctl_cases_.InsertString(item, "[JUMP]");
		break;
	case ID_DFFD_INSERT_EVAL:
		ee = ecase.InsertNewChild("eval");
		ee.SetAttr("expr", "true");
		item = ctl_cases_.InsertString(item, "[EXPR]");
		break;
	case ID_DFFD_INSERT_DATA:
		ee = ecase.InsertNewChild("data");
		ee.SetAttr("type", "none");
		item = ctl_cases_.InsertString(item, "[DATA]");
		break;
	default:
		ASSERT(0);
	}
	ctl_cases_.SetCurSel(item);

	do_edit(true);
}

void CDFFDSwitch::OnDelete()
{
	// Find out which child we are deleting
	int item = ctl_cases_.GetCurSel();
	ASSERT(item > -1 && item < ctl_cases_.GetCount()-1);
	if (item < 0 || item >= ctl_cases_.GetCount()-1) return;

	// Check with the user that they really want to delete
	if (TaskMessageBox("Delete SWITCH?", "The SWITCH and all child CASE elements will be removed.\n\n"
	                   "Are you sure you want to delete this element?", MB_YESNO) != IDYES)
		return;

	// Delete the node from the XML document
	pelt_->DeleteChild(pelt_->GetChild(long(item)));

	// Delete the node from the list box
	ctl_cases_.DeleteString(item);
	ctl_cases_.SetCurSel(item);

	modified_ = true;
}

void CDFFDSwitch::OnUp()
{
	// Find out which child to move
	int item = ctl_cases_.GetCurSel();
	ASSERT(item > 0 && item < ctl_cases_.GetCount()-1);
	if (item < 1 || item >= ctl_cases_.GetCount()-1) return;

	// Move the node
	CXmlTree::CElt ee = pelt_->GetChild(long(item-1));
	pelt_->InsertChild(pelt_->GetChild(long(item)), &ee);

	// Move in the list control too
	CString ss;
	ctl_cases_.GetText(item, ss);
	ctl_cases_.DeleteString(item);
	ctl_cases_.InsertString(item-1, ss);
	ctl_cases_.SetCurSel(item-1);

	modified_ = true;
}

void CDFFDSwitch::OnDown()
{
	// Find out which child to move
	int item = ctl_cases_.GetCurSel();
	ASSERT(item > -1 && item < ctl_cases_.GetCount()-2);
	if (item < 0 || item >= ctl_cases_.GetCount()-2) return;

	// Move the node
	if (item == ctl_cases_.GetCount()-3)     // 2nd last item which is last that can move down
		pelt_->InsertChild(pelt_->GetChild(long(item)), NULL);
	else
	{
		CXmlTree::CElt ee = pelt_->GetChild(long(item+2));
		pelt_->InsertChild(pelt_->GetChild(long(item)), &ee);
	}

	// Move in the list control too
	CString ss;
	ctl_cases_.GetText(item, ss);
	ctl_cases_.DeleteString(item);
	ctl_cases_.InsertString(item+1, ss);
	ctl_cases_.SetCurSel(item+1);

	modified_ = true;
}

// The parameter says to delete the current element if the dialog is cancelled (but it will not
// be deleted if the prev/next button has been used even if we are back on the same sub-element).
void CDFFDSwitch::do_edit(bool delete_on_cancel /*=false*/)
{
	// Find out which child we are editing
	int item = ctl_cases_.GetCurSel();
	ASSERT(item > LB_ERR && item < ctl_cases_.GetCount()-1);
	if (item < 0 || item >= ctl_cases_.GetCount()-1) return;

	bool continue_edit = true;        // For loop that allows the user to cycle through editing siblings
	bool changes_made = false;
	ASSERT(!pelt_->GetChild(long(item)).IsEmpty() &&
		   pelt_->GetChild(long(item)).GetName() == "case" &&
		   !pelt_->GetChild(long(item)).GetFirstChild().IsEmpty());
	CXmlTree::CElt current_elt = pelt_->GetChild(long(item)).GetFirstChild();

	// Work out where to put the dialog
	pos_ = update_posn(this);
	if (first_) orig_ = pos_;
	CPoint pt = get_posn(this, orig_);

	int dlg_ret = IDCANCEL;
	int current_item = item;
	while (continue_edit)
	{
		CString elt_type = current_elt.GetName();

		if (elt_type == "struct")
		{
			// Edit the struct element node
			CDFFDStruct dlg(&current_elt, CHexEditDoc::DF_SWITCH, this);
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
			CDFFDUseStruct dlg(&current_elt, CHexEditDoc::DF_SWITCH, this);
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
			CDFFDFor dlg(&current_elt, CHexEditDoc::DF_SWITCH, this);
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
			CDFFDIf dlg(&current_elt, CHexEditDoc::DF_SWITCH, this);
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
			CDFFDSwitch dlg(&current_elt, CHexEditDoc::DF_SWITCH, this);
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
			CDFFDJump dlg(&current_elt, CHexEditDoc::DF_SWITCH, this);
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
			CDFFDEval dlg(&current_elt, CHexEditDoc::DF_SWITCH, this);
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
			CDFFDData dlg(&current_elt, CHexEditDoc::DF_SWITCH, this);
			if (delete_on_cancel)
				dlg.SetModified();      // Force validation as it is new & incomplete
			dlg.SetPosition(pt, orig_);
			dlg_ret = dlg.DoModal();
			dlg.GetPosition(pt);
			if (dlg_ret != IDCANCEL && dlg.IsModified())
				changes_made = true;
		}
		else
			ASSERT(0);

		// Update the name in the list in case it has been modified
		if (changes_made)
		{
			ctl_cases_.InsertString(current_item, ::get_name(current_elt));
			ctl_cases_.DeleteString(current_item+1);
			ctl_cases_.SetCurSel(current_item);
			modified_ = true;
		}

		// If the user pressed the next or previous button start editing the sibling
		if (dlg_ret == ID_DFFD_PREV)
		{
			// Edit previous sibling
			ASSERT(current_item != LB_ERR && current_item > 0);
			current_item = current_item - 1;
			ctl_cases_.SetCurSel(current_item);   // move to previous item in list box

			ASSERT(!pelt_->GetChild(long(current_item)).IsEmpty() &&
					pelt_->GetChild(long(current_item)).GetName() == "case" &&
				   !pelt_->GetChild(long(current_item)).GetFirstChild().IsEmpty());
			current_elt = pelt_->GetChild(long(current_item)).GetFirstChild();  // get previous elt

			delete_on_cancel = false;
		}
		else if (dlg_ret == ID_DFFD_NEXT)
		{
			// Edit next sibling
			ASSERT(current_item != LB_ERR && current_item < ctl_cases_.GetCount()-2);
			current_item = current_item + 1;
			ctl_cases_.SetCurSel(current_item);   // move to next item in list box

			ASSERT(!pelt_->GetChild(long(current_item)).IsEmpty() &&
					pelt_->GetChild(long(current_item)).GetName() == "case" &&
				   !pelt_->GetChild(long(current_item)).GetFirstChild().IsEmpty());
			current_elt = pelt_->GetChild(long(current_item)).GetFirstChild();  // get next elt

			delete_on_cancel = false;
		}
		else if (dlg_ret == ID_DFFD_CLONE)
		{
			ASSERT(current_item != LB_ERR && current_item < ctl_cases_.GetCount()-1);

			// Create a clone of the current case element
			CXmlTree::CElt next_elt = pelt_->GetChild(long(current_item+1));   // we need this for insert location
			(void)pelt_->InsertClone(pelt_->GetChild(long(current_item)), next_elt.IsEmpty() ? NULL : &next_elt);

			current_item = current_item + 1;

			// Get the case "range" and set to next value
			CXmlTree::CElt ecase = pelt_->GetChild(long(current_item));
			ASSERT(!ecase.IsEmpty() && ecase.GetName() == "case");
			CString ss = ecase.GetAttr("range");

			range_set<__int64> rs;
			std::istringstream strstr((const char *)ss);
			if (!(strstr >> rs) || rs.empty())
				ss.Empty();
			else
			{
				range_set<FILE_ADDRESS>::iterator plast = rs.end();
				plast--;
				ss.Format("%d", int(*plast + 1));  // Set to one more than the last value of cloned range
			}
			ecase.SetAttr("range", ss);

			// Get the clone elt
			ASSERT(!pelt_->GetChild(long(current_item)).IsEmpty() &&
					pelt_->GetChild(long(current_item)).GetName() == "case" &&
				   !pelt_->GetChild(long(current_item)).GetFirstChild().IsEmpty());
			current_elt = pelt_->GetChild(long(current_item)).GetFirstChild();  // get next elt

			// Add new element into list box
			ctl_cases_.InsertString(current_item, ::get_name(current_elt));
			ctl_cases_.SetCurSel(current_item);

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

		// Update range to reflect current item
		ASSERT(!pelt_->GetChild(long(current_item)).IsEmpty());
		ctl_range_.SetWindowText(pelt_->GetChild(long(current_item)).GetAttr("range"));
	}

	if (delete_on_cancel)
	{
		ASSERT(item == current_item && dlg_ret == IDCANCEL);

		// Cancel hit when inserting element (and OK/prev/next buttons never used)
		pelt_->DeleteChild(current_elt.GetParent());  // Remove inserted elt from the XML tree
		ctl_cases_.DeleteString(item);                // Remove from the list box
		ASSERT(item < ctl_cases_.GetCount());
		ctl_cases_.SetCurSel(item);
	}
	else
	{
		ASSERT(item < ctl_cases_.GetCount() - 1);
//        ctl_cases_.SetCurSel(item+1);
	}
}

void CDFFDSwitch::OnGetExprVar()
{
	if (ctl_expr_var_.m_nMenuResult != 0)
	{
		CString strVar = get_menu_text(pexpr_menu_, ctl_expr_var_.m_nMenuResult);
		ASSERT(!strVar.IsEmpty());      // The selected menu item should be found in the menu tree

		// Put the var name into the "expression" control
		for (int ii = 0; ii < strVar.GetLength (); ii++)
			ctl_expr_.SendMessage(WM_CHAR, (TCHAR)strVar[ii]);
		ctl_expr_.SetFocus();
		modified_ = true;
	}
}

void CDFFDSwitch::load_data()
{
	// Get values from XML data node
	ASSERT(pelt_ != NULL && pelt_->GetName() == "switch");

	// Fill in list of elements
	int item = -1;                      // Index of last item added
	for (CXmlTree::CElt ee(pelt_->GetFirstChild()); !ee.IsEmpty(); ++ee)
	{
		// Add this child to the CListBox
		item = ctl_cases_.AddString(::get_name(ee.GetFirstChild()));
		ASSERT(item > -1);
	}
	ctl_cases_.AddString(CString(""));   // empty item at end that allows appending
	ctl_cases_.SetCurSel(0);

	expr_ = pelt_->GetAttr("test");
	comment_ = pelt_->GetAttr("comment");
	UpdateData(FALSE);
}

bool CDFFDSwitch::check_data()
{
	// Get values from the controls and validate them
	UpdateData();

	CString ss;
	ctl_expr_.GetWindowText(ss);
	ss.TrimLeft();
	if (ss.IsEmpty())
	{
		// The switch expression is empty
		TaskMessageBox("No Expression",
		               "A SWITCH element requires an expression (of integer or string type) "
					   "that is evaluated to determine which CASE is chosen.\n\n"
					   "Please enter an integer or string expression.");
		ctl_expr_.SetFocus();
		return false;
	}

	if (pelt_->GetNumChildren() == 0)
	{
		// There is only an empty trailing entry
		TaskMessageBox("No CASE Elements", "A SWITCH requires at least one child CASE element.\n\n"
			           "Please use the \"Insert\" button to add a CASE element.\n");
		ctl_cases_.SetFocus();
		return false;
	}

#if 0  // We can now switch on a string expression whence this is no longer valid, although
	   // we could reintroduce it if we coulde determine the type of the expression in ctl_expr_
	// Check each case range
	for (int item = 0; item < ctl_cases_.GetCount() - 1; ++item)
	{
		CXmlTree::CElt ee = pelt_->GetChild(long(item));
		ASSERT(!ee.IsEmpty() && ee.GetName() == "case");
		ss = ee.GetAttr("range");

		std::istringstream strstr((const char *)ss);
		range_set<__int64> rs;
		if (!(strstr >> rs) || rs.empty())
		{
			ctl_cases_.SetCurSel(item);
			ctl_range_.SetWindowText(ss);
			AfxMessageBox("Please enter a valid range string\n"
						  "of the form \"1, 2, 6-10\".");
			ctl_range_.SetFocus();
			return false;
		}
	}
#else
	for (int item = 0; item < ctl_cases_.GetCount() - 1; ++item)
	{
		CXmlTree::CElt ee = pelt_->GetChild(long(item));
		ASSERT(!ee.IsEmpty() && ee.GetName() == "case");
		ss = ee.GetAttr("range");

		std::istringstream strstr((const char *)ss);
		// Make sure there are no empty cases (except the last)
		if (strstr.str().size() == 0 && item < ctl_cases_.GetCount() - 2)
		{
			ctl_cases_.SetCurSel(item);
			ctl_range_.SetWindowText(ss);
			TaskMessageBox("No CASE Value(s)",
				"Each CASE requires value(s) to match against the SWITCH expression.  "
				"For an integer SWITCH expression you enter an integer or a range string (eg, \"1, 2, 6-10\").  "
				"For a string switch expression just enter a string to match against.  "
				"(The exception is the last CASE in the SWITCH which may be empty to match all remaining cases.)\n\n"
				"Please enter the value(s) for this CASE element.");
			ctl_range_.SetFocus();
			return false;
		}
	}
#endif

	return true;
}

void CDFFDSwitch::save_data()
{
	// Save data to XML element node
	pelt_->SetAttr("test", expr_);
	pelt_->SetAttr("comment", comment_);
}

