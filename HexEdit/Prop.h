// Prop.h : header file for Properties tabbed dialog box
//

// Copyright (c) 1999-2010 by Andrew W. Phillips.
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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxdisp.h>

class CHexEditView;
class CSimpleGraph;

#ifdef SUBCLASS_UNICODE_CONTROL
/////////////////////////////////////////////////////////////////////////////
// CUnicodeControl

// Used to display Unicode characters.

class CUnicodeControl : public CEdit
{
protected:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	DECLARE_MESSAGE_MAP()
};
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropEditControl

// Used to subclass edit controls that can be used to modify values of the
// active view.  The only reason we do this is so that we can accept
// WM_GETDLGCODE messages (via OnGetDlgCode) and return DLGC_WANTALLKEYS.

class CPropEditControl : public CEdit
{
protected:
	afx_msg UINT OnGetDlgCode();
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CCommentEditControl

// Used to subclass edit controls that are used for comment fields.
// We do this to exclude entry of vertical bar (|) and so we can accept
// WM_GETDLGCODE messages (via OnGetDlgCode) and return DLGC_WANTALLKEYS.

class CCommentEditControl : public CEdit
{
protected:
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg UINT OnGetDlgCode();
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CBinEditControl window

class CBinEditControl : public CEdit
{
// Construction
public:
	CBinEditControl();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBinEditControl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CBinEditControl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CBinEditControl)
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg UINT OnGetDlgCode();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CPropDecEditControl - handles thousands separator when editing

class CPropDecEditControl : public CEdit
{
// Construction
public:
	CPropDecEditControl();

public:
	void add_commas();
	char sep_char_;		// Char to use as "thousands" separator
	int group_;			// Number of digits in "thousands" group
	bool allow_neg_;    // Leading minus sign allowed

protected:
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg UINT OnGetDlgCode();
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CPropUpdatePage

// CPropUpdatePage is used as the base class for all property pages used
// in the "Properties" property sheet CPropSheet.  It is provided simply
// so that we can use the virtual function "Update" with a pointer
// obtained with GetActivePage() without regard to the particular derived
// class of the currently active page.

class CPropUpdatePage : public CPropertyPage
{
protected:
	CPropUpdatePage(int id) : CPropertyPage(id) { }
	DECLARE_DYNAMIC(CPropUpdatePage)
public:
	virtual void Update(CHexEditView *pv = NULL, FILE_ADDRESS address = -1) =0;
protected:
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CPropFilePage dialog

class CPropFilePage : public CPropUpdatePage
{
	DECLARE_DYNCREATE(CPropFilePage)

// Construction
public:
	CPropFilePage();
	~CPropFilePage();

// Dialog Data
	//{{AFX_DATA(CPropFilePage)
	enum { IDD = IDD_PROP_FILE };
	CString	file_name_;
	CString	file_path_;
	int		file_hidden_;
	int		file_readonly_;
	int		file_system_;
	CString	file_modified_;
	CString	file_type_;
	CString	file_created_;
	CString	file_accessed_;
	int		file_archived_;
	//}}AFX_DATA
	CString file_size_;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPropFilePage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
	virtual void Update(CHexEditView *pv = NULL, FILE_ADDRESS address = -1);
	virtual BOOL OnInitDialog();

protected:
	// Generated message map functions
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	DECLARE_MESSAGE_MAP()

private:
	CResizeCtrl resizer_;              // Used to move controls around when the window is resized
};

/////////////////////////////////////////////////////////////////////////////
// CPropInfoPage dialog

class CPropInfoPage : public CPropUpdatePage
{
	DECLARE_DYNCREATE(CPropInfoPage)

// Construction
public:
	CPropInfoPage();
	~CPropInfoPage();

// Dialog Data
	enum { IDD = IDD_PROP_INFO };
	CString category_;
	CString keywords_;
	CString comments_;
	//CString	disk_size_;
	CString view_time_;
	CString edit_time_;

	// Subclassed controls
	CCommentEditControl category_ctl_;
	CCommentEditControl keywords_ctl_;
	CCommentEditControl comments_ctl_;
	CMFCMenuButton cat_sel_ctl_;

