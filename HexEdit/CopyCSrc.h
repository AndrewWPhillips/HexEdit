#if !defined(AFX_COPYCSRC_H__2F30ECE7_0758_4E2F_AAFF_A76FB85F3C4D__INCLUDED_)
#define AFX_COPYCSRC_H__2F30ECE7_0758_4E2F_AAFF_A76FB85F3C4D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CopyCSrc.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCopyCSrc dialog

class CCopyCSrc : public CDialog
{
public:
    enum { STRING, CHAR, INT, FLOAT };  // Matches type_ (which radio button is selected)
    enum { INT_UNSIGNED, INT_SIGNED, INT_OCTAL, INT_HEX };  // Matches order in IDC_CSRC_INT_TYPE
    enum { INT_8, INT_16, INT_32, INT_64 };                 // Matches order IDC_CSRC_INT_SIZE
    enum { FLOAT_32, FLOAT_64, REAL_48 };                   // Matches order IDC_CSRC_FLOAT_SIZE

// Construction
public:
	CCopyCSrc(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCopyCSrc)
	enum { IDD = IDD_CSRC };
	CSpinButtonCtrl	ctl_indent_spin_;
	CComboBox	ctl_int_type_;
	CComboBox	ctl_int_size_;
	CComboBox	ctl_float_size_;
	CButton	ctl_big_endian_;
	BOOL	big_endian_;
	int		type_;
	int		float_size_;
	int		int_size_;
	int		int_type_;
	BOOL	show_address_;
	UINT	indent_;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCopyCSrc)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCopyCSrc)
	afx_msg void OnChangeType();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnCsrcHelp();
	//}}AFX_MSG
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()

	void		 fix_controls();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COPYCSRC_H__2F30ECE7_0758_4E2F_AAFF_A76FB85F3C4D__INCLUDED_)
