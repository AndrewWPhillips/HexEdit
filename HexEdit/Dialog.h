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

#if !defined(AFX_DIALOG_H__55A0CEE1_3245_11D2_B012_0020AFDC3196__INCLUDED_)
#define AFX_DIALOG_H__55A0CEE1_3245_11D2_B012_0020AFDC3196__INCLUDED_

#include "HexEdit.h"
#include <vector>

#include "SimpleSplitter.h"
#include "ResizeCtrl.h"

#ifndef DIALOG_BAR // Modeless property sheets are control bars
#define ModelessDialogBaseClass  CDialog
#else
#define ModelessDialogBaseClass  CHexDialogBar
#endif

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Dialog.h : header file
//

#include <Dlgs.h>               // For file dialog control IDs

class CHexEditApp;

/////////////////////////////////////////////////////////////////////////////
// CHexDialogBar - dialog bar that supports DDX and OnInitDialog
class CHexDialogBar : public CBCGDialogBar 
{
	DECLARE_DYNAMIC(CHexDialogBar)

// Construction
public:
    CHexDialogBar() { }
	virtual ~CHexDialogBar() { }
	BOOL Create(UINT nID, CWnd* pParentWnd, UINT nStyle = CBRS_LEFT )
	{
		return CBCGDialogBar::Create(pParentWnd, nID, nStyle, nID);
	}
	void Unroll();
    void FixAndFloat(BOOL show = FALSE);

	virtual BOOL OnInitDialog() { return FALSE; }
	// This stops buttons being disabled because they have no command handler
	virtual void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler) { }

protected:

	//afx_msg void OnInitialUpdate();
	afx_msg LRESULT InitDialogBarHandler(WPARAM, LPARAM);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	DECLARE_MESSAGE_MAP()
};

#ifdef EXPLORER_WND
class CExplorerWnd;

/////////////////////////////////////////////////////////////////////////////
// CHistoryShellList - keeps a history of folders displayed
class CHistoryShellList : public CBCGShellList
{
	// This is a continuation of BCGShellListColumns
	enum
	{
		ColumnFirst = 4,   // Must be the same number as the next one
		ColumnAttr = 4,    // File attributes
		ColumnOpened = 5,  // Time last opened in HexEdit
		ColumnLast         // Leave this one at the end (to be one past last used value)
	};
public:
    CHistoryShellList() : pExpl_(NULL), pos_(-1), in_move_(false) { }
	void Start(CExplorerWnd *pExpl) { pExpl_ = pExpl; }

	void Back(int count = 1);
	void Forw(int count = 1);
	bool BackAllowed() { return pos_ > 0; }
	bool ForwAllowed() { return pos_ < int(name_.size()) - 1; }

   	virtual HRESULT DisplayFolder(LPBCGCBITEMINFO lpItemInfo);
	virtual HRESULT DisplayFolder(LPCTSTR lpszPath) { return CBCGShellList::DisplayFolder(lpszPath); }
	virtual void OnSetColumns();                                                   // sets up columns (in detail view mode)
	virtual CString OnGetItemText(int iItem, int iColumn, LPBCGCBITEMINFO pItem);  // used in displaying text in columns
	virtual int OnCompareItems(LPARAM lParam1, LPARAM lParam2, int iColumn);       // used in sorting on a column

	CString Folder() { ASSERT(pos_ > -1 && pos_ < int(name_.size())); return name_[pos_]; }

    virtual void AdjustMenu(HMENU);
    virtual void MenuCommand(HMENU, UINT, LPCTSTR);

protected:
	afx_msg void OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()

private:
    void CHistoryShellList::do_move(int ii);

	CExplorerWnd *pExpl_;
    std::vector<CString> name_;  // Names of folders
    int pos_;                    // Index of current folder
    bool in_move_;               // Used to prevent storing moves when just going forward/backward in the list
    int normal_count_;           // No of columns added in base class (CBCGSHellList)
	int fl_idx_;                 // This is saved index into recent file list (cached for list box row while processing fields)
};

/////////////////////////////////////////////////////////////////////////////
// CFilterEdit
class CFilterEdit : public CEdit
{
public:
	CFilterEdit() : pExpl_(NULL) { }
	void Start(CExplorerWnd *pExpl) { pExpl_ = pExpl; }

protected:
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg UINT OnGetDlgCode();
    DECLARE_MESSAGE_MAP()

private:
	CExplorerWnd *pExpl_;
};

