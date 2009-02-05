// TParseDlg.cpp : implementation file
//

#include "stdafx.h"
#include <MultiMon.h>
#include "hexedit.h"
#include "TParseDlg.h"

#include "resource.hm"    // help IDs
#include "HelpID.hm"            // User defined help IDs

#include <HtmlHelp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// xxx add checkbox for "use INCLUDE env var" and button for include directories (use CBCGEditListBox?)
// xxx see TParser::search_include_ and GetProfileString("DataFormat", "IncludeFolders")

/////////////////////////////////////////////////////////////////////////////
// TParseDlg dialog

TParseDlg::TParseDlg(CWnd* pParent /*=NULL*/)
	: CDialog(TParseDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(TParseDlg)
	m_source_code = _T("");
	m_pack = -1;
	m_bitfield_up = -1;
	m_types_common = FALSE;
	m_types_custom = FALSE;
	m_types_save = FALSE;
	m_types_std = FALSE;
	m_types_windows = FALSE;
	m_values_custom = FALSE;
	m_values_save = FALSE;
	m_values_windows = FALSE;
	//}}AFX_DATA_INIT
	m_pack = theApp.GetProfileInt("Parse-Settings", "Pack", 3);    // VC++ default packing is 8 bytes
	m_bitfield_up = theApp.GetProfileInt("Parse-Settings", "BitfieldDirn", 0);
	m_types_common = theApp.GetProfileInt("Parse-Settings", "TypesCommon", TRUE);
	m_types_custom = theApp.GetProfileInt("Parse-Settings", "TypesCustom", TRUE);
	m_types_save = theApp.GetProfileInt("Parse-Settings", "TypesSave", FALSE);
	m_types_std = theApp.GetProfileInt("Parse-Settings", "TypesStd", TRUE);
	m_types_windows = theApp.GetProfileInt("Parse-Settings", "TypesWindows", TRUE);
	m_values_custom = theApp.GetProfileInt("Parse-Settings", "ValuesCustom", TRUE);
	m_values_save = theApp.GetProfileInt("Parse-Settings", "ValuesSave", FALSE);
	m_values_windows = theApp.GetProfileInt("Parse-Settings", "ValuesWindows", TRUE);
}

void TParseDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(TParseDlg)
	DDX_Text(pDX, IDC_SOURCE_CODE, m_source_code);
	DDX_Radio(pDX, IDC_PACK1, m_pack);
	DDX_Check(pDX, IDC_TYPES_COMMON, m_types_common);
	DDX_Check(pDX, IDC_TYPES_CUSTOM, m_types_custom);
	DDX_Check(pDX, IDC_TYPES_SAVE, m_types_save);
	DDX_Check(pDX, IDC_TYPES_STD, m_types_std);
	DDX_Check(pDX, IDC_TYPES_WINDOWS, m_types_windows);
	DDX_Check(pDX, IDC_VALUES_CUSTOM, m_values_custom);
	DDX_Check(pDX, IDC_VALUES_SAVE, m_values_save);
	DDX_Check(pDX, IDC_VALUES_WINDOWS, m_values_windows);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(TParseDlg, CDialog)
	//{{AFX_MSG_MAP(TParseDlg)
	ON_BN_CLICKED(IDC_VALUES_CUSTOM, OnValuesCustom)
	ON_BN_CLICKED(IDC_TYPES_CUSTOM, OnTypesCustom)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(ID_DFFD_HELP, OnHelp)
    ON_WM_HELPINFO()
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// TParseDlg message handlers

