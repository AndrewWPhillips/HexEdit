// DFFDGlobal.h
//
// Copyright (c) 2015 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#if !defined(AFX_DFFDGLOBAL_H__01F7C4F4_5B61_44A4_8E24_23A21CFB9EEF__INCLUDED_)
#define AFX_DFFDGLOBAL_H__01F7C4F4_5B61_44A4_8E24_23A21CFB9EEF__INCLUDED_

#include "XMLTree.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// DFFDGlobal.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDFFDGlobal dialog

class CDFFDGlobal : public CDialog
{
// Construction
public:
	CDFFDGlobal(CXmlTree::CElt *pp, CWnd* pParent = NULL);   // standard constructor

	bool IsModified() const { return modified_; }
	void SetModified(bool mm = true) { modified_ = mm; }

// Dialog Data
	//{{AFX_DATA(CDFFDGlobal)
	enum { IDD = IDD_DFFD_GLOBAL };
	CEdit   ctl_web_site_;
	CString web_site_;
	int     default_char_set_;
	BOOL    default_big_endian_;
	BOOL    default_read_only_;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDFFDGlobal)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDFFDGlobal)
	afx_msg void OnChangeWebAddress();
	afx_msg void OnWebCheck();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnChange();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()

	CXmlTree::CElt *pelt_;      // The "binary_file_format" element we are editing
	bool modified_;             // Has anything been changed?
	bool web_changed_;          // Has the web address been changed?
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DFFDGLOBAL_H__01F7C4F4_5B61_44A4_8E24_23A21CFB9EEF__INCLUDED_)
