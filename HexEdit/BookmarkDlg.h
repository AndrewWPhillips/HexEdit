// BookmarkDlg.h : header file for Bookmarks Dialog
//
// Copyright (c) 2003-2010 by Andrew W. Phillips.
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

#if !defined(AFX_BOOKMARKDLG_H__B31E7B1A_8652_4D8E_B526_3786950F743B__INCLUDED_)
#define AFX_BOOKMARKDLG_H__B31E7B1A_8652_4D8E_B526_3786950F743B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

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

	// Generated message map functions
	afx_msg void OnDestroy();
	virtual void OnOK();
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
	//afx_msg void OnInitialUpdate();
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

	DECLARE_MESSAGE_MAP()

	HWND help_hwnd_;                    // HWND of window for which context help is pending (usually 0)

	void InitColumnHeadings();
    void FillGrid();
	void UpdateRow(int index, int row, BOOL select = FALSE);

    CDocument *pdoc_;                   // Ptr to document if last bookmark is for open file (else NULL)
    CString last_file_;                 // Name of the file to which pdoc_ points
//    BOOL show_;                       // Show bookmarks for the current file

private:
	bool m_first;                       // Remember first call to OnKickIdle (we can't add the controls to the resizer till then)
	CResizeCtrl m_resizer;              // Used to move controls around when the window is resized
	static CBrush * m_pBrush;           // brush used for background
	static COLORREF m_col;              // colour used for background
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BOOKMARKDLG_H__B31E7B1A_8652_4D8E_B526_3786950F743B__INCLUDED_)
