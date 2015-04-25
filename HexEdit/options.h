// options.h : header file for Options tabbed dialog box
//
// Copyright (c) 2000-2010 by Andrew W. Phillips.
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

#ifndef _OPTIONS_H
#define _OPTIONS_H

#include "GridCtrl_src/GridCtrl.h"
#include "GridBtnCell_src/GridBtnCell.h"
#include "Scheme.h"

#include "control.h"

// NOTES
// To add a new property:
//   1. Add field to OptValues to hold the value for the dialog
//   2. Init the field in the COptSheet c'tor (even though it gets overwritten later)
//   3. Add a control into a page to let the user change it
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

// These values corresp. to radio buttons for open_locn_, save_locn_
enum FOLDER_LOCN
{
	FL_DOC,            // folder of currently active doc (or specified if no file open)
	FL_LAST,           // folder of last folder used (open for open_locn_, save for save_locn_)
	FL_BOTH,           // folder of last open or save
	FL_SPECIFIED,      // use explicitly specified folder (open_folder_ or save_folder_)
};  

// These values correspond to the radio buttons for cb_text_type_
enum CB_TEXT_TYPE      // defines how text data is written to the clipboard
{
	CB_TEXT_BIN_CHARS, // write bytes as binary and chars
	CB_TEXT_HEXTEXT,   // convert each byte to 2 hex digits (as well as adding spaces, end of lines)
	CB_TEXT_AUTO,      // use hex text if it appears to be binary data, else use binary/chars
	CB_TEXT_AREA,      // use hex text in hex area, use binary/chars in text area

	CB_TEXT_LAST       // marks end of values - add new entries before this
};

// Unicode chars used to signifies special bytes/values in char area
// NOTE: These values correspond to the list bopx indices for IDC_CONT_CHAR and IDC_INVALID_CHAR
enum UCODE_CHAR_TYPE
{
	UCODE_BLANK,          // space
	UCODE_DOT,            // interpunct (middle dot)
	UCODE_BULLET,         // large dot
	UCODE_ELLIPSIS,       // 3 dots
	UCODE_QMARK,          // questions mark
	UCODE_INV_QMARK,      // inverted question mark
	UCODE_SUBST,          // subst char (decimal 2426) (rectangle?)
	UCODE_FFF9,           // anchor char (filled rectangle?)
	UCODE_FFFD,           // replacement char (diamond with question mark inside)
	UCODE_LAST
};

struct OptValues
{
	// System options
	BOOL	save_exit_;
	BOOL	shell_open_;
	BOOL	one_only_;
	BOOL	open_restore_;
	BOOL    special_list_scan_;
	BOOL    splash_;
	BOOL    tipofday_;
	BOOL    run_autoexec_;
	BOOL	update_check_;

	// Folder options
	int     open_locn_;
	CString open_folder_;
	int     save_locn_;
	CString save_folder_;

	// Preview (thumbnail) options
	BOOL thumbnail_;
	BOOL thumb_frame_;
	int thumb_size_;
	int thumb_type_;
	double thumb_zoom_;
	int cleanup_days_;

	// Explorer options
	int wipe_type_;
	BOOL show_not_indexed_;
	BOOL sync_tree_;
	BOOL custom_explorer_menu_;

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

	// Macros
	int		refresh_;
	long	num_secs_;
	long	num_keys_;
	long	num_plays_;
	BOOL	refresh_props_;
	BOOL	refresh_bars_;
	int		halt_level_;

	// Printer
	BOOL	border_;
	BOOL	headings_;
	BOOL    print_mark_, print_bookmarks_, print_highlights_, print_search_, 
	        print_change_, print_compare_, print_sectors_;
	BOOL	print_watermark_;
	CString	watermark_;
	CString	header_;
	BOOL	diff_first_header_;
	CString	first_header_;
	CString	footer_;
	BOOL	diff_first_footer_;
	CString	first_footer_;
	BOOL	even_reverse_;
	int     units_;  // 0 = inches, 1 = cm
	int		spacing_;
	double	left_;
	double	top_;
	double	right_;
	double	bottom_;
	double	header_edge_;
	double	footer_edge_;

	// History options
	UINT	recent_files_;
	BOOL    no_recent_add_;
	UINT	max_search_hist_;
	UINT	max_replace_hist_;
	UINT	max_hex_jump_hist_;
	UINT	max_dec_jump_hist_;
	UINT	max_expl_dir_hist_;
	UINT	max_expl_filt_hist_;
	BOOL    clear_recent_file_list_;
	BOOL    clear_bookmarks_;
	BOOL    clear_on_exit_;


