#if !defined(AFX_FINDDLG_H__3D8E0BF6_FE29_4214_B511_E080600F214F__INCLUDED_)
#define AFX_FINDDLG_H__3D8E0BF6_FE29_4214_B511_E080600F214F__INCLUDED_

// FindDlg.h : header file
//

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ResizeCtrl.h"

// Classes that handle each page (derived from CPropertyPage)
class CSimplePage;
class CHexPage;
class CTextPage;
class CNumberPage;
class CReplacePage;

// Handles edit control for accepting binary search string as hex digits (derived from CEdit)
class CHexEdit;

/////////////////////////////////////////////////////////////////////////////
// CFindSheet

class CFindSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CFindSheet)

// Construction
public:
//	CFindSheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
//	CFindSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CFindSheet(UINT iSelectPage = 0);

// Attributes
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFindSheet)
	//}}AFX_VIRTUAL

public:
	virtual BOOL Create(CWnd* pParentWnd, DWORD dwStyle);

// Implementation
public:
	virtual ~CFindSheet();

	// Generated message map functions
protected:
	afx_msg BOOL OnNcCreate(LPCREATESTRUCT lpCreateStruct);
	virtual BOOL OnInitDialog();
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	afx_msg void OnApplyNow();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
	afx_msg LRESULT OnResizePage(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

public:
	CSimplePage *p_page_simple_;
	CHexPage *p_page_hex_;
	CTextPage *p_page_text_;
	CNumberPage *p_page_number_;
	CReplacePage *p_page_replace_;

	CString combined_string_;           // string with hex digits or a string to search for
	CString hex_string_;                // string with hex digits to search for
	CString mask_string_;               // string of hex digits to indicate which bits to consider in a search
	CString text_string_;               // string to search for
	CString replace_string_;            // replacement hex digits used in find & replace
	CString number_string_;             // string containing a number (decimal integer or float) to search for

	CString bookmark_prefix_;           // Bookmarks are created using this prefix when using Bookmark All button
	BOOL prefix_entered_;               // If the user has eneterd a prefix don't auto-generate one

	// Some buffers that are allocated and returned
	unsigned char *search_buf_;         // Actual bytes to search for
	unsigned char *mask_buf_;           // Mask to use (or NULL if none)
	unsigned char *replace_buf_;        // Actual bytes to replace with

	BOOL whole_word_;                   // Only match whole words (text search) ie must have no alpha on either side
	BOOL match_case_;                   // If FALSE case is ignored (text search only)

	enum dirn_t { DIRN_UP, DIRN_DOWN }; // These match the order of direction radio buttons
	dirn_t direction_;
	enum scope_t { SCOPE_TOMARK, SCOPE_FILE, SCOPE_EOF, SCOPE_ALL }; // Matches the order of scope radio buttons
	scope_t scope_;

//    BOOL use_mask_;                     // indicates to use mask_string in hex searches

	BOOL wildcards_allowed_;            // are wildcards allowed in a text search
	CString wildcard_char_;             // Character to use as a wildcard (usually "?")
	enum charset_t { RB_CHARSET_UNKNOWN = -1, RB_CHARSET_ASCII = 0, RB_CHARSET_UNICODE, RB_CHARSET_EBCDIC }; // Matches order of char set radios in text search page
	charset_t charset_;

	// Number search options (used in number page only)
	BOOL big_endian_;                   // When searching for numbers indicates the byte order to use
	int number_format_;                 // Format of number to search for: 0 = unsigned int, 1 = signed int, 2 = IEEE float
	int number_size_;                   // (valid values depend on format) 0 = byte, 1 = word, 2 = dword, 3 = qword

	// Alignment options (only hex and number pages currently)
	UINT	align_;
	UINT	offset_;
	BOOL    relative_;                  // relative to current cursor position

	HWND help_hwnd_;                    // HWND of window for which context help is pending (usually 0)
	DWORD (*id_pairs_)[100];

// Operations
public:
	static enum string_t { STRING_UNKNOWN, STRING_TEXT, STRING_HEX } StringType(LPCTSTR ss);
	void ShowPage(int page);
	void Redisplay();
	void AddText(LPCTSTR txt);
	void AddHex(LPCTSTR txt);

	void NewSearch(LPCTSTR);
	void NewText(LPCTSTR);
	void NewHex(LPCTSTR);
	void NewReplace(LPCTSTR);

	void TextFromHex(LPCTSTR, LPCTSTR mask = NULL);
	void FixHexString();
	void HexFromText(const char *ss, size_t len = -1);
	void AdjustPrefix(LPCTSTR, BOOL underscore_space = FALSE); // Fix bookmark prefix (if not entered by user)

	// Encode (& decode) all options in a 64 bit int for storing in macros
	__int64 GetOptions();
	void SetOptions(__int64);

	void SetDirn(dirn_t);
	dirn_t GetDirn() const { GetActivePage()->UpdateData(); return direction_; }
	void SetScope(scope_t);
	scope_t GetScope() const { GetActivePage()->UpdateData(); return scope_; }
	void SetWholeWord(BOOL);
	BOOL GetWholeWord() const;
	void SetMatchCase(BOOL match);
	BOOL GetMatchCase() const;
	void SetWildcardsAllowed(BOOL);
	BOOL GetWildcardsAllowed() const;
	void SetCharSet(charset_t);
	charset_t GetCharSet() const;
	int GetAlignment();
	void SetAlignment(int aa);
	int GetOffset();                // Offset from alignment
	bool AlignRel();                // Align relative to current cursor position

	void GetSearch(const unsigned char **pps, const unsigned char **mask, size_t *plen);
	void GetReplace(unsigned char **pps, size_t *plen);
	bool HexReplace() const;

private:
	CRect m_rctPrev;         // previous window size
	CRect m_rctPage;         // current window size for child pages
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CSimplePage dialog

class CSimplePage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSimplePage)

