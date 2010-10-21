// DFFDData.h
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

#if !defined(AFX_DFFDDATA_H__34A0C1CD_8F7E_4BBB_A2A8_5C19F91CDE13__INCLUDED_)
#define AFX_DFFDDATA_H__34A0C1CD_8F7E_4BBB_A2A8_5C19F91CDE13__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "XMLTree.h"

// DFFDData.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDFFDData dialog

class CDFFDData : public CDialog
{
// Construction
public:
	CDFFDData(CXmlTree::CElt *pp, signed char parent_type, CWnd* pParent = NULL);   // standard constructor
	virtual ~CDFFDData();

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
	//{{AFX_DATA(CDFFDData)
	enum { IDD = IDD_DFFD_DATA };
	CEdit   ctl_name_;
	CButton ctl_prev_;
	CButton ctl_next_;
	CEdit   ctl_display_;
	CButton ctl_big_endian_;
	CStatic ctl_units_desc_;
	CEdit   ctl_units_;
	CComboBox   ctl_type_;
	CButton ctl_read_inly_;
	CStatic ctl_length_desc_;
	CEdit   ctl_length_;
	CStatic ctl_domain_desc_;
	CComboBox   ctl_format_;
	CStatic ctl_format_desc_;
	CEdit   ctl_domain_;
	CStatic ctl_display_desc_;
	CString comments_;
	CString domain_;
	CString name_;
	CString type_name_;
	CString units_;
	int     big_endian_;
	CString display_;
	int     read_only_;
	int     type_;
	int     format_;
	CString length_;
	//}}AFX_DATA
	CMFCMenuButton ctl_length_var_;
	CMFCMenuButton ctl_domain_var_;
	CButton ctl_clone_;

	int bitfield_;                  // 1 if this is a bit-field
	CButton ctl_bitfield_;
	UINT bits_;
	CEdit ctl_bits_;
	int dirn_;
	CComboBox ctl_dirn_;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDFFDData)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDFFDData)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnNext();
	afx_msg void OnPrev();
	afx_msg void OnChange();
	virtual void OnCancel();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	afx_msg void OnSelchangeType();
	afx_msg void OnSelchangeFormat();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
	afx_msg void OnHelp();
	afx_msg void OnGetLengthVar();
	afx_msg void OnGetDomainVar();
	afx_msg void OnClone();
	afx_msg void OnChangeLength();
	afx_msg void OnBitfield();
	DECLARE_MESSAGE_MAP()

	CXmlTree::CElt *pelt_;
	CXmlTree::CFrag saved_;     // Saved copy of the children (before editing) in case Cancel pressed
	bool saved_mod_flag_;       // If dialog cancelled then we have to restore the modified status
	bool show_prev_next_;       // Show << and >> buttons to move between sibling (if parent is struct)
	CPoint pos_;                // Position of this dialog
	CPoint orig_;               // Position of the first dialog opened
	bool modified_;
	CMenu button_menu_;         // Contains menus for selection of lengths of integer and reals
	bool first_;                // Is this the first dialog called (since any DFFD dialog can call any other)
	signed char parent_type_;   // Type of container of this element
	CMenu *plength_menu_;       // Menu displaying all available vars for "length"
	CMenu *pdomain_menu_;       // Menu displaying all available vars for "domain"

	void            fix_formats();
	void            fix_controls();
	void            fix_big_endian();
	void            load_data();
	bool            check_data();
	void            save_data();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DFFDDATA_H__34A0C1CD_8F7E_4BBB_A2A8_5C19F91CDE13__INCLUDED_)