	// Background processing options
	BOOL	bg_search_;
	BOOL	bg_stats_;
	  BOOL bg_stats_crc32_;
	  BOOL bg_stats_md5_;
	  BOOL bg_stats_sha1_;
	BOOL    bg_exclude_network_;
	BOOL    bg_exclude_removeable_;
	BOOL    bg_exclude_optical_;
	BOOL    bg_exclude_device_;

	// System layout
	BOOL	mditabs_;
	BOOL	tabsbottom_;
	BOOL    tabicons_;
	BOOL    tabclose_;
	BOOL    tabcolour_;

	BOOL    dlg_dock_, dlg_move_;
	BOOL	hex_ucase_;
	int     k_abbrev_;

	// Global document options
	BOOL	large_cursor_;
	BOOL	show_other_;
	BOOL    nice_addr_;
	BOOL    sel_len_tip_, sel_len_div2_;
	BOOL    ruler_;
	UINT    ruler_dec_ticks_, ruler_dec_nums_;
	UINT    ruler_hex_ticks_, ruler_hex_nums_;
	BOOL    hl_caret_, hl_mouse_;
	int     cont_char_, invalid_char_;

	// Workspace - editing
	BOOL    intelligent_undo_;
	UINT    undo_limit_;
	int     cb_text_type_;
	BOOL    scroll_past_ends_;
	int     autoscroll_accel_;
	BOOL    reverse_zoom_;

	// Template
	UINT	max_fix_for_elts_;
	CString	default_char_format_;
	CString	default_date_format_;
	CString	default_int_format_;
	CString	default_real_format_;
	CString	default_string_format_;
	CString	default_unsigned_format_;

	// Aerial
	UINT aerial_max_;

	// The rest are only used if there is a window open
	CString window_name_;       // Active view's window name

	// How other views are displayed
	int display_template_, display_aerial_, display_comp_, display_prevw_;

	// Window display
	int		show_area_;
	int		charset_;
	int		code_page_;
	int		control_;

	LOGFONT lf_;                 // Default logical font (normal ASCII, ANSI, EBCDIC)
	LOGFONT oem_lf_;             // Logical font if displaying IBM/OEM character set
	LOGFONT mb_lf_;              // Logical font if displaying Code page or Unicode characters

	UINT	cols_;
	UINT	offset_;
	UINT	grouping_;
	UINT    vertbuffer_;
	BOOL	autofit_;
	BOOL	maximize_;
	BOOL    borders_;
	BOOL	addr_dec_, addr_hex_;
	BOOL    line_nums_;
	BOOL	addrbase1_;

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

	// Colour scheme
	CString scheme_name_;
};

class COptSheet;

/////////////////////////////////////////////////////////////////////////////
// COptPage - base class for all options pages
class COptPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(COptPage)
public:
	explicit COptPage(UINT nIDD = 0, UINT nIDCaption = 0) : CMFCPropertyPage(nIDD, nIDCaption), pParent(NULL) { }
	virtual LRESULT OnIdle(long) { return FALSE; }
	COptSheet *pParent;
protected:
	virtual BOOL OnInitDialog();
};

/////////////////////////////////////////////////////////////////////////////
// CSystemGeneralPage dialog

class CSystemGeneralPage : public COptPage
{
	DECLARE_DYNCREATE(CSystemGeneralPage)

// Construction
public:
	CSystemGeneralPage() : COptPage(IDD), pHistPage(NULL) { }

// Dialog Data
	enum { IDD = IDD_OPT_SYSTEM };

	void SetHistPage(COptPage * pPage)
	{
		pHistPage = pPage;
		if (m_hWnd != (HWND)0)
			GetDlgItem(IDC_HIST_PAGE)->EnableWindow(pHistPage != NULL);
	}

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
	afx_msg void OnRestoreAvoidable();
	afx_msg void OnShellopen();
	afx_msg void OnHistPage();
	afx_msg void OnChange();
	DECLARE_MESSAGE_MAP()

private:
	COptPage * pHistPage;
	CMFCButton ctl_hist_butn_;
};

/////////////////////////////////////////////////////////////////////////////
// CFoldersPage dialog

