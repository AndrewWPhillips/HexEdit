// DFFDGlobal.cpp : implementation file
//
// Copyright (c) 2004 by Andrew W. Phillips.
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
#include "hexedit.h"
#include "hexeditDoc.h"
#include "DFFDGlobal.h"
#include "resource.hm"      // Help IDs
#include "HelpID.hm"            // User defined help IDs

#include <HtmlHelp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDFFDGlobal dialog


CDFFDGlobal::CDFFDGlobal(CXmlTree::CElt *pp, CWnd* pParent /*=NULL*/)
    : CDialog(CDFFDGlobal::IDD, pParent)
{
    pelt_ = pp;
    modified_ = false;
    web_changed_ = false;

    //{{AFX_DATA_INIT(CDFFDGlobal)
    web_site_ = _T("");
    default_char_set_ = -1;
    default_big_endian_ = FALSE;
    default_read_only_ = FALSE;
    //}}AFX_DATA_INIT
}

void CDFFDGlobal::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDFFDGlobal)
    DDX_Control(pDX, IDC_WEB_ADDRESS, ctl_web_site_);
    DDX_Text(pDX, IDC_WEB_ADDRESS, web_site_);
    DDX_Radio(pDX, IDC_DFFD_ASCII, default_char_set_);
    DDX_Check(pDX, IDC_DFFD_DEFAULT_BIG_ENDIAN, default_big_endian_);
    DDX_Check(pDX, IDC_DFFD_DEFAULT_READ_ONLY, default_read_only_);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDFFDGlobal, CDialog)
    //{{AFX_MSG_MAP(CDFFDGlobal)
    ON_EN_CHANGE(IDC_WEB_ADDRESS, OnChangeWebAddress)
    ON_BN_CLICKED(IDC_WEB_CHECK, OnWebCheck)
    ON_BN_CLICKED(IDC_DFFD_OEM, OnChange)
    ON_BN_CLICKED(IDC_DFFD_EBCDIC, OnChange)
    ON_BN_CLICKED(IDC_DFFD_ANSI, OnChange)
    ON_BN_CLICKED(IDC_DFFD_ASCII, OnChange)
    ON_BN_CLICKED(IDC_DFFD_DEFAULT_BIG_ENDIAN, OnChange)
    ON_BN_CLICKED(IDC_DFFD_DEFAULT_READ_ONLY, OnChange)
    ON_WM_HELPINFO()
    //}}AFX_MSG_MAP
    ON_WM_CONTEXTMENU()
	ON_BN_CLICKED(ID_DFFD_HELP, OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDFFDGlobal message handlers

BOOL CDFFDGlobal::OnInitDialog() 
{
    CDialog::OnInitDialog();

	// Show the template name in the dialog title
	CView *pv = dynamic_cast<CView *>(m_pParentWnd);
	ASSERT(pv != NULL);
	if (pv != NULL)
	{
		CHexEditDoc *pdoc = dynamic_cast<CHexEditDoc *>(pv->GetDocument());
		ASSERT(pdoc != NULL);
		if (pdoc != NULL)
			SetWindowText("Global Settings for \"" + pdoc->GetXMLFileDesc() +".XML\"");
	}

    ASSERT(pelt_ != NULL && pelt_->GetName() == "binary_file_format");

    CString ss;

    ss = pelt_->GetAttr("default_byte_order");
    default_big_endian_ = (ss == "big");

    ss = pelt_->GetAttr("default_read_only");
    default_read_only_ = (ss == "true");

    ss = pelt_->GetAttr("default_char_set");
    if (ss == "ansi")
        default_char_set_ = 1;
    else if (ss == "oem")
        default_char_set_ = 2;
    else if (ss == "ebcdic")
        default_char_set_ = 3;
    else
    {
        ASSERT(ss == "ascii");
        default_char_set_ = 0;
    }

    ss = pelt_->GetAttr("web_site");
    if (ss.IsEmpty())
        web_site_ = "http://";
    else
        web_site_ = ss;

    UpdateData(FALSE);

    return TRUE;
}

static DWORD id_pairs[] = { 
    IDC_DFFD_DEFAULT_READ_ONLY, HIDC_DFFD_DEFAULT_READ_ONLY,
    IDC_DFFD_DEFAULT_BIG_ENDIAN, HIDC_DFFD_DEFAULT_BIG_ENDIAN,
    IDC_DFFD_ASCII, HIDC_DFFD_ASCII,
    IDC_DFFD_ANSI, HIDC_DFFD_ANSI,
    IDC_DFFD_OEM, HIDC_DFFD_OEM,
    IDC_DFFD_EBCDIC, HIDC_DFFD_EBCDIC,
    IDC_WEB_ADDRESS, HIDC_WEB_ADDRESS,
    IDC_WEB_CHECK, HIDC_WEB_CHECK,
    0,0 
}; 

BOOL CDFFDGlobal::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs);
    return TRUE;
}

void CDFFDGlobal::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	theApp.HtmlHelpContextMenu(pWnd, id_pairs);
}

void CDFFDGlobal::OnHelp() 
{
    if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_DFFD_GLOBAL))
        AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

void CDFFDGlobal::OnOK() 
{
    if (modified_)
    {
        UpdateData();

        if (default_big_endian_)
            pelt_->SetAttr("default_byte_order", "big");
        else
            pelt_->SetAttr("default_byte_order", "little");

        if (default_read_only_)
            pelt_->SetAttr("default_read_only", "true");
        else
            pelt_->SetAttr("default_read_only", "false");

        switch (default_char_set_)
        {
        default:
            ASSERT(0);
        case 0:
            pelt_->SetAttr("default_char_set", "ascii");
            break;
        case 1:
            pelt_->SetAttr("default_char_set", "ansi");
            break;
        case 2:
            pelt_->SetAttr("default_char_set", "oem");
            break;
        case 3:
            pelt_->SetAttr("default_char_set", "ebcdic");
            break;
        }

        // On ly write back if changed (avoids replacing an mpty string with "http://")
        if (web_changed_)
            pelt_->SetAttr("web_site", web_site_);
    }

    CDialog::OnOK();
}

void CDFFDGlobal::OnChange() 
{
    modified_ = true;
}

void CDFFDGlobal::OnChangeWebAddress() 
{
    modified_ = true;
    web_changed_ = true;
}

void CDFFDGlobal::OnWebCheck() 
{
    UpdateData();

    CString ss = web_site_;
    if (ss.Left(7) != "http://")
        ss = "http://" + ss;
    ::ShellExecute(AfxGetMainWnd()->m_hWnd, _T("open"), ss, NULL, NULL, SW_SHOWNORMAL);
}
