#if !defined(AFX_CALCEDIT_H__5F719241_DA25_11D3_A23F_0020AFDC3196__INCLUDED_)
#define AFX_CALCEDIT_H__5F719241_DA25_11D3_A23F_0020AFDC3196__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CalcEdit.h : header file
//

enum STATE {
	ERROR,         // The last operation generated an error, E is displayed in the status 
	OVERFLOW,      // Integer result that overflowed the current bits, O is displayed in the status
	INTRES,        // Edit box displays an integer result, eg: after = button pressed (status is blank)
	INTLIT  =10,   // User has entered/is entering an integer literal, eg: "1,234"
	INTEXPR =20,   // User has entered/is entering an integer expression, eg "N + 2"
	REALEXPR,      // User entered real expression, eg "1 / 2.0"
	DATEEXPR,      // User entered date expression, eg "now()" 
	STREXPR,       // User entered string expression, eg: "left(str, 1)"
	BOOLEXPR,      // User entered Boolean expression, eg "N > 2"
	OTHER   =30,   // User has entered something else, probably an incomplete expression, eg: "N +"
	OTHRES  =40,   // Non-integer result, eg: after = button pressed with non-int expression (status is R,S,D,B)
} state_;

/////////////////////////////////////////////////////////////////////////////
// CCalcListBox - the dop down list part of CCalcComboBox
class CCalcListBox : public CListBox
{
// Construction
public:
	CCalcListBox() { }

protected:
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT nIDEvent);

	DECLARE_MESSAGE_MAP()

private:
};

/////////////////////////////////////////////////////////////////////////////
// CCalcComboBox - edit box (see CCalcEdit below) with drop down (for history)
class CCalcComboBox : public CComboBox
{
public:
	CCalcComboBox() { }

protected:
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnDestroy();

	DECLARE_MESSAGE_MAP()

private:
	CCalcListBox listbox_;
};

/////////////////////////////////////////////////////////////////////////////
// CCalcEdit - edit box for the calculator for entering values and displaying results

class CCalcEdit : public CEdit
{
	friend class CCalcDlg;

// Construction
public:
	CCalcEdit();

// Attributes
public:
	CCalcDlg *pp_;

// Operations
public:
	void Put();                         // Set edit text from pp_->current_
	void PutStr();                      // Set edit text from pp_->current_str_
//    void Get();                         // Store edit text as number in pp_->current_
	void ClearResult(bool clear = true);
	CALCSTATE update_value(bool side_effects = true);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCalcEdit)
	//}}AFX_VIRTUAL
	virtual BOOL PreTranslateMessage(MSG* pMsg);

// Implementation
public:
	virtual ~CCalcEdit();

	// Generated message map functions
protected:
	//{{AFX_MSG(CCalcEdit)
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);

	DECLARE_MESSAGE_MAP()

private:
	void add_sep();

	// This edit box is now used in a drop list, which seems to fiddle with the
	// current selected chars in its text box when focus changes.  To cope with this
	// we save the selection in sel_ in OnKillFocus & restore it in OnSetFocus.
	DWORD sel_;

	bool is_number(LPCTSTR ss);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CALCEDIT_H__5F719241_DA25_11D3_A23F_0020AFDC3196__INCLUDED_)
