// CalcDlg.h : header file for Calculator Dialog
//
// Copyright (c) 2000-2010 by Andrew W. Phillips.
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

#if !defined(AFX_CALCDLG_H__DA8B4B10_D7C4_11D3_A23F_0020AFDC3196__INCLUDED_)
#define AFX_CALCDLG_H__DA8B4B10_D7C4_11D3_A23F_0020AFDC3196__INCLUDED_

#include <vector>
#include "CalcEdit.h"
#include "Expr.h"
#include "optypes.h"
#include "ResizeCtrl.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CalcDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCalcBits

class CCalcBits : public CStatic
{
	DECLARE_DYNAMIC(CCalcBits)

public:
	CCalcBits(CCalcDlg * pParent) : m_pParent(pParent) { }

protected:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	DECLARE_MESSAGE_MAP()

private:
	CCalcDlg * m_pParent;   // owning calc dlg, so we can get bits etc

	int m_ww, m_nn, m_bb, m_cc;  // widths for size/spacing of bit display
	int m_hh;                    // height of each bit display
};

/////////////////////////////////////////////////////////////////////////////
// CCalcDlg dialog
class CMainFrame;

class CCalcDlg : public CDialog
{
    friend class CCalcEdit;
	friend class CCalcBits;
    friend class CHexEditApp;           // Allows macros to call protected members

// Construction
public:
    CCalcDlg(CWnd* pParent = NULL);   // standard constructor

    virtual BOOL Create(CWnd* pParentWnd = NULL);   // Normal way to create the modeless dialog
	CSize m_sizeInitial;

    void Redisplay();
    void FinishMacro();
    void ShowStatus();
    void ShowBinop(int ii = -1);
    void FixFileButtons();
    void StartEdit();           // Set focus to text control and move caret to end
	void update_controls();
	bool IsVisible() { return (GetStyle() & WS_VISIBLE) != 0; }
    void Set(unsigned __int64 v) { current_ = v; if (IsVisible()) edit_.Put(); }
    void change_base(int);
    void change_bits(int);
    int ByteSize() const { return (bits_-1)/8 + 1; }
    unsigned __int64 GetValue() const { return current_ & mask_; }

// Dialog Data
    enum { IDD = IDD_CALC_NEW };
    int     bits_index_;
    int     base_index_;

    CComboBox ctl_edit_combo_;      // Combo box with main edit controls
    CCalcEdit edit_;                // Subclassed edit control (child of above combo)
	CCalcBits ctl_calc_bits_;       // Just for drawing the bits

	// Calculator buttons
#if 1
	// These are black so we don't have to do anything special
    CMFCButton ctl_digit_0_;
    CMFCButton ctl_digit_1_;
    CMFCButton ctl_digit_2_;
    CMFCButton ctl_digit_3_;
    CMFCButton ctl_digit_4_;
    CMFCButton ctl_digit_5_;
    CMFCButton ctl_digit_6_;
    CMFCButton ctl_digit_7_;
    CMFCButton ctl_digit_8_;
    CMFCButton ctl_digit_9_;
    CMFCButton ctl_digit_a_;
    CMFCButton ctl_digit_b_;
    CMFCButton ctl_digit_c_;
    CMFCButton ctl_digit_d_;
    CMFCButton ctl_digit_e_;
    CMFCButton ctl_digit_f_;
	CMFCButton ctl_mem_get_;
	CMFCButton ctl_mark_get_;
	CMFCButton ctl_sel_get_;
	CMFCButton ctl_sel_at_;
	CMFCButton ctl_sel_len_;
	CMFCButton ctl_mark_at_;
	CMFCButton ctl_eof_get_;
#endif
	CMFCButton ctl_backspace_;
	CMFCButton ctl_clear_entry_;
	CMFCButton ctl_clear_;
	CMFCButton ctl_equals_;
	CMFCButton ctl_and_;
	CMFCButton ctl_asr_;
	CMFCButton ctl_divide_;
	CMFCButton ctl_lsl_;
	CMFCButton ctl_lsr_;
	CMFCButton ctl_rol_;
	CMFCButton ctl_ror_;
	CMFCButton ctl_xor_;
	CMFCButton ctl_mod_;
	CMFCButton ctl_multiply_;
	CMFCButton ctl_or_;
	CMFCButton ctl_unary_dec_;
	CMFCButton ctl_unary_factorial_;
	CMFCButton ctl_unary_flip_;
	CMFCButton ctl_unary_rev_;
	CMFCButton ctl_unary_inc_;
	CMFCButton ctl_unary_not_;
	CMFCButton ctl_unary_sign_;
	CMFCButton ctl_unary_square_;
	CMFCButton ctl_subtract_;
	CMFCButton ctl_mem_clear_;
	CMFCButton ctl_mem_add_;
	CMFCButton ctl_mem_subtract_;
	CMFCButton ctl_mark_add_;
	CMFCButton ctl_mark_subtract_;
	CMFCButton ctl_gtr_;
	CMFCButton ctl_less_;
	CMFCButton ctl_pow_;
	CMFCButton ctl_mark_at_store_;
	CMFCButton ctl_mark_clear_;
	CMFCButton ctl_mark_store_;
	CMFCButton ctl_mem_store_;
	CMFCButton ctl_sel_at_store_;
	CMFCButton ctl_sel_store_;
	CMFCButton ctl_unary_rol_;
	CMFCButton ctl_unary_ror_;
	CMFCButton ctl_unary_lsl_;
	CMFCButton ctl_unary_lsr_;
	CMFCButton ctl_unary_asr_;
	CMFCButton ctl_unary_cube_;
	CMFCButton ctl_unary_at_;
	CMFCButton ctl_unary_square_root_;
	CMFCButton ctl_sel_len_store_;
	CMFCButton ctl_addop_;
	CMFCMenuButton ctl_go_;

