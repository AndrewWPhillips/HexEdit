#if !defined(DFFDUSESTRUCT_H_)
#define DFFDUSESTRUCT_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "XMLTree.h"

// DFFDUseStruct.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDFFDUseStruct dialog

class CDFFDUseStruct : public CDialog
{
// Construction
public:
    CDFFDUseStruct(CXmlTree::CElt *pp, signed char parent_type, CWnd* pParent = NULL);   // standard constructor
    virtual ~CDFFDUseStruct();

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
    //{{AFX_DATA(CDFFDUseStruct)
    enum { IDD = IDD_DFFD_USE_STRUCT };
    CEdit   ctl_name_;
    CButton ctl_prev_;
    CButton ctl_next_;
    CString name_;
    CString expr_;
    CString struct_;
    CString comments_;
    //}}AFX_DATA
    CEdit ctl_struct_;
    CMFCMenuButton ctl_struct_var_;

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDFFDUseStruct)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CDFFDUseStruct)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnNext();
    afx_msg void OnPrev();
    afx_msg void OnChange();
    virtual void OnCancel();
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    //}}AFX_MSG
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
	afx_msg void OnHelp();
    afx_msg void OnGetStructVar();
    DECLARE_MESSAGE_MAP()

    CXmlTree::CElt *pelt_;
    CXmlTree::CFrag saved_;     // Saved copy of the children (before editing) in case Cancel pressed
    bool saved_mod_flag_;       // If dialog cancelled then we have to restore the modified status
    bool show_prev_next_;       // Show << and >> buttons to move between sibling (if parent is struct)
    CPoint pos_;                // Position of this dialog
    CPoint orig_;               // Position of the first dialog opened
    bool modified_;
    bool first_;                // Is this the first dialog called (since any DFFD dialog can call any other)
    signed char parent_type_;   // Type of container of this element
    CMenu *pstruct_menu_;

    void            load_data();
    bool            check_data();
    void            save_data();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DFFDDATA_H__34A0C1CD_8F7E_4BBB_A2A8_5C19F91CDE13__INCLUDED_)
