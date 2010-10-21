// HexPrintDialog.cpp : implementation file
//

#include "stdafx.h"
#include "hexedit.h"
#include "HexPrintDialog.h"
#include "Dlgs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
 Note: After editing IDD_PRINTDLG in the dialog editor the icon size becomes stuffed
	   and can't be resized.  You need to manually patch HexEdit.rc with this line:

	ICON            "",1086,162,124,76,24,SS_CENTERIMAGE | WS_GROUP
*/

/////////////////////////////////////////////////////////////////////////////
// CHexPrintDialog

IMPLEMENT_DYNAMIC(CHexPrintDialog, CPrintDialog)

CHexPrintDialog::CHexPrintDialog(BOOL bPrintSetupOnly, DWORD dwFlags, CWnd* pParentWnd) :
	CPrintDialog(bPrintSetupOnly, dwFlags, pParentWnd)
{
	m_pd.hInstance = AfxGetResourceHandle();
	m_pd.lpPrintTemplateName = MAKEINTRESOURCE(IDD_PRINTDLG);
	m_pd.Flags |= PD_ENABLEPRINTTEMPLATE;
}

void CHexPrintDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_PRINT_DUP_LINES, dup_lines_);
}

BEGIN_MESSAGE_MAP(CHexPrintDialog, CPrintDialog)
	//{{AFX_MSG_MAP(CHexPrintDialog)
	ON_BN_CLICKED(IDC_PRINT_OPTIONS, OnPrintOptions)
	//}}AFX_MSG_MAP

	// We can't use the ON_BN_CLICKED macro as it assumes the called function is void.
	// We need to return FALSE from the function so that the normal Windows handling of
	// the radio buttons still works.
#if _MFC_VER < 0x0700
	{ WM_COMMAND, (WORD)BN_CLICKED, (WORD)rad1, (WORD)rad1, AfxSig_bv, (AFX_PMSG)&OnClickAll },
	{ WM_COMMAND, (WORD)BN_CLICKED, (WORD)rad3, (WORD)rad3, AfxSig_bv, (AFX_PMSG)&OnClickPages },
	{ WM_COMMAND, (WORD)BN_CLICKED, (WORD)rad2, (WORD)rad2, AfxSig_bv, (AFX_PMSG)&OnClickSel },
#else
	{ WM_COMMAND, (WORD)BN_CLICKED, (WORD)rad1, (WORD)rad1, AfxSigCmd_b, (AFX_PMSG)&OnClickAll },
	{ WM_COMMAND, (WORD)BN_CLICKED, (WORD)rad3, (WORD)rad3, AfxSigCmd_b, (AFX_PMSG)&OnClickPages },
	{ WM_COMMAND, (WORD)BN_CLICKED, (WORD)rad2, (WORD)rad2, AfxSigCmd_b, (AFX_PMSG)&OnClickSel },
#endif
END_MESSAGE_MAP()

