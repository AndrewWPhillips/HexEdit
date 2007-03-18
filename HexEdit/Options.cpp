// Options.cpp : implements the Options tabbed dialog box
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
#include "HexEditView.h"
#include "Dialog.h"             // For Macro Save dialog etc
#include "DirDialog.h"          // For directory selection dialog
#include "ClearHistDlg.h"
#include "HexFileList.h"
#include "NewScheme.h"
#include "NewCellTypes/GridCellCombo.h"     // For CGridCellCombo
#include "resource.hm"          // For control help IDs

#include <bcgbarres.h>          // For IDC_BCGBARRES_SKINS
#include "options.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CHexEditApp theApp;

/////////////////////////////////////////////////////////////////////////////
// COptSheet

IMPLEMENT_DYNAMIC(COptSheet, CPropertySheet)

COptSheet::COptSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
        :CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
}

COptSheet::COptSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
        :CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
}

COptSheet::~COptSheet()
{
}

BEGIN_MESSAGE_MAP(COptSheet, CPropertySheet)
        //{{AFX_MSG_MAP(COptSheet)
        ON_WM_NCCREATE()
	//}}AFX_MSG_MAP
    ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptSheet message handlers

BOOL COptSheet::OnNcCreate(LPCREATESTRUCT lpCreateStruct) 
{
        if (!CPropertySheet::OnNcCreate(lpCreateStruct))
                return FALSE;
        
        ModifyStyleEx(0, WS_EX_CONTEXTHELP);
        
        return TRUE;
}

BOOL COptSheet::DestroyWindow() 
{
    theApp.p_last_page_ = GetPage(GetActiveIndex());
//    theApp.last_opt_class_ptr_ = GetPage(theApp.last_opt_page_)->GetRuntimeClass();
	
    return CPropertySheet::DestroyWindow();
}

LRESULT COptSheet::OnKickIdle(WPARAM, LPARAM lCount)
{
    if (GetActivePage() == theApp.p_filters)
        return theApp.p_filters->OnIdle(lCount);
    else if (GetActivePage() == theApp.p_tips)
        return theApp.p_tips->OnIdle(lCount);
    else if (GetActivePage() == theApp.p_colours)
        return theApp.p_colours->OnIdle(lCount);
    else
        return FALSE;
}

//===========================================================================
/////////////////////////////////////////////////////////////////////////////
// CGeneralPage property page

IMPLEMENT_DYNCREATE(CGeneralPage, CPropertyPage)

CGeneralPage::CGeneralPage() : CPropertyPage(CGeneralPage::IDD)
{
        //{{AFX_DATA_INIT(CGeneralPage)
        save_exit_ = FALSE;
	one_only_ = FALSE;
	shell_open_ = FALSE;
	bg_search_ = FALSE;
	address_specified_ = -1;
	export_line_len_ = 0;
	recent_files_ = 0;
	backup_ = FALSE;
	backup_space_ = FALSE;
	backup_size_ = 0;
	backup_prompt_ = FALSE;
	backup_if_size_ = FALSE;
	//}}AFX_DATA_INIT
        base_address_ = 0;

    HICON hh = AfxGetApp()->LoadIcon(IDI_COGS);
    m_psp.hIcon = hh;
    m_psp.dwFlags &= ~PSP_USEICONID;
    m_psp.dwFlags |= PSP_USEHICON;
}

CGeneralPage::~CGeneralPage()
{
}

void CGeneralPage::DoDataExchange(CDataExchange* pDX)
{
        CPropertyPage::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CGeneralPage)
	DDX_Control(pDX, IDC_BACKUP_SPACE, ctl_backup_space_);
	DDX_Control(pDX, IDC_BACKUP_SIZE, ctl_backup_size_);
	DDX_Control(pDX, IDC_BACKUP_PROMPT, ctl_backup_prompt_);
	DDX_Control(pDX, IDC_BACKUP_IF_SIZE, ctl_backup_if_size_);
	DDX_Control(pDX, IDC_EXPORT_ADDRESS, address_ctl_);
        DDX_Check(pDX, IDC_SAVE_EXIT, save_exit_);
        DDX_Check(pDX, IDC_ONE_ONLY, one_only_);
        DDX_Check(pDX, IDC_SHELLOPEN, shell_open_);
        DDX_Check(pDX, IDC_BG_SEARCH, bg_search_);
	DDX_Radio(pDX, IDC_ADDRESS_FILE, address_specified_);
	DDX_Text(pDX, IDC_EXPORT_LINELEN, export_line_len_);
	DDV_MinMaxUInt(pDX, export_line_len_, 8, 250);
	DDX_Text(pDX, IDC_RECENT_FILES, recent_files_);
	DDV_MinMaxUInt(pDX, recent_files_, 1, 16);
	DDX_Check(pDX, IDC_BACKUP, backup_);
	DDX_Check(pDX, IDC_BACKUP_SPACE, backup_space_);
	DDX_Text(pDX, IDC_BACKUP_SIZE, backup_size_);
	DDX_Check(pDX, IDC_BACKUP_PROMPT, backup_prompt_);
	DDX_Check(pDX, IDC_BACKUP_IF_SIZE, backup_if_size_);
	//}}AFX_DATA_MAP
        if (pDX->m_bSaveAndValidate)
        {
            if (address_specified_)
            {
                CString ss;

                address_ctl_.GetWindowText(ss);
//                base_address_ = strtol(ss, NULL, 16);
                base_address_ = long(::strtoi64(ss, 16));
            }
        }
        else
        {
            if (address_specified_)
            {
                char buf[22];

                if (theApp.hex_ucase_)
                    sprintf(buf, "%I64X", __int64(base_address_));
                else
                    sprintf(buf, "%I64x", __int64(base_address_));
                address_ctl_.SetWindowText(buf);
                address_ctl_.add_spaces();
            }
        }
}


BEGIN_MESSAGE_MAP(CGeneralPage, CPropertyPage)
        //{{AFX_MSG_MAP(CGeneralPage)
        ON_BN_CLICKED(IDC_SAVE_NOW, OnSaveNow)
        ON_WM_HELPINFO()
        ON_BN_CLICKED(IDC_SHELLOPEN, OnShellopen)
	ON_BN_CLICKED(IDC_ADDRESS_SPECIFIED, OnAddressSpecified)
	ON_BN_CLICKED(IDC_ADDRESS_FILE, OnAddressFile)
	ON_BN_CLICKED(IDC_CLEAR_HIST, OnClearHist)
        ON_BN_CLICKED(IDC_SAVE_EXIT, OnChange)
	ON_BN_CLICKED(IDC_BACKUP, OnBackup)
	ON_BN_CLICKED(IDC_BACKUP_IF_SIZE, OnBackupIfSize)
        ON_BN_CLICKED(IDC_ONE_ONLY, OnChange)
        ON_BN_CLICKED(IDC_BG_SEARCH, OnChange)
	ON_EN_CHANGE(IDC_EXPORT_LINELEN, OnChange)
	ON_EN_CHANGE(IDC_EXPORT_ADDRESS, OnChange)
	ON_EN_CHANGE(IDC_RECENT_FILES, OnChange)
	ON_BN_CLICKED(IDC_BACKUP_PROMPT, OnChange)
	ON_EN_CHANGE(IDC_BACKUP_SIZE, OnChange)
	ON_BN_CLICKED(IDC_BACKUP_SPACE, OnChange)
	//}}AFX_MSG_MAP
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGeneralPage message handlers

BOOL CGeneralPage::OnInitDialog() 
{
    CPropertyPage::OnInitDialog();

    ((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_RECENT_FILES))->SetRange(1, 16);
    ((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_EXPORT_LINELEN))->SetRange(8, 250);

    ctl_backup_space_.EnableWindow(backup_);
    ctl_backup_if_size_.EnableWindow(backup_);
    ctl_backup_size_.EnableWindow(backup_ && backup_if_size_);
    ctl_backup_prompt_.EnableWindow(backup_);

    return TRUE;
}

void CGeneralPage::OnOK() 
{
    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
    aa->set_general();

    CPropertyPage::OnOK();
}

static DWORD id_pairs7[] = { 
    IDC_SHELLOPEN, HIDC_SHELLOPEN,
    IDC_ONE_ONLY, HIDC_ONE_ONLY,
    IDC_BG_SEARCH, HIDC_BG_SEARCH,
    IDC_SAVE_EXIT, HIDC_SAVE_EXIT,
    IDC_SAVE_NOW, HIDC_SAVE_NOW,
    IDC_RECENT_FILES, HIDC_RECENT_FILES,
    IDC_CLEAR_HIST, HIDC_CLEAR_HIST,
    IDC_SPIN_RECENT_FILES, HIDC_RECENT_FILES,
    IDC_BACKUP, HIDC_BACKUP,
    IDC_BACKUP_SPACE, HIDC_BACKUP_SPACE,
    IDC_BACKUP_IF_SIZE, HIDC_BACKUP_IF_SIZE,
    IDC_BACKUP_SIZE, HIDC_BACKUP_SIZE,
    IDC_BACKUP_PROMPT, HIDC_BACKUP_PROMPT,
    IDC_ADDRESS_FILE, HIDC_ADDRESS_FILE,
    IDC_ADDRESS_SPECIFIED, HIDC_ADDRESS_SPECIFIED,
    IDC_EXPORT_ADDRESS, HIDC_EXPORT_ADDRESS,
    IDC_EXPORT_LINELEN, HIDC_EXPORT_LINELEN,
    IDC_SPIN_EXPORT_LINELEN, HIDC_EXPORT_LINELEN,
    0,0 
}; 

BOOL CGeneralPage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs7);
    return TRUE;
}

void CGeneralPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	theApp.HtmlHelpContextMenu((HWND)pWnd->GetSafeHwnd(), id_pairs7);
}

void CGeneralPage::OnChange() 
{
    SetModified(TRUE);
}

void CGeneralPage::OnShellopen() 
{
    UpdateData();
    if (shell_open_)
    {
        if (AfxMessageBox("This option affects all users of the system and\n"
                          "can cause problems with some program launchers.\n"
                          "Do you want to enable this option anyway?",
                          MB_YESNO) != IDYES)
        {
            shell_open_ = FALSE;
            UpdateData(FALSE);
        }
    }
    SetModified(TRUE);
}

void CGeneralPage::OnClearHist() 
{
    CClearHistDlg dlg;
    dlg.DoModal();
}

void CGeneralPage::OnSaveNow() 
{
    CPropertySheet *oo = dynamic_cast<CPropertySheet *>(GetParent());

    // BUG: PressButton() always returns zero - so ignore return value
    (void)oo->PressButton(PSBTN_APPLYNOW);
    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
    aa->SaveOptions();
}

void CGeneralPage::OnAddressFile() 
{
    address_ctl_.EnableWindow(FALSE);
    SetModified(TRUE);
}

void CGeneralPage::OnAddressSpecified() 
{
    CString ss;
    address_ctl_.EnableWindow();
    address_ctl_.GetWindowText(ss);
    if (ss.IsEmpty())
        address_ctl_.SetWindowText("0");
//    address_ctl_.Redisplay();
    address_ctl_.SetFocus();
    SetModified(TRUE);
}

void CGeneralPage::OnBackup() 
{
    UpdateData();
    ctl_backup_space_.EnableWindow(backup_);
    ctl_backup_if_size_.EnableWindow(backup_);
    ctl_backup_size_.EnableWindow(backup_ && backup_if_size_);
    ctl_backup_prompt_.EnableWindow(backup_);
	
    SetModified(TRUE);
}

void CGeneralPage::OnBackupIfSize() 
{
    UpdateData();
    ASSERT(backup_);
    ctl_backup_size_.EnableWindow(backup_if_size_);
	
    SetModified(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CSysDisplayPage property page

IMPLEMENT_DYNCREATE(CSysDisplayPage, CPropertyPage)

CSysDisplayPage::CSysDisplayPage() : CPropertyPage(CSysDisplayPage::IDD)
{
	//{{AFX_DATA_INIT(CSysDisplayPage)
	large_cursor_ = FALSE;
	hex_ucase_ = FALSE;
	open_restore_ = FALSE;
	mditabs_ = FALSE;
	tabsbottom_ = FALSE;
	show_other_ = FALSE;
	max_fix_for_elts_ = 0;
	default_char_format_ = _T("");
	default_date_format_ = _T("");
	default_int_format_ = _T("");
	default_real_format_ = _T("");
	default_string_format_ = _T("");
	default_unsigned_format_ = _T("");
	//}}AFX_DATA_INIT
	dffd_view_ = -1;

    HICON hh = AfxGetApp()->LoadIcon(IDI_DISPLAY);
    m_psp.hIcon = hh;
    m_psp.dwFlags &= ~PSP_USEICONID;
    m_psp.dwFlags |= PSP_USEHICON;
}

CSysDisplayPage::~CSysDisplayPage()
{
}

void CSysDisplayPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSysDisplayPage)
	DDX_Check(pDX, IDC_LARGE_CURSOR, large_cursor_);
	DDX_Check(pDX, IDC_HEX_UCASE, hex_ucase_);
	DDX_Check(pDX, IDC_RESTORE, open_restore_);
	DDX_Check(pDX, IDC_MDITABS, mditabs_);
	DDX_Check(pDX, IDC_TABSBOTTOM, tabsbottom_);
	DDX_Check(pDX, IDC_SHOW_OTHER, show_other_);
	DDX_Text(pDX, IDC_DFFD_ARRAY_MAX, max_fix_for_elts_);
	DDV_MinMaxUInt(pDX, max_fix_for_elts_, 2, 999999);
	DDX_Text(pDX, IDC_DFFD_FORMAT_CHAR, default_char_format_);
	DDX_Text(pDX, IDC_DFFD_FORMAT_DATE, default_date_format_);
	DDX_Text(pDX, IDC_DFFD_FORMAT_INT, default_int_format_);
	DDX_Text(pDX, IDC_DFFD_FORMAT_REAL, default_real_format_);
	DDX_Text(pDX, IDC_DFFD_FORMAT_STRING, default_string_format_);
	DDX_Text(pDX, IDC_DFFD_FORMAT_UINT, default_unsigned_format_);
	//}}AFX_DATA_MAP
	DDX_Radio(pDX, IDC_DFFD_NONE, dffd_view_);
//	DDX_Check(pDX, IDC_NICE_ADDR, nice_addr_);
}


BEGIN_MESSAGE_MAP(CSysDisplayPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSysDisplayPage)
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_HEX_UCASE, OnChange)
	ON_BN_CLICKED(IDC_MDITABS, OnChangeMditabs)
	ON_BN_CLICKED(IDC_VISUALIZATIONS, OnVisualizations)
	ON_BN_CLICKED(IDC_RESTORE, OnChange)
	ON_BN_CLICKED(IDC_LARGE_CURSOR, OnChange)
	ON_BN_CLICKED(IDC_TABSBOTTOM, OnChange)
	ON_BN_CLICKED(IDC_SHOW_OTHER, OnChange)
	ON_EN_CHANGE(IDC_DFFD_FORMAT_CHAR, OnChange)
	ON_EN_CHANGE(IDC_DFFD_FORMAT_DATE, OnChange)
	ON_EN_CHANGE(IDC_DFFD_FORMAT_INT, OnChange)
	ON_EN_CHANGE(IDC_DFFD_FORMAT_REAL, OnChange)
	ON_EN_CHANGE(IDC_DFFD_FORMAT_STRING, OnChange)
	ON_EN_CHANGE(IDC_DFFD_FORMAT_UINT, OnChange)
	ON_BN_CLICKED(IDC_DFFD_SPLIT, OnChange)
	ON_BN_CLICKED(IDC_DFFD_TAB, OnChange)
	ON_EN_CHANGE(IDC_DFFD_ARRAY_MAX, OnChange)
	ON_WM_RBUTTONDBLCLK()
	//}}AFX_MSG_MAP
    ON_WM_CONTEXTMENU()
//	ON_BN_CLICKED(IDC_NICE_ADDR, OnChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSysDisplayPage message handlers

BOOL CSysDisplayPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
    ASSERT(GetDlgItem(IDC_SPIN_DFFD_ARRAY_MAX) != NULL);
    ((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_DFFD_ARRAY_MAX))->SetRange(2, UD_MAXVAL);
	
	return TRUE;
}

void CSysDisplayPage::OnChange() 
{
    SetModified(TRUE);
}

void CSysDisplayPage::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
	CPropertyPage::OnRButtonDblClk(nFlags, point);
}

