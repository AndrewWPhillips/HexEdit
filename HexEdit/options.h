#ifndef _OPTIONS_H
#define _OPTIONS_H

// options.h : header file for Options tabbed dialog box
//

// Copyright (c) 1999 by Andrew W. Phillips.
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

#include "GridCtrl_src/GridCtrl.h"
#include "GridBtnCell_src/GridBtnCell.h"
#include "Scheme.h"

#include "control.h"

// NOTES
// To add a new property:
//   1. Add field to OptValues to hold the value for the dialog
//   2. Init the field in the COptSheet c'tor (even though it gets overwritten later)
//   3. Add a control into a page to let the user chnage it
//   4. Add DDX (and validation) to the page's class
//   5. Init field in CHexEditApp::get_options (before dlg called)
//   6. Save the field in CHexEditApp::put_* method for the page (for OK/Apply)
// To move a field between pages:
//   1. Move the control to the new page
//   2. Move DXX (validation etc) to the new pages class
//   3. Move the saving of the field to the CHexEditApp::put_* method for the page
// This last step is VERY IMPORTANT as OnOK is not necessarilly called for all pages
// when the user clicks OK or APPLY (ie if the user never went into the page).
// If the saving of the field is in the wrong place it may sometimes work and
// sometimes not which could be very confusing to the user (and in testing).

/////////////////////////////////////////////////////////////////////////////
// OptValues - stores all option values in one place
struct OptValues
{
    // System options
	BOOL	shell_open_;
	BOOL	one_only_;
    BOOL    run_autoexec_;
	BOOL	save_exit_;
	UINT	recent_files_;
	BOOL    no_recent_add_;

    // Workspace
	BOOL	bg_search_;
	BOOL    intelligent_undo_;
    UINT    undo_limit_;

    // Backup options
	int		backup_;
	BOOL	backup_space_;
	BOOL	backup_if_size_;
	UINT	backup_size_;
	BOOL	backup_prompt_;

    // Export options
	int		address_specified_;
    long    base_address_;
	UINT	export_line_len_;

    // System display
	BOOL	open_restore_;
	BOOL	mditabs_;
	BOOL	tabsbottom_;
    BOOL    tabicons_;
	BOOL	large_cursor_;
	BOOL	hex_ucase_;
	BOOL	show_other_;
    BOOL    nice_addr_;

    // Template
	int		dffd_view_;
	UINT	max_fix_for_elts_;
	CString	default_char_format_;
	CString	default_date_format_;
	CString	default_int_format_;
	CString	default_real_format_;
	CString	default_string_format_;
	CString	default_unsigned_format_;

    // Macros
	int		refresh_;
	long	num_secs_;
	long	num_keys_;
	long	num_plays_;
	BOOL	refresh_props_;
	BOOL	refresh_bars_;
	int		halt_level_;

    // Printer
	CString	header_;
	CString	footer_;
	BOOL	border_;
	BOOL	headings_;
	int     units_;  // 0 = inches, 1 = cm
	int		spacing_;
	double	left_;
	double	top_;
	double	right_;
	double	bottom_;
	double	header_edge_;
	double	footer_edge_;

    // The rest are only used if there is a window open
    CString window_name_;       // Active view's window name

    // Window display
	int		show_area_;
	int		charset_;
	int		control_;

    // xxx check if these can be stored in the page (only used temporailly I think)
    LOGFONT lf_;                 // Default logical font (normal ASCII, ANSI, EBCDIC)
    LOGFONT oem_lf_;             // Logical font if displaying IBM/OEM character set

	UINT	cols_;
	UINT	offset_;
	UINT	grouping_;
    UINT    vertbuffer_;
	BOOL	autofit_;
	BOOL	maximize_;
    BOOL    borders_;
	BOOL    ruler_;
	BOOL	addr_dec_, addr_hex_;
    BOOL    line_nums_;
	BOOL	addrbase1_;
    BOOL    scroll_past_ends_;
    // Display state stored in a DWORD (as in view)
    union
    {
        DWORD disp_state_;
        struct display_bits display_;
    };

    // Window edit
	int		modify_;
	int		insert_;
	BOOL    big_endian_;
    //int     change_tracking_;
    BOOL    ct_modifications_, ct_insertions_, ct_deletions_, ct_delcount_; // change tracking options
    BOOL    show_bookmarks_, show_highlights_;
};

/////////////////////////////////////////////////////////////////////////////
// COptSheet