/////////////////////////////////////////////////////////////////////////////
// CFolderEdit
class CFolderEdit : public CEdit
{
public:
	CFolderEdit() : pExpl_(NULL) { }
	void Start(CExplorerWnd *pExpl) { pExpl_ = pExpl; }

protected:
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg UINT OnGetDlgCode();
    DECLARE_MESSAGE_MAP()

private:
	CExplorerWnd *pExpl_;
};

/////////////////////////////////////////////////////////////////////////////
// CExplorerWnd - modeless dialog bar that contains 2 panes with CBCGShellTree + CBCGShellList
class CExplorerWnd : public CHexDialogBar
{
public:
    CExplorerWnd() : splitter_(2), hh_(0), update_required_(false) { }
	enum { IDD = IDD_EXPLORER };
	virtual BOOL Create(CWnd* pParentWnd);
	virtual BOOL OnInitDialog();

	void UpdateFolderInfo(CString folder);   // called when current folder changes
	void Refresh();             // Set folder/filter from edit controls and refresh the folder display
	void Update(LPCTSTR file_name = NULL); // Mark for possible later (idle) refresh
	void NewFilter();           // New filter name entered
	void OldFilter();           // Restore old filter name (Esc hit).
	void NewFolder();           // New folder name entered
	void OldFolder();           // Restore old folder name (Esc hit).

	// overrides
//	virtual BOOL PreTranslateMessage(MSG* pMsg);

	CBCGButton ctl_back_;      // back (undo)
	CBCGButton ctl_forw_;      // forward (redo)
	CBCGButton ctl_up_;        // up to parent folder
	CBCGButton ctl_refresh_;   // redraws the list
	//CBCGButton ctl_showall_;   // show hidden and system files too
    CBCGMenuButton ctl_view_;  // shows menu of 4 items allowing user to say how the view is to be displayed
    CBCGButton ctl_show_all_;  // Show hidden/system files
    CBCGButton ctl_flip_;      // determines orientation of tree/list in splitter window

    CBCGMenuButton ctl_filter_opts_;  // Shows menu with filter list

    CComboBox ctl_filter_;     // restricts files displayed in list
	CFilterEdit ctl_filter_edit_; // The edit control within the combo
    CComboBox ctl_name_;       // user can enter folder name here or select from history list
	CFolderEdit ctl_name_edit_; // The edit control within the combo

	CMenu m_menu;              // menu for ctl_view_

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
    //afx_msg void OnSize(UINT nType,int cx,int cy);
	afx_msg void OnDestroy();
    afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
	afx_msg void OnFolderBack();
	afx_msg void OnFolderForw();
	afx_msg void OnFolderParent();
	afx_msg void OnFilterOpts();
	afx_msg void OnFolderRefresh();
	afx_msg void OnFolderView();
	afx_msg void OnFolderHidden();
	afx_msg void OnFolderFlip();
	afx_msg void OnSelchangeFolderName();
	afx_msg void OnSelchangeFilter();
	DECLARE_MESSAGE_MAP()

	CSimpleSplitter   splitter_;        // Contains the tree and folder view with a splitter in between
	CBCGShellTree	  tree_;            // Tree view linked to folder view (list_)
	CHistoryShellList list_;            // Our class derived from CBCGShellFolder

	CResizeCtrl resizer_;               // Used to move controls around when the window is resized

private:
    void build_filter_menu();
    CString filters_;                   // Current filter string (from last GetCurrentFilters) as in filter menu
    HICON arrow_icon_;
	CString curr_name_, curr_filter_;   // Current values in edit boxes of combos (current folder name and filters)
	HANDLE hh_;                         // Used to monitor directory changes (returned from FindFirstChangeNotification)
	bool update_required_;              // Keeps track of consecutive updates to prevent unnecessary refreshes
};
#endif // EXPLORER_WND