BOOL CHexPrintDialog::OnInitDialog() 
{
	dup_lines_ = theApp.GetProfileInt("Printer", "MergeDup", 1);

	CPrintDialog::OnInitDialog();

	if ((m_pd.Flags & PD_NOSELECTION) != 0)
	{
		// Disable repeated lines control (only available when there is a selection)
		ASSERT(GetDlgItem(IDC_PRINT_DUP_LINES) != NULL);
		GetDlgItem(IDC_PRINT_DUP_LINES)->EnableWindow(FALSE);
	}

	// Disable page range controls
	ASSERT(GetDlgItem(stc2) != NULL);
	ASSERT(GetDlgItem(stc3) != NULL);
	ASSERT(GetDlgItem(edt1) != NULL);
	ASSERT(GetDlgItem(edt2) != NULL);
	GetDlgItem(stc2)->EnableWindow(FALSE);
	GetDlgItem(stc3)->EnableWindow(FALSE);
	GetDlgItem(edt1)->EnableWindow(FALSE);
	GetDlgItem(edt2)->EnableWindow(FALSE);

#if 0
	// Create Help button if it does not exist
	if (GetDlgItem(pshHelp) == NULL)
	{
		CRect rct, rctOK, rctGroup;

		GetDlgItem(IDOK)->GetWindowRect(&rctOK);
		ScreenToClient(rctOK);
		GetDlgItem(grp4)->GetWindowRect(&rctGroup);
		ScreenToClient(rctGroup);

		HWND hwndHelp = ::CreateWindow(
			"button",
			"&Help Button",
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
			rctGroup.left,
			rctOK.top,
			rctOK.Width(),
			rctOK.Height(),
			m_hWnd,
			(HMENU)pshHelp,
			(HINSTANCE)GetWindowLong(m_hWnd, GWL_HINSTANCE),
			NULL);
		
		HFONT hFont = (HFONT)SendDlgItemMessage(IDOK, WM_GETFONT, 0, 0);
		::SendMessage(hwndHelp, WM_SETFONT, (WPARAM)hFont, 0);
	}
#endif

	return TRUE;
}

void CHexPrintDialog::OnOK()
{
	CPrintDialog::OnOK();
	theApp.WriteProfileInt("Printer", "MergeDup", dup_lines_);
}

BOOL CHexPrintDialog::OnClickAll() 
{
	ASSERT(GetDlgItem(stc2) != NULL);
	ASSERT(GetDlgItem(stc3) != NULL);
	ASSERT(GetDlgItem(edt1) != NULL);
	ASSERT(GetDlgItem(edt2) != NULL);
	GetDlgItem(stc2)->EnableWindow(FALSE);
	GetDlgItem(stc3)->EnableWindow(FALSE);
	GetDlgItem(edt1)->EnableWindow(FALSE);
	GetDlgItem(edt2)->EnableWindow(FALSE);

	ASSERT(GetDlgItem(IDC_PRINT_DUP_LINES) != NULL);
	GetDlgItem(IDC_PRINT_DUP_LINES)->EnableWindow(FALSE);

	return FALSE;
}

BOOL CHexPrintDialog::OnClickPages() 
{
	ASSERT(GetDlgItem(stc2) != NULL);
	ASSERT(GetDlgItem(stc3) != NULL);
	ASSERT(GetDlgItem(edt1) != NULL);
	ASSERT(GetDlgItem(edt2) != NULL);
	GetDlgItem(stc2)->EnableWindow(TRUE);
	GetDlgItem(stc3)->EnableWindow(TRUE);
	GetDlgItem(edt1)->EnableWindow(TRUE);
	GetDlgItem(edt2)->EnableWindow(TRUE);

	ASSERT(GetDlgItem(IDC_PRINT_DUP_LINES) != NULL);
	GetDlgItem(IDC_PRINT_DUP_LINES)->EnableWindow(FALSE);

	return FALSE;
}

BOOL CHexPrintDialog::OnClickSel() 
{
	ASSERT(GetDlgItem(stc2) != NULL);
	ASSERT(GetDlgItem(stc3) != NULL);
	ASSERT(GetDlgItem(edt1) != NULL);
	ASSERT(GetDlgItem(edt2) != NULL);
	GetDlgItem(stc2)->EnableWindow(FALSE);
	GetDlgItem(stc3)->EnableWindow(FALSE);
	GetDlgItem(edt1)->EnableWindow(FALSE);
	GetDlgItem(edt2)->EnableWindow(FALSE);

	ASSERT(GetDlgItem(IDC_PRINT_DUP_LINES) != NULL);
	GetDlgItem(IDC_PRINT_DUP_LINES)->EnableWindow(TRUE);

	return FALSE;
}

void CHexPrintDialog::OnPrintOptions() 
{
	theApp.display_options(PRINTER_OPTIONS_PAGE, TRUE);
}