BOOL TParseDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	resizer_.Create(this);
	CRect rct;
	GetWindowRect(&rct);
	resizer_.SetMinimumTrackingSize(CSize((rct.right-rct.left)/2, rct.bottom-rct.top));
	resizer_.SetGripEnabled(TRUE);
    resizer_.Add(IDC_SOURCE_CODE, 0, 0, 100, 100);
    resizer_.Add(IDC_PACK_GROUP, 100, 0, 0, 0);
    resizer_.Add(IDC_PACK1, 100, 0, 0, 0);
    resizer_.Add(IDC_PACK2, 100, 0, 0, 0);
    resizer_.Add(IDC_PACK4, 100, 0, 0, 0);
    resizer_.Add(IDC_PACK8, 100, 0, 0, 0);
    resizer_.Add(IDC_PACK16, 100, 0, 0, 0);
    resizer_.Add(IDC_VALUES_GROUP, 100, 0, 0, 0);
    resizer_.Add(IDC_VALUES_WINDOWS, 100, 0, 0, 0);
    resizer_.Add(IDC_VALUES_CUSTOM, 100, 0, 0, 0);
    resizer_.Add(IDC_VALUES_SAVE, 100, 0, 0, 0);
    resizer_.Add(IDC_TYPES_GROUP, 100, 0, 0, 0);
    resizer_.Add(IDC_TYPES_STD, 100, 0, 0, 0);
    resizer_.Add(IDC_TYPES_COMMON, 100, 0, 0, 0);
    resizer_.Add(IDC_TYPES_WINDOWS, 100, 0, 0, 0);
    resizer_.Add(IDC_TYPES_CUSTOM, 100, 0, 0, 0);
    resizer_.Add(IDC_TYPES_SAVE, 100, 0, 0, 0);
    resizer_.Add(IDOK, 100, 0, 0, 0);
    resizer_.Add(IDCANCEL, 100, 0, 0, 0);
    resizer_.Add(ID_DFFD_HELP, 100, 100, 0, 0);

    int posx = theApp.GetProfileInt("Window-Settings", "SourceParseDlgX", -30000);
    int posy = theApp.GetProfileInt("Window-Settings", "SourceParseDlgY", -30000);
    int width = theApp.GetProfileInt("Window-Settings", "SourceParseDlgWidth", -30000);
    int height = theApp.GetProfileInt("Window-Settings", "SourceParseDlgHeight", -30000);

    if (posx != -30000)
    {
        CRect rr;               // Rectangle where we will put the dialog
        GetWindowRect(&rr);

        // Move to where it was when it was last closed
//        rr.OffsetRect(posx - rr.left, posy - rr.top);
		rr.left = posx;
		rr.top = posy;
		rr.right = rr.left + width;
		rr.bottom = rr.top + height;

        CRect scr_rect;         // Rectangle that we want to make sure the window is within

        // Get the rectangle that contains the screen work area (excluding system bars etc)
        if (theApp.mult_monitor_)
        {
            HMONITOR hh = MonitorFromRect(&rr, MONITOR_DEFAULTTONEAREST);
            MONITORINFO mi;
            mi.cbSize = sizeof(mi);
            if (hh != 0 && GetMonitorInfo(hh, &mi))
                scr_rect = mi.rcWork;  // work area of nearest monitor
            else
            {
                // Shouldn't happen but if it does use the whole virtual screen
                ASSERT(0);
                scr_rect = CRect(::GetSystemMetrics(SM_XVIRTUALSCREEN),
                    ::GetSystemMetrics(SM_YVIRTUALSCREEN),
                    ::GetSystemMetrics(SM_XVIRTUALSCREEN) + ::GetSystemMetrics(SM_CXVIRTUALSCREEN),
                    ::GetSystemMetrics(SM_YVIRTUALSCREEN) + ::GetSystemMetrics(SM_CYVIRTUALSCREEN));
            }
        }
        else if (!::SystemParametersInfo(SPI_GETWORKAREA, 0, &scr_rect, 0))
        {
            // I don't know if this will ever happen since the Windows documentation
            // is pathetic and does not say when or why SystemParametersInfo might fail.
            scr_rect = CRect(0, 0, ::GetSystemMetrics(SM_CXFULLSCREEN),
                                   ::GetSystemMetrics(SM_CYFULLSCREEN));
        }

        if (rr.left > scr_rect.right - 20)              // off right edge?
            rr.OffsetRect(scr_rect.right - (rr.left+rr.right)/2, 0);
        if (rr.right < scr_rect.left + 20)              // off left edge?
            rr.OffsetRect(scr_rect.left - (rr.left+rr.right)/2, 0);
        if (rr.top > scr_rect.bottom - 20)              // off bottom?
            rr.OffsetRect(0, scr_rect.bottom - (rr.top+rr.bottom)/2);
        // This is not analogous to the prev. 3 since we don't want the window
        // off the top at all, otherwise you can get to the drag bar to move it.
        if (rr.top < scr_rect.top)                      // off top at all?
            rr.OffsetRect(0, scr_rect.top - rr.top);

        MoveWindow(&rr);
    }

	return TRUE;
}

