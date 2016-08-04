// BookmarkFind.h : dialog for overwrite/append to a set of bookmarks
//
// Copyright (c) 2016 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#if !defined(AFX_BOOKMARKFIND_H__29747AED_4ACB_44F6_8663_104FB819A341__INCLUDED_)
#define AFX_BOOKMARKFIND_H__29747AED_4ACB_44F6_8663_104FB819A341__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CBookmarkFind dialog

class CBookmarkFind : public CDialog
{
// Construction
public:
	CBookmarkFind(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CBookmarkFind)
	enum { IDD = IDD_BM_FIND };
	CString	mess_;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBookmarkFind)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CBookmarkFind)
	virtual BOOL OnInitDialog();
	afx_msg void OnAppend();
	afx_msg void OnOverwrite();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BOOKMARKFIND_H__29747AED_4ACB_44F6_8663_104FB819A341__INCLUDED_)
