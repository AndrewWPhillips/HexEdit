// DFFDUseStruct.cpp : implementation file
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
#include "DFFDUseStruct.h"
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
// CDFFDUseStruct dialog

CDFFDUseStruct::CDFFDUseStruct(CXmlTree::CElt *pp, signed char parent_type, CWnd* pParent /*=NULL*/)
	: CDialog(CDFFDUseStruct::IDD, pParent), saved_(pp->GetOwner())
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

	pstruct_menu_  = new CMenu;
	pstruct_menu_->CreatePopupMenu();

	// Build menu of structure definitions that the user can use
	// Note: define_struct elements are always the first children of the root (file) node
	CXmlTree::CElt ee = pelt_->GetOwner()->GetRoot().GetFirstChild();
	int item_no;
	for (item_no = 1; !ee.IsEmpty() && ee.GetName() == "define_struct"; ++item_no, ++ee)
		pstruct_menu_->AppendMenu(MF_ENABLED, item_no, ee.GetAttr("type_name"));

	// Check if no items were added (no struct definitions found)
	if (item_no == 1)
		pstruct_menu_->AppendMenu(MF_GRAYED, -1, _T("No definitions available"));

	// Hook up the menu to the BCG button
	ctl_struct_var_.m_hMenu = pstruct_menu_->GetSafeHmenu();
	ctl_struct_var_.m_bOSMenu = TRUE;
	ctl_struct_var_.m_bStayPressed = TRUE;
	ctl_struct_var_.m_bRightArrow = TRUE;

	//{{AFX_DATA_INIT(CDFFDUseStruct)
	comments_ = _T("");
	name_ = _T("");
	struct_ = _T("");
	expr_ = _T("");
	//}}AFX_DATA_INIT
}

CDFFDUseStruct::~CDFFDUseStruct()
{
	if (pstruct_menu_ != NULL)
	{
		// Destroy the menu and free the memory
		pstruct_menu_->DestroyMenu();
		delete pstruct_menu_;
	}
}

void CDFFDUseStruct::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDFFDUseStruct)
	DDX_Control(pDX, IDC_DFFD_NAME, ctl_name_);
	DDX_Control(pDX, ID_DFFD_PREV, ctl_prev_);
	DDX_Control(pDX, ID_DFFD_NEXT, ctl_next_);
	DDX_Text(pDX, IDC_DFFD_COMMENTS, comments_);
	DDX_Text(pDX, IDC_DFFD_NAME, name_);
	DDX_Text(pDX, IDC_DFFD_STRUCT, struct_);
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_DFFD_TREE_DISPLAY, expr_);
	DDX_Control(pDX, IDC_DFFD_STRUCT, ctl_struct_);
	DDX_Control(pDX, IDC_DFFD_STRUCT_VAR, ctl_struct_var_);
}

BEGIN_MESSAGE_MAP(CDFFDUseStruct, CDialog)
	//{{AFX_MSG_MAP(CDFFDUseStruct)
	ON_BN_CLICKED(ID_DFFD_NEXT, OnNext)
	ON_BN_CLICKED(ID_DFFD_PREV, OnPrev)
	ON_EN_CHANGE(IDC_DFFD_NAME, OnChange)
	ON_EN_CHANGE(IDC_DFFD_COMMENTS, OnChange)
	ON_EN_CHANGE(IDC_DFFD_STRUCT, OnChange)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
	ON_EN_CHANGE(IDC_DFFD_TREE_DISPLAY, OnChange)
	ON_WM_CONTEXTMENU()
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_BN_CLICKED(ID_DFFD_HELP, OnHelp)
	ON_BN_CLICKED(IDC_DFFD_STRUCT_VAR, OnGetStructVar)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDFFDUseStruct message handlers

BOOL CDFFDUseStruct::OnInitDialog()
{
	CDialog::OnInitDialog();
	SetIcon(AfxGetApp()->LoadIcon(IDI_STRUCT), TRUE);

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

	return TRUE;
}