void CSysDisplayPage::OnVisualizations() 
{
    theApp.GetSkinManager()->ShowSelectSkinDlg();
}

void CSysDisplayPage::OnChangeMditabs() 
{
    UpdateData();
    ASSERT(GetDlgItem(IDC_TABSBOTTOM) != NULL);
    GetDlgItem(IDC_TABSBOTTOM)->EnableWindow(mditabs_);
    SetModified(TRUE);
}

void CSysDisplayPage::OnOK() 
{
    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
    aa->set_sysdisplay();
        
    CPropertyPage::OnOK();
}

static DWORD id_pairs1[] = { 
    IDC_RESTORE, HIDC_RESTORE,
    IDC_HEX_UCASE, HIDC_HEX_UCASE,
    IDC_MDITABS, HIDC_MDITABS,
    IDC_TABSBOTTOM, HIDC_TABSBOTTOM,
//        IDC_NICE_ADDR, HIDC_NICE_ADDR,
    IDC_LARGE_CURSOR, HIDC_LARGE_CURSOR,
    IDC_SHOW_OTHER, HIDC_SHOW_OTHER,
    IDC_DFFD_NONE, HIDC_DFFD_SPLIT,  // xxx need HIDC_DFFD_NONE
    IDC_DFFD_SPLIT, HIDC_DFFD_SPLIT,
    IDC_DFFD_TAB, HIDC_DFFD_TAB,
    IDC_DFFD_ARRAY_MAX, HIDC_DFFD_ARRAY_MAX,
    IDC_DESC_DFFD_ARRAY_MAX, HIDC_DFFD_ARRAY_MAX,
    IDC_SPIN_DFFD_ARRAY_MAX, HIDC_DFFD_ARRAY_MAX,

    IDC_DFFD_FORMAT_CHAR, HIDC_DFFD_FORMAT_CHAR,
    IDC_DESC_DFFD_FORMAT_CHAR, HIDC_DFFD_FORMAT_CHAR,
    IDC_DFFD_FORMAT_INT, HIDC_DFFD_FORMAT_INT,
    IDC_DESC_DFFD_FORMAT_INT, HIDC_DFFD_FORMAT_INT,
    IDC_DFFD_FORMAT_UINT, HIDC_DFFD_FORMAT_UINT,
    IDC_DESC_DFFD_FORMAT_UINT, HIDC_DFFD_FORMAT_UINT,
    IDC_DFFD_FORMAT_STRING, HIDC_DFFD_FORMAT_STRING,
    IDC_DESC_DFFD_FORMAT_STRING, HIDC_DFFD_FORMAT_STRING,
    IDC_DFFD_FORMAT_REAL, HIDC_DFFD_FORMAT_REAL,
    IDC_DESC_DFFD_FORMAT_REAL, HIDC_DFFD_FORMAT_REAL,
    IDC_DFFD_FORMAT_DATE, HIDC_DFFD_FORMAT_DATE,
    IDC_DESC_DFFD_FORMAT_DATE, HIDC_DFFD_FORMAT_DATE,
//        IDC_VISUALIZATIONS, 0x41000+IDC_BCGBARRES_SKINS,
    0,0
};

BOOL CSysDisplayPage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs1);
    return TRUE;
}

void CSysDisplayPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	theApp.HtmlHelpContextMenu((HWND)pWnd->GetSafeHwnd(), id_pairs1);
}

//===========================================================================
/////////////////////////////////////////////////////////////////////////////
// CMacroPage property page

IMPLEMENT_DYNCREATE(CMacroPage, CPropertyPage)

CMacroPage::CMacroPage() : CPropertyPage(CMacroPage::IDD)
{
        //{{AFX_DATA_INIT(CMacroPage)
        num_keys_ = 0;
        num_plays_ = 0;
        num_secs_ = 0;
        refresh_ = -1;
        refresh_bars_ = FALSE;
        refresh_props_ = FALSE;
        halt_level_ = -1;
        //}}AFX_DATA_INIT
    HICON hh = AfxGetApp()->LoadIcon(IDI_MACRO);
    m_psp.hIcon = hh;
    m_psp.dwFlags &= ~PSP_USEICONID;
    m_psp.dwFlags |= PSP_USEHICON;
}

CMacroPage::~CMacroPage()
{
}

void CMacroPage::DoDataExchange(CDataExchange* pDX)
{
        CPropertyPage::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CMacroPage)
        DDX_Text(pDX, IDC_NUM_KEYS, num_keys_);
        DDV_MinMaxLong(pDX, num_keys_, 1, 999);
        DDX_Text(pDX, IDC_NUM_PLAYS, num_plays_);
        DDV_MinMaxLong(pDX, num_plays_, 1, 999);
        DDX_Text(pDX, IDC_NUM_SECS, num_secs_);
        DDV_MinMaxLong(pDX, num_secs_, 1, 999);
        DDX_Radio(pDX, IDC_REFRESH_NEVER, refresh_);
        DDX_Check(pDX, IDC_REFRESH_BARS, refresh_bars_);
        DDX_Check(pDX, IDC_REFRESH_PROPS, refresh_props_);
        DDX_Radio(pDX, IDC_HALT0, halt_level_);
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMacroPage, CPropertyPage)
        //{{AFX_MSG_MAP(CMacroPage)
        ON_WM_HELPINFO()
        ON_BN_CLICKED(IDC_HALT0, OnChange)
        ON_BN_CLICKED(IDC_REFRESH_NEVER, OnRefreshNever)
        ON_BN_CLICKED(IDC_REFRESH_PLAYS, OnRefreshPlays)
        ON_BN_CLICKED(IDC_REFRESH_SECS, OnRefreshSecs)
        ON_BN_CLICKED(IDC_REFRESH_KEYS, OnRefreshKeys)
        ON_BN_CLICKED(IDC_HALT1, OnChange)
        ON_BN_CLICKED(IDC_HALT2, OnChange)
        ON_EN_CHANGE(IDC_NUM_KEYS, OnChange)
        ON_EN_CHANGE(IDC_NUM_PLAYS, OnChange)
        ON_EN_CHANGE(IDC_NUM_SECS, OnChange)
        ON_BN_CLICKED(IDC_REFRESH_BARS, OnChange)
        ON_BN_CLICKED(IDC_REFRESH_PROPS, OnChange)
	ON_BN_CLICKED(IDC_SAVEMACRO, OnSavemacro)
	ON_BN_CLICKED(IDC_LOADMACRO, OnLoadmacro)
	ON_BN_CLICKED(IDC_MACRODIR, OnMacrodir)
	//}}AFX_MSG_MAP
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMacroPage message handlers

BOOL CMacroPage::OnInitDialog() 
{
    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
    CPropertyPage::OnInitDialog();

    ASSERT(GetDlgItem(IDC_SPIN_SECS) != NULL);
    ((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_SECS))->SetRange(1, 999);
    ASSERT(GetDlgItem(IDC_SPIN_KEYS) != NULL);
    ((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_KEYS))->SetRange(1, 999);
    ASSERT(GetDlgItem(IDC_SPIN_PLAYS) != NULL);
    ((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_PLAYS))->SetRange(1, 999);

    // Set edit box, spin control, static text for all 3 refresh options
    ASSERT(GetDlgItem(IDC_NUM_SECS) != NULL);
    ASSERT(GetDlgItem(IDC_SPIN_SECS) != NULL);
    ASSERT(GetDlgItem(IDC_DESC_SECS) != NULL);
    GetDlgItem(IDC_NUM_SECS)->EnableWindow(refresh_ == 1);
    GetDlgItem(IDC_SPIN_SECS)->EnableWindow(refresh_ == 1);
    GetDlgItem(IDC_DESC_SECS)->EnableWindow(refresh_ == 1);

    ASSERT(GetDlgItem(IDC_NUM_KEYS) != NULL);
    ASSERT(GetDlgItem(IDC_SPIN_KEYS) != NULL);
    ASSERT(GetDlgItem(IDC_DESC_KEYS) != NULL);
    GetDlgItem(IDC_NUM_KEYS)->EnableWindow(refresh_ == 2);
    GetDlgItem(IDC_SPIN_KEYS)->EnableWindow(refresh_ == 2);
    GetDlgItem(IDC_DESC_KEYS)->EnableWindow(refresh_ == 2);

    ASSERT(GetDlgItem(IDC_NUM_PLAYS) != NULL);
    ASSERT(GetDlgItem(IDC_SPIN_PLAYS) != NULL);
    ASSERT(GetDlgItem(IDC_DESC_PLAYS) != NULL);
    GetDlgItem(IDC_NUM_PLAYS)->EnableWindow(refresh_ == 3);
    GetDlgItem(IDC_SPIN_PLAYS)->EnableWindow(refresh_ == 3);
    GetDlgItem(IDC_DESC_PLAYS)->EnableWindow(refresh_ == 3);

    ASSERT(GetDlgItem(IDC_SAVEMACRO) != NULL);
    GetDlgItem(IDC_SAVEMACRO)->EnableWindow(!aa->recording_ && aa->mac_.size() > 0);
    return TRUE;
}

void CMacroPage::OnChange() 
{
    SetModified(TRUE);
}

void CMacroPage::OnOK() 
{
    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
    aa->set_macro();

    CPropertyPage::OnOK();
}

static DWORD id_pairs2[] = { 
    IDC_REFRESH_NEVER, HIDC_REFRESH_NEVER,
    IDC_REFRESH_SECS, HIDC_REFRESH_SECS,
    IDC_REFRESH_KEYS, HIDC_REFRESH_KEYS,
    IDC_REFRESH_PLAYS, HIDC_REFRESH_PLAYS,
    IDC_NUM_SECS, HIDC_NUM_SECS,
    IDC_SPIN_SECS, HIDC_NUM_SECS,
    IDC_DESC_SECS, HIDC_NUM_SECS,
    IDC_NUM_KEYS, HIDC_NUM_KEYS,
    IDC_SPIN_KEYS, HIDC_NUM_KEYS,
    IDC_DESC_KEYS, HIDC_NUM_KEYS,
    IDC_NUM_PLAYS, HIDC_NUM_PLAYS,
    IDC_SPIN_PLAYS, HIDC_NUM_PLAYS,
    IDC_DESC_PLAYS, HIDC_NUM_PLAYS,
    IDC_REFRESH_BARS, HIDC_REFRESH_BARS,
    IDC_REFRESH_PROPS, HIDC_REFRESH_PROPS,
    IDC_HALT0, HIDC_HALT0,
    IDC_HALT1, HIDC_HALT1,
    IDC_HALT2, HIDC_HALT2,
    IDC_SAVEMACRO, HIDC_SAVEMACRO,
    IDC_LOADMACRO, HIDC_LOADMACRO,
    IDC_MACRODIR, HIDC_MACRODIR,
    0,0 
};

BOOL CMacroPage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs2);
    return TRUE;
}

void CMacroPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	theApp.HtmlHelpContextMenu((HWND)pWnd->GetSafeHwnd(), id_pairs2);
}

void CMacroPage::OnRefreshNever() 
{
    // Disable edit box, spin control, static text for all 3 refresh options
    ASSERT(GetDlgItem(IDC_NUM_SECS) != NULL);
    ASSERT(GetDlgItem(IDC_SPIN_SECS) != NULL);
    ASSERT(GetDlgItem(IDC_DESC_SECS) != NULL);
    GetDlgItem(IDC_NUM_SECS)->EnableWindow(FALSE);
    GetDlgItem(IDC_SPIN_SECS)->EnableWindow(FALSE);
    GetDlgItem(IDC_DESC_SECS)->EnableWindow(FALSE);

    ASSERT(GetDlgItem(IDC_NUM_PLAYS) != NULL);
    ASSERT(GetDlgItem(IDC_SPIN_PLAYS) != NULL);
    ASSERT(GetDlgItem(IDC_DESC_PLAYS) != NULL);
    GetDlgItem(IDC_NUM_PLAYS)->EnableWindow(FALSE);
    GetDlgItem(IDC_SPIN_PLAYS)->EnableWindow(FALSE);
    GetDlgItem(IDC_DESC_PLAYS)->EnableWindow(FALSE);

    ASSERT(GetDlgItem(IDC_NUM_KEYS) != NULL);
    ASSERT(GetDlgItem(IDC_SPIN_KEYS) != NULL);
    ASSERT(GetDlgItem(IDC_DESC_KEYS) != NULL);
    GetDlgItem(IDC_NUM_KEYS)->EnableWindow(FALSE);
    GetDlgItem(IDC_SPIN_KEYS)->EnableWindow(FALSE);
    GetDlgItem(IDC_DESC_KEYS)->EnableWindow(FALSE);

    SetModified(TRUE);
}

void CMacroPage::OnRefreshPlays() 
{
    // Disable edit box, spin control, static text KEYS and SECS
    // Enable them for PLAYS
    ASSERT(GetDlgItem(IDC_NUM_SECS) != NULL);
    ASSERT(GetDlgItem(IDC_SPIN_SECS) != NULL);
    ASSERT(GetDlgItem(IDC_DESC_SECS) != NULL);
    GetDlgItem(IDC_NUM_SECS)->EnableWindow(FALSE);
    GetDlgItem(IDC_SPIN_SECS)->EnableWindow(FALSE);
    GetDlgItem(IDC_DESC_SECS)->EnableWindow(FALSE);

    ASSERT(GetDlgItem(IDC_NUM_PLAYS) != NULL);
    ASSERT(GetDlgItem(IDC_SPIN_PLAYS) != NULL);
    ASSERT(GetDlgItem(IDC_DESC_PLAYS) != NULL);
    GetDlgItem(IDC_NUM_PLAYS)->EnableWindow(TRUE);
    GetDlgItem(IDC_SPIN_PLAYS)->EnableWindow(TRUE);
    GetDlgItem(IDC_DESC_PLAYS)->EnableWindow(TRUE);

    ASSERT(GetDlgItem(IDC_NUM_KEYS) != NULL);
    ASSERT(GetDlgItem(IDC_SPIN_KEYS) != NULL);
    ASSERT(GetDlgItem(IDC_DESC_KEYS) != NULL);
    GetDlgItem(IDC_NUM_KEYS)->EnableWindow(FALSE);
    GetDlgItem(IDC_SPIN_KEYS)->EnableWindow(FALSE);
    GetDlgItem(IDC_DESC_KEYS)->EnableWindow(FALSE);

    SetModified(TRUE);
}

void CMacroPage::OnRefreshSecs() 
{
    // Disable edit box, spin control, static text KEYS and PLAYS
    // Enable them for SECS
    ASSERT(GetDlgItem(IDC_NUM_SECS) != NULL);
    ASSERT(GetDlgItem(IDC_SPIN_SECS) != NULL);
    ASSERT(GetDlgItem(IDC_DESC_SECS) != NULL);
    GetDlgItem(IDC_NUM_SECS)->EnableWindow(TRUE);
    GetDlgItem(IDC_SPIN_SECS)->EnableWindow(TRUE);
    GetDlgItem(IDC_DESC_SECS)->EnableWindow(TRUE);

    ASSERT(GetDlgItem(IDC_NUM_PLAYS) != NULL);
    ASSERT(GetDlgItem(IDC_SPIN_PLAYS) != NULL);
    ASSERT(GetDlgItem(IDC_DESC_PLAYS) != NULL);
    GetDlgItem(IDC_NUM_PLAYS)->EnableWindow(FALSE);
    GetDlgItem(IDC_SPIN_PLAYS)->EnableWindow(FALSE);
    GetDlgItem(IDC_DESC_PLAYS)->EnableWindow(FALSE);

    ASSERT(GetDlgItem(IDC_NUM_KEYS) != NULL);
    ASSERT(GetDlgItem(IDC_SPIN_KEYS) != NULL);
    ASSERT(GetDlgItem(IDC_DESC_KEYS) != NULL);
    GetDlgItem(IDC_NUM_KEYS)->EnableWindow(FALSE);
    GetDlgItem(IDC_SPIN_KEYS)->EnableWindow(FALSE);
    GetDlgItem(IDC_DESC_KEYS)->EnableWindow(FALSE);

    SetModified(TRUE);
}