/////////////////////////////////////////////////////////////////////////////
// CHexFileDialog - just adds the facility to CFileDialog for saving and restoring
// the window size and position.
class CHexFileDialog : public CFileDialog
{
public:
    CHexFileDialog(LPCTSTR name,
                   BOOL bOpenFileDialog,
                   LPCTSTR lpszDefExt = NULL,
                   LPCTSTR lpszFileName = NULL,
                   DWORD dwFlags = OFN_HIDEREADONLY  | OFN_OVERWRITEPROMPT,
                   LPCTSTR lpszFilter = NULL,
                   CWnd* pParentWnd = NULL) : 
      	CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags | OFN_ENABLESIZING, lpszFilter, pParentWnd),
        strName(name)
    {
        first_time = true;
    }

    // Overriden members of CFileDialog
    virtual void OnInitDone()
    {
        CFileDialog::OnInitDone();
    }
	virtual BOOL OnFileNameOK()
    {
        CRect rr;
        GetParent()->GetWindowRect(&rr);
        theApp.WriteProfileInt("Window-Settings", strName+"X1", rr.left);
        theApp.WriteProfileInt("Window-Settings", strName+"Y1", rr.top);
        theApp.WriteProfileInt("Window-Settings", strName+"X2", rr.right);
        theApp.WriteProfileInt("Window-Settings", strName+"Y2", rr.bottom);

        return CFileDialog::OnFileNameOK();
    }
    virtual void OnFileNameChange()
    {
        if (first_time)
        {
            // We put this stuff here as it appears to be too early when just in OnInitDone

            // Restore the window position and size
            CRect rr(theApp.GetProfileInt("Window-Settings", strName+"X1", -30000),
                     theApp.GetProfileInt("Window-Settings", strName+"Y1", -30000),
                     theApp.GetProfileInt("Window-Settings", strName+"X2", -30000),
                     theApp.GetProfileInt("Window-Settings", strName+"Y2", -30000));
            if (rr.top != -30000)
                GetParent()->MoveWindow(&rr);

            first_time = false;
        }

        CFileDialog::OnFileNameChange();
    }
private:
    CString strName;                    // Registry entries are based on this name
    bool first_time;                    // Only restore window on first call of OnFileNameChange
};

// CImportDialog - derived from CFileDialog to support extra controls (see theApp.open_file_shared_)
class CImportDialog : public CHexFileDialog
{
public:
    CImportDialog(LPCTSTR lpszFileName = NULL,
                  DWORD dwFlags = OFN_HIDEREADONLY | OFN_SHOWHELP | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_DONTADDTORECENT,
                  LPCTSTR lpszFilter = NULL,
                  CWnd* pParentWnd = NULL) : 
      	CHexFileDialog("FileImportDlg", TRUE, NULL, lpszFileName, dwFlags, lpszFilter, pParentWnd)
    {
    }

    // Overriden members of CFileDialog
    virtual void OnInitDone();
	virtual BOOL OnFileNameOK();

private:
    CButton m_discon;                   // Check box for discontiguous addresses
    CButton m_highlight;                // Check box for highlight of imported records
};

// CFileOpenDialog - derived from CFileDialog to support open shared control
class CFileOpenDialog : public CHexFileDialog
{
public:
    CFileOpenDialog(LPCTSTR lpszFileName = NULL,
                    DWORD dwFlags = OFN_SHOWHELP | OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR,
                    LPCTSTR lpszFilter = NULL,
                    CWnd* pParentWnd = NULL) : 
      	CHexFileDialog("FileOpenDlg", TRUE, NULL, lpszFileName, dwFlags, lpszFilter, pParentWnd)
    {
    }

    // Overriden members of CFileDialog
    virtual void OnInitDone();
	virtual BOOL OnFileNameOK();

private:
    CButton m_open_shared;
};


/////////////////////////////////////////////////////////////////////////////
// CMultiplay dialog

class CMultiplay : public CDialog
{
// Construction
public:
	CMultiplay(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMultiplay)
	enum { IDD = IDD_MULTIPLAY };
	CComboBox	name_ctrl_;
	long	plays_;
	//}}AFX_DATA
        CString macro_name_;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMultiplay)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMultiplay)
	afx_msg void OnPlayOptions();
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnSelchangePlayName();
	virtual void OnOK();
	afx_msg void OnHelp();
	//}}AFX_MSG
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()

    void FixControls() ;
};

/////////////////////////////////////////////////////////////////////////////
// CSaveMacro dialog

class CSaveMacro : public CDialog
{
// Construction
public:
	CSaveMacro(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSaveMacro)
	enum { IDD = IDD_MACROSAVE };
	long	plays_;
	CString	name_;
	CString	comment_;
	int		halt_level_;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSaveMacro)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSaveMacro)
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	virtual void OnOK();
	afx_msg void OnMacroHelp();
	//}}AFX_MSG
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()

    CHexEditApp *aa_;                       // Ptr to the app (mainly for macro handling)
};
/////////////////////////////////////////////////////////////////////////////
// CMacroMessage dialog

class CMacroMessage : public CDialog
{
// Construction
public:
	CMacroMessage(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMacroMessage)
	enum { IDD = IDD_MACRO_MESSAGE };
	CString	message_;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMacroMessage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMacroMessage)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_H__55A0CEE1_3245_11D2_B012_0020AFDC3196__INCLUDED_)
