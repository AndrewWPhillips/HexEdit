// DFFDEVAL.cpp : implementation file
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
// CDFFDEval dialog


CDFFDEval::CDFFDEval(CXmlTree::CElt *pp, signed char parent_type,
				 CWnd* pParent /*=NULL*/)
	: CDialog(CDFFDEval::IDD, pParent), saved_(pp->GetOwner())
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
	pexpr_menu_ = NULL;

	//{{AFX_DATA_INIT(CDFFDEval)
	expr_ = _T("");
	display_error_ = 0;
	display_result_ = 0;
	comment_ = _T("");
	//}}AFX_DATA_INIT
}

CDFFDEval::~CDFFDEval()
{
	if (pexpr_menu_ != NULL)
	{
		// Destroy the menu and free the memory
		pexpr_menu_->DestroyMenu();
		delete pexpr_menu_;
	}
}

void CDFFDEval::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDFFDEval)
	DDX_Control(pDX, ID_DFFD_PREV, ctl_prev_);
	DDX_Control(pDX, ID_DFFD_NEXT, ctl_next_);
	DDX_Text(pDX, IDC_DFFD_EXPR, expr_);
	DDX_Check(pDX, IDC_DFFD_DISPLAY_ERROR, display_error_);
	DDX_Check(pDX, IDC_DFFD_DISPLAY_RESULT, display_result_);
	DDX_Text(pDX, IDC_DFFD_COMMENTS, comment_);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_DFFD_EXPR, ctl_expr_);
	DDX_Control(pDX, IDC_DFFD_EXPR_VAR, ctl_expr_var_);
}


BEGIN_MESSAGE_MAP(CDFFDEval, CDialog)
	//{{AFX_MSG_MAP(CDFFDEval)
	ON_BN_CLICKED(ID_DFFD_NEXT, OnNext)
	ON_BN_CLICKED(ID_DFFD_PREV, OnPrev)
	ON_EN_CHANGE(IDC_DFFD_EXPR, OnChange)
	ON_BN_CLICKED(IDC_DFFD_DISPLAY_ERROR, OnChange)
	ON_BN_CLICKED(IDC_DFFD_DISPLAY_RESULT, OnChange)
	ON_EN_CHANGE(IDC_DFFD_COMMENTS, OnChange)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
	ON_WM_CONTEXTMENU()
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_BN_CLICKED(ID_DFFD_HELP, OnHelp)
	ON_BN_CLICKED(IDC_DFFD_EXPR_VAR, OnGetExprVar)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDFFDEval message handlers

BOOL CDFFDEval::OnInitDialog()
{
	CDialog::OnInitDialog();
	SetIcon(AfxGetApp()->LoadIcon(IDI_EVAL), TRUE);

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

	// Set up menu that allows the user to choose any valid variable name
	pexpr_menu_ = make_var_menu_tree(*pelt_);
	ctl_expr_var_.m_hMenu = pexpr_menu_->GetSafeHmenu();
	ctl_expr_var_.m_bOSMenu = TRUE;
	ctl_expr_var_.m_bStayPressed = TRUE;
	ctl_expr_var_.m_bRightArrow = TRUE;
	ctl_expr_.SetSel(0, -1);           // Select all so a var selection (see OnGetConditionVar) replaces current contents
	return TRUE;
}

void CDFFDEval::OnCancel()
{
	pelt_->DeleteAllChildren();
	saved_.InsertKids(pelt_);
	pelt_->GetOwner()->SetModified(saved_mod_flag_);

	CDialog::OnCancel();
}

void CDFFDEval::OnOK()
{
	if (modified_)
	{
		if (!check_data())
			return;

		save_data();
	}

	CDialog::OnOK();
}

void CDFFDEval::OnNext()
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

void CDFFDEval::OnPrev()
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

LRESULT CDFFDEval::OnKickIdle(WPARAM, LPARAM lCount)
{
	return FALSE;
}

void CDFFDEval::OnChange()
{
	modified_ = true;
}

void CDFFDEval::OnGetExprVar()
{
	if (ctl_expr_var_.m_nMenuResult != 0)
	{
		CString strVar = get_menu_text(pexpr_menu_, ctl_expr_var_.m_nMenuResult);
		ASSERT(!strVar.IsEmpty());      // The selected menu item should be found in the menu tree

		// Put the var name into the "expr" control
		for (int ii = 0; ii < strVar.GetLength (); ii++)
			ctl_expr_.SendMessage(WM_CHAR, (TCHAR)strVar[ii]);
		ctl_expr_.SetFocus();
		modified_ = true;
	}
}

void CDFFDEval::load_data()
{
	expr_ = pelt_->GetAttr("expr");
	display_error_  = pelt_->GetAttr("display_error").CompareNoCase("true") == 0 ? 1 : 0;
	display_result_ = pelt_->GetAttr("display_result").CompareNoCase("true") == 0 ? 1 : 0;
	comment_ = pelt_->GetAttr("comment");

	UpdateData(FALSE);
}

bool CDFFDEval::check_data()
{
	// Get values from the controls and validate them
	UpdateData();

	if (expr_.IsEmpty())
	{
		TaskMessageBox("Expression Not Specified",
			"Please enter an expression to be evaluated.\n\n"
			"This can simply be the name of a field or variable or "
			"a more complex expression.");
		GetDlgItem(IDC_DFFD_EXPR)->SetFocus();
		return false;
	}

	return true;
}

void CDFFDEval::save_data()
{
	// Save data to XML element node
	pelt_->SetAttr("expr", expr_);
	pelt_->SetAttr("display_error", display_error_ ? "true" : "false");
	pelt_->SetAttr("display_result", display_result_ ? "true" : "false");
	pelt_->SetAttr("comment", comment_);
}

void CDFFDEval::OnHelp()
{
	// Display help for this page
	if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_DFFD_EVAL))
		AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

static DWORD id_pairs[] = { 
	IDC_DFFD_EXPR, HIDC_DFFD_EXPR,
	IDC_DFFD_EXPR_VAR, HIDC_DFFD_EXPR_VAR,
	IDC_DFFD_DISPLAY_ERROR, HIDC_DFFD_DISPLAY_ERROR,
	IDC_DFFD_DISPLAY_RESULT, HIDC_DFFD_DISPLAY_RESULT,
	IDC_DFFD_COMMENTS, HIDC_DFFD_COMMENTS,
	ID_DFFD_PREV, HID_DFFD_PREV,
	ID_DFFD_NEXT, HID_DFFD_NEXT,
	0,0
};

BOOL CDFFDEval::OnHelpInfo(HELPINFO* pHelpInfo)
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs);
	return TRUE;
}

void CDFFDEval::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.HtmlHelpContextMenu(pWnd, id_pairs);
}