void CMacroPage::OnRefreshKeys() 
{
    // Disable edit box, spin control, static text PLAYS and SECS
    // Enable them for KEYS
    ASSERT(GetDlgItem(IDC_NUM_SECS) != NULL);
    ASSERT(GetDlgItem(IDC_SPIN_SECS) != NULL);
    ASSERT(GetDlgItem(IDC_DESC_SECS) != NULL);
    GetDlgItem(IDC_NUM_SECS)->EnableWindow(FALSE);
    GetDlgItem(IDC_SPIN_SECS)->EnableWindow(FALSE);
    GetDlgItem(IDC_DESC_SECS)->EnableWindow(FALSE);

    ASSERT(GetDlgItem(IDC_NUM_PLAYS) != NULL);
    ASSERT(GetDlgItem(IDC_SPIN_PLAYS) != NULL);
    ASSERT(GetDlgItem(IDC_DESC_PLAYS) != NULL);
    GetDlgItem(IDC_NUM_PLAYS)->EnableWindow(FALSE);
    GetDlgItem(IDC_SPIN_PLAYS)->EnableWindow(FALSE);
    GetDlgItem(IDC_DESC_PLAYS)->EnableWindow(FALSE);

    ASSERT(GetDlgItem(IDC_NUM_KEYS) != NULL);
    ASSERT(GetDlgItem(IDC_SPIN_KEYS) != NULL);
    ASSERT(GetDlgItem(IDC_DESC_KEYS) != NULL);
    GetDlgItem(IDC_NUM_KEYS)->EnableWindow(TRUE);
    GetDlgItem(IDC_SPIN_KEYS)->EnableWindow(TRUE);
    GetDlgItem(IDC_DESC_KEYS)->EnableWindow(TRUE);

    SetModified(TRUE);
}

void CMacroPage::OnSavemacro() 
{
    if (!UpdateData(TRUE))
        return;                         // DDV failed

    CSaveMacro dlg;
    dlg.halt_level_ = halt_level_;

    dlg.DoModal();
}

void CMacroPage::OnLoadmacro() 
{
    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
    ASSERT_VALID(aa);

    ASSERT(aa->mac_dir_.Right(1) == "\\");
    CHexFileDialog dlg("MacroFileDlg", TRUE, "hem", NULL,
		               OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_DONTADDTORECENT | OFN_SHOWHELP,
                       "Macro Files (*.hem)|*.hem|All Files (*.*)|*.*||", this);
	dlg.m_ofn.lpstrTitle = "Select Macro File";
    dlg.m_ofn.lpstrInitialDir = aa->mac_dir_;

    if (dlg.DoModal() == IDOK)
	{
        std::vector<key_macro> tmp;     // Load macro into temp so that if something goes wrong we don't lose the current macro
        CString comment;
        int halt_lev;
        long plays;
		int version;  // Version of HexEdit in which the macro was recorded

        if (aa->macro_load(dlg.GetPathName(), &tmp,
                           comment, halt_lev, plays, version))
        {
            aa->mac_ = tmp;             // Store the temp macro to current HexEdit macro
            aa->mac_filename_ = dlg.GetFileTitle();
            aa->mac_comment_ = comment;
            aa->plays_ = plays;
			aa->macro_version_ = version;
            halt_level_ = halt_lev;     // Set halt level requested by saved macro in this dialog
            UpdateData(FALSE);

            // Now enable save button since we now have a macro loaded
            ASSERT(GetDlgItem(IDC_SAVEMACRO) != NULL);
            GetDlgItem(IDC_SAVEMACRO)->EnableWindow(TRUE);

            // Display the comment for this macro
            if (!comment.IsEmpty())
                ::HMessageBox(comment);
        }
	}
}

void CMacroPage::OnMacrodir() 
{
    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
    ASSERT_VALID(aa);

    ASSERT(aa->mac_dir_.Right(1) == "\\");
    CDirDialog dlg(aa->mac_dir_, "Macro Files (*.hem)|*.hem|All Files (*.*)|*.*||", this);
    dlg.m_ofn.lpstrTitle = "Select Folder for HexEdit Macros";

    if (dlg.DoModal() == IDOK)
        aa->mac_dir_ = dlg.GetPath();
    ASSERT(aa->mac_dir_.Right(1) == "\\");
}

/////////////////////////////////////////////////////////////////////////////
// CPrintPage property page

IMPLEMENT_DYNCREATE(CPrintPage, CPropertyPage)

CPrintPage::CPrintPage() : CPropertyPage(CPrintPage::IDD)
{
	//{{AFX_DATA_INIT(CPrintPage)
	bottom_ = 0.0;
	top_ = 0.0;
	left_ = 0.0;
	right_ = 0.0;
	footer_ = _T("");
	header_ = _T("");
	spacing_ = -1;
	border_ = FALSE;
	headings_ = FALSE;
	header_edge_ = 0.0;
	footer_edge_ = 0.0;
	//}}AFX_DATA_INIT
	units_ = 0;   // default to inches
    HICON hh = AfxGetApp()->LoadIcon(IDI_PRINTER);
    m_psp.hIcon = hh;
    m_psp.dwFlags &= ~PSP_USEICONID;
    m_psp.dwFlags |= PSP_USEHICON;
}

CPrintPage::~CPrintPage()
{
}

void CPrintPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPrintPage)
	DDX_Control(pDX, IDC_PRINT_FOOTER, ctl_footer_);
	DDX_Control(pDX, IDC_PRINT_HEADER, ctl_header_);
	DDX_Control(pDX, IDC_FOOTER_OPTS, footer_args_);
	DDX_Control(pDX, IDC_HEADER_OPTS, header_args_);
	DDX_Text(pDX, IDC_PRINT_BOTTOM, bottom_);
	DDX_Text(pDX, IDC_PRINT_TOP, top_);
	DDX_Text(pDX, IDC_PRINT_LEFT, left_);
	DDX_Text(pDX, IDC_PRINT_RIGHT, right_);
	DDX_Text(pDX, IDC_PRINT_FOOTER, footer_);
	DDX_Text(pDX, IDC_PRINT_HEADER, header_);
	DDX_Radio(pDX, IDC_PRINT_SPACE1, spacing_);
	DDX_Check(pDX, IDC_PRINT_BORDER, border_);
	DDX_Check(pDX, IDC_PRINT_HEADINGS, headings_);
	DDX_Text(pDX, IDC_PRINT_HEADER_EDGE, header_edge_);
	DDX_Text(pDX, IDC_PRINT_FOOTER_EDGE, footer_edge_);
	//}}AFX_DATA_MAP
	DDX_CBIndex(pDX, IDC_PRINT_UNITS, units_);
}

BEGIN_MESSAGE_MAP(CPrintPage, CPropertyPage)
	//{{AFX_MSG_MAP(CPrintPage)
	ON_WM_HELPINFO()
	ON_EN_CHANGE(IDC_PRINT_BOTTOM, OnChange)
	ON_EN_CHANGE(IDC_PRINT_FOOTER, OnChange)
	ON_EN_CHANGE(IDC_PRINT_HEADER, OnChange)
	ON_EN_CHANGE(IDC_PRINT_LEFT, OnChange)
	ON_EN_CHANGE(IDC_PRINT_RIGHT, OnChange)
	ON_EN_CHANGE(IDC_PRINT_TOP, OnChange)
	ON_BN_CLICKED(IDC_PRINT_SPACE1, OnChange)
	ON_BN_CLICKED(IDC_PRINT_SPACE1HALF, OnChange)
	ON_BN_CLICKED(IDC_PRINT_SPACE2, OnChange)
	ON_BN_CLICKED(IDC_FOOTER_OPTS, OnFooterOpts)
	ON_BN_CLICKED(IDC_HEADER_OPTS, OnHeaderOpts)
	//}}AFX_MSG_MAP
    ON_WM_CONTEXTMENU()
	ON_CBN_SELCHANGE(IDC_PRINT_UNITS, OnChangeUnits)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPrintPage message handlers

BOOL CPrintPage::OnInitDialog() 
{
    CPropertyPage::OnInitDialog();

    VERIFY(arrow_icon_ = AfxGetApp()->LoadIcon(IDI_ARROW));
    ASSERT(GetDlgItem(IDC_HEADER_OPTS) != NULL);
    ((CButton *)GetDlgItem(IDC_HEADER_OPTS))->SetIcon(arrow_icon_);
    ASSERT(GetDlgItem(IDC_FOOTER_OPTS) != NULL);
    ((CButton *)GetDlgItem(IDC_FOOTER_OPTS))->SetIcon(arrow_icon_);

    if (args_menu_.m_hMenu == NULL)
        args_menu_.LoadMenu(IDR_PRINT_ARGS);
    header_args_.m_hMenu = args_menu_.GetSubMenu(0)->GetSafeHmenu();
    header_args_.m_bRightArrow = TRUE;
    footer_args_.m_hMenu = args_menu_.GetSubMenu(0)->GetSafeHmenu();
    footer_args_.m_bRightArrow = TRUE;

    return TRUE;
}

void CPrintPage::OnOK() 
{
    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
    ASSERT_VALID(aa);
    (void)aa->set_printer();

    CPropertyPage::OnOK();
}

void CPrintPage::OnHeaderOpts() 
{
    if (header_args_.m_nMenuResult != 0)
    {
        CString ss;
        ss.LoadString (header_args_.m_nMenuResult);

        for (int i = 0; i < ss.GetLength (); i++)
        {
            ctl_header_.SendMessage (WM_CHAR, (TCHAR) ss [i]);
        }
        SetModified(TRUE);
    }
}

void CPrintPage::OnFooterOpts() 
{
    if (footer_args_.m_nMenuResult != 0)
    {
        CString ss;
        ss.LoadString (footer_args_.m_nMenuResult);

        for (int i = 0; i < ss.GetLength (); i++)
        {
            ctl_footer_.SendMessage (WM_CHAR, (TCHAR) ss [i]);
        }
        SetModified(TRUE);
    }
}

void CPrintPage::OnChangeUnits()
{
	// Get factor for units converting from
	double factor = 1.0;
	switch (units_)
	{
	case 1:
		factor = 2.54;
		break;
	}
	UpdateData();
	// Modify factor for units converting to
	switch (units_)
	{
	case 1:
		factor /= 2.54;
		break;
	}
	// Fix all distance values according to new units
	bottom_      = floor(1000.0*bottom_     /factor + 0.5)/1000.0;
	top_         = floor(1000.0*top_        /factor + 0.5)/1000.0;
	left_        = floor(1000.0*left_       /factor + 0.5)/1000.0;
	right_       = floor(1000.0*right_      /factor + 0.5)/1000.0;
	header_edge_ = floor(1000.0*header_edge_/factor + 0.5)/1000.0;
	footer_edge_ = floor(1000.0*footer_edge_/factor + 0.5)/1000.0;

	UpdateData(FALSE);    // Put new values back into the controls

    SetModified(TRUE);
}

void CPrintPage::OnChange() 
{
    SetModified(TRUE);
}

static DWORD id_pairs3[] = { 
    IDC_PRINT_HEADER, HIDC_PRINT_HEADER,
    IDC_HEADER_OPTS, HIDC_HEADER_OPTS,
    IDC_PRINT_FOOTER, HIDC_PRINT_FOOTER,
    IDC_FOOTER_OPTS, HIDC_FOOTER_OPTS,
    IDC_PRINT_BORDER, HIDC_PRINT_BORDER,
    IDC_PRINT_HEADINGS, HIDC_PRINT_HEADINGS,

    IDC_PRINT_UNITS, HIDC_PRINT_UNITS,
    IDC_PRINT_SPACE1, HIDC_PRINT_SPACE1,
    IDC_PRINT_SPACE1HALF, HIDC_PRINT_SPACE1HALF,
    IDC_PRINT_SPACE2, HIDC_PRINT_SPACE2,
    IDC_PRINT_LEFT, HIDC_PRINT_LEFT,
    IDC_PRINT_RIGHT, HIDC_PRINT_RIGHT,
    IDC_PRINT_TOP, HIDC_PRINT_TOP,
    IDC_PRINT_BOTTOM, HIDC_PRINT_BOTTOM,
    IDC_PRINT_HEADER_EDGE, HIDC_PRINT_HEADER_EDGE,
    IDC_PRINT_FOOTER_EDGE, HIDC_PRINT_FOOTER_EDGE,
    0,0 
};

BOOL CPrintPage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs3);
    return TRUE;
}

void CPrintPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	theApp.HtmlHelpContextMenu((HWND)pWnd->GetSafeHwnd(), id_pairs3);
}

/////////////////////////////////////////////////////////////////////////////
// CFiltersPage property page

IMPLEMENT_DYNCREATE(CFiltersPage, CPropertyPage)

CFiltersPage::CFiltersPage() : CPropertyPage(CFiltersPage::IDD)
{
	//{{AFX_DATA_INIT(CFiltersPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
    HICON hh = AfxGetApp()->LoadIcon(IDI_FILTER);
    m_psp.hIcon = hh;
    m_psp.dwFlags &= ~PSP_USEICONID;
    m_psp.dwFlags |= PSP_USEHICON;
}

CFiltersPage::~CFiltersPage()
{
}

void CFiltersPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFiltersPage)
	DDX_Control(pDX, IDC_UP, ctl_up_);
	DDX_Control(pDX, IDC_NEW, ctl_new_);
	DDX_Control(pDX, IDC_DOWN, ctl_down_);
	DDX_Control(pDX, IDC_DEL, ctl_del_);
	//}}AFX_DATA_MAP
    DDX_GridControl(pDX, IDC_GRID_FILTERS, grid_);             // associate the grid window with a C++ object
}

BEGIN_MESSAGE_MAP(CFiltersPage, CPropertyPage)
	//{{AFX_MSG_MAP(CFiltersPage)
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_DEL, OnDel)
	ON_BN_CLICKED(IDC_NEW, OnNew)
	ON_BN_CLICKED(IDC_UP, OnUp)
	ON_BN_CLICKED(IDC_DOWN, OnDown)
	//}}AFX_MSG_MAP
    ON_WM_CONTEXTMENU()
    ON_NOTIFY(GVN_ENDLABELEDIT, IDC_GRID_FILTERS, OnGridEndEdit)
    ON_NOTIFY(NM_CLICK, IDC_GRID_FILTERS, OnGridClick)
END_MESSAGE_MAP()

void CFiltersPage::add_row(int row, BOOL is_checked /*=TRUE*/, CString s1/*=""*/, CString s2 /*=""*/)
{
    row = grid_.InsertRow("", row);
    
//    CString str;
//    str.Format("%ld", long(row-header_rows+1));
//    grid_.SetItemText(row, column_number, str);

    CGridBtnCell *pbtn;
    grid_.SetCellType(row, column_check, RUNTIME_CLASS(CGridBtnCell));
    pbtn = (CGridBtnCell *)grid_.GetCell(row, column_check);
    if (pbtn != NULL)
    {
        pbtn->SetBtnDataBase(&btn_db_);
        pbtn->SetupBtns(0, DFC_BUTTON, DFCS_BUTTONCHECK, CGridBtnCellBase::CTL_ALIGN_LEFT, 20, FALSE, " ");
        UINT state = pbtn->GetDrawCtlState(0);
        if (is_checked)
            pbtn->SetDrawCtlState(0, state | DFCS_CHECKED);
        else
            pbtn->SetDrawCtlState(0, state & ~DFCS_CHECKED);
    }

    grid_.SetItemText(row, column_files, s1);
    grid_.SetItemText(row, column_filter, s2);
}

/////////////////////////////////////////////////////////////////////////////
// CFiltersPage message handlers

