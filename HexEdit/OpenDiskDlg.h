// OpenDiskDlg.h : header file for Open Disk Dialog
//
// Copyright (c) 2005 by Andrew W. Phillips.
//
// No restrictions are placed on the noncommercial use of this code,
// as long as this text (from the above copyright notice to the
// disclaimer below) is preserved.
//
// This code may be redistributed as long as it remains unmodified
// and is not sold for profit without the author's written consent.
//
// This code, or any part of it, may not be used in any software that
// is sold for profit, without the author's written consent.
//
// DISCLAIMER: This file is provided "as is" with no expressed or
// implied warranty. The author accepts no liability for any damage
// or loss of business that this product may cause.
//

#if !defined(AFX_OPENSPECIALDLG_H__DCD79807_8825_4EA9_9DD4_0A13BA41D0A8__INCLUDED_)
#define AFX_OPENSPECIALDLG_H__DCD79807_8825_4EA9_9DD4_0A13BA41D0A8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ResizeCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// COpenDiskDlg dialog

class COpenDiskDlg : public CDialog
{
// Construction
public:
	COpenDiskDlg(CWnd* pParent = NULL);   // standard constructor

    CString SelectedDevice() { return (m_selected == -1) ? "" : m_device_name[m_selected]; }
    BOOL ReadOnly() { return m_readonly; }

// Dialog Data
	//{{AFX_DATA(COpenDiskDlg)
	enum { IDD = IDD_OPEN_SPECIAL };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COpenDiskDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(COpenDiskDlg)
	afx_msg void OnSelchangedOpenTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkOpenTree(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg void OnSelchangingOpenTree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
	afx_msg BOOL OnDeviceChange(UINT, DWORD);
    afx_msg void OnTimer(UINT nIDEvent);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()

	CResizeCtrl resizer_;               // Used to move controls around when the window is resized

public:
	virtual BOOL OnInitDialog();

private:
    void Update(int ii);
    int BuildTree(LPCTSTR current_device = NULL);
	bool m_do_rebuild, m_next_time;

    int m_selected;                     // device selected when OK clicked
	CTreeCtrl m_ctrl_tree;              // tree control containing device names
	CEdit m_ctl_info;                   // displays extra info about selected device
	CStatic m_ctl_desc;                 // heading for info display about selected device
	CButton m_ctl_readonly;             // Read-only check box
	CButton m_ctl_ok;					// OK button
	BOOL m_readonly;                    // is the device to be opened read only?

	std::vector<CString> m_device_name; // List of device names corresp. to tree leaf nodes
	int m_first_physical_device;        // Index of first physical device (up to this are drive volumes)

    CImageList m_imagelist;             // Needed for tree icons
    enum
    {
        IMG_PHYS_DRIVE,
        IMG_PHYS_FLOPPY,
        IMG_PHYS_DRIVES,
        IMG_PHYS_REMOVE,
        IMG_PHYS_CD,
        IMG_MEDIA,
        IMG_MEDIUM,
        IMG_SCSI,
        IMG_DRIVE,
        IMG_FLOPPY,
        IMG_OPT_DRIVE,
        IMG_DRIVES,
        IMG_REMOVE,
        IMG_SHARE,
        IMG_NET,
        IMG_NET_BAD,
    };
protected:
	virtual void OnOK();
public:
	afx_msg void OnClose();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPENSPECIALDLG_H__DCD79807_8825_4EA9_9DD4_0A13BA41D0A8__INCLUDED_)