	bool category_changed_;
	bool keywords_changed_;
	bool comments_changed_;

// Overrides
public:
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
	virtual void Update(CHexEditView *pv = NULL, FILE_ADDRESS address = -1);
	virtual BOOL OnInitDialog();

protected:
	// Message map functions
	afx_msg void OnChangeCategory();
	afx_msg void OnChangeKeywords();
	afx_msg void OnChangeComments();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnKillFocusCategory();
	afx_msg void OnKillFocusKeywords();
	afx_msg void OnKillFocusComments();
	afx_msg void OnSelCategory();
	DECLARE_MESSAGE_MAP()

private:
	CResizeCtrl resizer_;              // Used to move controls around when the window is resized
};

/////////////////////////////////////////////////////////////////////////////
// CPropCharPage dialog

class CPropCharPage : public CPropUpdatePage
{
	DECLARE_DYNCREATE(CPropCharPage)

// Construction
public:
	CPropCharPage();
	~CPropCharPage();

// Dialog Data
	//{{AFX_DATA(CPropCharPage)
	enum { IDD = IDD_PROP_CHAR };
	CString	char_ascii_;
	CString	char_binary_;
	CString	char_ebcdic_;
	CString	char_hex_;
	CString	char_octal_;
	CString	char_dec_;
	//}}AFX_DATA
	int active_code_page_;      // System's active code page (used for displaying ANSI chars)
	bool is_dbcs_;              // Is system active code page DBCS (ie has lead-in bytes)?
#ifdef SHOW_CODE_PAGE
	CComboBox ctl_code_page_;
	int code_page_;             // Current code page used to display multibyte characters
#endif

	// Stuff for displaying a Unicode character
	HFONT ufont_, nfont_;       // Font to display Unicode char & font to display info
	LOGFONT lf_;                // Used for creating and selecting a Unicode font

	// Subclassed controls so we can see CR/Esc keys when user changes values
	CPropEditControl edit_dec_;
	CPropEditControl edit_octal_;
	CBinEditControl edit_binary_;

#ifdef SUBCLASS_UNICODE_CONTROL
	CUnicodeControl ctl_unicode_;
#endif

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPropCharPage)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
	virtual void Update(CHexEditView *pv = NULL, FILE_ADDRESS address = -1);

protected:
	// Generated message map functions
	//{{AFX_MSG(CPropCharPage)
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	//}}AFX_MSG
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
#ifdef SHOW_CODE_PAGE
	afx_msg void OnSelchangeCodePage();
#endif
	DECLARE_MESSAGE_MAP()

	HWND CreateUnicodeControl(int holder_id, int new_id);

public:
	afx_msg void OnBnClickedSelectFont();

private:
	CResizeCtrl resizer_;              // Used to move controls around when the window is resized
};

/////////////////////////////////////////////////////////////////////////////
// CPropDecPage dialog

class CPropDecPage : public CPropUpdatePage
{
	DECLARE_DYNCREATE(CPropDecPage)

// Construction
public:
	CPropDecPage();
	~CPropDecPage();

// Dialog Data
	//{{AFX_DATA(CPropDecPage)
	enum { IDD = IDD_PROP_DECIMAL };
	CString	dec_64bit_;
	CString	dec_16bit_;
	CString	dec_32bit_;
	CString	dec_8bit_;
	int		signed_;
	BOOL	big_endian_;
	//}}AFX_DATA

	CPropEditControl edit_8bit_;
	CPropDecEditControl edit_16bit_;
	CPropDecEditControl edit_32bit_;
	CPropDecEditControl edit_64bit_;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPropDecPage)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
	virtual void Update(CHexEditView *pv = NULL, FILE_ADDRESS address = -1);

protected:
	// Generated message map functions
	//{{AFX_MSG(CPropDecPage)
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnChangeFormat();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()

private:
	CResizeCtrl resizer_;              // Used to move controls around when the window is resized
};

/////////////////////////////////////////////////////////////////////////////
// CPropRealPage dialog

class CPropRealPage : public CPropUpdatePage
{
	DECLARE_DYNCREATE(CPropRealPage)

// Construction
public:
	CPropRealPage();
	~CPropRealPage();

// Dialog Data
	enum { IDD = IDD_PROP_REAL };
	CString	val_;
	CString	mant_;
	CString	exp_;
	CString desc_, exp_desc_;    // static text
	BOOL	big_endian_;
	int		format_;

	// Controls
	CPropEditControl ctl_val_;   // edit box
	CWnd ctl_group_;             // group box

// Overrides
public:
	virtual BOOL OnSetActive();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
	virtual void Update(CHexEditView *pv = NULL, FILE_ADDRESS address = -1);

protected:
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnChangeFormat();
	virtual BOOL OnInitDialog();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()

	// This relates format types to Combo Box elements
	enum { FMT_IEEE32, FMT_IEEE64, FMT_IBM32, FMT_IBM64, FMT_REAL48, FMT_DECIMAL,
		   FMT_LAST };
	void FixDesc() ;

private:
	CResizeCtrl resizer_;              // Used to move controls around when the window is resized
};

/////////////////////////////////////////////////////////////////////////////
// CPropDatePage dialog

class CPropDatePage : public CPropUpdatePage
{
	DECLARE_DYNCREATE(CPropDatePage)

// Construction
public:
	CPropDatePage();
	~CPropDatePage();

// Dialog Data
	//{{AFX_DATA(CPropDatePage)
	enum { IDD = IDD_PROP_DATE };
	CDateTimeCtrl	time_ctrl_;
	CDateTimeCtrl	date_ctrl_;
	int		format_;
	CString	format_desc_;
	CString	date_display_;
	BOOL	big_endian_;
	BOOL	local_time_;
	//}}AFX_DATA