class CFoldersPage : public COptPage
{
	DECLARE_DYNCREATE(CFoldersPage)

public:
	CFoldersPage() : COptPage(IDD_OPT_FOLDERS) { }
// Overrides
	virtual void OnOK();
	virtual BOOL OnApply();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
// Implementation
	virtual BOOL OnInitDialog();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnOpenDir();
	afx_msg void OnSaveDir();
	afx_msg void OnChange();
	DECLARE_MESSAGE_MAP()

private:
	void fix_controls();
};

/////////////////////////////////////////////////////////////////////////////
// CPreviewPage dialog

class CPreviewPage : public COptPage
{
	DECLARE_DYNCREATE(CPreviewPage)

public:
	CPreviewPage() : COptPage(IDD_OPT_PREVIEW) { }
// Overrides
	virtual void OnOK();
	virtual BOOL OnApply();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
// Implementation
	virtual BOOL OnInitDialog();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnChange();
	DECLARE_MESSAGE_MAP()

private:
	void fix_controls();
};

/////////////////////////////////////////////////////////////////////////////
// CExplorerPage dialog

class CExplorerPage : public COptPage
{
	DECLARE_DYNCREATE(CExplorerPage)

public:
	CExplorerPage() : COptPage(IDD_OPT_EXPLORER), pFiltersPage(NULL) { }

	void SetFiltersPage(COptPage * pPage)
	{
		pFiltersPage = pPage;
		if (m_hWnd != (HWND)0 && GetDlgItem(IDC_FILTERS_PAGE) != NULL)
			GetDlgItem(IDC_FILTERS_PAGE)->EnableWindow(pFiltersPage != NULL);
	}
// Overrides
	virtual void OnOK();
	virtual BOOL OnApply();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
// Implementation
	virtual BOOL OnInitDialog();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnChange();
	afx_msg void OnFiltersPage();
	DECLARE_MESSAGE_MAP()

private:
	void fix_controls();

	COptPage * pFiltersPage;
	CMFCButton ctl_filters_butn_;
};

/////////////////////////////////////////////////////////////////////////////
// CBackupPage dialog

class CBackupPage : public COptPage
{
	DECLARE_DYNCREATE(CBackupPage)

public:
	CBackupPage() : COptPage(IDD_OPT_BACKUP) { }
// Overrides
	virtual void OnOK();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
// Implementation
	virtual BOOL OnInitDialog();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnChange();
	afx_msg void OnBackup();
	afx_msg void OnBackupIfSize();
	afx_msg void OnAddressSpecified();
	afx_msg void OnAddressFile();
	DECLARE_MESSAGE_MAP()

private:
	CHexEdit	address_ctl_;
	void fix_controls();
};

/////////////////////////////////////////////////////////////////////////////
// CHistoryPage dialog

class CHistoryPage : public COptPage
{
	DECLARE_DYNCREATE(CHistoryPage)

public:
	CHistoryPage() : COptPage(IDD_OPT_HISTORY) { }
// Overrides
	virtual void OnOK();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
// Implementation
	virtual BOOL OnInitDialog();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnClearNow();
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
// CWorkspaceLayoutPage dialog - main window layout options

class CWorkspaceLayoutPage : public COptPage
{
	DECLARE_DYNCREATE(CWorkspaceLayoutPage)

// Construction
public:
	CWorkspaceLayoutPage() : COptPage(IDD), pStartupPage(NULL) { }

// Dialog Data
	enum { IDD = IDD_OPT_WORKLAYOUT };

	void SetStartupPage(COptPage * pPage)
	{
		pStartupPage = pPage;
		if (m_hWnd != (HWND)0)
			GetDlgItem(IDC_STARTUP_PAGE)->EnableWindow(pStartupPage != NULL);
	}

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
	//afx_msg void OnVisualizations();
	afx_msg void OnChange();
	DECLARE_MESSAGE_MAP()
private:
	void fix_controls();

	COptPage * pStartupPage;
	CMFCButton ctl_startup_butn_;
};

/////////////////////////////////////////////////////////////////////////////
// CWorkspaceDisplayPage dialog - global display options

class CWorkspaceDisplayPage : public COptPage
{
	DECLARE_DYNCREATE(CWorkspaceDisplayPage)

// Construction
public:
	CWorkspaceDisplayPage() : COptPage(IDD), pDocPage(NULL) { }

// Dialog Data
	enum { IDD = IDD_OPT_WORKDISPLAY };
	//CSliderCtrl ctl_slider_autoscroll_;

