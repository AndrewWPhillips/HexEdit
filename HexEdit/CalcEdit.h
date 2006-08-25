#if !defined(AFX_CALCEDIT_H__5F719241_DA25_11D3_A23F_0020AFDC3196__INCLUDED_)
#define AFX_CALCEDIT_H__5F719241_DA25_11D3_A23F_0020AFDC3196__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CalcEdit.h : header file
//

class CCalcDlg;

/////////////////////////////////////////////////////////////////////////////
// CCalcEdit window

class CCalcEdit : public CEdit
{
	friend CCalcDlg;

// Construction
public:
	CCalcEdit();

// Attributes
public:
    CCalcDlg *pp_;
    char dec_sep_char_;		// Char to use as "thousands" separator for decimals
    int dec_group_;			// Number of digits in "thousands" group

// Operations
public:
    void Put();                         // Set edit text from pp_->current_
//    void Get();                         // Store edit text as number in pp_->current_
	bool update_value();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCalcEdit)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCalcEdit();

	// Generated message map functions
protected:
	//{{AFX_MSG(CCalcEdit)
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:
    void add_sep();
#ifdef CALC_EXPR
	bool is_number(LPCTSTR ss);
#endif
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CALCEDIT_H__5F719241_DA25_11D3_A23F_0020AFDC3196__INCLUDED_)
