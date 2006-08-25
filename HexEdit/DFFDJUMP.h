// DFFDJump.h
//
// Copyright (c) 2004 by Andrew W. Phillips.
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

#ifndef DFFDJUMP_H_
#define DFFDJUMP_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "XMLTree.h"

// DFFDJump.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDFFDJump dialog

class CDFFDJump : public CDialog
{
// Construction
public:
    CDFFDJump(CXmlTree::CElt *pp, signed char parent_type, CWnd* pParent = NULL);   // standard constructor
    virtual ~CDFFDJump();

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
    //{{AFX_DATA(CDFFDJump)
    enum { IDD = IDD_DFFD_JUMP };
    CButton ctl_edit_;
    CButton ctl_prev_;
    CButton ctl_next_;
    CString comment_;
    CString offset_;
    CString elt_name_;
    //}}AFX_DATA
    CBCGMenuButton  ctl_replace_;
    CEdit ctl_offset_;
    CBCGMenuButton ctl_offset_var_;
    int origin_;    // 0 = start of file, 1 = current, 2 = end of file

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDFFDJump)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CDFFDJump)
    afx_msg void OnNext();
    afx_msg void OnPrev();
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    afx_msg void OnChange();
    afx_msg void OnEdit();
    afx_msg void OnReplace();
    virtual void OnCancel();
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    //}}AFX_MSG
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
	afx_msg void OnHelp();
    afx_msg void OnGetOffsetVar();
    DECLARE_MESSAGE_MAP()

    CXmlTree::CElt *pelt_;      // The JUMP element we are editing
    CXmlTree::CFrag saved_;     // Saved copy of the children (before editing) in case Cancel pressed
    bool saved_mod_flag_;       // If dialog cancelled then we have to restore the modified status
    bool show_prev_next_;       // Show the << and >> buttons (parent is STRUCT)
    CPoint pos_;                // Position of this dialog
    CPoint orig_;               // Position of the first dialog opened
    bool modified_;             // Have any of the values been changed?
    bool valid_element_;        // Is there a sublement for the JUMP (if valid must have exactly 1)
    CMenu button_menu_;         // We need to keep the menu for the Replace/Insert button in memory
    bool first_;                // Is this the first dialog called (since any DFFD dialog can call any other)
    signed char parent_type_;   // Type of container of this element
    CMenu *poffset_menu_;       // Menu displaying all available vars for "offset"

    void            load_data();
    bool            check_data();
    void            save_data();
};

#endif // DFFDJUMP_H_
