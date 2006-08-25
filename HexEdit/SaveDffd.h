#if !defined(AFX_SAVEDFFD_H__DBCB8CB1_3F1F_4410_81A7_5D58619D6B18__INCLUDED_)
#define AFX_SAVEDFFD_H__DBCB8CB1_3F1F_4410_81A7_5D58619D6B18__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SaveDffd.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSaveDffd dialog

class CSaveDffd : public CDialog
{
// Construction
public:
	CSaveDffd(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSaveDffd)
	enum { IDD = IDD_SAVE_TEMPLATE };
	CString	message_;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSaveDffd)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSaveDffd)
	afx_msg void OnSaveAs();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SAVEDFFD_H__DBCB8CB1_3F1F_4410_81A7_5D58619D6B18__INCLUDED_)
