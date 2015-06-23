// Password.cpp : implements dialog that gets password used for encryption
//
// Copyright (c) 2015 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#include "stdafx.h"
#include "HexEdit.h"
#include "Password.h"
#include "resource.hm"
#include "HelpID.hm"            // User defined help IDs

#include <HtmlHelp.h>

extern CHexEditApp theApp;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPassword dialog


CPassword::CPassword(CWnd* pParent /*=NULL*/)
	: CDialog(CPassword::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPassword)
	m_password = _T("");
	m_reentered = _T("");
	m_mask = FALSE;
	m_note = _T("");
	m_min = 0;
	//}}AFX_DATA_INIT

	m_note = "Notes:\n"
			 "1. Do not lose this password, as any data excrypted with it can only be\n"
			 "    recovered by decrypting with the same password (and algorithm).\n"
			 "2. Passwords are case-sensitive.\n"
			 "3. Entering the masked password twice is used to catch typos.\n"
			 "4. A good password is at least 8 characters long, does not use common \n"
			 "    words or names, and contains some non-alphabetic characters.";
}

void CPassword::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPassword)
	DDX_Text(pDX, IDC_PASSWORD_ENTER, m_password);
	DDX_Text(pDX, IDC_PASSWORD_REENTER, m_reentered);
	DDX_Check(pDX, IDC_PASSWORD_MASK, m_mask);
	DDX_Text(pDX, IDC_PASSWORD_NOTE, m_note);
	DDX_Text(pDX, IDC_PASSWORD_MIN, m_min);
	DDV_MinMaxInt(pDX, m_min, 1, 9999);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPassword, CDialog)
	//{{AFX_MSG_MAP(CPassword)
	ON_BN_CLICKED(IDC_PASSWORD_MASK, OnPasswordMask)
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_PASSWORD_HELP, OnPasswordHelp)
	//}}AFX_MSG_MAP
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPassword message handlers

BOOL CPassword::OnInitDialog()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

	m_mask = aa->password_mask_;
	m_min = aa->password_min_;

	m_reentered = m_password;

	CDialog::OnInitDialog();

	ASSERT(GetDlgItem(IDC_PASSWORD_SPIN) != NULL);
	((CSpinButtonCtrl *)GetDlgItem(IDC_PASSWORD_SPIN))->SetRange(1, 9999);

	if (m_mask)
	{
		ASSERT(GetDlgItem(IDC_PASSWORD_ENTER) != NULL);
		GetDlgItem(IDC_PASSWORD_ENTER)->SendMessage(EM_SETPASSWORDCHAR, '*');
		GetDlgItem(IDC_PASSWORD_ENTER)->Invalidate();
		ASSERT(GetDlgItem(IDC_PASSWORD_REENTER) != NULL);
		GetDlgItem(IDC_PASSWORD_REENTER)->EnableWindow(TRUE);
		ASSERT(GetDlgItem(IDC_PASSWORD_DESC) != NULL);
		GetDlgItem(IDC_PASSWORD_DESC)->EnableWindow(TRUE);
//        GetDlgItem(IDC_PASSWORD_REENTER)->SendMessage(EM_SETPASSWORDCHAR, '*');
//        GetDlgItem(IDC_PASSWORD_REENTER)->Invalidate();
	}
	else
	{
		ASSERT(GetDlgItem(IDC_PASSWORD_ENTER) != NULL);
		GetDlgItem(IDC_PASSWORD_ENTER)->SendMessage(EM_SETPASSWORDCHAR, 0);
		GetDlgItem(IDC_PASSWORD_ENTER)->Invalidate();
		ASSERT(GetDlgItem(IDC_PASSWORD_REENTER) != NULL);
		GetDlgItem(IDC_PASSWORD_REENTER)->EnableWindow(FALSE);
		ASSERT(GetDlgItem(IDC_PASSWORD_DESC) != NULL);
		GetDlgItem(IDC_PASSWORD_DESC)->EnableWindow(FALSE);
//        GetDlgItem(IDC_PASSWORD_REENTER)->SendMessage(EM_SETPASSWORDCHAR, 0);
//        GetDlgItem(IDC_PASSWORD_REENTER)->Invalidate();
	}

	return TRUE;
}