BOOL CFiltersPage::OnInitDialog() 
{
    CPropertyPage::OnInitDialog();

    // Set up icons on buttons
    VERIFY(icon_new_ = AfxGetApp()->LoadIcon(IDI_NEW));
    ctl_new_.SetIcon(icon_new_);
    VERIFY(icon_del_ = AfxGetApp()->LoadIcon(IDI_DEL));
    ctl_del_.SetIcon(icon_del_);
    VERIFY(icon_up_ = AfxGetApp()->LoadIcon(IDI_UP));
    ctl_up_.SetIcon(icon_up_);
    VERIFY(icon_down_ = AfxGetApp()->LoadIcon(IDI_DOWN));
    ctl_down_.SetIcon(icon_down_);

    // Add tooltips for buttons
    if (m_cToolTip.Create(this))
    {
        m_cToolTip.AddTool(&ctl_new_, IDS_NEW);
        m_cToolTip.AddTool(&ctl_del_, IDS_DEL);
        m_cToolTip.AddTool(&ctl_up_, IDS_UP);
        m_cToolTip.AddTool(&ctl_down_, IDS_DOWN);
        m_cToolTip.Activate(TRUE);
    }

    // Set up the grid
    grid_.SetEditable(TRUE);
    grid_.GetDefaultCell(FALSE, FALSE)->SetBackClr(RGB(0xF8, 0xF8, 0xFF));

    grid_.SetFixedColumnSelection(FALSE);
    grid_.SetFixedRowSelection(FALSE);

    grid_.AutoSize();

//    grid_.SetSortColumn(-1);
    btn_db_.SetGrid(&grid_);

    // Set up the grid rows and columns
    grid_.SetColumnCount(column_count);
    grid_.SetFixedColumnCount(0);
    grid_.SetRowCount(header_rows);
    grid_.SetFixedRowCount(header_rows);

    // Set up the grid sizes
//    grid_.SetColumnWidth(column_number, 28);
    grid_.SetColumnWidth(column_check, 22);
    grid_.SetColumnWidth(column_files, 185);
    grid_.SetColumnWidth(column_filter, 146);

    // Set up column headers
//    grid_.SetItemText(0, column_files, "Filter Name");
//    grid_.SetItemText(0, column_filter, "Files to Filter");

    // Set up the grid cells
    for (int ii = 0; ; ++ii)
    {
        CString s1, s2;
        BOOL is_checked = TRUE;

        AfxExtractSubString(s1, theApp.current_filters_, ii*2, '|');

        AfxExtractSubString(s2, theApp.current_filters_, ii*2+1, '|');

        if (s1.IsEmpty() && s2.IsEmpty())
            break;

        // Check if this is a disabled filter
        if (s2.IsEmpty() || s2[0] == '>')
        {
            is_checked = FALSE;
            if (!s2.IsEmpty()) s2 = s2.Mid(1);
        }

        add_row(-1, is_checked, s1, s2);
    }
    grid_.RedrawWindow();

    grid_.SetListMode(TRUE);
    grid_.SetSingleRowSelection(TRUE);

    return TRUE;
}

void CFiltersPage::OnOK() 
{
    int max_filt = grid_.GetRowCount() - header_rows;

    theApp.current_filters_.Empty();

    for (int ii = 0; ii < max_filt; ++ii)
    {
        CString s1 = grid_.GetItemText(ii+header_rows, column_files);
        CString s2 = grid_.GetItemText(ii+header_rows, column_filter);

        if (s1.IsEmpty() && s2.IsEmpty())
            continue;

        theApp.current_filters_ += s1;
        theApp.current_filters_ += '|';

        CGridBtnCell *pbtn;
        pbtn = (CGridBtnCell *)grid_.GetCell(ii+header_rows, column_check);
        if ((pbtn->GetDrawCtlState(0) & DFCS_CHECKED) == 0)
            theApp.current_filters_ += '>';

        theApp.current_filters_ += s2;
        theApp.current_filters_ += '|';
    }
    theApp.current_filters_ +='|';
	
    CPropertyPage::OnOK();
}

BOOL CFiltersPage::PreTranslateMessage(MSG* pMsg) 
{
    m_cToolTip.RelayEvent(pMsg);	

    return CDialog::PreTranslateMessage(pMsg);
}

static DWORD id_pairs4[] = { 
    IDC_GRID_FILTERS, HIDC_GRID_FILTERS,
    IDC_NEW, HIDC_NEW,
    IDC_DEL, HIDC_DEL,
    IDC_UP, HIDC_UP,
    IDC_DOWN, HIDC_DOWN,
    0,0 
}; 

BOOL CFiltersPage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs4);
    return TRUE;
}

void CFiltersPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	theApp.HtmlHelpContextMenu((HWND)pWnd->GetSafeHwnd(), id_pairs4);
}

void CFiltersPage::OnNew() 
{
    int row;
    CCellRange sel = grid_.GetSelectedCellRange();

    if (sel.IsValid())
        row = sel.GetMinRow();
    else
        row = header_rows;       // If no row selected add new row at top

    add_row(row);
    grid_.RedrawWindow();
    grid_.SetSelectedRange(row, 0, row, column_count-1);
    grid_.SetFocusCell(row, column_files);

    grid_.RedrawWindow();
    grid_.SetFocus();

    SetModified(TRUE);
}

void CFiltersPage::OnDel() 
{
    CCellRange sel = grid_.GetSelectedCellRange();
    if (sel.IsValid())
    {
        int row = sel.GetMinRow();
        grid_.DeleteRow(row);
        if (row < grid_.GetRowCount())
            grid_.SetSelectedRange(row, 0, row, column_count-1);

        grid_.RedrawWindow();

        SetModified(TRUE);
    }
}

void CFiltersPage::OnUp() 
{
    CCellRange sel = grid_.GetSelectedCellRange();
    int row;

    // If there is a row selected and its not at the top
    if (sel.IsValid() && (row = sel.GetMinRow()) > header_rows)
    {
        // Save info from the current and above row
        CString s1 = grid_.GetItemText(row, column_files);
        CString s2 = grid_.GetItemText(row, column_filter);
        CString s1_above = grid_.GetItemText(row-1, column_files);
        CString s2_above = grid_.GetItemText(row-1, column_filter);
        BOOL is_checked, is_checked_above;

        CGridBtnCell *pbtn;
        UINT state;

        pbtn = (CGridBtnCell *)grid_.GetCell(row, column_check);
        is_checked = (pbtn->GetDrawCtlState(0) & DFCS_CHECKED) != 0;
        pbtn = (CGridBtnCell *)grid_.GetCell(row-1, column_check);
        is_checked_above = (pbtn->GetDrawCtlState(0) & DFCS_CHECKED) != 0;

        // Swap the contents of the rows
        grid_.SetItemText(row-1, column_files, s1);
        grid_.SetItemText(row-1, column_filter, s2);
        pbtn = (CGridBtnCell *)grid_.GetCell(row-1, column_check);
        state = pbtn->GetDrawCtlState(0);
        if (is_checked)
            pbtn->SetDrawCtlState(0, state | DFCS_CHECKED);
        else
            pbtn->SetDrawCtlState(0, state & ~DFCS_CHECKED);

        grid_.SetItemText(row, column_files, s1_above);
        grid_.SetItemText(row, column_filter, s2_above);
        pbtn = (CGridBtnCell *)grid_.GetCell(row, column_check);
        state = pbtn->GetDrawCtlState(0);
        if (is_checked_above)
            pbtn->SetDrawCtlState(0, state | DFCS_CHECKED);
        else
            pbtn->SetDrawCtlState(0, state & ~DFCS_CHECKED);


        grid_.SetSelectedRange(row-1, 0, row-1, column_count-1);

        grid_.RedrawWindow();
        SetModified(TRUE);
    }
}

void CFiltersPage::OnDown() 
{
    CCellRange sel = grid_.GetSelectedCellRange();
    int row;
    if (sel.IsValid() && (row = sel.GetMinRow()) < grid_.GetRowCount()-1)
    {
        // Save info from current row and row below
        CString s1 = grid_.GetItemText(row, column_files);
        CString s2 = grid_.GetItemText(row, column_filter);
        CString s1_below = grid_.GetItemText(row+1, column_files);
        CString s2_below = grid_.GetItemText(row+1, column_filter);
        BOOL is_checked, is_checked_below;

        CGridBtnCell *pbtn;
        UINT state;

        pbtn = (CGridBtnCell *)grid_.GetCell(row, column_check);
        is_checked = (pbtn->GetDrawCtlState(0) & DFCS_CHECKED) != 0;
        pbtn = (CGridBtnCell *)grid_.GetCell(row+1, column_check);
        is_checked_below = (pbtn->GetDrawCtlState(0) & DFCS_CHECKED) != 0;

        // Swap the contents of the rows
        grid_.SetItemText(row+1, column_files, s1);
        grid_.SetItemText(row+1, column_filter, s2);
        pbtn = (CGridBtnCell *)grid_.GetCell(row+1, column_check);
        state = pbtn->GetDrawCtlState(0);
        if (is_checked)
            pbtn->SetDrawCtlState(0, state | DFCS_CHECKED);
        else
            pbtn->SetDrawCtlState(0, state & ~DFCS_CHECKED);

        grid_.SetItemText(row, column_files, s1_below);
        grid_.SetItemText(row, column_filter, s2_below);
        pbtn = (CGridBtnCell *)grid_.GetCell(row, column_check);
        state = pbtn->GetDrawCtlState(0);
        if (is_checked_below)
            pbtn->SetDrawCtlState(0, state | DFCS_CHECKED);
        else
            pbtn->SetDrawCtlState(0, state & ~DFCS_CHECKED);

        grid_.SetSelectedRange(row+1, 0, row+1, column_count-1);

        grid_.RedrawWindow();
        SetModified(TRUE);
    }
}

LRESULT CFiltersPage::OnIdle(long lCount)
{
    if (lCount == 0)
    {
        int num_rows = grid_.GetRowCount() - header_rows;
        int curr_row = -1;

        CCellRange sel = grid_.GetSelectedCellRange();
        if (sel.IsValid())
        {
            curr_row = sel.GetMinRow() - header_rows;
        }
//        ASSERT(GetDlgItem(IDC_NEW) != NULL);
//        GetDlgItem(IDC_NEW)->EnableWindow(curr_row != -1);
        ASSERT(GetDlgItem(IDC_DEL) != NULL);
        GetDlgItem(IDC_DEL)->EnableWindow(curr_row != -1);
        ASSERT(GetDlgItem(IDC_UP) != NULL);
        GetDlgItem(IDC_UP)->EnableWindow(curr_row != -1 && curr_row > 0);
        ASSERT(GetDlgItem(IDC_DOWN) != NULL);
        GetDlgItem(IDC_DOWN)->EnableWindow(curr_row != -1 && curr_row < num_rows - 1);
    }

    return FALSE;
}

#define FILTER_DISALLOWED_CHARACTERS "\\/:\"<>|"

void CFiltersPage::OnGridEndEdit(NMHDR *pNotifyStruct, LRESULT* pResult)
{
    NM_GRIDVIEW* pItem = (NM_GRIDVIEW*) pNotifyStruct;
    TRACE(_T("End Edit on row %d, col %d\n"), pItem->iRow, pItem->iColumn);
    if (pItem->iColumn == column_filter)
    {
        CString ss = grid_.GetItemText(pItem->iRow, pItem->iColumn);
        if (ss.FindOneOf(FILTER_DISALLOWED_CHARACTERS) != -1)
        {
            AfxMessageBox("Filters may not contain any of these characters:\n" FILTER_DISALLOWED_CHARACTERS);
            *pResult = -1;
            return;
        }
    }
    else if (pItem->iColumn == column_files)
    {
        CString ss = grid_.GetItemText(pItem->iRow, pItem->iColumn);
        if (ss.FindOneOf("|") != -1)
        {
            AfxMessageBox("Please do not use a vertical bar (|)");
            *pResult = -1;
            return;
        }
    }
    SetModified(TRUE);
    *pResult = 0;
}

void CFiltersPage::OnGridClick(NMHDR *pNotifyStruct, LRESULT* /*pResult*/)
{
    NM_GRIDVIEW* pItem = (NM_GRIDVIEW*) pNotifyStruct;
    if (pItem->iColumn == 0)
        SetModified(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CTipsPage property page

IMPLEMENT_DYNCREATE(CTipsPage, CPropertyPage)

CTipsPage::CTipsPage() : CPropertyPage(CTipsPage::IDD)
{
    HICON hh = AfxGetApp()->LoadIcon(IDI_TIPS);
    m_psp.hIcon = hh;
    m_psp.dwFlags &= ~PSP_USEICONID;
    m_psp.dwFlags |= PSP_USEHICON;
}

CTipsPage::~CTipsPage()
{
}

void CTipsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SLIDER, ctl_slider_);
	DDX_Control(pDX, IDC_UP, ctl_up_);
	DDX_Control(pDX, IDC_NEW, ctl_new_);
	DDX_Control(pDX, IDC_DOWN, ctl_down_);
	DDX_Control(pDX, IDC_DEL, ctl_del_);
    DDX_GridControl(pDX, IDC_GRID_TIP, grid_);             // associate the grid window with a C++ object
}

BEGIN_MESSAGE_MAP(CTipsPage, CPropertyPage)
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_DEL, OnDel)
	ON_BN_CLICKED(IDC_NEW, OnNew)
	ON_BN_CLICKED(IDC_UP, OnUp)
	ON_BN_CLICKED(IDC_DOWN, OnDown)
    ON_WM_CONTEXTMENU()
    ON_NOTIFY(GVN_ENDLABELEDIT, IDC_GRID_TIP, OnGridEndEdit)
    ON_NOTIFY(NM_CLICK, IDC_GRID_TIP, OnGridClick)
END_MESSAGE_MAP()

void CTipsPage::add_row(int row, BOOL is_checked /*=TRUE*/, CString s1/*=""*/, CString s2 /*=""*/, CString s3 /*=""*/)
{
    row = grid_.InsertRow("", row);

    CGridBtnCell *pbtn;
    grid_.SetCellType(row, column_check, RUNTIME_CLASS(CGridBtnCell));
    pbtn = (CGridBtnCell *)grid_.GetCell(row, column_check);
    if (pbtn != NULL)
    {
        pbtn->SetBtnDataBase(&btn_db_);
        pbtn->SetupBtns(0, DFC_BUTTON, DFCS_BUTTONCHECK, CGridBtnCellBase::CTL_ALIGN_LEFT, 20, FALSE, " ");
        UINT state = pbtn->GetDrawCtlState(0);
        if (is_checked)
            pbtn->SetDrawCtlState(0, state | DFCS_CHECKED);
        else
            pbtn->SetDrawCtlState(0, state & ~DFCS_CHECKED);
    }

    grid_.SetItemText(row, column_name, s1);
    grid_.SetItemText(row, column_expr, s2);
    grid_.SetItemText(row, column_format, s3);
}

/////////////////////////////////////////////////////////////////////////////
// CTipsPage message handlers

BOOL CTipsPage::OnInitDialog() 
{
	if (var_list.IsEmpty())
	{
		var_list.Add("address");
		var_list.Add("cursor");
		var_list.Add("sel_len");
		var_list.Add("mark");
		var_list.Add("eof");
		var_list.Add("sector");
		var_list.Add("offset");
		var_list.Add("byte");
		var_list.Add("sbyte");
		var_list.Add("word");
		var_list.Add("uword");
		var_list.Add("dword");
		var_list.Add("udword");
		var_list.Add("qword");
		var_list.Add("uqword");
		var_list.Add("ieee32");
		var_list.Add("ieee64");
		var_list.Add("ibm32");
		var_list.Add("ibm64");
		var_list.Add("time_t");
		var_list.Add("time_t_80");
		var_list.Add("time_t_1899");
		var_list.Add("time_t_mins");
#ifdef TIME64_T
		var_list.Add("time64_t");
#endif
		//var_list.Add("char_ascii");
		//var_list.Add("char_ebcdic");
		//var_list.Add("char_unicode");

		int_list.Add("dec");
		int_list.Add("%d");
		uint_list.Add("hex");
		uint_list.Add("dec");
		uint_list.Add("oct");
		uint_list.Add("bin");
		uint_list.Add("%u");
		uint_list.Add("%X");
		real32_list.Add("%e");
		real32_list.Add("%f");
		real32_list.Add("%g");
		real32_list.Add("%.7g");
		real64_list.Add("%e");
		real64_list.Add("%f");
		real64_list.Add("%g");
		real64_list.Add("%.15g");
		date_list.Add("%c");
		date_list.Add("%x");
		date_list.Add("%X");
		date_list.Add("%x %X");
		date_list.Add("%Y %m %d %H %M %S");
		char_list.Add("%c");
		char_list.Add("%d");
		char_list.Add("%2.2X");
		string_list.Add("%s");
	}

    CPropertyPage::OnInitDialog();

	ctl_slider_.SetRange(1, 255);
	ctl_slider_.SetPos(theApp.tip_transparency_);

    // Set up icons on buttons
    VERIFY(icon_new_ = AfxGetApp()->LoadIcon(IDI_NEW));
    ctl_new_.SetIcon(icon_new_);
    VERIFY(icon_del_ = AfxGetApp()->LoadIcon(IDI_DEL));
    ctl_del_.SetIcon(icon_del_);
    VERIFY(icon_up_ = AfxGetApp()->LoadIcon(IDI_UP));
    ctl_up_.SetIcon(icon_up_);
    VERIFY(icon_down_ = AfxGetApp()->LoadIcon(IDI_DOWN));
    ctl_down_.SetIcon(icon_down_);

    // Add tooltips for buttons
    if (m_cToolTip.Create(this))
    {
        m_cToolTip.AddTool(&ctl_new_, IDS_NEW_TIP);
        m_cToolTip.AddTool(&ctl_del_, IDS_DEL_TIP);
        m_cToolTip.AddTool(&ctl_up_, IDS_UP);
        m_cToolTip.AddTool(&ctl_down_, IDS_DOWN);
        m_cToolTip.Activate(TRUE);
    }

    // Set up the grid
    grid_.SetEditable(TRUE);
    grid_.GetDefaultCell(FALSE, FALSE)->SetBackClr(RGB(0xFF, 0xFF, 0xE0));

    grid_.SetFixedColumnSelection(FALSE);
    grid_.SetFixedRowSelection(FALSE);

    grid_.AutoSize();

    btn_db_.SetGrid(&grid_);

    // Set up the grid rows and columns
    grid_.SetColumnCount(column_count);
    grid_.SetFixedColumnCount(0);
    grid_.SetRowCount(header_rows);
    grid_.SetFixedRowCount(header_rows);

    // Set up the grid sizes
    grid_.SetColumnWidth(column_check, 22);
    grid_.SetColumnWidth(column_name, 100);
    grid_.SetColumnWidth(column_expr, 162);
    grid_.SetColumnWidth(column_format, 70);

    // Set up the grid cells
    for (int ii = 0; ii < theApp.tip_name_.size(); ++ii)
	{
		if (ii < FIRST_USER_TIP)
		{
			add_row(-1, theApp.tip_on_[ii], theApp.tip_name_[ii]);

			// Make text fields read-only
			CGridCellBase * pcell = grid_.GetCell(ii, column_name);
			pcell->SetState(pcell->GetState() | GVIS_READONLY);
			pcell = grid_.GetCell(ii, column_expr);
			pcell->SetState(pcell->GetState() | GVIS_READONLY);
			pcell = grid_.GetCell(ii, column_format);
			pcell->SetState(pcell->GetState() | GVIS_READONLY);
		}
		else
			add_row(-1, theApp.tip_on_[ii], theApp.tip_name_[ii], theApp.tip_expr_[ii], theApp.tip_format_[ii]);
	}
    grid_.RedrawWindow();

    grid_.SetListMode(TRUE);
    grid_.SetSingleRowSelection(TRUE);

    return TRUE;
}

