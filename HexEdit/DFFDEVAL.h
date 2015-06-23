// DFFDEval.h
//
// Copyright (c) 2015 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#ifndef DFFDEVAL_H_
#define DFFDEVAL_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "XMLTree.h"

// DFFDEVAL.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDFFDEval dialog

class CDFFDEval : public CDialog
{
// Construction
public:
	CDFFDEval(CXmlTree::CElt *pp, signed char parent_type, CWnd* pParent = NULL);   // standard constructor
	virtual ~CDFFDEval();

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
	//{{AFX_DATA(CDFFDEval)
	enum { IDD = IDD_DFFD_EVAL };
	CButton ctl_prev_;
	CButton ctl_next_;
	CString expr_;
	int     display_result_;
	int     display_error_;
	CString comment_;
	//}}AFX_DATA
	CEdit ctl_expr_;
	CMFCMenuButton ctl_expr_var_;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDFFDEval)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDFFDEval)
	afx_msg void OnNext();
	afx_msg void OnPrev();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnChange();
	virtual void OnCancel();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
	afx_msg void OnHelp();
	afx_msg void OnGetExprVar();
	DECLARE_MESSAGE_MAP()

	CXmlTree::CElt *pelt_;      // The EVAL element we are editing
	CXmlTree::CFrag saved_;     // Saved copy of the children (before editing) in case Cancel pressed
	bool saved_mod_flag_;       // If dialog cancelled then we have to restore the modified status
	bool show_prev_next_;       // Show the << and >> buttons (parent is STRUCT)
	CPoint pos_;                // Position of this dialog
	CPoint orig_;               // Position of the first dialog opened
	bool modified_;             // Have any of the values been changed?
	bool first_;                // Is this the first dialog called (since any DFFD dialog can call any other)
	signed char parent_type_;   // Type of container of this element
	CMenu *pexpr_menu_;         // Menu displaying all availbale vars for "expr"

	void            load_data();
	bool            check_data();
	void            save_data();
};

#endif
