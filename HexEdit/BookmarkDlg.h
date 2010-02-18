// BookmarkDlg.h : header file for Bookmarks Dialog
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

#if !defined(AFX_BOOKMARKDLG_H__B31E7B1A_8652_4D8E_B526_3786950F743B__INCLUDED_)
#define AFX_BOOKMARKDLG_H__B31E7B1A_8652_4D8E_B526_3786950F743B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "range_set.h"
#include "GridCtrl_src/GridCtrl.h"
#include "Dialog.h"
#include "ResizeCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// CBookmarkDlg dialog

class CBookmarkDlg : public CHexDialogBar
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
	virtual BOOL Create(CWnd* pParentWnd, UINT nIDTemplate,
			UINT nStyle, UINT nID);
	//}}AFX_VIRTUAL
	virtual void DelayShow(BOOL bShow) { theApp.SaveToMacro(km_bookmarks, bShow ? 5 : 6);
                                         CHexDialogBar::DelayShow(bShow); }

public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CBookmarkDlg)
	afx_msg void OnDestroy();
	virtual void OnOK();
	afx_msg void OnAdd();
	afx_msg void OnGoTo();
	afx_msg void OnRemove();
	afx_msg void OnValidate();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnHelp();
	//}}AFX_MSG
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
    afx_msg void OnGridClick(NMHDR *pNotifyStruct, LRESULT* pResult);
    afx_msg void OnGridDoubleClick(NMHDR *pNotifyStruct, LRESULT* pResult);
    afx_msg void OnGridRClick(NMHDR *pNotifyStruct, LRESULT* pResult);
	//afx_msg void OnInitialUpdate();
	DECLARE_MESSAGE_MAP()

	CResizeCtrl resizer_;               // Used to move controls around when the window is resized
	HWND help_hwnd_;                    // HWND of window for which context help is pending (usually 0)

	void InitColumnHeadings();
    void FillGrid();
	void UpdateRow(int index, int row, BOOL select = FALSE);

    CDocument *pdoc_;                   // Ptr to document if last bookmark is for open file (else NULL)
    CString last_file_;                 // Name of the file to which pdoc_ points
//    BOOL show_;                         // Show bookmarks for the current file
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BOOKMARKDLG_H__B31E7B1A_8652_4D8E_B526_3786950F743B__INCLUDED_)