void CPassword::OnOK()
{
	if (!UpdateData())
		return;

	// Check that the password is the minimum size
	if (m_password.IsEmpty())
	{
		TaskMessageBox("No Password", "Please enter the password");
		ASSERT(GetDlgItem(IDC_PASSWORD_ENTER) != NULL);
		GetDlgItem(IDC_PASSWORD_ENTER)->SetFocus();
		return;
	}
	else if (m_mask && m_reentered.IsEmpty())
	{
		TaskMessageBox("No Password", "Please re-enter the password");
		ASSERT(GetDlgItem(IDC_PASSWORD_REENTER) != NULL);
		GetDlgItem(IDC_PASSWORD_REENTER)->SetFocus();
		return;
	}
	else if (m_mask && m_password != m_reentered)
	{
		TaskMessageBox("Passwords Different", "The passwords are not the same.");
		ASSERT(GetDlgItem(IDC_PASSWORD_ENTER) != NULL);
		GetDlgItem(IDC_PASSWORD_ENTER)->SetFocus();
		return;
	}
	else if (m_password.GetLength() < m_min)
	{
		TaskMessageBox("Password Too Short", "The password is too short");
		ASSERT(GetDlgItem(IDC_PASSWORD_ENTER) != NULL);
		GetDlgItem(IDC_PASSWORD_ENTER)->SetFocus();
		return;
	}

	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	aa->password_mask_ = m_mask;
	aa->password_min_ = m_min;

	CDialog::OnOK();
}

void CPassword::OnCancel()
{
	if (UpdateData())
	{
		CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
		aa->password_mask_ = m_mask;
		aa->password_min_ = m_min;
	}

	CDialog::OnCancel();
}

void CPassword::OnPasswordMask()
{
	UpdateData();

	if (m_mask)
	{
		ASSERT(GetDlgItem(IDC_PASSWORD_ENTER) != NULL);
		GetDlgItem(IDC_PASSWORD_ENTER)->SendMessage(EM_SETPASSWORDCHAR, '*');
		GetDlgItem(IDC_PASSWORD_ENTER)->Invalidate();
		ASSERT(GetDlgItem(IDC_PASSWORD_REENTER) != NULL);
		GetDlgItem(IDC_PASSWORD_REENTER)->EnableWindow(TRUE);
		ASSERT(GetDlgItem(IDC_PASSWORD_DESC) != NULL);
		GetDlgItem(IDC_PASSWORD_DESC)->EnableWindow(TRUE);
//        GetDlgItem(IDC_PASSWORD_REENTER)->SendMessage(EM_SETPASSWORDCHAR, '*');
//        GetDlgItem(IDC_PASSWORD_REENTER)->Invalidate();
	}
	else
	{
		ASSERT(GetDlgItem(IDC_PASSWORD_ENTER) != NULL);
		GetDlgItem(IDC_PASSWORD_ENTER)->SendMessage(EM_SETPASSWORDCHAR, 0);
		GetDlgItem(IDC_PASSWORD_ENTER)->Invalidate();
		ASSERT(GetDlgItem(IDC_PASSWORD_REENTER) != NULL);
		GetDlgItem(IDC_PASSWORD_REENTER)->EnableWindow(FALSE);
		ASSERT(GetDlgItem(IDC_PASSWORD_DESC) != NULL);
		GetDlgItem(IDC_PASSWORD_DESC)->EnableWindow(FALSE);
//        GetDlgItem(IDC_PASSWORD_REENTER)->SendMessage(EM_SETPASSWORDCHAR, 0);
//        GetDlgItem(IDC_PASSWORD_REENTER)->Invalidate();
	}
}

static DWORD id_pairs[] = { 
	IDC_PASSWORD_ENTER, HIDC_PASSWORD_ENTER,
	IDC_PASSWORD_REENTER, HIDC_PASSWORD_REENTER,
	IDC_PASSWORD_MIN, HIDC_PASSWORD_MIN,
	IDC_PASSWORD_MASK, HIDC_PASSWORD_MASK,
	0,0 
};

BOOL CPassword::OnHelpInfo(HELPINFO* pHelpInfo)
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs);
	return TRUE;
}

void CPassword::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.HtmlHelpContextMenu(pWnd, id_pairs);
}

void CPassword::OnPasswordHelp()
{
	// Display help for the dialog
	if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_PASSWORD_HELP))
		AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}