	// Menu buttons
    CMFCMenuButton ctl_hex_hist_;
    CMFCMenuButton ctl_dec_hist_;
    CMFCMenuButton ctl_vars_;
    CMFCMenuButton ctl_func_;

	CMenu func_menu_;  // For functions
	CMenu go_menu_;    // Mneu for go button (go address, sector etc)

	// We need to know if there is an expression or a simple integer constant currently
	// in the edit box.  This is necessary for backward compatibility (when expression
	// evaluation added to the calculator).  Integer constants may have separator
	// characters added.  Also we need to be able to set the current calculator value
	// if it has been changed in a macro (refresh off) at the end of the macro.
	BOOL current_const_;            // TRUE if the current "expression" is an integer constant
	expr_eval::type_t current_type_; // Type of current expression (TYPE_NONE if invalid)
	ExprStringType current_str_;           // Contains the result of the expression or error message (if current_type_ == TYPE_NONE)

    BOOL overflow_;                 // Has a numeric overflow occurred?
    BOOL error_;                    // Has a numeric error (eg. Div by 0) occurred?

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CCalcDlg)
    public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    //}}AFX_VIRTUAL

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
    afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
	afx_msg LRESULT HandleInitDialog(WPARAM, LPARAM);
    // Generated message map functions
    //{{AFX_MSG(CCalcDlg)
    afx_msg void OnDigit0();
    afx_msg void OnDigit1();
    afx_msg void OnDigit2();
    afx_msg void OnDigit3();
    afx_msg void OnDigit4();
    afx_msg void OnDigit5();
    afx_msg void OnDigit6();
    afx_msg void OnDigit7();
    afx_msg void OnDigit8();
    afx_msg void OnDigit9();
    afx_msg void OnDigitA();
    afx_msg void OnDigitB();
    afx_msg void OnDigitC();
    afx_msg void OnDigitD();
    afx_msg void OnDigitE();
    afx_msg void OnDigitF();
    afx_msg void OnBackspace();
    afx_msg void OnClearEntry();
    afx_msg void OnClear();
    afx_msg void OnEquals();
    afx_msg void OnAnd();
    afx_msg void OnAsr();
    afx_msg void OnDivide();
    afx_msg void OnLsl();
    afx_msg void OnLsr();
    afx_msg void OnRol();
    afx_msg void OnRor();
    afx_msg void OnXor();
    afx_msg void OnMod();
    afx_msg void OnMultiply();
    afx_msg void OnOr();
    afx_msg void OnUnaryDec();
    afx_msg void OnUnaryFactorial();
    afx_msg void OnUnaryFlip();
    afx_msg void OnUnaryRev();
    afx_msg void OnUnaryInc();
    afx_msg void OnUnaryNot();
    afx_msg void OnUnarySign();
    afx_msg void OnUnarySquare();
    afx_msg void OnSubtract();
    afx_msg void OnGo();
    afx_msg void OnMemGet();
    afx_msg void OnMemClear();
    afx_msg void OnMemAdd();
    afx_msg void OnMemSubtract();
    afx_msg void OnMarkGet();
    afx_msg void OnMarkAt();
    afx_msg void OnMarkAdd();
    afx_msg void OnMarkSubtract();
    afx_msg void OnSelGet();
    afx_msg void OnSelAt();
    afx_msg void OnEofGet();
    afx_msg void OnGtr();
    afx_msg void OnLess();
    afx_msg void OnPow();
    afx_msg void OnMarkAtStore();
    afx_msg void OnMarkClear();
    afx_msg void OnMarkStore();
    afx_msg void OnMemStore();
    afx_msg void OnSelAtStore();
    afx_msg void OnSelStore();
    afx_msg void OnUnaryRol();
    afx_msg void OnUnaryRor();
    afx_msg void OnUnaryLsl();
    afx_msg void OnUnaryLsr();
    afx_msg void OnUnaryAsr();
    afx_msg void OnUnaryCube();
    afx_msg void OnUnaryAt();
    afx_msg void OnUnarySquareRoot();
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
    afx_msg void On8bit();
    afx_msg void On16bit();
    afx_msg void On32bit();
    afx_msg void On64bit();
    afx_msg void OnBinary();
    afx_msg void OnOctal();
    afx_msg void OnDecimal();
    afx_msg void OnHex();
    afx_msg void OnSelLen();
    afx_msg void OnSelLenStore();
    afx_msg void OnClose();
    afx_msg void OnAdd();
    afx_msg void OnDestroy();
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    afx_msg void OnBigEndian();
    //}}AFX_MSG
    afx_msg void OnGetHexHist();
    afx_msg void OnGetDecHist();
    afx_msg void OnGetVar();
    afx_msg void OnGetFunc();

    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnTooltipsShow(NMHDR* pNMHDR, LRESULT* pResult);

	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

    DECLARE_MESSAGE_MAP()