	// These enum values need to be kept in sync with the entries in the 
	// IDC_DATE_FORMAT drop list combo and the date_size array.
	enum {
		FORMAT_TIME_T,          // Most common time_t (secs since 1/1/70)
		FORMAT_TIME64_T,        // Common (64-bit) time64_t (secs since 1/1/70)
		FORMAT_TIME_T_80,       // Seconds since 1/1/80 as 32 bit int
		FORMAT_TIME_T_1899,     // Seconds since 31/12/1899 as unsigned 32 bit int
		FORMAT_TIME_T_MINS,     // Minutes since 1/1/70
		FORMAT_OLE,             // DATE = days since 31/12/1899 as double
		FORMAT_SYSTEMTIME,      // structure wYear -> wMilliseconds
		FORMAT_FILETIME,        // nanoseconds since 1/1/1601 as 64 bit int
		FORMAT_MSDOS,           // two 16 bit values with encoded bits: hhhhhmmmmmmsssss yyyyyyymmmmddddd
								// yyyyyyy is years from 1980, sssss is in 2 second increments

		FORMAT_LAST
	};

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPropDatePage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
	virtual void Update(CHexEditView *pv = NULL, FILE_ADDRESS address = -1);

protected:
	// Generated message map functions
	//{{AFX_MSG(CPropDatePage)
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnDateFirst();
	afx_msg void OnDateLast();
	afx_msg void OnDateNow();
	afx_msg void OnDateNull();
	afx_msg void OnDateChange(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTimeChange(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChangeFormat();
	//}}AFX_MSG
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()

private:
	void save_date();
	size_t date_size_[FORMAT_LAST];
	COleDateTime date_first_[FORMAT_LAST];
	COleDateTime date_last_[FORMAT_LAST];
	bool stop_update_;

private:
	CResizeCtrl resizer_;              // Used to move controls around when the window is resized
};

/////////////////////////////////////////////////////////////////////////////
// CPropGraphPage dialog

class CPropGraphPage : public CPropUpdatePage
{
	DECLARE_DYNCREATE(CPropGraphPage)

// Construction
public:
	CPropGraphPage();
	~CPropGraphPage();

// Dialog Data
	enum { IDD = IDD_PROP_GRAPH };

// Overrides
public:
	virtual BOOL OnSetActive();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
	virtual void Update(CHexEditView *pv = NULL, FILE_ADDRESS address = -1);

protected:
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnStatsOptions();

	DECLARE_MESSAGE_MAP()

private:
	CSimpleGraph *m_graph;
};

/////////////////////////////////////////////////////////////////////////////
// CPropStatsPage dialog

class CPropStatsPage : public CPropUpdatePage
{
	DECLARE_DYNCREATE(CPropStatsPage)

// Construction
public:
	CPropStatsPage();
	~CPropStatsPage();

// Dialog Data
	enum { IDD = IDD_PROP_STATS };

// Overrides
public:
	virtual BOOL OnSetActive();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
	virtual void Update(CHexEditView *pv = NULL, FILE_ADDRESS address = -1);

protected:
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);

	DECLARE_MESSAGE_MAP()

private:
	CResizeCtrl resizer_;              // Used to move controls around when the window is resized
};
/////////////////////////////////////////////////////////////////////////////
// CPropSheet

#define WM_RESIZEPAGE (WM_USER+111)

class CPropSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CPropSheet)

// Construction
public:
	CPropSheet();

// Attributes
public:
	HWND help_hwnd_;                    // HWND of window for which context help is pending (usually 0)

// Operations
public:
	void Update(CHexEditView *pv = NULL, FILE_ADDRESS address = -1); // Update active page

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPropSheet)
	protected:
	//}}AFX_VIRTUAL

public:
	virtual BOOL Create(CWnd* pParentWnd, DWORD dwStyle);

// Implementation
public:
	virtual ~CPropSheet();

	// Generated message map functions
protected:
	virtual BOOL OnInitDialog();
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);

	afx_msg BOOL OnNcCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
	afx_msg LRESULT OnResizePage(WPARAM wParam, LPARAM lParam);
	afx_msg void OnApplyNow();

	DECLARE_MESSAGE_MAP()

public:
	CPropInfoPage     prop_info;
	CPropFilePage     prop_file;
	CPropCharPage     prop_char;
	CPropDecPage      prop_dec;
	CPropRealPage     prop_real;
	CPropDatePage     prop_date;
	CPropGraphPage    prop_graph;
	CPropStatsPage    prop_stats;

private:
	CRect m_rctPrev;         // previous window size
	CRect m_rctPage;         // current window size for child pages
};
