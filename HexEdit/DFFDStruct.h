// DFFDStruct.h - header for STRUCT editing dialog
//
// Copyright (c) 2004-2010 by Andrew W. Phillips.
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

#if !defined(AFX_DFFDSTRUCT_H__835B4B9C_0BF1_4197_A2B6_57187005DC9E__INCLUDED_)
#define AFX_DFFDSTRUCT_H__835B4B9C_0BF1_4197_A2B6_57187005DC9E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "XMLTree.h"

// DFFDStruct.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDFFDStruct dialog

class CDFFDStruct : public CDialog
{
// Construction
public:
	CDFFDStruct(CXmlTree::CElt *pp, signed char parent_type, CWnd* pParent = NULL);   // standard constructor

	bool IsModified() const { return modified_; }
	void SetModified(bool mm = true) { modified_ = mm; }
	void GetPosition(CPoint &pp) const { pp.x = pos_.x; pp.y = pos_.y; }
	void SetPosition(CPoint pp, CPoint oo = CPoint(-1,-1))
	{
		pos_ = pp;
		if (oo == CPoint(-1,-1))
			orig_ = pp, first_ = true;
		else
			orig_ = oo;
	}

// Dialog Data
	//{{AFX_DATA(CDFFDStruct)
	enum { IDD = IDD_DFFD_STRUCT };
	CButton ctl_down_;
	CButton ctl_up_;
	CEdit   ctl_name_;
	CListBox    ctl_elements_;
	CMFCMenuButton  ctl_insert_;
	CButton ctl_edit_;
	CButton ctl_delete_;
	CButton ctl_prev_;
	CButton ctl_next_;
	CString comment_;
	CString name_;
	CString type_name_;
	//}}AFX_DATA
	CEdit ctl_type_name_;
	CString expr_;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDFFDStruct)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDFFDStruct)
	virtual BOOL OnInitDialog();
	afx_msg void OnNext();
	afx_msg void OnPrev();
	virtual void OnOK();
	afx_msg void OnChange();
	afx_msg void OnInsert();
	afx_msg void OnDelete();
	afx_msg void OnEdit();
	afx_msg void OnUp();
	afx_msg void OnDown();
	virtual void OnCancel();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	afx_msg void OnCaseDClick();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
	afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()

	HICON icon_del_, icon_up_, icon_down_;

	CXmlTree::CElt *pelt_;      // The STRUCT element we are editing
	CXmlTree::CFrag saved_;     // Saved copy of the children (before editing) in case Cancel pressed
	bool saved_mod_flag_;       // If dialog cancelled then we have to restore the modified status
	bool show_prev_next_;       // Show << and >> buttons to move between sibling (if parent is struct)
	CPoint pos_;                // Position of this dialog
	CPoint orig_;               // Position of the first dialog opened
	bool modified_;             // Have any of the values been changed?
	CMenu button_menu_;         // We need to keep the menu in memory (for the Insert button)
	bool first_;                // Is this the first dialog called (since any DFFD dialog can call any other)
	signed char parent_type_;   // Type of container of this element

	void            do_edit(bool delete_on_cancel = false);
	void            load_data();
	bool            check_data();
	void            save_data();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DFFDSTRUCT_H__835B4B9C_0BF1_4197_A2B6_57187005DC9E__INCLUDED_)
