// CompareList.h - header file for calculator history dialog
//

#ifndef COMPARELIST__INCLUDED_
#define COMPARELIST__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HexEditDoc.h"
#include "ResizeCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// CGridCtrlComp - our class derived from CGridCtrl

class CGridCtrlComp : public CGridCtrl
{
	DECLARE_DYNCREATE(CGridCtrlComp)

public:
	void FixHeading(int col, UINT size);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
	virtual BOOL OnResizeColumn(int col, UINT size);

	//afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	DECLARE_MESSAGE_MAP()

private:
};

/////////////////////////////////////////////////////////////////////////////
// CCompareListDlg dialog

class CCompareListDlg : public CDialog
{
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
	afx_msg void OnDestroy();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnHelp();
	//afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
	//afx_msg void OnInitialUpdate();
	//afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnGridDoubleClick(NMHDR *pNotifyStruct, LRESULT* pResult);
	afx_msg void OnGridRClick(NMHDR *pNotifyStruct, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()
	void InitColumnHeadings();

	HWND help_hwnd_;                    // HWND of window for which context help is pending (usually 0)

private:
	void FillGrid(CHexEditDoc * pdoc);
	void AddMessage(const char * mess);

	bool m_first;                       // Remember first call to OnKickIdle (we can't add the controls to the resizer till then)
	CResizeCtrl m_resizer;              // Used to move controls around when the window is resized

	CHexEditView *phev_;                // ptr to hex view that we are displaying changes for
	clock_t last_change_;               // time that last change was made to the compare info for this view
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // COMPARELIST__INCLUDED_
