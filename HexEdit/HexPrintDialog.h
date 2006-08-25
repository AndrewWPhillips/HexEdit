#if !defined(AFX_HEXPRINTDIALOG_H__E44403E4_CB1B_4D7E_BE3D_30A8AB4016EC__INCLUDED_)
#define AFX_HEXPRINTDIALOG_H__E44403E4_CB1B_4D7E_BE3D_30A8AB4016EC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// HexPrintDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CHexPrintDialog dialog

class CHexPrintDialog : public CPrintDialog
{
	DECLARE_DYNAMIC(CHexPrintDialog)

public:
	CHexPrintDialog(BOOL bPrintSetupOnly,
		// TRUE for Print Setup, FALSE for Print Dialog
		DWORD dwFlags = PD_ALLPAGES | PD_USEDEVMODECOPIES | PD_NOPAGENUMS
			| PD_HIDEPRINTTOFILE | PD_NOSELECTION,
		CWnd* pParentWnd = NULL);

    BOOL dup_lines_;            // Merge duplicates lines (print selection only)

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();
	//{{AFX_MSG(CHexPrintDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnPrintOptions();
	//}}AFX_MSG
	afx_msg BOOL OnClickAll();
	afx_msg BOOL OnClickPages();
	afx_msg BOOL OnClickSel();
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HEXPRINTDIALOG_H__E44403E4_CB1B_4D7E_BE3D_30A8AB4016EC__INCLUDED_)
