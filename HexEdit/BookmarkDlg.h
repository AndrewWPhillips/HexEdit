// BookmarkDlg.h : header file for Bookmarks Dialog
//
// Copyright (c) 2015 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#if !defined(AFX_BOOKMARKDLG_H__B31E7B1A_8652_4D8E_B526_3786950F743B__INCLUDED_)
#define AFX_BOOKMARKDLG_H__B31E7B1A_8652_4D8E_B526_3786950F743B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ResizeCtrl.h"
#include "range_set.h"
#include "GridCtrl_src/GridCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// CBookmarkDlg dialog

class CBookmarkDlg : public CDialog
{
public:
	// Number the different types of columns we can display
	enum
	{
		COL_NAME,                           // Name of the bookmark
		COL_FILE,                           // File name
		COL_LOCN,                           // Location (path)
		COL_POS,                            // Byte position within the file
		COL_MODIFIED,                       // Time the bookmark was last modified
		COL_ACCESSED,                       // Time the bookmark was last jumped to
		COL_LAST                            // leave at end (signals end of list)
	};
// Construction
public:
	CBookmarkDlg(); // standard constructor

	void UpdateBookmark(int index, BOOL select = FALSE);
	void RemoveBookmark(int index);

	enum { IDD = IDD_BOOKMARKS };

	CMFCButton ctl_add_;
	CMFCButton ctl_del_;
	CMFCButton ctl_goto_;
	CMFCMenuButton ctl_validate_;
	CMFCButton ctl_help_;
	CMenu validate_menu_;                  // Menu of options for validate button
	CGridCtrl grid_;                       // MFC grid control

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBookmarkDlg)
	public:
	virtual BOOL Create(CWnd* pParentWnd);
	//}}AFX_VIRTUAL

public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	// Generated message map functions
	afx_msg void OnDestroy();
	//virtual void OnOK();
	afx_msg void OnAdd();
	afx_msg void OnGoTo();
	afx_msg void OnRemove();
	afx_msg void OnValidate();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnHelp();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
	afx_msg void OnGridClick(NMHDR *pNotifyStruct, LRESULT* pResult);
	afx_msg void OnGridDoubleClick(NMHDR *pNotifyStruct, LRESULT* pResult);
	afx_msg void OnGridRClick(NMHDR *pNotifyStruct, LRESULT* pResult);
	afx_msg void OnGridEndLabelEdit(NMHDR *pNotifyStruct, LRESULT* pResult);
	//afx_msg void OnInitialUpdate();
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

	DECLARE_MESSAGE_MAP()

	HWND help_hwnd_;                    // HWND of window for which context help is pending (usually 0)

	void InitColumnHeadings();
	void FillGrid();
	void UpdateRow(int index, int row, BOOL select = FALSE);
	void ValidateOptions();

	bool GetKeepNetwork() { return theApp.GetProfileInt("File-Settings", "BookmarkKeepNetwork", 1) != 0; }
	void SetKeepNetwork(bool b) { theApp.WriteProfileInt("File-Settings", "BookmarkKeepNetwork", b); }

	CDocument *pdoc_;                   // Ptr to document if last bookmark is for open file (else NULL)
	CString last_file_;                 // Name of the file to which pdoc_ points
//    BOOL show_;                       // Show bookmarks for the current file

private:
	bool m_first;                       // Remember first call to OnKickIdle (we can't add the controls to the resizer till then)
	CResizeCtrl m_resizer;              // Used to move controls around when the window is resized
	static CBrush * m_pBrush;           // brush used for background
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BOOKMARKDLG_H__B31E7B1A_8652_4D8E_B526_3786950F743B__INCLUDED_)