static DWORD id_pairs[] = {
	IDC_DFFD_NAME, HIDC_DFFD_NAME,
	IDC_DFFD_STRUCT, HIDC_DFFD_STRUCT,
	IDC_DFFD_STRUCT_VAR, HIDC_DFFD_STRUCT_VAR,
	IDC_DFFD_TREE_DISPLAY, HIDC_DFFD_TREE_DISPLAY,
	IDC_DFFD_COMMENTS, HIDC_DFFD_COMMENTS,
	ID_DFFD_PREV, HID_DFFD_PREV,
	ID_DFFD_NEXT, HID_DFFD_NEXT,
	0,0
};

BOOL CDFFDUseStruct::OnHelpInfo(HELPINFO* pHelpInfo)
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs);
	return TRUE;
}

void CDFFDUseStruct::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.HtmlHelpContextMenu(pWnd, id_pairs);
}

void CDFFDUseStruct::OnHelp()
{
	// Display help for this page
	if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_DFFD_USE_STRUCT))
		AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

void CDFFDUseStruct::OnCancel()
{
	pelt_->DeleteAllChildren();
	saved_.InsertKids(pelt_);
	pelt_->GetOwner()->SetModified(saved_mod_flag_);

	CDialog::OnCancel();
}

void CDFFDUseStruct::OnOK()
{
	if (modified_)
	{
		if (!check_data())
			return;

		save_data();
	}

	CDialog::OnOK();
}

void CDFFDUseStruct::OnNext()
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

void CDFFDUseStruct::OnPrev()
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

LRESULT CDFFDUseStruct::OnKickIdle(WPARAM, LPARAM lCount)
{
	return FALSE;
}

void CDFFDUseStruct::OnChange()
{
	modified_ = true;
}

void CDFFDUseStruct::OnGetStructVar()
{
	if (ctl_struct_var_.m_nMenuResult != 0)
	{
		ctl_struct_.SetWindowText(get_menu_text(pstruct_menu_, ctl_struct_var_.m_nMenuResult));
		ctl_struct_.SetFocus();
		modified_ = true;
	}
}

void CDFFDUseStruct::load_data()
{
	// Get values from XML data node
	ASSERT(pelt_ != NULL && pelt_->GetName() == "use_struct");

	name_ = pelt_->GetAttr("name");
	expr_ = pelt_->GetAttr("expr");
	struct_ = pelt_->GetAttr("type_name");
	comments_ = pelt_->GetAttr("comment");

	UpdateData(FALSE);
}

bool CDFFDUseStruct::check_data()
{
	// Get values from the controls and validate them
	UpdateData();

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
		name_.CompareNoCase("index") == 0 ||
		name_.CompareNoCase("member") == 0 ||
		name_.CompareNoCase("end") == 0 ||
		expr_eval::func_token(name_) != expr_eval::TOK_NONE)
	{
		TaskMessageBox("Reserved Name", name_ + " is reserved for internal use. Please choose another name.");
		ctl_name_.SetFocus();
		return false;
	}

	if (struct_.IsEmpty())
	{
		TaskMessageBox("No Defined Struct", "Please select a defined struct for this element");
		ctl_struct_.SetFocus();
		return false;
	}
	else if (!valid_id(struct_))
	{
		TaskMessageBox("Invalid Defined Struct Name", "Please use alphanumeric characters "
					  "(or underscore) and start with a alphabetic character.");
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

	// Check that older siblings don't have the same name
	for (ee--; !ee.IsEmpty(); ee--)
	{
		CXmlTree::CElt ee2;
		for (ee2 = ee; !ee2.IsEmpty() && (ee2.GetName() == "if" || ee2.GetName() == "jump"); ee2 = ee2.GetFirstChild())
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

	for (ee = *pelt_; ee.GetParent().GetName() == "if" || ee.GetParent().GetName() == "jump"; ee = ee.GetParent())
		; // empty loop body

	// Check that younger siblings don't have the same name
	for (++ee; !ee.IsEmpty(); ++ee)
	{
		CXmlTree::CElt ee2;
		for (ee2 = ee; !ee2.IsEmpty() && (ee2.GetName() == "if" || ee2.GetName() == "jump"); ee2 = ee2.GetFirstChild())
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

	return true;
}

void CDFFDUseStruct::save_data()
{
	// Save data to XML element node
	pelt_->SetAttr("name", name_);
	pelt_->SetAttr("expr", expr_);
	pelt_->SetAttr("type_name", struct_);
	pelt_->SetAttr("comment", comments_);
}
