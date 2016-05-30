// RecentFileDlg.h : header file for Recent File List Dialog
//
// Copyright (c) 2015 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#if !defined(AFX_RECENTFILEDLG_H__C05522E2_E23B_4B7F_9FF3_D65740FCF2BE__INCLUDED_)
#define AFX_RECENTFILEDLG_H__C05522E2_E23B_4B7F_9FF3_D65740FCF2BE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "range_set.h"
#include "GridCtrl_src/GridCtrl.h"
#include "ResizeCtrl.h"
#include "Preview.h"

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
	enum { IDD = IDD_RECENT_FILES_NEW };
	BOOL open_readonly_, open_shared_;    // flags that control open button behaviour
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
	afx_msg void OnRemove();
	afx_msg void OnSearch();
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

#ifdef FILE_PREVIEW
	int m_preview_width;                // Extra width added to dialog for preview (on right)
	static const int preview_border = 5;
	//int m_min_height;                   // Min height for dialog so that all of preview is seen
	CPreview m_preview;                 // preview panel in the dialog
#endif // FILE_PREVIEW
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RECENTFILEDLG_H__C05522E2_E23B_4B7F_9FF3_D65740FCF2BE__INCLUDED_)
