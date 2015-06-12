// Register.h : header file
//
#if !defined(AFX_REGISTER_H__AAFC4281_06F5_11D4_A240_0020AFDC3196__INCLUDED_)
#define AFX_REGISTER_H__AAFC4281_06F5_11D4_A240_0020AFDC3196__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TransparentListBox.h"
#include "TransparentStatic2.h"

/////////////////////////////////////////////////////////////////////////////
// CAbout dialog

class CAbout : public CDialog
{
// Construction
public:
	CAbout(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAbout)
	enum { IDD = IDD_ABOUT };
	CMFCLinkCtrl	url_ctl_;
	//}}AFX_DATA
	CTransparentStatic2 ctl_line1_;
	CTransparentStatic2 ctl_line2_;
	CTransparentStatic2 ctl_line3_;
	CTransparentStatic2 ctl_line4_;
	CTransparentStatic2 ctl_line5_;
	CTransparentListBox ctl_ack_;
	CString	text1_;

// Overrides
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAbout)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnEmail();
	afx_msg void OnAckMore();
	afx_msg void OnDblclkAck();
	DECLARE_MESSAGE_MAP()
	void fix_controls();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REGISTER_H__AAFC4281_06F5_11D4_A240_0020AFDC3196__INCLUDED_)
