// EmailDlg.cpp : implementation file
//
// Copyright (c) 2003 by Andrew W. Phillips.
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
#include "HexEdit.h"
#include <io.h>                 // for _access()
#include "EmailDlg.h"
#include "Dialog.h"             // for CHexFileDialog
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
// CEmailDlg dialog


CEmailDlg::CEmailDlg(CWnd* pParent /*=NULL*/)
        : CDialog(CEmailDlg::IDD, pParent)
{
        //{{AFX_DATA_INIT(CEmailDlg)
        to_ = _T("");
        subject_ = _T("");
        systype_ = _T("");
        text_ = _T("");
        type_ = -1;
        name_ = _T("");
        address_ = _T("");
        version_ = _T("");
        //}}AFX_DATA_INIT

    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

    to_ = "support@HexEdit.com";
    type_ = 1;
    systype_ = "This system";
//    version_ = CString("Version ") + CString(aa->version_);
    version_.Format("Version %g", double(aa->version_)/100.0);
    if (aa->beta_ != 0)
        version_ += " beta";
    attach_ = FALSE;
}

void CEmailDlg::DoDataExchange(CDataExchange* pDX)
{
        CDialog::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CEmailDlg)
        DDX_CBString(pDX, IDC_TO, to_);
        DDX_Text(pDX, IDC_SUBJECT, subject_);
        DDV_MaxChars(pDX, subject_, 60);
        DDX_CBString(pDX, IDC_SYSTYPE, systype_);
        DDX_Text(pDX, IDC_TEXT, text_);
        DDX_Radio(pDX, IDC_BUG, type_);
        DDX_Text(pDX, IDC_NAME, name_);
        DDX_Text(pDX, IDC_ADDRESS, address_);
        DDX_CBString(pDX, IDC_VERSION, version_);
        //}}AFX_DATA_MAP
    	DDX_Check(pDX, IDC_ATTACH, attach_);
        DDX_Text(pDX, IDC_ATTACHMENT, attachment_);
}

BEGIN_MESSAGE_MAP(CEmailDlg, CDialog)
        //{{AFX_MSG_MAP(CEmailDlg)
        ON_WM_HELPINFO()
        ON_BN_CLICKED(IDC_EMAIL_HELP, OnEmailHelp)
	ON_BN_CLICKED(IDC_ATTACH, OnAttach)
	ON_BN_CLICKED(IDC_ATTACHMENT_BROWSE, OnAttachmentBrowse)
	//}}AFX_MSG_MAP
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEmailDlg message handlers
BOOL CEmailDlg::OnInitDialog() 
{
    CDialog::OnInitDialog();

    ASSERT(GetDlgItem(IDC_SUBJECT) != NULL);
    GetDlgItem(IDC_SUBJECT)->SetFocus();

    ASSERT(GetDlgItem(IDC_ATTACHMENT) != NULL);
    GetDlgItem(IDC_ATTACHMENT)->EnableWindow(attach_);
    ASSERT(GetDlgItem(IDC_ATTACHMENT_BROWSE) != NULL);
    GetDlgItem(IDC_ATTACHMENT_BROWSE)->EnableWindow(attach_);

    return FALSE;
}

void CEmailDlg::OnOK() 
{
    if (!UpdateData(TRUE))
        return;

    if (subject_.IsEmpty())
    {
        ::HMessageBox("Please enter a brief description as the subject");
        ASSERT(GetDlgItem(IDC_SUBJECT) != NULL);
        GetDlgItem(IDC_SUBJECT)->SetFocus();
        return;
    }
    if (text_.IsEmpty())
    {
        ::HMessageBox("Please enter a detailed description");
        ASSERT(GetDlgItem(IDC_TEXT) != NULL);
        GetDlgItem(IDC_TEXT)->SetFocus();
        return;
    }
    if (attach_ && attachment_.IsEmpty())
    {
        ::HMessageBox("Please enter the attachment file name");
        ASSERT(GetDlgItem(IDC_ATTACHMENT) != NULL);
        GetDlgItem(IDC_ATTACHMENT)->SetFocus();
        return;
    }
    if (attach_ && _access(attachment_, 0) == -1)
    {
        ::HMessageBox("File to be attached not found");
        ASSERT(GetDlgItem(IDC_ATTACHMENT) != NULL);
        GetDlgItem(IDC_ATTACHMENT)->SetFocus();
        return;
    }

    CDialog::OnOK();
}

static DWORD id_pairs[] = { 
    IDC_TO, HIDC_TO,
    IDC_SUBJECT, HIDC_SUBJECT,
    IDC_BUG, HIDC_BUG,
    IDC_REGISTER, HIDC_REGISTER,
    IDC_ENHANCE, HIDC_ENHANCE,
    IDC_OTHER, HIDC_OTHER,
    IDC_TEXT, HIDC_TEXT,
    IDC_SYSTYPE, HIDC_SYSTYPE,
    IDC_VERSION, HIDC_VERSION,
    IDC_NAME, HIDC_NAME,
    IDC_ATTACH, HIDC_ATTACH,
    IDC_ATTACHMENT, HIDC_ATTACHMENT,
    IDC_ATTACHMENT_BROWSE, HIDC_ATTACHMENT_BROWSE,
    IDC_ADDRESS, HIDC_ADDRESS,
    IDOK, HID_EMAIL_SEND,
    IDC_EMAIL_HELP, HIDC_HELP_BUTTON,
    0,0 
}; 

BOOL CEmailDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs);
    return TRUE;
}

void CEmailDlg::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	theApp.HtmlHelpContextMenu((HWND)pWnd->GetSafeHwnd(), id_pairs);
}

void CEmailDlg::OnEmailHelp() 
{
    // Display help for the dialog
    if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_EMAIL_HELP))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

void CEmailDlg::OnAttach() 
{
    UpdateData();
	
    ASSERT(GetDlgItem(IDC_ATTACHMENT) != NULL);
    ASSERT(GetDlgItem(IDC_ATTACHMENT_BROWSE) != NULL);

    GetDlgItem(IDC_ATTACHMENT)->EnableWindow(attach_);
    GetDlgItem(IDC_ATTACHMENT_BROWSE)->EnableWindow(attach_);
    if (attach_)
        GetDlgItem(IDC_ATTACHMENT)->SetFocus();
}

void CEmailDlg::OnAttachmentBrowse() 
{
    UpdateData();

    CHexFileDialog dlgFile("AttachmentDlg", HIDD_FILE_ATTACH, TRUE, NULL, attachment_,
                           OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_DONTADDTORECENT,
                           theApp.GetCurrentFilters(), "Attach", this);

    if (dlgFile.DoModal() == IDOK)
    {
        attachment_ = dlgFile.GetPathName();
        UpdateData(FALSE);
    }
}