void TParseDlg::OnDestroy() 
{
	// Save window position so t can be restored when dialog is reopened
    CRect rr;
    GetWindowRect(&rr);
    theApp.WriteProfileInt("Window-Settings", "SourceParseDlgX", rr.left);
    theApp.WriteProfileInt("Window-Settings", "SourceParseDlgY", rr.top);
    theApp.WriteProfileInt("Window-Settings", "SourceParseDlgWidth", rr.right - rr.left);
    theApp.WriteProfileInt("Window-Settings", "SourceParseDlgHeight", rr.bottom - rr.top);

	CDialog::OnDestroy();

	theApp.WriteProfileInt("Parse-Settings", "Pack", m_pack);
	theApp.WriteProfileInt("Parse-Settings", "BitfieldDirn", m_bitfield_up);
	theApp.WriteProfileInt("Parse-Settings", "TypesCommon", m_types_common);
	theApp.WriteProfileInt("Parse-Settings", "TypesCustom", m_types_custom);
	theApp.WriteProfileInt("Parse-Settings", "TypesSave", m_types_save);
	theApp.WriteProfileInt("Parse-Settings", "TypesStd", m_types_std);
	theApp.WriteProfileInt("Parse-Settings", "TypesWindows", m_types_windows);
	theApp.WriteProfileInt("Parse-Settings", "ValuesCustom", m_values_custom);
	theApp.WriteProfileInt("Parse-Settings", "ValuesSave", m_values_save);
	theApp.WriteProfileInt("Parse-Settings", "ValuesWindows", m_values_windows);
}

BOOL TParseDlg::PreTranslateMessage(MSG* pMsg) 
{
	CWnd *pw = GetDlgItem(IDC_SOURCE_CODE);   // Get ptr to edit box window
    if (pMsg->message == WM_CHAR && 
		pw != NULL &&
		pMsg->hwnd == pw->m_hWnd && 
		pMsg->wParam == '\t')
    {
		// Convert tabs to 4 spaces
		pw->SendMessage(WM_CHAR, ' ', 1);
		pw->SendMessage(WM_CHAR, ' ', 1);
		pw->SendMessage(WM_CHAR, ' ', 1);
		pw->SendMessage(WM_CHAR, ' ', 1);
		pw->SetFocus();
		return TRUE;
	}

	return CDialog::PreTranslateMessage(pMsg);
}
static DWORD id_pairs[] = { 
	IDC_SOURCE_CODE, HIDC_SOURCE_CODE,
	IDC_PACK1, HIDC_PACK1,
	IDC_PACK2, HIDC_PACK2,
	IDC_PACK4, HIDC_PACK4,
	IDC_PACK8, HIDC_PACK8,
	IDC_PACK16, HIDC_PACK16,
	IDC_VALUES_WINDOWS, HIDC_VALUES_WINDOWS,
	IDC_VALUES_CUSTOM, HIDC_VALUES_CUSTOM,
	IDC_VALUES_SAVE, HIDC_VALUES_SAVE,
	IDC_TYPES_STD, HIDC_TYPES_STD,
	IDC_TYPES_COMMON, HIDC_TYPES_COMMON,
	IDC_TYPES_WINDOWS, HIDC_TYPES_WINDOWS,
	IDC_TYPES_CUSTOM, HIDC_TYPES_CUSTOM,
	IDC_TYPES_SAVE, HIDC_TYPES_SAVE,
    0,0
};

BOOL TParseDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs);
    return TRUE;
}

void TParseDlg::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	theApp.HtmlHelpContextMenu(pWnd, id_pairs);
}

void TParseDlg::OnHelp() 
{
    // Display help for this page
    if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_TPARSE))
        AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

void TParseDlg::OnOK() 
{
	CDialog::OnOK();
}

void TParseDlg::OnValuesCustom() 
{
	// TODO: Add your control notification handler code here
	
}

void TParseDlg::OnTypesCustom() 
{
	// TODO: Add your control notification handler code here
	
}