// Construction
public:
	CSimplePage();
	~CSimplePage();

// Dialog Data
	//{{AFX_DATA(CSimplePage)
	enum { IDD = IDD_FIND_SIMPLE };
	CComboBox	ctl_string_;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSimplePage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSimplePage)
	virtual BOOL OnInitDialog();
	afx_msg void OnFindNext();
	afx_msg void OnHelp();
	afx_msg void OnChangeDirn();
	afx_msg void OnChangeString();
	afx_msg void OnSelchangeString();
	afx_msg void OnChangeMatchCase();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()

	CFindSheet *pparent_;
	void FixType(BOOL fix_strings = FALSE);
	void FixDirn();

private:
	CResizeCtrl resizer_;              // Used to move controls around when the window is resized
};

/////////////////////////////////////////////////////////////////////////////
// CHexPage dialog

class CHexPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CHexPage)

// Construction
public:
	CHexPage();
	~CHexPage();

// Dialog Data
	//{{AFX_DATA(CHexPage)
	enum { IDD = IDD_FIND_HEX };
	CEdit	ctl_bookmark_prefix_;
	CComboBox	ctl_hex_string_;
	//}}AFX_DATA
	CMFCMenuButton ctl_align_select_;
	bool update_ok_;            // Stop use of edit control before inited (spin ctrl problem)

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CHexPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
	virtual void OnCancel();
	virtual void OnOK();
	virtual BOOL OnKillActive();

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CHexPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnUseMask();
	afx_msg void OnChangeDirn();
	afx_msg void OnFindNext();
	afx_msg void OnHelp();
	afx_msg void OnChangeString();
	afx_msg void OnChangeMask();
	afx_msg void OnSelchangeString();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnBookmarkAll();
	afx_msg void OnChangePrefix();
	//}}AFX_MSG
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnClose();
	afx_msg void OnChangeAlign();
	afx_msg void OnAlignSelect();
	afx_msg void OnChangeOffset();
	DECLARE_MESSAGE_MAP()

	CFindSheet *pparent_;
	CHexEdit *phex_;
	CHexEdit *pmask_;
	CMenu button_menu_;         // We need to keep the menu (for the Alignment select menu button) in memory

	void FixDirn();
	void FixMask();
	void FixStrings();
	void FixAlign();