void CTipsPage::OnOK()
{
    int max_row = grid_.GetRowCount() - header_rows;

    theApp.tip_on_.resize(FIRST_USER_TIP);
	theApp.tip_name_.resize(FIRST_USER_TIP);
	theApp.tip_expr_.resize(FIRST_USER_TIP);
	theApp.tip_format_.resize(FIRST_USER_TIP);

    for (int ii = 0; ii < max_row; ++ii)
    {
		if (ii < FIRST_USER_TIP)
		{
			CGridBtnCell *pbtn;
			pbtn = (CGridBtnCell *)grid_.GetCell(ii+header_rows, column_check);
			theApp.tip_on_[ii] = (pbtn->GetDrawCtlState(0) & DFCS_CHECKED) != 0;
			continue;
		}
        CString s1 = grid_.GetItemText(ii+header_rows, column_name);
        CString s2 = grid_.GetItemText(ii+header_rows, column_expr);
		CString s3 = grid_.GetItemText(ii+header_rows, column_format);

        if (s1.IsEmpty() && s2.IsEmpty())
            continue;

        CGridBtnCell *pbtn;
        pbtn = (CGridBtnCell *)grid_.GetCell(ii+header_rows, column_check);
        theApp.tip_on_.push_back((pbtn->GetDrawCtlState(0) & DFCS_CHECKED) != 0);
		theApp.tip_name_.push_back(s1);
		theApp.tip_expr_.push_back(s2);
		theApp.tip_format_.push_back(s3);
    }
	theApp.tip_transparency_ = ctl_slider_.GetPos();

    CPropertyPage::OnOK();
}

BOOL CTipsPage::PreTranslateMessage(MSG* pMsg) 
{
    m_cToolTip.RelayEvent(pMsg);	

    return CDialog::PreTranslateMessage(pMsg);
}

static DWORD id_pairs8[] = { 
    IDC_GRID_TIP, HIDC_GRID_TIP,
    IDC_NEW, HIDC_NEW,
    IDC_DEL, HIDC_DEL,
    IDC_UP, HIDC_UP,
    IDC_DOWN, HIDC_DOWN,
    0,0 
}; 

BOOL CTipsPage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs8);
    return TRUE;
}

void CTipsPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	theApp.HtmlHelpContextMenu((HWND)pWnd->GetSafeHwnd(), id_pairs8);
}

void CTipsPage::OnNew() 
{
    int row;
    CCellRange sel = grid_.GetSelectedCellRange();

    if (sel.IsValid())
        row = sel.GetMinRow();
    else
        row = header_rows;       // If no row selected add new row at top
	if (row < header_rows + FIRST_USER_TIP) row = header_rows + FIRST_USER_TIP;

    add_row(row);

	// Set up drop list so user can select the field type in Expression field
	VERIFY(grid_.SetCellType(row, column_expr, RUNTIME_CLASS(CGridCellCombo)));
    CGridCellCombo *pcell = (CGridCellCombo*) grid_.GetCell(row, column_expr);
	pcell->SetOptions(var_list);

    grid_.RedrawWindow();
    grid_.SetSelectedRange(row, 0, row, column_count-1);
    grid_.SetFocusCell(row, column_expr);

    grid_.RedrawWindow();
    grid_.SetFocus();

    SetModified(TRUE);
}

void CTipsPage::OnDel() 
{
    CCellRange sel = grid_.GetSelectedCellRange();
    if (sel.IsValid())
    {
        int row = sel.GetMinRow();
		if (row < FIRST_USER_TIP) return;
        grid_.DeleteRow(row);
        if (row < grid_.GetRowCount())
            grid_.SetSelectedRange(row, 0, row, column_count-1);

        grid_.RedrawWindow();

        SetModified(TRUE);
    }
}

void CTipsPage::OnUp() 
{
    CCellRange sel = grid_.GetSelectedCellRange();
    int row;

    // If there is a row selected and its not at the top
    if (sel.IsValid() && (row = sel.GetMinRow()) > header_rows + FIRST_USER_TIP)
    {
        // Save info from the current and above row
        CString s1 = grid_.GetItemText(row, column_name);
        CString s2 = grid_.GetItemText(row, column_expr);
        CString s3 = grid_.GetItemText(row, column_format);
        CString s1_above = grid_.GetItemText(row-1, column_name);
        CString s2_above = grid_.GetItemText(row-1, column_expr);
        CString s3_above = grid_.GetItemText(row-1, column_format);
        BOOL is_checked, is_checked_above;

        CGridBtnCell *pbtn;
        UINT state;

        pbtn = (CGridBtnCell *)grid_.GetCell(row, column_check);
        is_checked = (pbtn->GetDrawCtlState(0) & DFCS_CHECKED) != 0;
        pbtn = (CGridBtnCell *)grid_.GetCell(row-1, column_check);
        is_checked_above = (pbtn->GetDrawCtlState(0) & DFCS_CHECKED) != 0;

        // Swap the contents of the rows
        grid_.SetItemText(row-1, column_name, s1);
        grid_.SetItemText(row-1, column_expr, s2);
        grid_.SetItemText(row-1, column_format, s3);
        pbtn = (CGridBtnCell *)grid_.GetCell(row-1, column_check);
        state = pbtn->GetDrawCtlState(0);
        if (is_checked)
            pbtn->SetDrawCtlState(0, state | DFCS_CHECKED);
        else
            pbtn->SetDrawCtlState(0, state & ~DFCS_CHECKED);

        grid_.SetItemText(row, column_name, s1_above);
        grid_.SetItemText(row, column_expr, s2_above);
        grid_.SetItemText(row, column_format, s3_above);
        pbtn = (CGridBtnCell *)grid_.GetCell(row, column_check);
        state = pbtn->GetDrawCtlState(0);
        if (is_checked_above)
            pbtn->SetDrawCtlState(0, state | DFCS_CHECKED);
        else
            pbtn->SetDrawCtlState(0, state & ~DFCS_CHECKED);


        grid_.SetSelectedRange(row-1, 0, row-1, column_count-1);

        grid_.RedrawWindow();
        SetModified(TRUE);
    }
}

void CTipsPage::OnDown() 
{
    CCellRange sel = grid_.GetSelectedCellRange();
    int row;
    if (sel.IsValid() && (row = sel.GetMinRow()) < grid_.GetRowCount()-1 && row >= header_rows + FIRST_USER_TIP)
    {
        // Save info from current row and row below
        CString s1 = grid_.GetItemText(row, column_name);
        CString s2 = grid_.GetItemText(row, column_expr);
        CString s3 = grid_.GetItemText(row, column_format);
        CString s1_below = grid_.GetItemText(row+1, column_name);
        CString s2_below = grid_.GetItemText(row+1, column_expr);
        CString s3_below = grid_.GetItemText(row+1, column_format);
        BOOL is_checked, is_checked_below;

        CGridBtnCell *pbtn;
        UINT state;

        pbtn = (CGridBtnCell *)grid_.GetCell(row, column_check);
        is_checked = (pbtn->GetDrawCtlState(0) & DFCS_CHECKED) != 0;
        pbtn = (CGridBtnCell *)grid_.GetCell(row+1, column_check);
        is_checked_below = (pbtn->GetDrawCtlState(0) & DFCS_CHECKED) != 0;

        // Swap the contents of the rows
        grid_.SetItemText(row+1, column_name, s1);
        grid_.SetItemText(row+1, column_expr, s2);
        grid_.SetItemText(row+1, column_format, s3);
        pbtn = (CGridBtnCell *)grid_.GetCell(row+1, column_check);
        state = pbtn->GetDrawCtlState(0);
        if (is_checked)
            pbtn->SetDrawCtlState(0, state | DFCS_CHECKED);
        else
            pbtn->SetDrawCtlState(0, state & ~DFCS_CHECKED);

        grid_.SetItemText(row, column_name, s1_below);
        grid_.SetItemText(row, column_expr, s2_below);
        grid_.SetItemText(row, column_format, s3_below);
        pbtn = (CGridBtnCell *)grid_.GetCell(row, column_check);
        state = pbtn->GetDrawCtlState(0);
        if (is_checked_below)
            pbtn->SetDrawCtlState(0, state | DFCS_CHECKED);
        else
            pbtn->SetDrawCtlState(0, state & ~DFCS_CHECKED);

        grid_.SetSelectedRange(row+1, 0, row+1, column_count-1);

        grid_.RedrawWindow();
        SetModified(TRUE);
    }
}

LRESULT CTipsPage::OnIdle(long lCount)
{
    if (lCount == 0)
    {
		// Disable/enable up/down/del buttons depending on the row selected
        int num_rows = grid_.GetRowCount() - header_rows;
        int curr_row = -1;

        CCellRange sel = grid_.GetSelectedCellRange();
        if (sel.IsValid())
        {
            curr_row = sel.GetMinRow() - header_rows;
        }
        ASSERT(GetDlgItem(IDC_DEL) != NULL);
        GetDlgItem(IDC_DEL)->EnableWindow(curr_row != -1 && curr_row >= FIRST_USER_TIP);
        ASSERT(GetDlgItem(IDC_UP) != NULL);
        GetDlgItem(IDC_UP)->EnableWindow(curr_row != -1 && curr_row > FIRST_USER_TIP);
        ASSERT(GetDlgItem(IDC_DOWN) != NULL);
        GetDlgItem(IDC_DOWN)->EnableWindow(curr_row != -1 && curr_row < num_rows - 1 && curr_row >= FIRST_USER_TIP);
    }

    return FALSE;
}

void CTipsPage::OnGridEndEdit(NMHDR *pNotifyStruct, LRESULT* pResult)
{
    NM_GRIDVIEW* pItem = (NM_GRIDVIEW*) pNotifyStruct;
    CString ss = grid_.GetItemText(pItem->iRow, pItem->iColumn);
	// Make sure any text does not have a semi-colon as that is used as a separator in the registry string
    if (ss.FindOneOf(";") != -1)
    {
        AfxMessageBox("Please do not use a semi-colon (;)");
        *pResult = -1;
        return;
    }
	// If expression edited using drop down list then fill corresponding format list
    if (pItem->iColumn == column_expr && grid_.GetCell(pItem->iRow, column_expr)->IsKindOf(RUNTIME_CLASS(CGridCellCombo)))
	{
		enum { tnone, tint, tuint, treal32, treal64, tdate, tchar, tstring } tt = tnone;

		if (ss.Find("sbyte") != -1)
		    tt = tint;
		else if (ss.Find("byte") != -1 ||      // make sure this is done after "sbyte"
				 ss.Find("uword") != -1 ||
				 ss.Find("udword") != -1 ||
				 ss.Find("uqword") != -1 ||
			     ss.Find("address") != -1 ||
			     ss.Find("cursor") != -1 ||
			     ss.Find("sel_len") != -1 ||
			     ss.Find("mark") != -1 ||
			     ss.Find("eof") != -1 ||
			     ss.Find("sector") != -1 ||
			     ss.Find("offset") != -1
		        )
			tt = tuint;
		else if (ss.Find("word") != -1)      // includes dword/qword - must be done after uword etc
		    tt = tint;
		else if (ss.Find("ieee32") != -1 ||
			     ss.Find("ibm32") != -1
		        )
			tt = treal32;
		else if (ss.Find("ieee64") != -1 ||
			     ss.Find("ibm64") != -1
		        )
			tt = treal64;
		else if (ss.Find("date") != -1 ||
				 ss.Find("time") != -1       // includes time_t, time64_t etc
		        )
			tt = tdate;
		else if (ss.Find("char") != -1)
			tt = tchar;
		else if (ss.Find("string") != -1)
			tt = tchar;

		if (tt > tnone)
		{
			// Make sure the format field has a drop list
			if (!grid_.GetCell(pItem->iRow, column_format)->IsKindOf(RUNTIME_CLASS(CGridCellCombo)))
				VERIFY(grid_.SetCellType(pItem->iRow, column_format, RUNTIME_CLASS(CGridCellCombo)));

		    CGridCellCombo * pcell = (CGridCellCombo*) grid_.GetCell(pItem->iRow, column_format);
			switch (tt)
			{
			case tint:
				pcell->SetOptions(int_list);
				break;
			case tuint:
				pcell->SetOptions(uint_list);
				break;
			case treal32:
				pcell->SetOptions(real32_list);
				break;
			case treal64:
				pcell->SetOptions(real64_list);
				break;
			case tdate:
				pcell->SetOptions(date_list);
				break;
			case tchar:
				pcell->SetOptions(char_list);
				break;
			case tstring:
				pcell->SetOptions(string_list);
				break;
			}
		}
	}
    SetModified(TRUE);
    *pResult = 0;
}