class COptSheet : public CBCGPropertySheet
{
	DECLARE_DYNAMIC(COptSheet)

// Construction
public:
	COptSheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	COptSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COptSheet)
	public:
	virtual BOOL DestroyWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~COptSheet();

    struct OptValues val_;      // Property values that the user can change

	// Generated message map functions
protected:
	//{{AFX_MSG(COptSheet)
	afx_msg BOOL OnNcCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
    afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	DECLARE_MESSAGE_MAP()

	void init();
};

/////////////////////////////////////////////////////////////////////////////
// COptPage - base class for all options pages
class COptPage : public CBCGPropertyPage
{
	DECLARE_DYNAMIC(COptPage)
public:
    explicit COptPage(UINT nIDD = 0, UINT nIDCaption = 0) : CBCGPropertyPage(nIDD, nIDCaption), pParent(NULL) { }
    virtual LRESULT OnIdle(long) { return FALSE; }
    COptSheet *pParent;
protected:
	virtual BOOL OnInitDialog()
    {
        pParent = (COptSheet *)GetParent();
	    ASSERT(pParent != NULL && pParent->IsKindOf(RUNTIME_CLASS(COptSheet)));
        return CBCGPropertyPage::OnInitDialog();
    }
};

/////////////////////////////////////////////////////////////////////////////
// CSystemGeneralPage dialog

class CSystemGeneralPage : public COptPage
{
	DECLARE_DYNCREATE(CSystemGeneralPage)

// Construction
public:
    CSystemGeneralPage() : COptPage(IDD) { }

// Dialog Data
	enum { IDD = IDD_OPT_SYSTEM };

// Overrides
public:
	virtual void OnOK();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	virtual BOOL OnInitDialog();
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnSaveNow();
	afx_msg void OnShellopen();
	afx_msg void OnClearHist();
	afx_msg void OnChange();
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CWorkspacePage - general workspace options

class CWorkspacePage : public COptPage
{
	DECLARE_DYNCREATE(CWorkspacePage)

// Construction
public:
    CWorkspacePage() : COptPage(IDD) { }

	enum { IDD = IDD_OPT_WORKSPACE };
	CButton	ctl_backup_space_;
	CEdit	ctl_backup_size_;
	CButton	ctl_backup_prompt_;
	CButton	ctl_backup_if_size_;
	CHexEditControl	address_ctl_;

// Overrides
public:
	virtual void OnOK();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnBackup();
	afx_msg void OnBackupIfSize();
	afx_msg void OnAddressSpecified();
	afx_msg void OnAddressFile();
	afx_msg void OnChange();
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CMacroPage dialog

class CMacroPage : public COptPage
{
	DECLARE_DYNCREATE(CMacroPage)

// Construction
public:
    CMacroPage() : COptPage(CMacroPage::IDD) { }

// Dialog Data
	//{{AFX_DATA(CMacroPage)
	enum { IDD = IDD_OPT_MACRO };
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMacroPage)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMacroPage)
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnChange();
	afx_msg void OnRefreshNever();
	afx_msg void OnRefreshPlays();
	afx_msg void OnRefreshSecs();
	afx_msg void OnRefreshKeys();
	virtual BOOL OnInitDialog();
	afx_msg void OnSavemacro();
	afx_msg void OnLoadmacro();
	afx_msg void OnMacrodir();
	//}}AFX_MSG
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CWorkspaceDisplayPage dialog - global display options

class CWorkspaceDisplayPage : public COptPage
{
	DECLARE_DYNCREATE(CWorkspaceDisplayPage)

// Construction
public:
	CWorkspaceDisplayPage() : COptPage(IDD) { pStartupPage = NULL; }

// Dialog Data
	enum { IDD = IDD_OPT_WORKDISPLAY };