	LRESULT OnIdle(long);

	void SetDocDisplayPage(COptPage * pPage)
	{
		pDocPage = pPage;
		if (m_hWnd != (HWND)0)
			GetDlgItem(IDC_DOC_PAGE)->EnableWindow(pDocPage != NULL);
	}

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
	afx_msg void OnChangeUpdate();
	afx_msg void OnDocPage();
	DECLARE_MESSAGE_MAP()

private:
	void fix_controls();

	COptPage * pDocPage;
	CMFCButton ctl_doc_butn_;
};


/////////////////////////////////////////////////////////////////////////////
// CWorkspaceEditPage - workspace editing options

class CWorkspaceEditPage : public COptPage
{
	DECLARE_DYNCREATE(CWorkspaceEditPage)

// Construction
public:
	CWorkspaceEditPage() : COptPage(IDD), pDocPage(NULL), pBackupPage(NULL) { }

	enum { IDD = IDD_OPT_WORKEDIT };

	void SetDocEditPage(COptPage * pPage)
	{
		pDocPage = pPage;
		if (m_hWnd != (HWND)0)
			GetDlgItem(IDC_DOC_PAGE)->EnableWindow(pDocPage != NULL);
	}
	void SetBackupPage(COptPage * pPage)
	{
		pBackupPage = pPage;
		if (m_hWnd != (HWND)0)
			GetDlgItem(IDC_BACKUP_PAGE)->EnableWindow(pBackupPage != NULL);
	}

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
	afx_msg void OnChange();
	afx_msg void OnDocPage();
	afx_msg void OnBackupPage();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	DECLARE_MESSAGE_MAP()

private:
	void fix_controls();

	COptPage * pDocPage;        // corresp, document (edit) page
	CMFCButton ctl_doc_butn_;
	COptPage * pBackupPage;     // contains backup/export options that used to be on this page
	CMFCButton ctl_backup_butn_;
};
#if 0
/////////////////////////////////////////////////////////////////////////////
// CWorkspacePage - OLD workspace editing options

class CWorkspacePage : public COptPage
{
	DECLARE_DYNCREATE(CWorkspacePage)

// Construction
public:
	CWorkspacePage() : COptPage(IDD), pDocPage(NULL) { }

	enum { IDD = IDD_OPT_WORKSPACE };
	CButton	ctl_backup_space_;
	CEdit	ctl_backup_size_;
	CButton	ctl_backup_prompt_;
	CButton	ctl_backup_if_size_;
	CHexEdit	address_ctl_;

	void SetDocEditPage(COptPage * pPage) { pDocPage = pPage; }

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
	afx_msg void OnDocPage();
	DECLARE_MESSAGE_MAP()

private:
	COptPage * pDocPage;        // corresp, document (edit) page
	CMFCButton ctl_doc_butn_;
};
#endif

/////////////////////////////////////////////////////////////////////////////
// CBackgroundPage dialog - background processing options

class CBackgroundPage : public COptPage
{
	DECLARE_DYNCREATE(CBackgroundPage)

// Construction
public:
	CBackgroundPage() : COptPage(IDD_OPT_BACKGROUND) { }

// Dialog Data

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
	afx_msg void OnChangeBackground();
	DECLARE_MESSAGE_MAP()

private:
	void fix_controls();
};

/////////////////////////////////////////////////////////////////////////////
// CAerialPage dialog - background processing options

class CAerialPage : public COptPage
{
	DECLARE_DYNCREATE(CAerialPage)

// Construction
public:
	CAerialPage() : COptPage(IDD_OPT_AERIAL) { }

// Dialog Data

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
	afx_msg void OnTemplatedir();
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CPrintGeneralPage dialog

class CPrintGeneralPage : public COptPage
{
	DECLARE_DYNCREATE(CPrintGeneralPage)

// Construction
public:
	CPrintGeneralPage() : COptPage(IDD_OPT_PRINT_GENERAL) { }

public:
	virtual void OnOK();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnChange();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnChangeUnits();
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CPrintDecorationsPage dialog

class CPrintDecorationsPage : public COptPage
{
	DECLARE_DYNCREATE(CPrintDecorationsPage)

public:
	CPrintDecorationsPage() : COptPage(IDD_OPT_PRINT_DECORATIONS) { }

// Dialog Data
	CEdit	ctl_header_;
	CEdit	ctl_first_header_;
	CEdit	ctl_footer_;
	CEdit	ctl_first_footer_;
	CEdit	ctl_watermark_;
	CMFCMenuButton	header_args_;
	CMFCMenuButton	first_header_args_;
	CMFCMenuButton	footer_args_;
	CMFCMenuButton	first_footer_args_;
	CMFCMenuButton	watermark_args_;