private:
    CToolTipCtrl ttc_;                      // For button tooltips
	HWND help_hwnd_;                        // HWND of window for which context help is pending (usually 0)

	bool invalid_expression();              // Check if current expression is valid and show error message if not
	void build_menus();                     // Build menus for menu buttons
    void toggle_endian();
    void do_binop(binop_type binop);        // Handle binary operators
    void do_unary(unary_type unary);        // Handle unary operators
    void do_digit(char digit);              // Handle "digit" button on calculator
    void calc_previous();                   // Do binary operation using previous_ and current_
    void add_hist();                        // Add to drop-down history list

    CHexEditApp *aa_;                       // Ptr to the app (mainly for macro handling)
    CMainFrame *mm_;                        // Ptr to the main window

    CPen purple_pen;                        // Just to draw square root symbol

    // The following is used to keep track of where the current calculator contents came from.
    // That is, was the number typed in by the user or obtained using one of the buttons on
    // the left side of the calculator (memory recall etc).  This info is needed for later
    // playback of macros that use the calculator.  The value of source_ should be one of km_user,
    // km_memget, km_markget, km_markat, km_selget, km_selat, km_selend, km_sellen, km_eofget.

    km_type source_;                        // One of km_user to km_eofget, or km_result

    // The following is used to keep track of what binary operator is in use.  For example,
    // if the user does 123 + 456 then it has value km_add.  If no binary operation is currently
    // active then is has value binop_none.
    binop_type op_;

    BOOL in_edit_;                      // Can we edit the numbers in the edit box or should next edit clear it (eg. after "=" used)
    void inedit(km_type kk)
    {
		in_edit_ = TRUE;                // signal that we are editing
		source_ = theApp.recording_ ? kk : km_result;   // where to get value from in playback
        edit_.sel_ = (DWORD)-1;         // prevent selection being changed now
    }

    // Calculator values: current displayed value, 2nd value (for binop) and calc memory value
    unsigned __int64 current_;      // Current value in the edit control (used by edit_)
    unsigned __int64 previous_;     // Previous value of edit control (if binary operator active)

    unsigned __int64 memory_;       // Current contents of memory (used with Memory, MS, MC, etc buttons)

    // Other calculator state info
    int radix_;                     // Actual radix (base) in use (usually 2,8,10, or 16 but may be any value 2-36
    int bits_;                      // Numbers of bits in use: 8, 16, 32 or 64
    unsigned __int64 mask_;         // Mask for current value of bits_: 0xFF, 0xFFFF, 0xffffFFFF or 0xffffFFFFffffFFFF
    unsigned __int64 sign_mask_;    // Mask to check sign bit of bits_: 0x80, 0x8000, 0x80000000 or 0x8000000000000000

	bool m_first;                   // Remember first call to OnKickIdle (we can't add the controls to the resizer till then)
	CResizeCtrl m_resizer;          // Used to move controls around when the window is resized
	static CBrush * m_pBrush;       // brush used for background
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CALCDLG_H__DA8B4B10_D7C4_11D3_A23F_0020AFDC3196__INCLUDED_)
