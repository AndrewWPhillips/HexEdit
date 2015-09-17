// CalcHist.h - header file for calculator history dialog
//

#ifndef CALCHIST__INCLUDED_
#define CALCHIST__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ResizeCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// CCalcHistDlg dialog

class CCalcHistDlg : public CDialog
{
// Construction
public:
	CCalcHistDlg(); // standard constructor

	enum { IDD = IDD_CALC_HIST };

	void Add(LPCTSTR str);
	CMFCButton ctl_hist_clear_;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCalcHistDlg)
	public:
	virtual BOOL Create(CWnd* pParentWnd);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	// Generated message map functions
	afx_msg void OnClear();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnHelp();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
	//afx_msg void OnInitialUpdate();
	//afx_msg BOOL OnEraseBkgnd(CDC *pDC);
	//afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

	DECLARE_MESSAGE_MAP()

	HWND help_hwnd_;                    // HWND of window for which context help is pending (usually 0)

private:
	bool m_first;                       // Remember first call to OnKickIdle (we can't add the controls to the resizer till then)
	CResizeCtrl m_resizer;              // Used to move controls around when the window is resized
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // CALCHIST__INCLUDED_
