// CompareList.h - header file for calculator history dialog
//

#ifndef COMPARELIST__INCLUDED_
#define COMPARELIST__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ResizeCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// CGridCtrlComp - our class derived from CGridCtrl

class CGridCtrlComp : public CGridCtrl
{
	DECLARE_DYNCREATE(CGridCtrlComp)

public:
	void FixHeading(int col);

protected:
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP()

private:
};

/////////////////////////////////////////////////////////////////////////////
// CCompareListDlg dialog

class CCompareListDlg : public CDialog
{
	// Number the different columns we can display
	// Note these columns must match the heading strings in InitColumnHeadings
	enum
	{
		COL_ORIG_TYPE,        // Same, insert, delete, or replace
		COL_ORIG_HEX,         // Hex address in original file of the difference
		COL_ORIG_DEC,         // Dec address in original file of the difference
		COL_LEN_HEX,          // # of bytes inserted, deleted or replaced (hex)
		COL_LEN_DEC,          // # of bytes (decimal)
		COL_COMP_HEX,         // Hex address in compare file of the difference
		COL_COMP_DEC,         // Dec address in compare file of the difference
		COL_COMP_TYPE,        // Same as COL_ORIG_TYPE, except insert<->delete
		COL_LAST              // leave at end (not a real column but signals end of list)
	};

// Construction
public:
	CCompareListDlg(); // standard constructor

	enum { IDD = IDD_COMPARE_LIST };

	CGridCtrlComp grid_;            // to show each change as a row in the grid

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCompareListDlg)
	public:
	virtual BOOL Create(CWnd* pParentWnd);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	// Generated message map functions
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnHelp();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
	//afx_msg void OnInitialUpdate();
	//afx_msg BOOL OnEraseBkgnd(CDC *pDC);
	//afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnGridRClick(NMHDR *pNotifyStruct, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()
	void InitColumnHeadings();

	HWND help_hwnd_;                    // HWND of window for which context help is pending (usually 0)

private:
	bool m_first;                       // Remember first call to OnKickIdle (we can't add the controls to the resizer till then)
	CResizeCtrl m_resizer;              // Used to move controls around when the window is resized
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // COMPARELIST__INCLUDED_
