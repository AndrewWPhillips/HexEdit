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
// #include "Partition.h"  // no longer used when schemes added

/////////////////////////////////////////////////////////////////////////////
// CGeneralPage dialog

class CGeneralPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CGeneralPage)

// Construction
public:
	CGeneralPage();
	~CGeneralPage();

// Dialog Data
	//{{AFX_DATA(CGeneralPage)
	enum { IDD = IDD_OPT_SYSTEM };
	CButton	ctl_backup_space_;
	CEdit	ctl_backup_size_;
	CButton	ctl_backup_prompt_;
	CButton	ctl_backup_if_size_;
	CHexEditControl	address_ctl_;
	BOOL	save_exit_;
	BOOL	one_only_;
	BOOL	shell_open_;
	BOOL	bg_search_;
	int		address_specified_;
	UINT	export_line_len_;
	UINT	recent_files_;
	int		backup_;
	BOOL	backup_space_;
	UINT	backup_size_;
	BOOL	backup_prompt_;
	BOOL	backup_if_size_;
	//}}AFX_DATA

        long base_address_;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGeneralPage)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CGeneralPage)
	afx_msg void OnSaveNow();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnShellopen();
	afx_msg void OnAddressSpecified();
	afx_msg void OnAddressFile();
	virtual BOOL OnInitDialog();
	afx_msg void OnClearHist();
	afx_msg void OnChange();
	afx_msg void OnBackup();
	afx_msg void OnBackupIfSize();
	//}}AFX_MSG
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// COptSheet

class COptSheet : public CPropertySheet
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

	// Generated message map functions
protected:
	//{{AFX_MSG(COptSheet)
	afx_msg BOOL OnNcCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
    afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CMacroPage dialog

class CMacroPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CMacroPage)

// Construction
public:
	CMacroPage();
	~CMacroPage();

// Dialog Data
	//{{AFX_DATA(CMacroPage)
	enum { IDD = IDD_OPT_MACRO };
	long	num_keys_;
	long	num_plays_;
	long	num_secs_;
	int		refresh_;
	BOOL	refresh_bars_;
	BOOL	refresh_props_;
	int		halt_level_;
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
// CSysDisplayPage dialog

class CSysDisplayPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSysDisplayPage)

// Construction
public:
	CSysDisplayPage();
	~CSysDisplayPage();

// Dialog Data
	//{{AFX_DATA(CSysDisplayPage)
	enum { IDD = IDD_OPT_SYSDISPLAY };
	BOOL	large_cursor_;
	BOOL	hex_ucase_;
	BOOL	open_restore_;
	BOOL	mditabs_;
	BOOL	tabsbottom_;
	BOOL	show_other_;
	UINT	max_fix_for_elts_;
	CString	default_char_format_;
	CString	default_date_format_;
	CString	default_int_format_;
	CString	default_real_format_;
	CString	default_string_format_;
	CString	default_unsigned_format_;
	int		dffd_view_;
	BOOL	dffd_on_;
	//}}AFX_DATA
    BOOL tree_edit_;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSysDisplayPage)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSysDisplayPage)
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnChange();
	afx_msg void OnChangeMditabs();
	afx_msg void OnVisualizations();
	afx_msg void OnDffdOn();
	virtual BOOL OnInitDialog();
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()

    void FixDffd();
};
/////////////////////////////////////////////////////////////////////////////
// CPrintPage dialog

class CPrintPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CPrintPage)

// Construction
public:
	CPrintPage();
	~CPrintPage();

// Dialog Data
	//{{AFX_DATA(CPrintPage)
	enum { IDD = IDD_OPT_PRINT };
	CEdit	ctl_footer_;
	CEdit	ctl_header_;
	CBCGMenuButton	footer_args_;
	CBCGMenuButton	header_args_;
	double	bottom_;
	double	top_;
	double	left_;
	double	right_;
	CString	footer_;
	CString	header_;
	int		spacing_;
	BOOL	border_;
	BOOL	headings_;
	double	header_edge_;
	double	footer_edge_;
	//}}AFX_DATA
	int units_;  // 0 = inches, 1 = cm

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

class CFiltersPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CFiltersPage)

// Construction
public:
	CFiltersPage();
	~CFiltersPage();

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
// CWindowPage dialog

class CWindowPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CWindowPage)

// Construction
public:
	CWindowPage();
	~CWindowPage();

// Dialog Data
	//{{AFX_DATA(CWindowPage)
	enum { IDD = IDD_OPT_WINDISPLAY };
	int		charset_;
	int		insert_;
	BOOL	maximize_;
	int		modify_;
	int		show_area_;
	UINT	cols_;
	UINT	grouping_;
	UINT	offset_;
	int		control_;
	BOOL	addr_dec_;
	BOOL	autofit_;
	//}}AFX_DATA
    UINT    vertbuffer_;
	BOOL    big_endian_;
    BOOL    borders_;
    int     change_tracking_;

    void fix_controls();        // Disable/enable controls depending on value/state of other controls
    BOOL validated();           // Check if control state is valid

    CString window_name_;       // View's window name
	bool update_ok_;            // Stop use of edit control before inited (spin ctrl problem)

    // Display state stored in a DWORD (as in view)
    union
    {
        DWORD disp_state_;
        struct display_bits display_;
    };
    LOGFONT lf_;                 // Default logical font (normal ASCII, ANSI, EBCDIC)
    LOGFONT oem_lf_;             // Logical font if displaying IBM/OEM character set

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWindowPage)
	public:
	virtual void OnOK();
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	virtual void OnCancel();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CWindowPage)
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnSaveDefault();
	afx_msg void OnFont();
	afx_msg void OnChange();
	afx_msg void OnAutofit();
	afx_msg void OnChangeCols();
	afx_msg void OnSelchangeShowArea();
	afx_msg void OnSelchangeCharset();
	afx_msg void OnSelchangeControl();
	afx_msg void OnSelchangeModify();
	afx_msg void OnSelchangeInsert();
	afx_msg void OnDispReset();
	//}}AFX_MSG
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnChangeTracking();
	DECLARE_MESSAGE_MAP()

};
/////////////////////////////////////////////////////////////////////////////
// CColourSchemes dialog

class CColourSchemes : public CPropertyPage
{
	DECLARE_DYNCREATE(CColourSchemes)

// Construction
public:
    // These enums give the order of the fixed colours in the list box
    enum
    {
        INDEX_BG,
		INDEX_MARK, INDEX_HI, INDEX_BM, INDEX_SEARCH,       // background colours
#ifdef CHANGE_TRACKING
		INDEX_TRK,
		INDEX_ADDR_BG,   // address area background col
		INDEX_SECTOR,    // sector boundary (and bad sector background)
#endif
        INDEX_HEX_ADDR, INDEX_DEC_ADDR,                   // text colours
        INDEX_LAST                                        // count of colours (keep at end)
    };

    CColourSchemes();
    ~CColourSchemes();

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
