#if !defined(AFX_TPARSEDLG_H__12AEF2C9_7F12_4035_A4C1_D8C38C1CCE72__INCLUDED_)
#define AFX_TPARSEDLG_H__12AEF2C9_7F12_4035_A4C1_D8C38C1CCE72__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TParseDlg.h : header file
//

#include "ResizeCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// TParseDlg dialog

class TParseDlg : public CDialog
{
// Construction
public:
	TParseDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(TParseDlg)
	enum { IDD = IDD_TPARSE };
	CString	m_source_code;
	int		m_pack;
	int		m_bitfield_up;
	BOOL	m_types_common;
	BOOL	m_types_custom;
	BOOL	m_types_save;
	BOOL	m_types_std;
	BOOL	m_types_windows;
	BOOL	m_values_custom;
	BOOL	m_values_save;
	BOOL	m_values_windows;
	//}}AFX_DATA
	BOOL	m_base_storage_unit;

	virtual BOOL PreTranslateMessage(MSG* pMsg);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(TParseDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(TParseDlg)
	virtual void OnOK();
	afx_msg void OnValuesCustom();
	afx_msg void OnTypesCustom();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()

	CResizeCtrl resizer_;               // Used to move controls around when the window is resized
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TPARSEDLG_H__12AEF2C9_7F12_4035_A4C1_D8C38C1CCE72__INCLUDED_)
