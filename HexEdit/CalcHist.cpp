// CalcHist.cpp - implements CCalcHistDlg dialog
//
// Copyright (c) 2015 by Andrew W. Phillips.
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//
// Shows the calculator history (tape)
// which shows the expression and result of all
// added when the Calculator's = or Go button is selected
//

#include "stdafx.h"

#include "HexEdit.h"
#include "MainFrm.h"
#include <HtmlHelp.h>
#include "resource.hm"      // Help IDs
#include "HelpID.hm"        // For dlg help ID

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static DWORD id_pairs[] = { 
	IDC_CALC_HIST_CLEAR, HIDC_CALC_HIST_CLEAR,
	0,0
};

CCalcHistDlg::CCalcHistDlg() : CDialog()
{
	help_hwnd_ = (HWND)0;
	m_first = true;
}

BOOL CCalcHistDlg::Create(CWnd *pParentWnd)
{
	if (!CDialog::Create(MAKEINTRESOURCE(IDD), pParentWnd)) // IDD_CALC_HIST
	{
		TRACE0("Failed to create calculator history window\n");
		return FALSE; // failed to create
	}

	//ctl_hist_clear_.SetImage(IDB_BM_ADD, IDB_BM_ADD_HOT);
	ctl_hist_clear_.m_bTransparent = TRUE;
	ctl_hist_clear_.m_nFlatStyle = CMFCButton::BUTTONSTYLE_FLAT;
	ctl_hist_clear_.SetTooltip(_T("Clear calculator history"));
	//ctl_hist_clear_.Invalidate();

	// Set up resizer control
	// We must set the 4th parameter true else we get a resize border
	// added to the dialog and this really stuffs things up inside a pane.
	m_resizer.Create(GetSafeHwnd(), TRUE, 100, TRUE);

	// It needs an initial size for it's calcs
	CRect rct;
	GetWindowRect(&rct);
	m_resizer.SetInitialSize(rct.Size());
	m_resizer.SetMinimumTrackingSize(rct.Size());

	// This can cause problems if done too early (OnCreate or OnInitDialog)
	m_resizer.Add(IDC_CALC_HIST_TEXT, 0, 0, 100, 100);

	return TRUE;
}

void CCalcHistDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CALC_HIST_CLEAR,  ctl_hist_clear_);
}

BEGIN_MESSAGE_MAP(CCalcHistDlg, CDialog)
	//ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_CALC_HIST_CLEAR, OnClear)
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	//ON_MESSAGE_VOID(WM_INITIALUPDATE, OnInitialUpdate)
	//ON_WM_ERASEBKGND()
	//ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

void CCalcHistDlg::Add(LPCTSTR str)
{
	GetDlgItem(IDC_CALC_HIST_TEXT)->SendMessage(EM_SETSEL, 0, 0);
	GetDlgItem(IDC_CALC_HIST_TEXT)->SendMessage(EM_REPLACESEL, 0, LPARAM("\r\n"));
	GetDlgItem(IDC_CALC_HIST_TEXT)->SendMessage(EM_SETSEL, 0, 0);
	GetDlgItem(IDC_CALC_HIST_TEXT)->SendMessage(EM_REPLACESEL, 0, LPARAM(str));
}

// Message handlers
LRESULT CCalcHistDlg::OnKickIdle(WPARAM, LPARAM lCount)
{
	if (m_first)
	{
		m_first = false;
	}

	ASSERT(GetDlgItem(IDC_CALC_HIST_CLEAR) != NULL);
	GetDlgItem(IDC_CALC_HIST_CLEAR)->EnableWindow(TRUE);

	// Display context help for ctrl set up in OnHelpInfo
	if (help_hwnd_ != (HWND)0)
	{
		theApp.HtmlHelpWmHelp(help_hwnd_, id_pairs);
		help_hwnd_ = (HWND)0;
	}
	return FALSE;
}

void CCalcHistDlg::OnClear()
{
	SetDlgItemText(IDC_CALC_HIST_TEXT, "");
}

void CCalcHistDlg::OnHelp()
{
	if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_CALC_HIST_HELP))
		AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

BOOL CCalcHistDlg::OnHelpInfo(HELPINFO* pHelpInfo)
{
	// Note calling theApp.HtmlHelpWmHelp here seems to make the window go behind 
	// and then disappear when mouse up evenet is seen.  The only soln I could
	// find after a lot of experimenetation is to do it later (in OnKickIdle).
	help_hwnd_ = (HWND)pHelpInfo->hItemHandle;
	return TRUE;
}

void CCalcHistDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.HtmlHelpContextMenu(pWnd, id_pairs);
}
