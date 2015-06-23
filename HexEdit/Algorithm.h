// Algorithm.h - CAlgorithm accepts and encryption algorithm
//
// Copyright (c) 2015 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#if !defined(AFX_ALGORITHM_H__BB3DB185_9861_4A0C_A2CC_0E93997C9A7B__INCLUDED_)
#define AFX_ALGORITHM_H__BB3DB185_9861_4A0C_A2CC_0E93997C9A7B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Algorithm.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAlgorithm dialog

class CAlgorithm : public CDialog
{
// Construction
public:
	CAlgorithm(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAlgorithm)
	enum { IDD = IDD_ALGORITHM };
	CComboBox	m_control;
	CString	m_note;
	CString	m_blocklen;
	CString	m_keylen;
	int		m_alg;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAlgorithm)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAlgorithm)
	afx_msg void OnSelchangeAlg();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnAlgorithmHelp();
	//}}AFX_MSG
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ALGORITHM_H__BB3DB185_9861_4A0C_A2CC_0E93997C9A7B__INCLUDED_)