	CMenu args_menu_;

public:
	virtual void OnOK();
	LRESULT OnIdle(long);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnChange();
	afx_msg void OnChangeUpdate();
	afx_msg void OnHeaderOpts();
	afx_msg void OnFirstHeaderOpts();
	afx_msg void OnFooterOpts();
	afx_msg void OnFirstFooterOpts();
	afx_msg void OnWatermarkOpts();
	DECLARE_MESSAGE_MAP()

private:
	void fix_controls();        // Disable/enable controls depending on value/state of other controls
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
	CStringArray int_list,uint_list,real32_list,real64_list,real48_list,date_list,char_list,string_list;

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
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
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
	afx_msg void OnChange();

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
	~CWindowPage();

// Dialog Data
	enum { IDD = IDD_OPT_WINDISPLAY };

	void SetGlobalDisplayPage(COptPage * pPage)
	{
		pGlobalPage = pPage;
		if (m_hWnd != (HWND)0)
			GetDlgItem(IDC_GLOBAL_PAGE)->EnableWindow(pGlobalPage != NULL);
	}

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
	afx_msg void OnFontOem();
	afx_msg void OnFontUcode();
	afx_msg void OnChangeCols();
	afx_msg void OnSelchangeShowArea();
	afx_msg void OnSelchangeCharset();
	afx_msg void OnSelchangeCodePage();
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
	CMFCButton ctl_global_butn_;

	CComboBox ctl_code_page_;
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

	void SetGlobalEditPage(COptPage * pPage)
	{
		pGlobalPage = pPage;
		if (m_hWnd != (HWND)0)
			GetDlgItem(IDC_GLOBAL_PAGE)->EnableWindow(pGlobalPage != NULL);
	}

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
	CMFCButton ctl_global_butn_;
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
		INDEX_TRK, INDEX_COMP,
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
	CMFCColorButton	m_ColourPicker;
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

/////////////////////////////////////////////////////////////////////////////
// COptSheet

class COptSheet : public CMFCPropertySheet
{
	DECLARE_DYNAMIC(COptSheet)

// Construction
public:
	COptSheet(UINT nIDCaption, int display_page, BOOL must_show_page, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	COptSheet(LPCTSTR pszCaption, int display_page, BOOL must_show_page, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

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

	virtual BOOL OnInitDialog();

// Implementation
public:
	virtual ~COptSheet();

	struct OptValues val_;      // Property values that the user can change

	// Generated message map functions
protected:
	// These are the pages of the "property sheet"
	CSystemGeneralPage sysgeneralPage_;
	CFoldersPage foldersPage_;
	CPreviewPage previewPage_;
	CExplorerPage explorerPage_;
	CFiltersPage filtersPage_;
	CBackupPage backupPage_; 
	CPrintGeneralPage printGeneralPage_;
	CPrintDecorationsPage printDecorationsPage_;
	CMacroPage macroPage_;
	CHistoryPage histPage_;

	CWorkspaceLayoutPage workspacelayoutPage_;
	CWorkspaceDisplayPage workspacedisplayPage_;
	CWorkspaceEditPage workspaceeditPage_;
	CBackgroundPage backgroundPage_;
	CAerialPage aerialPage_;
	CTipsPage tipsPage_;
	CTemplatePage templatePage_;

	CWindowGeneralPage wingeneralPage_;
	CWindowPage windisplayPage_;
	CWindowEditPage wineditPage_;
	CColourSchemes coloursPage_;

	//{{AFX_MSG(COptSheet)
	afx_msg BOOL OnNcCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	DECLARE_MESSAGE_MAP()

private:
	CMFCPropertySheetCategoryInfo * pCatSys_;

	static int last_opt_page_;          // Index of last active options page

	int display_page_;                  // Page to display
	BOOL must_show_page_;               // Show display_page_ even if last_opt_page_ is valid

	void init(int display_page, BOOL must_show_page);
	void page_init();
};

#endif
