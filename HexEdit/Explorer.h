// Explorer.h - header file for CExplorerWnd and related classes
//
// Copyright (c) 2004-2010 by Andrew W. Phillips.
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


#if !defined(EXPLORER_INCLUDED_)
#define EXPLORER_INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif

#include "HexEdit.h"

#include "SimpleSplitter.h"
#include "ResizeCtrl.h"

class CHexEditApp;
class CExplorerWnd;             // see below

/////////////////////////////////////////////////////////////////////////////
// CHistoryShellList - keeps a history of folders displayed
class CHistoryShellList : public CMFCShellListCtrl
{
	friend class CExplorerDropTarget;
    friend class CExplorerWnd;
	// This replaces BCGShellListColumn* (Afx_ShellListColumn*)
	enum
	{
		COLNAME, COLSIZE, COLTYPE, COLMOD,		// Normal columns provided by BCG
        COLATTR,								// file attributes
        COLOPENED,								// time last opened in hexedit
		COLCATEGORY, COLKEYWORDS, COLCOMMENTS,	// Extra info on file entered by user (Summary page)
		COLLAST         // Leave this one at the end (to be one past last used value)
	};

public:
    CHistoryShellList() : pExpl_(NULL), pos_(-1), in_move_(false), add_to_hist_(true), m_pDropTarget (NULL) { }
	void Start(CExplorerWnd *pExpl) { pExpl_ = pExpl; }

    // Handles back and forward buttons
	void Back(int count = 1);
	void Forw(int count = 1);
	bool BackAllowed() { return pos_ > 0; }
	bool ForwAllowed() { return pos_ < int(name_.size()) - 1; }

    void SaveLayout();                                                          // save layout to registry (on close)

	virtual HRESULT DisplayFolder(LPAFX_SHELLITEMINFO lpItemInfo);
	virtual HRESULT DisplayFolder(LPCTSTR lpszPath) { return CMFCShellListCtrl::DisplayFolder(lpszPath); }
	virtual void OnSetColumns();                                                // sets up columns (in detail view mode)
	virtual CString OnGetItemText(int iItem, int iColumn, LPAFX_SHELLITEMINFO pItem); // used in displaying text in columns
	virtual int OnCompareItems(LPARAM lParam1, LPARAM lParam2, int iColumn);    // used in sorting on a column

	CString Folder() { if (pos_ > -1 && pos_ < int(name_.size())) return name_[pos_]; else return CString(); }

    virtual void AdjustMenu(HMENU);
    virtual void MenuCommand(HMENU, UINT, LPCTSTR);

    void SetFilter(LPCTSTR ff) { m_filter = CString(ff); }
    CString GetFilter() { return m_filter; }

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDblClk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()

	// Overrides
	virtual HRESULT EnumObjects(LPSHELLFOLDER pParentFolder, LPITEMIDLIST pidlParent);

private:
    void do_move(int ii);

	CExplorerWnd *pExpl_;
    std::vector<CString> name_;  // Names of folders
    int pos_;                    // Index of current folder
    bool in_move_;               // Used to prevent storing moves when just going forward/backward in the list
    bool add_to_hist_;           // Used to prevent adding to dop-down list in sme circumstances (default to true)
    int normal_count_;           // No of columns added in base class (CMFCShellListCtrl)
	int fl_idx_;                 // This is saved index into recent file list (cached for list box row while processing fields)
    static char * defaultWidths;

	CExplorerDropTarget * m_pDropTarget;

    CString m_filter;            // Use for filtering displayed file names
};

/////////////////////////////////////////////////////////////////////////////
// CExplorerDropTarget
class CExplorerDropTarget : public COleDropTarget
{
public:
	CExplorerDropTarget(CHistoryShellList* pWnd);
	virtual ~CExplorerDropTarget();

    DROPEFFECT OnDragEnter ( CWnd* pWnd, COleDataObject* pDataObject,
                             DWORD dwKeyState, CPoint point );

    DROPEFFECT OnDragOver ( CWnd* pWnd, COleDataObject* pDataObject,
                            DWORD dwKeyState, CPoint point );

