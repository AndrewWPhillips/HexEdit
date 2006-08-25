// ClearHistDlg.cpp : implementation file
//

#include "stdafx.h"
#include "HexEdit.h"
#include "ClearHistDlg.h"

#ifdef USE_HTML_HELP
#include <HtmlHelp.h>
#endif

#include "resource.hm"
#include "HelpID.hm"            // For help IDs for the pages

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CClearHistDlg dialog


CClearHistDlg::CClearHistDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CClearHistDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CClearHistDlg)
	clear_bookmarks_ = FALSE;
	clear_recent_file_list_ = FALSE;
	clear_hist_ = FALSE;
	clear_on_exit_ = FALSE;
	//}}AFX_DATA_INIT
}

void CClearHistDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CClearHistDlg)
	DDX_Check(pDX, IDC_CLEAR_BOOKMARKS, clear_bookmarks_);
	DDX_Check(pDX, IDC_CLEAR_RECENT_FILE, clear_recent_file_list_);
	DDX_Check(pDX, IDC_CLEAR_SEARCH, clear_hist_);
	DDX_Check(pDX, IDC_CLEAR_ON_EXIT, clear_on_exit_);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CClearHistDlg, CDialog)
	//{{AFX_MSG_MAP(CClearHistDlg)
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_CLEAR_HELP, OnHelp)
	//}}AFX_MSG_MAP
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CClearHistDlg message handlers

BOOL CClearHistDlg::OnInitDialog() 
{
    clear_hist_ = theApp.clear_hist_;
    clear_recent_file_list_ = theApp.clear_recent_file_list_;
    clear_bookmarks_ = theApp.clear_bookmarks_;
    clear_on_exit_ = theApp.clear_on_exit_;

	CDialog::OnInitDialog();
	
	return TRUE;
}

void CClearHistDlg::OnOK() 
{
    UpdateData();

    // Clear the lists
    theApp.ClearHist(clear_hist_, clear_recent_file_list_, clear_bookmarks_);

    // Save settings used
    theApp.clear_hist_ = clear_hist_;
    theApp.clear_recent_file_list_ = clear_recent_file_list_;
    theApp.clear_bookmarks_ = clear_bookmarks_;

    // Save clear on exit flag too
    theApp.clear_on_exit_ = clear_on_exit_;

	CDialog::OnOK();
}

void CClearHistDlg::OnHelp() 
{
    // Display help for this page
#ifdef USE_HTML_HELP
    if (!theApp.htmlhelp_file_.IsEmpty())
    {
        if (::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_CLEAR_HIST_HELP))
            return;
    }
#endif
    if (!::WinHelp(AfxGetMainWnd()->m_hWnd, AfxGetApp()->m_pszHelpFilePath,
                   HELP_CONTEXT, HIDD_CLEAR_HIST_HELP))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

static DWORD id_pairs[] = { 
    IDC_CLEAR_SEARCH, HIDC_CLEAR_SEARCH,
    IDC_CLEAR_RECENT_FILE, HIDC_CLEAR_RECENT_FILE,
    IDC_CLEAR_BOOKMARKS, HIDC_CLEAR_BOOKMARKS,
    IDC_CLEAR_ON_EXIT, HIDC_CLEAR_ON_EXIT,
    IDC_CLEAR_HELP, HIDC_HELP_BUTTON,
    IDOK, HIDC_CLEAR_LISTS,
    0,0
};

BOOL CClearHistDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    ASSERT(theApp.m_pszHelpFilePath != NULL);

    CWaitCursor wait;

    if (!::WinHelp((HWND)pHelpInfo->hItemHandle, theApp.m_pszHelpFilePath, 
                   HELP_WM_HELP, (DWORD) (LPSTR) id_pairs))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
    return TRUE;
//	return CDialog::OnHelpInfo(pHelpInfo);
}

void CClearHistDlg::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    ASSERT(theApp.m_pszHelpFilePath != NULL);

    CWaitCursor wait;

    if (!::WinHelp((HWND)pWnd->GetSafeHwnd(), theApp.m_pszHelpFilePath, 
                   HELP_CONTEXTMENU, (DWORD) (LPSTR) id_pairs))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