private:
	CResizeCtrl resizer_;              // Used to move controls around when the window is resized
};

/////////////////////////////////////////////////////////////////////////////
// CTextPage dialog

class CTextPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CTextPage)

// Construction
public:
	CTextPage();
	~CTextPage();

// Dialog Data
	//{{AFX_DATA(CTextPage)
	enum { IDD = IDD_FIND_TEXT };
	CEdit	ctl_bookmark_prefix_;
	CComboBox	ctl_text_string_;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CTextPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CTextPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnFindNext();
	afx_msg void OnAllowWildcard();
	afx_msg void OnChangeDirn();
	afx_msg void OnChangeType();
	afx_msg void OnHelp();
	afx_msg void OnChangeString();
	afx_msg void OnSelchangeString();
	afx_msg void OnChangeMatchCase();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnBookmarkAll();
	afx_msg void OnChangePrefix();
	//}}AFX_MSG
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()

	CFindSheet *pparent_;
	void FixDirn();
	void FixWildcard();
	void FixStrings();

private:
	CResizeCtrl resizer_;              // Used to move controls around when the window is resized
};

/////////////////////////////////////////////////////////////////////////////
// CNumberPage dialog

class CNumberPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CNumberPage)

// Construction
public:
	CNumberPage();
	~CNumberPage();

// Dialog Data
	//{{AFX_DATA(CNumberPage)
	enum { IDD = IDD_FIND_NUMBER };
	CEdit	ctl_bookmark_prefix_;
	CComboBox	ctl_number_size_;
	CEdit	ctl_number_string_;
	//}}AFX_DATA
	bool update_ok_;            // Stop use of edit control before inited (spin ctrl problem)

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNumberPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
	virtual BOOL OnKillActive();

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CNumberPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnFindNext();
	afx_msg void OnHelp();
	afx_msg void OnChangeDirn();
	afx_msg void OnChangeFormat();
	afx_msg void OnChangeString();
	afx_msg void OnChangeEndian();
	afx_msg void OnSelchangeSize();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnBookmarkAll();
	afx_msg void OnChangePrefix();
	//}}AFX_MSG
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnChangeAlign();
	afx_msg void OnChangeOffset();
	DECLARE_MESSAGE_MAP()

	CFindSheet *pparent_;
	void FixDirn();
	void FixSizes();
	BOOL FixStrings();
	void FixAlign();

private:
	CResizeCtrl resizer_;              // Used to move controls around when the window is resized
};

/////////////////////////////////////////////////////////////////////////////
// CReplacePage dialog

class CReplacePage : public CPropertyPage
{
	DECLARE_DYNCREATE(CReplacePage)

// Construction
public:
	CReplacePage();
	~CReplacePage();

// Dialog Data
	//{{AFX_DATA(CReplacePage)
	enum { IDD = IDD_FIND_REPLACE };
	CComboBox	ctl_string_;
	CComboBox	ctl_replace_string_;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CReplacePage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CReplacePage)
	virtual BOOL OnInitDialog();
	afx_msg void OnFindNext();
	afx_msg void OnReplace();
	afx_msg void OnReplaceAll();
	afx_msg void OnChangeDirn();
	afx_msg void OnHelp();
	afx_msg void OnChangeReplaceString();
	afx_msg void OnSelchangeReplaceString();
	afx_msg void OnChangeMatchCase();
	afx_msg void OnChangeString();
	afx_msg void OnSelchangeString();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()

	CFindSheet *pparent_;
	void FixType(BOOL fix_strings = FALSE);
	void FixDirn();

private:
	CResizeCtrl resizer_;              // Used to move controls around when the window is resized
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FINDDLG_H__3D8E0BF6_FE29_4214_B511_E080600F214F__INCLUDED_)
