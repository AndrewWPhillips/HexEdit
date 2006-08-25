#if !defined(AFX_CLEARHISTDLG_H__D5641CFE_ABCF_48C4_A09F_5A75FAF8DB7B__INCLUDED_)
#define AFX_CLEARHISTDLG_H__D5641CFE_ABCF_48C4_A09F_5A75FAF8DB7B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ClearHistDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CClearHistDlg dialog

class CClearHistDlg : public CDialog
{
// Construction
public:
	CClearHistDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CClearHistDlg)
	enum { IDD = IDD_CLEAR_HIST };
	BOOL	clear_bookmarks_;
	BOOL	clear_recent_file_list_;
	BOOL	clear_hist_;
	BOOL	clear_on_exit_;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CClearHistDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CClearHistDlg)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnHelp();
	//}}AFX_MSG
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CLEARHISTDLG_H__D5641CFE_ABCF_48C4_A09F_5A75FAF8DB7B__INCLUDED_)
