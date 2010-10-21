#if !defined(AFX_NEWSCHEME_H__FBB6ABB9_DDCC_401F_967C_E4F2040ACCAB__INCLUDED_)
#define AFX_NEWSCHEME_H__FBB6ABB9_DDCC_401F_967C_E4F2040ACCAB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NewScheme.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNewScheme dialog

class CNewScheme : public CDialog
{
// Construction
public:
	CNewScheme(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNewScheme)
	enum { IDD = IDD_NEW_SCHEME };
	CComboBox	scheme_list_ctrl_;
	CString	scheme_name_;
	int		copy_from_;
	//}}AFX_DATA
	std::vector<CScheme> *psvec_;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewScheme)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNewScheme)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnHelp();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWSCHEME_H__FBB6ABB9_DDCC_401F_967C_E4F2040ACCAB__INCLUDED_)