void CTipsPage::OnGridClick(NMHDR *pNotifyStruct, LRESULT* /*pResult*/)
{
    NM_GRIDVIEW* pItem = (NM_GRIDVIEW*) pNotifyStruct;
    if (pItem->iColumn == 0)
        SetModified(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CWindowPage property page

IMPLEMENT_DYNCREATE(CWindowPage, CPropertyPage)

CWindowPage::CWindowPage() : CPropertyPage(CWindowPage::IDD)
{
    update_ok_ = false;
    disp_state_ = 0;

	//{{AFX_DATA_INIT(CWindowPage)
	charset_ = -1;
	insert_ = -1;
	maximize_ = FALSE;
	modify_ = -1;
	show_area_ = -1;
	cols_ = 0;
	grouping_ = 0;
	offset_ = 0;
	control_ = -1;
	addr_dec_ = FALSE;
	autofit_ = FALSE;
	//}}AFX_DATA_INIT
    vertbuffer_ = 0;
	big_endian_ = FALSE;
    borders_ = FALSE;
    change_tracking_ = 0;

    HICON hh = AfxGetApp()->LoadIcon(IDI_WINDOW);
    m_psp.hIcon = hh;
    m_psp.dwFlags &= ~PSP_USEICONID;
    m_psp.dwFlags |= PSP_USEHICON;
}

CWindowPage::~CWindowPage()
{
}

void CWindowPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
    if (!pDX->m_bSaveAndValidate)
    {
        // Move info into member variables (before move to controls)
		if (display_.vert_display)
			show_area_ = 3;
        else if (display_.hex_area && display_.char_area)
            show_area_ = 2;
        else if (display_.char_area)
            show_area_ = 1;
        else
            show_area_ = 0;

        if (display_.char_set == CHARSET_EBCDIC)
            charset_ = 3;
        else if (display_.char_set == CHARSET_OEM)
            charset_ = 2;
        else if (display_.char_set == CHARSET_ANSI)
            charset_ = 1;
        else if (display_.char_set == CHARSET_ASCII)
            charset_ = 0;
        else
            charset_ = -1;

        control_ = display_.control;

        addr_dec_ = display_.dec_addr;
        autofit_ = display_.autofit;

        insert_ = !display_.overtype;
        modify_ = !display_.readonly;
		big_endian_ = display_.big_endian;

        borders_ = display_.borders;
        if (display_.hide_replace && display_.hide_insert && display_.hide_delete)
            change_tracking_ = 0;
        else if (!display_.hide_replace && !display_.hide_insert && !display_.hide_delete)
            change_tracking_ = 1;
        else
            change_tracking_ = 2;    // Some options on, some off

        // Display the name of the window in the group box
        CWnd *pwnd = GetDlgItem(IDC_BOX);
        if (pwnd != NULL)
        {
            CString ss;

            ss.Format("Display for %s", window_name_);
            pwnd->SetWindowText(ss);
        }
    }

	//{{AFX_DATA_MAP(CWindowPage)
	DDX_CBIndex(pDX, IDC_CHARSET, charset_);
	DDX_CBIndex(pDX, IDC_INSERT, insert_);
	DDX_Check(pDX, IDC_MAX, maximize_);
	DDX_CBIndex(pDX, IDC_MODIFY, modify_);
	DDX_CBIndex(pDX, IDC_SHOW_AREA, show_area_);
	DDX_Text(pDX, IDC_COLS, cols_);
	DDX_Text(pDX, IDC_GROUPING, grouping_);
	DDX_Text(pDX, IDC_OFFSET, offset_);
	DDX_CBIndex(pDX, IDC_CONTROL, control_);
	DDX_Check(pDX, IDC_ADDR_DEC, addr_dec_);
	DDX_Check(pDX, IDC_AUTOFIT, autofit_);
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_VERTBUFFER, vertbuffer_);
	DDX_Check(pDX, IDC_BIG_ENDIAN, big_endian_);
	DDX_Check(pDX, IDC_BORDERS, borders_);
	DDX_Check(pDX, IDC_CHANGE_TRACKING, change_tracking_);

    if (pDX->m_bSaveAndValidate)
    {
        // Get info from members (after filled from controls)
        switch (show_area_)
        {
        default:
            ASSERT(0);
            /* fall through */
        case 0:
            display_.hex_area = TRUE;
            display_.char_area = FALSE;
            display_.edit_char = FALSE;
			display_.vert_display = FALSE;
            break;
        case 1:
            display_.hex_area = FALSE;
            display_.char_area = TRUE;
            display_.edit_char = TRUE;
			display_.vert_display = FALSE;
            break;
        case 2:
            display_.hex_area = TRUE;
            display_.char_area = TRUE;
			display_.vert_display = FALSE;
            break;
		case 3:
			display_.vert_display = TRUE;
			break;
        }

        switch (charset_)
        {
        default:
            ASSERT(0);
            /* fall through */
        case 0:
			display_.char_set = CHARSET_ASCII;
            break;
        case 1:
			display_.char_set = CHARSET_ANSI;
            break;
        case 2:
			display_.char_set = CHARSET_OEM;
            break;
        case 3:
			display_.char_set = CHARSET_EBCDIC;
            break;
        }

        display_.control = control_;

        display_.dec_addr = addr_dec_;
        display_.autofit = autofit_;

        display_.overtype = !insert_;
        display_.readonly = !modify_;
		display_.big_endian = big_endian_;

        display_.borders = borders_;
        if (change_tracking_ != 2)
            display_.hide_insert = display_.hide_replace = display_.hide_delete = (change_tracking_ == 0);

    }
}

BEGIN_MESSAGE_MAP(CWindowPage, CPropertyPage)
	//{{AFX_MSG_MAP(CWindowPage)
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_SAVE_DEFAULT, OnSaveDefault)
	ON_BN_CLICKED(IDC_FONT, OnFont)
	ON_BN_CLICKED(IDC_ADDR_DEC, OnChange)
	ON_BN_CLICKED(IDC_AUTOFIT, OnAutofit)
	ON_EN_CHANGE(IDC_COLS, OnChangeCols)
	ON_CBN_SELCHANGE(IDC_SHOW_AREA, OnSelchangeShowArea)
	ON_CBN_SELCHANGE(IDC_CHARSET, OnSelchangeCharset)
	ON_CBN_SELCHANGE(IDC_CONTROL, OnSelchangeControl)
	ON_CBN_SELCHANGE(IDC_MODIFY, OnSelchangeModify)
	ON_CBN_SELCHANGE(IDC_INSERT, OnSelchangeInsert)
	ON_BN_CLICKED(IDC_DISP_RESET, OnDispReset)
	ON_BN_CLICKED(IDC_MAX, OnChange)
	ON_EN_CHANGE(IDC_GROUPING, OnChange)
	ON_EN_CHANGE(IDC_OFFSET, OnChange)
	//}}AFX_MSG_MAP
    ON_WM_CONTEXTMENU()
	ON_EN_CHANGE(IDC_VERTBUFFER, OnChange)
	ON_BN_CLICKED(IDC_BIG_ENDIAN, OnChange)
	ON_BN_CLICKED(IDC_BORDERS, OnChange)
	ON_BN_CLICKED(IDC_CHANGE_TRACKING, OnChangeTracking)
END_MESSAGE_MAP()

void CWindowPage::fix_controls()
{
    // Check that all the control we need are available
    ASSERT(GetDlgItem(IDC_COLS_DESC) != NULL);
    ASSERT(GetDlgItem(IDC_COLS) != NULL);
    ASSERT(GetDlgItem(IDC_SPIN_COLS) != NULL);
    ASSERT(GetDlgItem(IDC_GROUPING_DESC) != NULL);
    ASSERT(GetDlgItem(IDC_GROUPING) != NULL);
    ASSERT(GetDlgItem(IDC_SPIN_GROUPING) != NULL);
    ASSERT(GetDlgItem(IDC_CHARSET) != NULL);
    ASSERT(GetDlgItem(IDC_CHARSET_DESC) != NULL);
    ASSERT(GetDlgItem(IDC_CONTROL) != NULL);
    ASSERT(GetDlgItem(IDC_CONTROL_DESC) != NULL);
    ASSERT(GetDlgItem(IDC_INSERT) != NULL);
    ASSERT(GetDlgItem(IDC_INSERT_DESC) != NULL);
    ASSERT(GetDlgItem(IDC_SPIN_OFFSET) != NULL);

    // If autofit is on disable setting of no of columns
    GetDlgItem(IDC_COLS_DESC)->EnableWindow(!autofit_);
    GetDlgItem(IDC_COLS)->EnableWindow(!autofit_);
    GetDlgItem(IDC_SPIN_COLS)->EnableWindow(!autofit_);

    // If no char area disable charset and control char selection
    GetDlgItem(IDC_CHARSET)->EnableWindow(show_area_ != 0);
    GetDlgItem(IDC_CHARSET_DESC)->EnableWindow(show_area_ != 0);
    GetDlgItem(IDC_CONTROL)->EnableWindow(show_area_ != 0 && charset_ < 2);
    GetDlgItem(IDC_CONTROL_DESC)->EnableWindow(show_area_ != 0 && charset_ < 2);

    // If no hex area disable setting of column grouping
    GetDlgItem(IDC_GROUPING_DESC)->EnableWindow(show_area_ != 1);
    GetDlgItem(IDC_GROUPING)->EnableWindow(show_area_ != 1);
    GetDlgItem(IDC_SPIN_GROUPING)->EnableWindow(show_area_ != 1);

    // If readonly disable setting of mode (OVR/INS)
    GetDlgItem(IDC_INSERT)->EnableWindow(modify_ == 1);
    GetDlgItem(IDC_INSERT_DESC)->EnableWindow(modify_ == 1);

    // Set offset spin range to be 0 to cols-1
    ((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_OFFSET))->SetRange(0, cols_-1);
}

BOOL CWindowPage::validated()
{
    if (cols_ < 4)
    {
        ::HMessageBox("You must have at least 4 columns");
        GetDlgItem(IDC_COLS)->SetFocus();
        return FALSE;
    }
    if (cols_ > CHexEditView::max_buf)
    {
        ::HMessageBox("Too many columns");
        GetDlgItem(IDC_COLS)->SetFocus();
        return FALSE;
    }
    if (offset_ >= cols_)
    {
        ::HMessageBox("Offset must be less than the number of columns");
        GetDlgItem(IDC_OFFSET)->SetFocus();
        return FALSE;
    }
    if (display_.hex_area && grouping_ < 2)
    {
        ::HMessageBox("There must be at least 2 columns per group");
        GetDlgItem(IDC_GROUPING)->SetFocus();
        return FALSE;
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CWindowPage message handlers

BOOL CWindowPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
    // Set columns spin control range to 4 to max
    CSpinButtonCtrl *pspin;
    pspin = (CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_COLS);
    ASSERT(pspin != NULL);
    pspin->SetRange(4, CHexEditView::max_buf);

    // Set grouping spin ctrl to 2 to max (when >= columns no grouping is done)
    pspin = (CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_GROUPING);
    ASSERT(pspin != NULL);
    pspin->SetRange(2, CHexEditView::max_buf);

    pspin = (CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_VERTBUFFER);
    ASSERT(pspin != NULL);
    pspin->SetRange(0, 999);

    fix_controls();

	return TRUE;
}

void CWindowPage::OnOK() 
{
    theApp.set_windisplay();
	
	CPropertyPage::OnOK();
}

static DWORD id_pairs5[] = { 
    IDC_FONT, HIDC_FONT,
    IDC_SHOW_AREA, HIDC_SHOW_AREA,
    IDC_CHARSET, HIDC_CHARSET,
    IDC_CHARSET_DESC, HIDC_CHARSET,
    IDC_CONTROL, HIDC_CONTROL,
    IDC_CONTROL_DESC, HIDC_CONTROL,
    IDC_MAX, HIDC_MAX,
    IDC_ADDR_DEC, HIDC_ADDR_DEC,
    IDC_AUTOFIT, HIDC_AUTOFIT,
    IDC_COLS, HIDC_COLS,
    IDC_COLS_DESC, HIDC_COLS,
    IDC_SPIN_COLS, HIDC_COLS,
    IDC_OFFSET, HIDC_OFFSET,
    IDC_SPIN_OFFSET, HIDC_OFFSET,
    IDC_GROUPING, HIDC_GROUPING,
    IDC_GROUPING_DESC, HIDC_GROUPING,
    IDC_SPIN_GROUPING, HIDC_GROUPING,
    IDC_VERTBUFFER, HIDC_VERTBUFFER,
    IDC_VERTBUFFER_DESC, HIDC_VERTBUFFER,
    IDC_SPIN_VERTBUFFER, HIDC_VERTBUFFER,
    IDC_DISP_RESET, HIDC_DISP_RESET,
    IDC_MODIFY, HIDC_MODIFY,
    IDC_INSERT, HIDC_INSERT,
    IDC_INSERT_DESC, HIDC_INSERT,
    IDC_BIG_ENDIAN, HIDC_BIG_ENDIAN,
    IDC_SAVE_DEFAULT, HIDC_SAVE_DEFAULT,
    IDC_BORDERS, HIDC_BORDERS,
    IDC_CHANGE_TRACKING, HIDC_CHANGE_TRACKING,
    0,0 
}; 

BOOL CWindowPage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs5);
    return TRUE;
}

void CWindowPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	theApp.HtmlHelpContextMenu((HWND)pWnd->GetSafeHwnd(), id_pairs5);
}

BOOL CWindowPage::OnSetActive() 
{
    BOOL retval = CPropertyPage::OnSetActive();
    update_ok_ = true;          // Its now OK to allow changes to cols field to be processed
    return retval;
}

BOOL CWindowPage::OnKillActive() 
{
    BOOL retval = CPropertyPage::OnKillActive();

    if (retval)
    {
        if (!validated())
            return 0;

        // All validation is OK so we will be deactivated
        update_ok_ = false;
    }

    return retval;
}

void CWindowPage::OnCancel() 
{
    update_ok_ = false;
	
	CPropertyPage::OnCancel();
}

void CWindowPage::OnSaveDefault() 
{
    UpdateData();
    if (!validated())
        return;

    if (AfxMessageBox("The default settings are used when you open a file which you\n"
                      "haven't opened in HexEdit before, or when you create a new file.\n\n"
                      "Are you sure you want to use these settings as the default?", MB_OKCANCEL) != IDOK)
        return;

    theApp.open_disp_state_ = disp_state_;
    theApp.open_max_ = maximize_;
    theApp.open_rowsize_ = cols_;
    theApp.open_group_by_ = grouping_;
    theApp.open_offset_ = offset_;

    if (theApp.open_plf_ == NULL)
        theApp.open_plf_ = new LOGFONT;
    *theApp.open_plf_ = lf_;
    if (theApp.open_oem_plf_ == NULL)
        theApp.open_oem_plf_ = new LOGFONT;
    *theApp.open_oem_plf_ = oem_lf_;

    theApp.GetFileList()->SetDefaults();
}

void CWindowPage::OnDispReset() 
{
    // Reset to default display values
    ASSERT(theApp.open_disp_state_ != -1);
    disp_state_ = theApp.open_disp_state_;
    maximize_ = theApp.open_max_;
    cols_ = theApp.open_rowsize_;
    grouping_ = theApp.open_group_by_;
    offset_ = theApp.open_offset_;

    if (theApp.open_plf_ != NULL)
        lf_ = *theApp.open_plf_;
    if (theApp.open_oem_plf_ != NULL)
        oem_lf_ = *theApp.open_oem_plf_;
	
    UpdateData(FALSE);
    fix_controls();
    SetModified(TRUE);
}

void CWindowPage::OnFont() 
{
    UpdateData();

    CFontDialog dlg;
//    dlg.SetWindowText(open_display_char_ && !open_ebcdic_ && open_graphic_ && open_oem_ ?
//                      "Default OEM/IBM window font" : "Default font");
    if (display_.FontRequired() == FONT_OEM)
        dlg.m_cf.lpLogFont = &oem_lf_;
    else
        dlg.m_cf.lpLogFont = &lf_;
    dlg.m_cf.Flags |= CF_INITTOLOGFONTSTRUCT | CF_SHOWHELP;
    dlg.m_cf.Flags &= ~(CF_EFFECTS);              // Disable selection of strikethrough, colours etc

    if (dlg.DoModal() == IDOK)
    {
        if (display_.FontRequired() == FONT_OEM)
        {
            if (oem_lf_.lfHeight < 0) oem_lf_.lfHeight = -oem_lf_.lfHeight;
        }
        else
        {
            if (lf_.lfHeight < 0) lf_.lfHeight = -lf_.lfHeight;
        }
        SetModified(TRUE);
    }
}

void CWindowPage::OnChange() 
{
    SetModified(TRUE);
}

void CWindowPage::OnAutofit() 
{
    UpdateData();
    fix_controls();
	
    SetModified(TRUE);
}

void CWindowPage::OnChangeTracking() 
{
    // This will cause change_tracking_ to be set to 0 or 1 in DoDataExchange
    display_.hide_replace = display_.hide_insert = display_.hide_delete = change_tracking_;
    UpdateData(FALSE);

    SetModified(TRUE);
}

void CWindowPage::OnChangeCols() 
{
    if (!update_ok_)
        return;
    update_ok_ = false;

    // Get cols_ value from dialog controls
    if (UpdateData())
    {
        // Change the offset spin control range
        CSpinButtonCtrl *pspin;
        pspin = (CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_OFFSET);
        ASSERT(pspin != NULL);
        pspin->SetRange(0, cols_-1);
    }

    SetModified(TRUE);
    update_ok_ = true;
}

void CWindowPage::OnSelchangeShowArea() 
{
    UpdateData();
    if (show_area_ == 3 && vertbuffer_ < 2)
    {
        vertbuffer_ = 2;
        UpdateData(FALSE);
    }
	fix_controls();
	
    SetModified(TRUE);
}

