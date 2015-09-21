// NewScheme.cpp : implements dialog that creates a new colour scheme
//
// Copyright (c) 2015 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#include "stdafx.h"
#include "HexEdit.h"
#include "NewScheme.h"

#include <HtmlHelp.h>

#include "resource.hm"
#include "HelpID.hm"            // For help IDs for the pages

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CHexEditApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CNewScheme dialog

CNewScheme::CNewScheme(CWnd* pParent /*=NULL*/)
	: CDialog(CNewScheme::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNewScheme)
	scheme_name_ = _T("");
	copy_from_ = -1;
	//}}AFX_DATA_INIT
		psvec_ = NULL;
}

void CNewScheme::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewScheme)
	DDX_Control(pDX, IDC_SCHEME_LIST, scheme_list_ctrl_);
	DDX_Text(pDX, IDC_SCHEME_NAME, scheme_name_);
	DDX_CBIndex(pDX, IDC_SCHEME_LIST, copy_from_);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewScheme, CDialog)
	//{{AFX_MSG_MAP(CNewScheme)
	ON_BN_CLICKED(IDC_SCHEME_HELP, OnHelp)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewScheme message handlers

BOOL CNewScheme::OnInitDialog()
{
	ASSERT(psvec_ != NULL);
	CDialog::OnInitDialog();

	if (::IsUs())
		SetWindowText("New Color Scheme");

	std::vector<CScheme>::const_iterator ps;

	ASSERT(psvec_->size() > 0);
	for (ps = psvec_->begin(); ps != psvec_->end(); ++ps)
	{
		scheme_list_ctrl_.AddString(ps->name_);
	}
	scheme_list_ctrl_.SetCurSel(0);

	return TRUE;
}

void CNewScheme::OnHelp()
{
	// Display help for this page
	if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_NEW_SCHEME_HELP))
		AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

static DWORD id_pairs[] = {
	IDC_SCHEME_NAME, HIDC_SCHEME_NAME,
	IDC_SCHEME_LIST, HIDC_SCHEME_LIST,
	IDC_SCHEME_HELP, HIDC_HELP_BUTTON,
	0,0
};

BOOL CNewScheme::OnHelpInfo(HELPINFO* pHelpInfo)
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs);
	return TRUE;
}

void CNewScheme::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.HtmlHelpContextMenu(pWnd, id_pairs);
}

void CNewScheme::OnOK()
{
	UpdateData();

	// Make sure a name has been entered
	if (scheme_name_.IsEmpty())
	{
		TaskMessageBox("Name Required", "Please enter a name for the scheme.");
		ASSERT(GetDlgItem(IDC_SCHEME_NAME) != NULL);
		GetDlgItem(IDC_SCHEME_NAME)->SetFocus();
		return;
	}
	if (strchr(scheme_name_, '|') != NULL)
	{
		TaskMessageBox("Invalid Name", "A scheme name may not contain a vertical bar character(|).");
		ASSERT(GetDlgItem(IDC_SCHEME_NAME) != NULL);
		GetDlgItem(IDC_SCHEME_NAME)->SetFocus();
		return;
	}

	// make sure the name is unique
	std::vector<CScheme>::const_iterator ps;

	for (ps = psvec_->begin(); ps != psvec_->end(); ++ps)
		if (scheme_name_ == ps->name_)
		{
			TaskMessageBox("Duplicate Name", "This name is in use.  "
			               "Please enter a different name for the scheme.");
			ASSERT(GetDlgItem(IDC_SCHEME_NAME) != NULL);
			GetDlgItem(IDC_SCHEME_NAME)->SetFocus();
			return;
		}

	CDialog::OnOK();
}