    BOOL OnDrop ( CWnd* pWnd, COleDataObject* pDataObject,
                  DROPEFFECT dropEffect, CPoint point );

    void OnDragLeave ( CWnd* pWnd );

private:
	CString GetDst(CPoint point);
	size_t GetNames(COleDataObject* pDataObject, std::vector<CString> & retval);
	DROPEFFECT DragCheck(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	DROPEFFECT CanMove(LPCTSTR file, LPCTSTR dst);
	DROPEFFECT CanMoveFolder(LPCTSTR folder, LPCTSTR dst);
    CHistoryShellList * m_pParent;
    IDropTargetHelper * m_pHelper;
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
// CExplorerWnd - modeless dialog bar that contains 2 panes with CMFCShellTreeCtrl + CMFCShellListCtrl
class CExplorerWnd : public CDialog
{
public:
    CExplorerWnd() : splitter_(2), hh_(0), update_required_(false), help_hwnd_(0), init_(false), m_first(true) { }
	enum { IDD = IDD_EXPLORER };
	virtual BOOL Create(CWnd* pParentWnd);

	void UpdateFolderInfo(CString folder);   // called when current folder changes
	void Refresh();                 // Set folder/filter from edit controls and refresh the folder display
	void Update(LPCTSTR file_name = NULL); // Mark for possible later (idle) refresh
	void NewFilter();               // New filter name entered
    void AddFilter();               // Add current filter to the history list
	void OldFilter();               // Restore old filter name (Esc hit).
	void NewFolder();               // New folder name entered
	void OldFolder();               // Restore old folder name (Esc hit).
    void AddFolder();               // Add current folder to history list

	// overrides
//	virtual BOOL PreTranslateMessage(MSG* pMsg);

	CMFCButton ctl_back_;           // back (undo)
	CMFCButton ctl_forw_;           // forward (redo)
	CMFCButton ctl_up_;             // up to parent folder
	CMFCButton ctl_refresh_;        // redraws the list
    CMFCMenuButton ctl_view_;       // shows menu of 4 items allowing user to say how the view is to be displayed
    CMFCButton ctl_flip_;           // determines orientation of tree/list in splitter window

    CMFCMenuButton ctl_filter_opts_;// Shows menu with filter list

    CComboBox ctl_filter_;          // restricts files displayed in list
	CFilterEdit ctl_filter_edit_;   // The edit control within the combo
    CComboBox ctl_name_;            // user can enter folder name here or select from history list
	CFolderEdit ctl_name_edit_;     // The edit control within the combo

	CMenu m_menu_;                  // menus for explorer button(s) (ctl_view_)

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
	afx_msg void OnFolderFlip();
	afx_msg void OnSelchangeFolderName();
	afx_msg void OnSelchangeFilter();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);

	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

	DECLARE_MESSAGE_MAP()

	CSimpleSplitter   splitter_;        // Contains the tree and folder view with a splitter in between
	CMFCShellTreeCtrl	  tree_;            // Tree view linked to folder view (list_)
	CHistoryShellList list_;            // Our class derived from CMFCShellListCtrl

	HWND help_hwnd_;                    // HWND of window for which context help is pending (usually 0)

private:
    void build_filter_menu();

	bool init_;
    CString filters_;                   // Current filter string (from last GetCurrentFilters) as in filter menu
    //HICON arrow_icon_;
	CString curr_name_, curr_filter_;   // Current values in edit boxes of combos (current folder name and filters)
	HANDLE hh_;                         // Used to monitor directory changes (returned from FindFirstChangeNotification)
	bool update_required_;              // Keeps track of consecutive updates to prevent unnecessary refreshes

	bool m_first;                   // Remember first call to OnKickIdle (we can't add the controls to the resizer till then)
	CResizeCtrl m_resizer;          // Used to move controls around when the window is resized
	static CBrush * m_pBrush;       // brush used for background
	static COLORREF m_col;          // colour used for background
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(EXPLORER_INCLUDED_)