	void SetStartupPage(COptPage * pPage) { pStartupPage = pPage; }

// Overrides
public:
	virtual void OnOK();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	virtual BOOL OnInitDialog();
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnStartupPage();
	afx_msg void OnChangeMditabs();
	afx_msg void OnVisualizations();
	afx_msg void OnChange();
	DECLARE_MESSAGE_MAP()
private:
	COptPage * pStartupPage;
    void fix_controls();
};

/////////////////////////////////////////////////////////////////////////////
// CTemplatePage dialog - global template options

class CTemplatePage : public COptPage
{
	DECLARE_DYNCREATE(CTemplatePage)

// Construction
public:
	CTemplatePage() : COptPage(IDD) { }

// Dialog Data
	enum { IDD = IDD_OPT_WORKTEMPLATE };

// Overrides
public:
	virtual void OnOK();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	virtual BOOL OnInitDialog();
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnChange();
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CPrintPage dialog

class CPrintPage : public COptPage
{
	DECLARE_DYNCREATE(CPrintPage)

// Construction
public:
	CPrintPage() : COptPage(IDD) { }

// Dialog Data
	//{{AFX_DATA(CPrintPage)
	enum { IDD = IDD_OPT_PRINT };
	CEdit	ctl_footer_;
	CEdit	ctl_header_;
	CBCGMenuButton	footer_args_;
	CBCGMenuButton	header_args_;
	//}}AFX_DATA

// Controls
        HICON arrow_icon_;
	CMenu args_menu_;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPrintPage)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPrintPage)
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnChange();
	afx_msg void OnFooterOpts();
	afx_msg void OnHeaderOpts();
	//}}AFX_MSG
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnChangeUnits();
	DECLARE_MESSAGE_MAP()

};
/////////////////////////////////////////////////////////////////////////////
// CFiltersPage dialog

class CFiltersPage : public COptPage
{
	DECLARE_DYNCREATE(CFiltersPage)

// Construction
public:
	CFiltersPage() : COptPage(IDD) { }

// Dialog Data
	//{{AFX_DATA(CFiltersPage)
	enum { IDD = IDD_OPT_FILTERS };
	CButton	ctl_up_;
	CButton	ctl_new_;
	CButton	ctl_down_;
	CButton	ctl_del_;
	//}}AFX_DATA