void CWindowPage::OnSelchangeCharset() 
{
    UpdateData();
	fix_controls();

    SetModified(TRUE);
}

void CWindowPage::OnSelchangeControl() 
{
    SetModified(TRUE);
}

void CWindowPage::OnSelchangeModify() 
{
    UpdateData();
	fix_controls();
	
    SetModified(TRUE);
}

void CWindowPage::OnSelchangeInsert() 
{
    SetModified(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CColourSchemes property page

IMPLEMENT_DYNCREATE(CColourSchemes, CPropertyPage)

CColourSchemes::CColourSchemes() : CPropertyPage(CColourSchemes::IDD, theApp.is_us_ ? IDS_COLORS : 0)
{
	//{{AFX_DATA_INIT(CColourSchemes)
	scheme_no_ = -1;
	name_no_ = -1;
	//}}AFX_DATA_INIT
    HICON hh = AfxGetApp()->LoadIcon(IDI_COLOUR);
    m_psp.hIcon = hh;
    m_psp.dwFlags &= ~PSP_USEICONID;
    m_psp.dwFlags |= PSP_USEHICON;
}

CColourSchemes::~CColourSchemes()
{
}

void CColourSchemes::DoDataExchange(CDataExchange* pDX)
{
        ASSERT(scheme_no_ >= 0 && scheme_no_ < scheme_.size());

	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CColourSchemes)
	DDX_Control(pDX, IDC_COLOUR_PICKER, m_ColourPicker);
	DDX_LBIndex(pDX, IDC_SCHEMES, scheme_no_);
	DDX_LBIndex(pDX, IDC_NAMES, name_no_);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CColourSchemes, CPropertyPage)
	//{{AFX_MSG_MAP(CColourSchemes)
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_ADD_SCHEME, OnAddScheme)
	ON_BN_CLICKED(IDC_REMOVE_SCHEME, OnRemoveScheme)
	ON_LBN_SELCHANGE(IDC_SCHEMES, OnSelchangeScheme)
	ON_BN_CLICKED(IDC_ADD, OnAddRange)
	ON_BN_CLICKED(IDC_REMOVE, OnRemoveRange)
	ON_BN_CLICKED(IDC_UP, OnUp)
	ON_BN_CLICKED(IDC_DOWN, OnDown)
	ON_BN_CLICKED(IDC_RESET, OnSchemeReset)
	ON_EN_CHANGE(IDC_CURRENT, OnChangeCurrent)
	ON_LBN_SELCHANGE(IDC_NAMES, OnSelchangeRange)
	ON_EN_CHANGE(IDC_RANGES, OnChangeRange)
	ON_BN_DOUBLECLICKED(IDC_UP, OnUp)
	ON_BN_DOUBLECLICKED(IDC_DOWN, OnDown)
	ON_BN_CLICKED(IDC_COLOUR_PICKER, OnColourPicker)
	//}}AFX_MSG_MAP
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColourSchemes message handlers

BOOL CColourSchemes::OnInitDialog() 
{
    CHexEditView *pview;

    scheme_ = theApp.scheme_;
    change_name_ = false;
    change_range_ = false;
    ASSERT(theApp.scheme_.size() > 0);
    scheme_no_ = 0;                     // default to first scheme

    if ((pview = GetView()) != NULL)
    {
        // Get scheme of currently active view
        std::vector<CScheme>::const_iterator ps;
        int ii;

        for (ii = 0, ps = theApp.scheme_.begin(); ps != theApp.scheme_.end(); ++ii, ++ps)
        {
            if (ps->name_ == pview->GetSchemeName())
            {
                scheme_no_ = ii;
                break;
            }
        }
    }

    CPropertyPage::OnInitDialog();

    if (theApp.is_us_)
    {
        ASSERT(GetDlgItem(IDC_DESC_COLOUR) != NULL);
#ifdef _DEBUG
        CString ss;
        GetDlgItem(IDC_DESC_COLOUR)->GetWindowText(ss);
        ASSERT(ss == "&Colour:");
#endif
        GetDlgItem(IDC_DESC_COLOUR)->SetWindowText("&Color:");
    }

    CListBox *plist;

    // Add scheme names to IDC_SCHEMES
    plist = (CListBox *)GetDlgItem(IDC_SCHEMES);
    ASSERT(plist != NULL);
    plist->ResetContent();
    for (std::vector<CScheme>::iterator ps = scheme_.begin();
         ps != scheme_.end(); ++ps)
    {
        plist->AddString(ps->name_);
    }

    VERIFY(icon_up_ = AfxGetApp()->LoadIcon(IDI_UP));
    ASSERT(GetDlgItem(IDC_UP) != NULL);
    ((CButton *)GetDlgItem(IDC_UP))->SetIcon(icon_up_);
    VERIFY(icon_down_ = AfxGetApp()->LoadIcon(IDI_DOWN));
    ASSERT(GetDlgItem(IDC_DOWN) != NULL);
    ((CButton *)GetDlgItem(IDC_DOWN))->SetIcon(icon_down_);
    VERIFY(icon_new_ = AfxGetApp()->LoadIcon(IDI_NEW));
    ASSERT(GetDlgItem(IDC_ADD_SCHEME) != NULL);
    ASSERT(GetDlgItem(IDC_ADD) != NULL);
    ((CButton *)GetDlgItem(IDC_ADD_SCHEME))->SetIcon(icon_new_);
    ((CButton *)GetDlgItem(IDC_ADD))->SetIcon(icon_new_);
    VERIFY(icon_del_ = AfxGetApp()->LoadIcon(IDI_DEL));
    ASSERT(GetDlgItem(IDC_REMOVE_SCHEME) != NULL);
    ASSERT(GetDlgItem(IDC_REMOVE) != NULL);
    ((CButton *)GetDlgItem(IDC_REMOVE_SCHEME))->SetIcon(icon_del_);
    ((CButton *)GetDlgItem(IDC_REMOVE))->SetIcon(icon_del_);

    set_scheme();

    // Set up BCG colour picker control
    m_ColourPicker.EnableAutomaticButton(_T("Automatic"), ::GetSysColor(COLOR_WINDOWTEXT));
    m_ColourPicker.EnableOtherButton(_T("Other"));
    m_ColourPicker.SetColor((COLORREF)-1);
    m_ColourPicker.SetColumnsNumber(10);

    return TRUE;
}

// Set up name list and range controls etc based on the currently selected scheme
void CColourSchemes::set_scheme()
{
    ASSERT(scheme_no_ >= 0 && scheme_no_ < scheme_.size());

    // Add the names of the ranges to the names list
    CListBox *plist = (CListBox *)GetDlgItem(IDC_NAMES);
    ASSERT(plist != NULL);
    plist->ResetContent();

    // Add strings for INDEX_BG, INDEX_HI, INDEX_BM, INDEX_SEARCH, INDEX_HEX_ADDR, INDEX_DEC_ADDR,                   // text colours
    plist->AddString("Background");
	plist->AddString("Mark");
    plist->AddString("Highlighting");
    plist->AddString("Bookmarks");
    plist->AddString("Search occurrences");
#ifdef CHANGE_TRACKING
    plist->AddString("Change Tracking");
    plist->AddString("Address background");
    plist->AddString("Sector boundary");
#endif
    plist->AddString("Hex addresses");
    plist->AddString("Decimal addresses");

	// Add byte range colours to the end of the list
    for (std::vector<CString>::iterator pp = scheme_[scheme_no_].range_name_.begin();
         pp != scheme_[scheme_no_].range_name_.end(); ++pp)
        plist->AddString(*pp);

    // Deselect any selection in the names list
    name_no_ = -1;
    ASSERT(name_no_ == plist->GetCurSel());
    UpdateData(FALSE);

	// Scroll byte range colours into view
	plist->SetTopIndex(INDEX_LAST);

    // Disable the Remove, Up and Down buttons
    ASSERT(GetDlgItem(IDC_REMOVE) != NULL);
    GetDlgItem(IDC_REMOVE)->EnableWindow(FALSE);
    ASSERT(GetDlgItem(IDC_UP) != NULL);
    GetDlgItem(IDC_UP)->EnableWindow(FALSE);
    ASSERT(GetDlgItem(IDC_DOWN) != NULL);
    GetDlgItem(IDC_DOWN)->EnableWindow(FALSE);

    // Blank the name in IDC_CURRENT
    ASSERT(GetDlgItem(IDC_CURRENT) != NULL);
    change_name_ = true;
    GetDlgItem(IDC_CURRENT)->SetWindowText("");
    change_name_ = false;
    GetDlgItem(IDC_CURRENT)->SetFocus();

    // Disable the colour picker
    m_ColourPicker.EnableWindow(FALSE);

    // Blank and disable the ranges box
    ASSERT(GetDlgItem(IDC_RANGES) != NULL);
    change_range_ = true;
    GetDlgItem(IDC_RANGES)->SetWindowText("");
    change_range_ = false;
    GetDlgItem(IDC_RANGES)->EnableWindow(FALSE);
}

void CColourSchemes::OnOK()
{
    theApp.set_schemes();

    CPropertyPage::OnOK();
}

LRESULT CColourSchemes::OnIdle(long lCount)
{
    if (lCount == 0)
    {
        ASSERT(scheme_no_ > -1 && scheme_no_ < scheme_.size());

        ASSERT(GetDlgItem(IDC_REMOVE_SCHEME) != NULL);
        GetDlgItem(IDC_REMOVE_SCHEME)->EnableWindow(scheme_[scheme_no_].can_delete_);

        ASSERT(GetDlgItem(IDC_UP) != NULL);
        GetDlgItem(IDC_UP)->EnableWindow(name_no_ > INDEX_LAST);
        ASSERT(GetDlgItem(IDC_DOWN) != NULL);
        GetDlgItem(IDC_DOWN)->EnableWindow(name_no_ >= INDEX_LAST && 
                                           name_no_ < INDEX_LAST + scheme_[scheme_no_].range_name_.size() - 1);

        // Enable the Remove button if there is a selection
        ASSERT(GetDlgItem(IDC_REMOVE) != NULL);
        GetDlgItem(IDC_REMOVE)->EnableWindow(name_no_ >= INDEX_LAST);
    }

    return FALSE;
}

static DWORD id_pairs6[] = { 
    IDC_ADD_SCHEME, HIDC_ADD_SCHEME,
    IDC_REMOVE_SCHEME, HIDC_REMOVE_SCHEME,
    IDC_SCHEMES, HIDC_SCHEMES,
    IDC_RESET, HIDC_RESET,
    IDC_CURRENT, HIDC_CURRENT,
    IDC_NAMES, HIDC_NAMES,
    IDC_DESC_COLOUR, HIDC_COLOUR_PICKER,
    IDC_COLOUR_PICKER, HIDC_COLOUR_PICKER,
    IDC_RANGES, HIDC_RANGES,
    IDC_UP, HIDC_UP,
    IDC_DOWN, HIDC_DOWN,
    IDC_ADD, HIDC_ADD,
    IDC_REMOVE, HIDC_REMOVE,
    0,0 
};

BOOL CColourSchemes::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs6);
    return TRUE;
}

void CColourSchemes::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	theApp.HtmlHelpContextMenu((HWND)pWnd->GetSafeHwnd(), id_pairs6);
}

// Add Scheme button
void CColourSchemes::OnAddScheme() 
{
    CNewScheme dlg;

    dlg.psvec_ = &scheme_;
    if (dlg.DoModal() == IDOK)
    {
        scheme_.push_back(scheme_[dlg.copy_from_]);
        scheme_.back().name_ = dlg.scheme_name_;
        scheme_.back().can_delete_ = TRUE;
        scheme_no_ = scheme_.size() - 1;

        CListBox *plist = (CListBox *)GetDlgItem(IDC_SCHEMES);
        ASSERT(plist != NULL);
        plist->AddString(dlg.scheme_name_);

        SetModified();

        UpdateData(FALSE);

        set_scheme();
    }
}

// Remove scheme button
void CColourSchemes::OnRemoveScheme() 
{
    ASSERT(scheme_no_ >= 0 && scheme_no_ < scheme_.size() && scheme_[scheme_no_].can_delete_);

    CListBox *plist = (CListBox *)GetDlgItem(IDC_SCHEMES);
    ASSERT(plist != NULL);
    plist->DeleteString(scheme_no_);
    scheme_.erase(scheme_.begin() + scheme_no_);

    SetModified();

    // If scheme deleted was at end make scheme_no_ valid
    if (scheme_no_ >= scheme_.size())
        scheme_no_ = scheme_.size() - 1;
    UpdateData(FALSE);

    set_scheme();
}

// Selection changed in schemes list
void CColourSchemes::OnSelchangeScheme() 
{
    // Get new scheme no
    UpdateData();

    // If there is an active view then selecting a different scheme changes the colours
    // in that view.  If no active view then this has no effect outside the options dlg.
    if (GetView() != NULL)
        SetModified();
    set_scheme();
}

// New range button
void CColourSchemes::OnAddRange() 
{
    // Get the range name from the edit control
    CString name;
    ASSERT(GetDlgItem(IDC_CURRENT) != NULL);
    GetDlgItem(IDC_CURRENT)->GetWindowText(name);

    // Convert the current range string to range_t
    CString range;
    ASSERT(GetDlgItem(IDC_RANGES) != NULL);
    GetDlgItem(IDC_RANGES)->GetWindowText(range);

    // Add new one to the end of the vector
    ASSERT(scheme_no_ >= 0 && scheme_no_ < scheme_.size());
    scheme_[scheme_no_].AddRange(name, m_ColourPicker.GetColor(), range);

    SetModified(TRUE);

    // Add to the end of names list and select it
    CListBox *plist = (CListBox *)GetDlgItem(IDC_NAMES);
    ASSERT(plist != NULL);
    ASSERT(name_no_ == plist->GetCurSel());
    plist->InsertString(-1, name);
    name_no_ = plist->GetCount() - 1;
    plist->SetCurSel(name_no_);

    // (Remove/Up/Down buttons are handled in OnIdle) but we still need to disable Add button
    ASSERT(GetDlgItem(IDC_ADD) != NULL);
    GetDlgItem(IDC_ADD)->EnableWindow(FALSE);
}

// Delete button
void CColourSchemes::OnRemoveRange() 
{
    ASSERT(scheme_no_ >= 0 && scheme_no_ < scheme_.size());
    ASSERT(name_no_ >= INDEX_LAST && name_no_ < INDEX_LAST + scheme_[scheme_no_].range_val_.size());
    if (name_no_ < INDEX_LAST) return;  // For safety

    // Get the current selection
    CListBox *plist = (CListBox *)GetDlgItem(IDC_NAMES);
    ASSERT(plist != NULL);
    ASSERT(name_no_ == plist->GetCurSel());

    ASSERT(name_no_ >= INDEX_LAST && name_no_ < INDEX_LAST + scheme_[scheme_no_].range_val_.size());

    // Remove the item from the list box and corresponding vector element
    scheme_[scheme_no_].range_name_.erase(scheme_[scheme_no_].range_name_.begin() + name_no_ - INDEX_LAST);
    scheme_[scheme_no_].range_col_.erase(scheme_[scheme_no_].range_col_.begin() + name_no_ - INDEX_LAST);
    scheme_[scheme_no_].range_val_.erase(scheme_[scheme_no_].range_val_.begin() + name_no_ - INDEX_LAST);
    plist->DeleteString(name_no_);
    name_no_ = -1;

    ASSERT(scheme_[scheme_no_].range_name_.size() == scheme_[scheme_no_].range_col_.size());
    ASSERT(scheme_[scheme_no_].range_name_.size() == scheme_[scheme_no_].range_val_.size());
    SetModified(TRUE);

    ASSERT(GetDlgItem(IDC_ADD) != NULL);
    GetDlgItem(IDC_ADD)->EnableWindow(TRUE);  // This allows re-add of just removed range
}

