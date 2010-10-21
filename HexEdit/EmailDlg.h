#if !defined(AFX_EMAILDLG_H__C0AA8EC1_8393_11D2_B02B_0020AFDC3196__INCLUDED_)
#define AFX_EMAILDLG_H__C0AA8EC1_8393_11D2_B02B_0020AFDC3196__INCLUDED_

// Copyright (c) 1999-2010 by Andrew W. Phillips.
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

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// EmailDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEmailDlg dialog

class CEmailDlg : public CDialog
{
// Construction
public:
	CEmailDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEmailDlg)
	enum { IDD = IDD_EMAIL };
	CString	to_;
	CString	subject_;
	CString	systype_;
	CString	text_;
	int		type_;
	CString	name_;
	CString	address_;
	CString	version_;
	//}}AFX_DATA

	BOOL attach_;
	CString attachment_;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEmailDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEmailDlg)
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	virtual BOOL OnInitDialog();
	afx_msg void OnEmailHelp();
	afx_msg void OnAttach();
	afx_msg void OnAttachmentBrowse();
	//}}AFX_MSG
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EMAILDLG_H__C0AA8EC1_8393_11D2_B02B_0020AFDC3196__INCLUDED_)