    LRESULT OnIdle(long);

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CFiltersPage)
	public:
	virtual void OnOK();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    enum { /*column_number,*/ column_check, column_files, column_filter,
           /* leave this one at end*/ column_count };
    enum { header_rows = 0 };

    CGridCtrl grid_;                       // MFC grid control
    CBtnDataBase btn_db_;                  // Needed for button cells (check box)

    CToolTipCtrl m_cToolTip;
    void add_row(int row, BOOL is_checked = TRUE, CString s1 = "", CString s2 = "");
    HICON icon_new_, icon_del_, icon_up_, icon_down_;

	// Generated message map functions
	//{{AFX_MSG(CFiltersPage)
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnDel();
	afx_msg void OnNew();
	afx_msg void OnUp();
	afx_msg void OnDown();
	//}}AFX_MSG
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void OnGridEndEdit(NMHDR *pNotifyStruct, LRESULT* pResult);
    afx_msg void OnGridClick(NMHDR *pNotifyStruct, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CTipsPage dialog

class CTipsPage : public COptPage
{
	DECLARE_DYNCREATE(CTipsPage)

// Construction
public:
	CTipsPage() : COptPage(IDD) { }

// Dialog Data
	enum { IDD = IDD_OPT_TIPS };
	CSliderCtrl ctl_slider_;
	CButton	ctl_up_;
	CButton	ctl_new_;
	CButton	ctl_down_;
	CButton	ctl_del_;

    LRESULT OnIdle(long);

// Overrides
public:
	virtual void OnOK();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
    enum { column_check, column_name, column_expr, column_format,
           /* leave this one at end*/ column_count };
    enum { header_rows = 0 };

    CGridCtrl grid_;                       // MFC grid control
    CBtnDataBase btn_db_;                  // Needed for button cells (check box)

    CStringArray var_list;                 // Stores all "variable" names that can be used in an expression used for column_expr drop list
	CStringArray int_list,uint_list,real32_list,real64_list,date_list,char_list,string_list;

    CToolTipCtrl m_cToolTip;
    void add_row(int row, BOOL is_checked = TRUE, CString s1 = "", CString s2 = "", CString s3 = "");
    HICON icon_new_, icon_del_, icon_up_, icon_down_;

	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnDel();
	afx_msg void OnNew();
	afx_msg void OnUp();
	afx_msg void OnDown();
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void OnGridEndEdit(NMHDR *pNotifyStruct, LRESULT* pResult);
    afx_msg void OnGridClick(NMHDR *pNotifyStruct, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CWindowGeneralPage dialog

class CWindowGeneralPage : public COptPage
{
	DECLARE_DYNCREATE(CWindowGeneralPage)

// Construction
public:
	CWindowGeneralPage() : COptPage(IDD) { }

// Dialog Data
	enum { IDD = IDD_OPT_WINGENERAL };
    HICON hicon_;               // Large icon for the file type
    CStatic ctl_icon_;

// Overrides
public:
	virtual void OnOK();
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	virtual void OnCancel();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	// Message map functions
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSaveDefault();
	afx_msg void OnDispReset();
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CWindowPage dialog
class CWindowPage : public COptPage
{
	DECLARE_DYNCREATE(CWindowPage)

// Construction
public:
	CWindowPage();

// Dialog Data
	enum { IDD = IDD_OPT_WINDISPLAY };

	void SetGlobalDisplayPage(COptPage * pPage) { pGlobalPage = pPage; }

    LRESULT OnIdle(long);

// Overrides
public:
	virtual void OnOK();
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	virtual void OnCancel();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	// Message map functions
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnGlobalPage();
	afx_msg void OnFont();
	afx_msg void OnChangeCols();
	afx_msg void OnSelchangeShowArea();
	afx_msg void OnSelchangeCharset();
	afx_msg void OnSelchangeControl();
	afx_msg void OnChangeAddrDec();
	afx_msg void OnChangeAddrHex();
	afx_msg void OnChangeLineNos();
	afx_msg void OnChangeUpdate();
	afx_msg void OnChange();
	DECLARE_MESSAGE_MAP()

private:
    void fix_controls();        // Disable/enable controls depending on value/state of other controls
    BOOL validated();           // Check if control state is valid

	bool update_ok_;            // Stop use of edit control before inited (spin ctrl problem)
	COptPage * pGlobalPage;
};

/////////////////////////////////////////////////////////////////////////////
// CWindowEditPage dialog
class CWindowEditPage : public COptPage
{
	DECLARE_DYNCREATE(CWindowEditPage)

// Construction
public:
	CWindowEditPage();

// Dialog Data
	enum { IDD = IDD_OPT_WINEDIT };

	void SetGlobalEditPage(COptPage * pPage) { pGlobalPage = pPage; }

    LRESULT OnIdle(long);

// Overrides
public:
	virtual void OnOK();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	// Message map functions
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnGlobalPage();
	afx_msg void OnSelchangeModify();
	afx_msg void OnSelchangeInsert();
	afx_msg void OnChangeTracking();
	afx_msg void OnChangeUpdate();
	afx_msg void OnChange();
	DECLARE_MESSAGE_MAP()

private:
    void fix_controls();        // Disable/enable controls depending on value/state of other controls

	COptPage * pGlobalPage;
};

/////////////////////////////////////////////////////////////////////////////
// CColourSchemes dialog

class CColourSchemes : public COptPage
{
	DECLARE_DYNCREATE(CColourSchemes)

// Construction
public:
	CColourSchemes();

    // These enums give the order of the fixed colours in the list box
    enum
    {
        INDEX_BG,
		INDEX_MARK, INDEX_HI, INDEX_BM, INDEX_SEARCH,       // background colours
		INDEX_TRK,
		INDEX_ADDR_BG,   // address area background col
		INDEX_SECTOR,    // sector boundary (and bad sector background)
        INDEX_HEX_ADDR, INDEX_DEC_ADDR,                   // text colours
        INDEX_LAST                                        // count of colours (keep at end)
    };

    HICON icon_new_, icon_del_, icon_up_, icon_down_;
    std::vector<CScheme> scheme_;

        // Dialog Data
	//{{AFX_DATA(CColourSchemes)
	enum { IDD = IDD_OPT_COLOURS };
	CBCGColorButton	m_ColourPicker;
	int		scheme_no_;
	int		name_no_;
	//}}AFX_DATA

    LRESULT OnIdle(long);
    void set_scheme();

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CColourSchemes)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CColourSchemes)
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnAddScheme();
	afx_msg void OnRemoveScheme();
	afx_msg void OnSelchangeScheme();
	afx_msg void OnAddRange();
	afx_msg void OnRemoveRange();
	afx_msg void OnUp();
	afx_msg void OnDown();
	afx_msg void OnSchemeReset();
	afx_msg void OnChangeCurrent();
	afx_msg void OnSelchangeRange();
	afx_msg void OnChangeRange();
	afx_msg void OnColourPicker();
	//}}AFX_MSG
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()

    bool change_name_;              // Is the change to the current name not done by user?
    bool change_range_;             // Is change to range not done by user?
};
#endif
