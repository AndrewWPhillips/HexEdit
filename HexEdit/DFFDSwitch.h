// DFFDSwitch.h
//
// Copyright (c) 2015 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#if !defined(DFFDSWITCH_INCLUDED)
#define DFFDSWITCH_INCLUDED

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "XMLTree.h"

/////////////////////////////////////////////////////////////////////////////
// CDFFDSwitch dialog

class CDFFDSwitch : public CDialog
{
// Construction
public:
	CDFFDSwitch(CXmlTree::CElt *pp, signed char parent_type, CWnd* pParent = NULL);   // standard constructor
	virtual ~CDFFDSwitch();

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
	enum { IDD = IDD_DFFD_SWITCH };
	CString expr_;
	CString comment_;

	CEdit ctl_expr_;
	CMFCMenuButton ctl_expr_var_;
	CListBox ctl_cases_;
	CEdit ctl_range_;

	CButton ctl_edit_;
	CMFCMenuButton ctl_insert_;
	CButton ctl_delete_;
	CButton ctl_up_;
	CButton ctl_down_;

	CButton ctl_prev_;
	CButton ctl_next_;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
	afx_msg void OnHelp();

	afx_msg void OnChange();
	afx_msg void OnChangeRange();
	afx_msg void OnGetExprVar();
	afx_msg void OnCaseDClick();
	afx_msg void OnEdit();
	afx_msg void OnInsert();
	afx_msg void OnDelete();
	afx_msg void OnUp();
	afx_msg void OnDown();

	afx_msg void OnNext();
	afx_msg void OnPrev();
	DECLARE_MESSAGE_MAP()

private:
	HICON icon_del_, icon_up_, icon_down_;

	CXmlTree::CElt *pelt_;      // The IF element we are editing
	CXmlTree::CFrag saved_;     // Saved copy of the children (before editing) in case Cancel pressed
	bool saved_mod_flag_;       // If dialog cancelled then we have to restore the modified status
	bool show_prev_next_;       // Show the << and >> buttons (parent is STRUCT)
	CPoint pos_;                // Position of this dialog
	CPoint orig_;               // Position of the first dialog opened
	bool modified_;             // Have any of the values been changed?

	CMenu button_menu_;         // We need to keep the menu for the Replace/Insert button in memory
	bool first_;                // Is this the first dialog called (since any DFFD dialog can call any other)
	signed char parent_type_;   // Type of container of this element
	CMenu *pexpr_menu_;         // Menu displaying all availbale vars for the switch "expression"

	void            do_edit(bool delete_on_cancel = false);
	void            load_data();
	bool            check_data();
	void            save_data();
};

#endif // DFFDSWITCH_INCLUDED
