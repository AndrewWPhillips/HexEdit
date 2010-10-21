// RecentFileDlg.h : header file for Recent File List Dialog
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

#if !defined(AFX_RECENTFILEDLG_H__C05522E2_E23B_4B7F_9FF3_D65740FCF2BE__INCLUDED_)
#define AFX_RECENTFILEDLG_H__C05522E2_E23B_4B7F_9FF3_D65740FCF2BE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "range_set.h"
#include "GridCtrl_src/GridCtrl.h"
#include "ResizeCtrl.h"

// RecentFileDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRecentFileDlg dialog

class CRecentFileDlg : public CDialog
{
public:
	// Number the different types of columns we can display
	enum
	{
		COL_NAME,                           // Name of the recent file
		COL_LOCN,                           // Location (path)
		COL_USED,                           // Date/time the file was last opened in HexEdit
		COL_SIZE,                           // Size of the file on disk
		COL_TYPE,                           // Type of the file (from extension)
		COL_MODIFIED,                       // Time the file was last modified
		COL_ATTR,                           // File attributes
		COL_CREATED,                        // Time the file was created
		COL_ACCESSED,                       // Time the file was last accessed
#ifdef PROP_INFO
		COL_CATEGORY,                       // Category entered by user
		COL_KEYWORDS,                       // Keywords entered by user
		COL_COMMENTS,                       // Comments entered by user
#endif
		COL_LAST                            // leave at end (signals end of list)
	};

// Construction
public:
	CRecentFileDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRecentFileDlg)
	enum { IDD = IDD_RECENT_FILES };
	CButton	ctl_remove_;
	CButton	ctl_open_ro_;
	CButton	ctl_open_;
	BOOL	net_retain_;
	//}}AFX_DATA

	CGridCtrl grid_;                       // MFC grid control

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRecentFileDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRecentFileDlg)
	afx_msg void OnValidate();
	afx_msg void OnOpen();
	virtual BOOL OnInitDialog();
	afx_msg void OnOpenRO();
	afx_msg void OnRemove();
	virtual void OnOK();
	afx_msg void OnDestroy();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnHelp();
	//}}AFX_MSG
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
	//afx_msg BOOL OnEraseBkgnd(CDC *pDC);
	//afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnGridClick(NMHDR *pNotifyStruct, LRESULT* pResult);
	afx_msg void OnGridDoubleClick(NMHDR *pNotifyStruct, LRESULT* pResult);
	afx_msg void OnGridRClick(NMHDR *pNotifyStruct, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()

	void InitColumnHeadings();
	void FillGrid();
	void DeleteEntries();

	CResizeCtrl resizer_;               // Used to move controls around when the window is resized
	//static CBrush * m_pBrush;           // brush used for background

	range_set<int> to_delete_;			// Keeps track of files to delete from list when OK clicked
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RECENTFILEDLG_H__C05522E2_E23B_4B7F_9FF3_D65740FCF2BE__INCLUDED_)
