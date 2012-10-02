#if !defined(AFX_CALCEDIT_H__5F719241_DA25_11D3_A23F_0020AFDC3196__INCLUDED_)
#define AFX_CALCEDIT_H__5F719241_DA25_11D3_A23F_0020AFDC3196__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TipWnd.h"

// CalcEdit.h : header file
//

enum CALCSTATE {
	CALCERROR,         // The last operation generated an error, E is displayed in the status 
	CALCOVERFLOW,      // Integer result that overflowed the current bits, O is displayed in the status
	CALCINTRES,        // Final integer result eg: after = button pressed
	CALCINTUNARY,      // Computed integer from unary operation, eg: INC button 
	CALCINTBINARY,     // Computed integer from binary operation, eg + button
	CALCINTLIT  =10,   // User has entered/is entering an integer literal, eg: "1,234"
	CALCINTEXPR =20,   // User has entered/is entering an integer expression, eg "N + 2"
	CALCREALEXPR,      // User entered real expression, eg "1 / 2.0"
	CALCDATEEXPR,      // User entered date expression, eg "now()" 
	CALCSTREXPR,       // User entered string expression, eg: "left(str, 1)"
	CALCBOOLEXPR,      // User entered Boolean expression, eg "N > 2"
	CALCOTHER   =30,   // User has entered something else, probably an incomplete expression, eg: "N +"
	CALCOTHRES  =40,   // Non-integer result, eg: after = button pressed with non-int expression (status is R,S,D,B)
	CALCREALRES,       // Must be 20 more than REALEXPR
	CALCDATERES,
	CALCSTRRES,
	CALCBOOLRES,
};

class CCalcDlg;

/////////////////////////////////////////////////////////////////////////////
// CCalcListBox - the dop down list part of CCalcComboBox

class CCalcListBox : public CListBox
{
// Construction
public:
	CCalcListBox() : curr_(-1), pp_(NULL) { }
	CCalcDlg *pp_;

protected:
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT nIDEvent);

	DECLARE_MESSAGE_MAP()

private:
	CTipWnd tip_;
	int curr_;       // item we are currently showing a tip window for
};

/////////////////////////////////////////////////////////////////////////////
// CCalcComboBox - edit box (see CCalcEdit below) with drop down (for history)
class CCalcComboBox : public CComboBox
{
public:
	CCalcComboBox() : pp_(NULL) { }
	CCalcDlg *pp_;

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
//	void Put();                         // Set edit text from pp_->current_
//	void PutStr();                      // Set edit text from pp_->current_str_
//    void Get();                         // Store edit text as number in pp_->current_

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCalcEdit)
	//}}AFX_VIRTUAL
	virtual BOOL PreTranslateMessage(MSG* pMsg);

// Implementation
public:
	virtual ~CCalcEdit();

	void put();       // put current value into edit box
	void get();       // get current value from edit box into current_str_
	CALCSTATE update_value(bool side_effects = true); // get info from current_str_
	void clear_result(bool clear = true);             // make edit box editable (not a "result")

	// Generated message map functions
protected:
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnEnChange();

	DECLARE_MESSAGE_MAP()

private:
	void add_sep();

	// We only really need this is OnKillFocus but keep it in case it comes in handy 
	DWORD sel_;

	bool in_edit_;

	CString get_number(LPCTSTR ss);  // Remove sep chars from number or return empty string if not a number
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CALCEDIT_H__5F719241_DA25_11D3_A23F_0020AFDC3196__INCLUDED_)
