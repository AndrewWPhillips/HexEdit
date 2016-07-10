#pragma once
#include "afxwin.h"
#include "Dialog.h"
#include "ResizeCtrl.h"

// CNewCompare dialog

class CNewCompare : public CHexDialog
{
	DECLARE_DYNAMIC(CNewCompare)

public:
	CNewCompare(CWnd* pParent = NULL);   // standard constructor
	virtual ~CNewCompare();

// Dialog Data
	enum { IDD = IDD_NEW_COMPARE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:
	CResizeCtrl resizer_;
	void fix_controls();
	CEdit ctl_file_name_;
	CButton ctl_browse_;
	CStatic ctl_comment_;
	CStatic ctl_static1_;
	CEdit ctl_minmatch_;

public:
	int compare_type_;    // 0 = self, 1 = another file
	CString file_name_;   // name of file (if compare_type_ == 1)
	int compare_display_; // 0 = split window, 1 = tabbed
	int insdel_;          // detect inserions/deltions (0/1)
	UINT minmatch_;       // min match when searching for insertions/deletions
	int auto_sync_;       // auto sync (0/1)
	int auto_scroll_;     // auto scroll (0/1)
	bool orig_shared_;    // can only do self-compare if the original is shareable

public:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnBnClickedCompareSelf();
	afx_msg void OnBnClickedCompareFile();
	afx_msg void OnBnClickedAttachmentBrowse();
	afx_msg void OnBnClickedInsDel();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnHelp();
};