// Move selected range up in list
void CColourSchemes::OnUp() 
{
    ASSERT(scheme_no_ >= 0 && scheme_no_ < scheme_.size());
    ASSERT(name_no_ > INDEX_LAST && name_no_ < INDEX_LAST + scheme_[scheme_no_].range_val_.size());
    if (name_no_ <= INDEX_LAST) return;  // For safety

    CListBox *plist = (CListBox *)GetDlgItem(IDC_NAMES);
    ASSERT(plist != NULL);
    ASSERT(name_no_ == plist->GetCurSel());

    // Move the entry in the vectors down one position
    CString range_name = scheme_[scheme_no_].range_name_[name_no_ - INDEX_LAST];
    COLORREF range_col = scheme_[scheme_no_].range_col_[name_no_ - INDEX_LAST];
    range_set<int> range_val = scheme_[scheme_no_].range_val_[name_no_ - INDEX_LAST];
    scheme_[scheme_no_].range_name_.erase(scheme_[scheme_no_].range_name_.begin() + name_no_ - INDEX_LAST);
    scheme_[scheme_no_].range_col_.erase(scheme_[scheme_no_].range_col_.begin() + name_no_ - INDEX_LAST);
    scheme_[scheme_no_].range_val_.erase(scheme_[scheme_no_].range_val_.begin() + name_no_ - INDEX_LAST);
    scheme_[scheme_no_].range_name_.insert(scheme_[scheme_no_].range_name_.begin() + name_no_ - INDEX_LAST- 1, range_name);
    scheme_[scheme_no_].range_col_.insert(scheme_[scheme_no_].range_col_.begin() + name_no_ - INDEX_LAST - 1, range_col);
    scheme_[scheme_no_].range_val_.insert(scheme_[scheme_no_].range_val_.begin() + name_no_ - INDEX_LAST - 1, range_val);

    ASSERT(scheme_[scheme_no_].range_name_.size() == scheme_[scheme_no_].range_col_.size());
    ASSERT(scheme_[scheme_no_].range_name_.size() == scheme_[scheme_no_].range_val_.size());
    SetModified(TRUE);

    // Move the corresponding entry in the IDC_NAMES list
    plist->DeleteString(name_no_);
    plist->InsertString(name_no_ - 1, range_name);
    plist->SetCurSel(--name_no_);      // Set selection on same elt at new posn
}

// Move selected range down in list
void CColourSchemes::OnDown() 
{
    ASSERT(scheme_no_ >= 0 && scheme_no_ < scheme_.size());
    ASSERT(name_no_ >= INDEX_LAST && name_no_ < INDEX_LAST + scheme_[scheme_no_].range_val_.size() - 1);
    if (name_no_ < INDEX_LAST) return;  // For safety

    CListBox *plist = (CListBox *)GetDlgItem(IDC_NAMES);
    ASSERT(plist != NULL);
    ASSERT(name_no_ == plist->GetCurSel());

    // Move the entry in the vectors down one position
    CString range_name = scheme_[scheme_no_].range_name_[name_no_ - INDEX_LAST];
    COLORREF range_col = scheme_[scheme_no_].range_col_[name_no_ - INDEX_LAST];
    range_set<int> range_val = scheme_[scheme_no_].range_val_[name_no_ - INDEX_LAST];
    scheme_[scheme_no_].range_name_.erase(scheme_[scheme_no_].range_name_.begin() + name_no_ - INDEX_LAST);
    scheme_[scheme_no_].range_col_.erase(scheme_[scheme_no_].range_col_.begin() + name_no_ - INDEX_LAST);
    scheme_[scheme_no_].range_val_.erase(scheme_[scheme_no_].range_val_.begin() + name_no_ - INDEX_LAST);
    scheme_[scheme_no_].range_name_.insert(scheme_[scheme_no_].range_name_.begin() + name_no_ - INDEX_LAST + 1, range_name);
    scheme_[scheme_no_].range_col_.insert(scheme_[scheme_no_].range_col_.begin() + name_no_ - INDEX_LAST + 1, range_col);
    scheme_[scheme_no_].range_val_.insert(scheme_[scheme_no_].range_val_.begin() + name_no_ - INDEX_LAST + 1, range_val);

    ASSERT(scheme_[scheme_no_].range_name_.size() == scheme_[scheme_no_].range_col_.size());
    ASSERT(scheme_[scheme_no_].range_name_.size() == scheme_[scheme_no_].range_val_.size());
    SetModified(TRUE);

    // Move the corresponding entry in the IDC_NAMES list
    plist->DeleteString(name_no_);
    plist->InsertString(name_no_ + 1, range_name);
    plist->SetCurSel(++name_no_);      // Set selection on same elt at new posn
}

// Reset the curren scheme to the default
void CColourSchemes::OnSchemeReset() 
{
    ASSERT(scheme_no_ >= 0 && scheme_no_ < scheme_.size());

    CHexEditView *pview = GetView();
    CString saved_name = scheme_[scheme_no_].name_;

    // If standard scheme reset to factory default, else if there is a view then
    // reset depending on current char set, else (no view) set to default ASCII
    if (scheme_[scheme_no_].name_ == ASCII_NAME)
        scheme_[scheme_no_] = theApp.default_ascii_scheme_;
    else if (scheme_[scheme_no_].name_ == ANSI_NAME)
        scheme_[scheme_no_] = theApp.default_ansi_scheme_;
    else if (scheme_[scheme_no_].name_ == OEM_NAME)
        scheme_[scheme_no_] = theApp.default_oem_scheme_;
    else if (scheme_[scheme_no_].name_ == EBCDIC_NAME)
        scheme_[scheme_no_] = theApp.default_ebcdic_scheme_;
//    else if (pview != NULL && pview->EbcdicMode())
//        scheme_[scheme_no_] = theApp.default_ebcdic_scheme_;
//    else if (pview != NULL && pview->OEMMode())
//        scheme_[scheme_no_] = theApp.default_oem_scheme_;
//    else if (pview != NULL && pview->ANSIMode())
//        scheme_[scheme_no_] = theApp.default_ansi_scheme_;
//    else
//        scheme_[scheme_no_] = theApp.default_ascii_scheme_;
    else
        scheme_[scheme_no_] = theApp.default_scheme_;

    scheme_[scheme_no_].name_ = saved_name;
    if (scheme_no_ > 3)
        scheme_[scheme_no_].can_delete_ = true;

    SetModified();
    set_scheme();
}

// Change name of current range in edit control (IDC_CURRENT)
void CColourSchemes::OnChangeCurrent() 
{
    if (change_name_)
        return;

    bool disable = false;               // Disable adding to list
    CString ss;
    ASSERT(GetDlgItem(IDC_CURRENT) != NULL);
    GetDlgItem(IDC_CURRENT)->GetWindowText(ss);

    // Check if we need to disable adding
    if (ss.IsEmpty())
        disable = true;                     // Disable for empty current name
    else
    {
        ASSERT(scheme_no_ >= 0 && scheme_no_ < scheme_.size());

        // But also disable if current name is already in the names list
        for (std::vector<CString>::iterator pp = scheme_[scheme_no_].range_name_.begin();
             pp != scheme_[scheme_no_].range_name_.end(); ++pp)
            if (ss.CompareNoCase(*pp) == 0)
                disable = true;
    }

    // Disable colour & range controls if no name here and no selection in list
    // since we can't set these for an existing range or one to be added
    ASSERT(GetDlgItem(IDC_RANGES) != NULL);

    if (disable && name_no_ == -1)
    {
        // No valid entry to add and we have not selected a range in the list
        GetDlgItem(IDC_RANGES)->EnableWindow(FALSE);
        m_ColourPicker.EnableWindow(FALSE);
    }
    else if (disable && name_no_ < INDEX_LAST)
    {
        // No valid entry to add and we have not selected a range in the list
        GetDlgItem(IDC_RANGES)->EnableWindow(FALSE);
        m_ColourPicker.EnableWindow(TRUE);
    }
    else if (!disable)
    {
        // Changes are for the name to be added so remove any selection in
        // the names list so that we don't change an existing range as well
        name_no_ = -1;

        GetDlgItem(IDC_RANGES)->EnableWindow(TRUE);
        m_ColourPicker.EnableWindow(TRUE);
        m_ColourPicker.EnableAutomaticButton(_T("Automatic"), ::GetSysColor(COLOR_WINDOWTEXT));

        // Since there is no existing name now selected: disable the
        // Remove, Up and Down buttons
        ASSERT(GetDlgItem(IDC_REMOVE) != NULL);
        GetDlgItem(IDC_REMOVE)->EnableWindow(FALSE);
        ASSERT(GetDlgItem(IDC_UP) != NULL);
        GetDlgItem(IDC_UP)->EnableWindow(FALSE);
        ASSERT(GetDlgItem(IDC_DOWN) != NULL);
        GetDlgItem(IDC_DOWN)->EnableWindow(FALSE);
    }

    // Disable the "Add" button if there is no current name
    ASSERT(GetDlgItem(IDC_ADD) != NULL);
    GetDlgItem(IDC_ADD)->EnableWindow(!disable);
    UpdateData(FALSE);
}

// Change selection in range list
void CColourSchemes::OnSelchangeRange() 
{
    UpdateData();
    ASSERT(scheme_no_ >= 0 && scheme_no_ < scheme_.size());
    ASSERT(name_no_ >= 0 && name_no_ < INDEX_LAST + scheme_[scheme_no_].range_val_.size());

    m_ColourPicker.EnableWindow(TRUE);
    if (name_no_ < INDEX_LAST)
    {
		// Note: If the Automatic colour is changed here it must also be changed where the colour
		// is used (if -1). For example in CHexEditView::set_colours().
        switch (name_no_)
        {
        case INDEX_BG:
            m_ColourPicker.EnableAutomaticButton(_T("Automatic"), ::GetSysColor(COLOR_WINDOW));
            m_ColourPicker.SetColor(scheme_[scheme_no_].bg_col_);
            break;
        case INDEX_MARK:
            m_ColourPicker.EnableAutomaticButton(_T("Automatic"), RGB(224, 224, 224)); // grey?
            m_ColourPicker.SetColor(scheme_[scheme_no_].mark_col_);
            break;
        case INDEX_HI:
            m_ColourPicker.EnableAutomaticButton(_T("Automatic"), RGB(255, 255, 0));  // yellow
            m_ColourPicker.SetColor(scheme_[scheme_no_].hi_col_);
            break;
        case INDEX_BM:
            m_ColourPicker.EnableAutomaticButton(_T("Automatic"), RGB(160, 192, 255)); // bluish
            m_ColourPicker.SetColor(scheme_[scheme_no_].bm_col_);
            break;
        case INDEX_SEARCH:
            m_ColourPicker.EnableAutomaticButton(_T("Automatic"), RGB(160, 255, 224)); // greenish
            m_ColourPicker.SetColor(scheme_[scheme_no_].search_col_);
            break;
#ifdef CHANGE_TRACKING
        case INDEX_TRK:
            m_ColourPicker.EnableAutomaticButton(_T("Automatic"), RGB(255, 0, 0));  // red
            m_ColourPicker.SetColor(scheme_[scheme_no_].trk_col_);
            break;
        case INDEX_ADDR_BG:
            m_ColourPicker.EnableAutomaticButton(_T("Automatic"),
				tone_down(::GetSysColor(COLOR_INACTIVEBORDER), scheme_[scheme_no_].bg_col_ == -1 ? ::GetSysColor(COLOR_WINDOW) : scheme_[scheme_no_].bg_col_));
            m_ColourPicker.SetColor(scheme_[scheme_no_].addr_bg_col_);
            break;
        case INDEX_SECTOR:
            m_ColourPicker.EnableAutomaticButton(_T("Automatic"), RGB(255, 160, 128));  // pinkish orange
            m_ColourPicker.SetColor(scheme_[scheme_no_].sector_col_);
            break;
#endif
        case INDEX_HEX_ADDR:
            m_ColourPicker.EnableAutomaticButton(_T("Automatic"), ::GetSysColor(COLOR_WINDOWTEXT));
            m_ColourPicker.SetColor(scheme_[scheme_no_].hex_addr_col_);
            break;
        case INDEX_DEC_ADDR:
            m_ColourPicker.EnableAutomaticButton(_T("Automatic"), ::GetSysColor(COLOR_WINDOWTEXT));
            m_ColourPicker.SetColor(scheme_[scheme_no_].dec_addr_col_);
            break;
        default:
            ASSERT(0);
        }

        ASSERT(GetDlgItem(IDC_RANGES) != NULL);
        change_range_ = true;
        GetDlgItem(IDC_RANGES)->SetWindowText("");
        change_range_ = false;
        GetDlgItem(IDC_RANGES)->EnableWindow(FALSE);

        ASSERT(GetDlgItem(IDC_CURRENT) != NULL);
        change_name_ = true;
        GetDlgItem(IDC_CURRENT)->SetWindowText("");
        change_name_ = false;
    }
    else
    {
        m_ColourPicker.EnableAutomaticButton(_T("Automatic"), ::GetSysColor(COLOR_WINDOWTEXT));
        m_ColourPicker.SetColor(scheme_[scheme_no_].range_col_[name_no_ - INDEX_LAST]);

        // Get the range as a string and display in IDC_RANGES
        std::ostringstream strstr;
        strstr << scheme_[scheme_no_].range_val_[name_no_ - INDEX_LAST];

        // Convert std::stringstream to std::string to C string to MFC CString
        const CString ss = (const char *)strstr.str().c_str();

        ASSERT(GetDlgItem(IDC_RANGES) != NULL);
        change_range_ = true;
        GetDlgItem(IDC_RANGES)->SetWindowText(ss);
        change_range_ = false;
        GetDlgItem(IDC_RANGES)->EnableWindow(TRUE);

        // Display the name in IDC_CURRENT
        ASSERT(GetDlgItem(IDC_CURRENT) != NULL);
        change_name_ = true;
        GetDlgItem(IDC_CURRENT)->SetWindowText(scheme_[scheme_no_].range_name_[name_no_ - INDEX_LAST]);
        change_name_ = false;
    }
    ASSERT(GetDlgItem(IDC_ADD) != NULL);
    GetDlgItem(IDC_ADD)->EnableWindow(FALSE);
}

// new colour chosen
void CColourSchemes::OnColourPicker() 
{
    // If there is a name selected update the corresponding colour
    if (name_no_ != -1)
    {
        ASSERT(scheme_no_ >= 0 && scheme_no_ < scheme_.size());
        ASSERT(name_no_ >= 0 && name_no_ < INDEX_LAST + scheme_[scheme_no_].range_val_.size());

        // Set the colour for this element
        if (name_no_ < INDEX_LAST)
        {
            switch (name_no_)
            {
            case INDEX_BG:
                scheme_[scheme_no_].bg_col_ = m_ColourPicker.GetColor();
                break;
            case INDEX_MARK:
                scheme_[scheme_no_].mark_col_ = m_ColourPicker.GetColor();
                break;
            case INDEX_HI:
                scheme_[scheme_no_].hi_col_ = m_ColourPicker.GetColor();
                break;
            case INDEX_BM:
                scheme_[scheme_no_].bm_col_ = m_ColourPicker.GetColor();
                break;
            case INDEX_SEARCH:
                scheme_[scheme_no_].search_col_ = m_ColourPicker.GetColor();
                break;
#ifdef CHANGE_TRACKING
            case INDEX_TRK:
                scheme_[scheme_no_].trk_col_ = m_ColourPicker.GetColor();
                break;
            case INDEX_ADDR_BG:
                scheme_[scheme_no_].addr_bg_col_ = m_ColourPicker.GetColor();
                break;
            case INDEX_SECTOR:
                scheme_[scheme_no_].sector_col_ = m_ColourPicker.GetColor();
                break;
#endif
            case INDEX_HEX_ADDR:
                scheme_[scheme_no_].hex_addr_col_ = m_ColourPicker.GetColor();
                break;
            case INDEX_DEC_ADDR:
                scheme_[scheme_no_].dec_addr_col_ = m_ColourPicker.GetColor();
                break;
            default:
                ASSERT(0);
            }
        }
        else
        {
            scheme_[scheme_no_].range_col_[name_no_ - INDEX_LAST] = m_ColourPicker.GetColor();
        }
        SetModified(TRUE);
    }
}

// Ranges string changed
void CColourSchemes::OnChangeRange() 
{
    // If the user change the range string and there is a name selected
    if (!change_range_ && name_no_ >= INDEX_LAST)
    {
        ASSERT(scheme_no_ >= 0 && scheme_no_ < scheme_.size());
        ASSERT(name_no_ >= INDEX_LAST && name_no_ < INDEX_LAST + scheme_[scheme_no_].range_val_.size());

        // Get the new range string
        CString ss;
        ASSERT(GetDlgItem(IDC_RANGES) != NULL);
        GetDlgItem(IDC_RANGES)->GetWindowText(ss);

        // Set the range for this element
        std::istringstream strstr((const char *)ss);
        strstr >> scheme_[scheme_no_].range_val_[name_no_ - INDEX_LAST];
        SetModified(TRUE);
    }
}

