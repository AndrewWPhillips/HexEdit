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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <vector>
#include "CalcEdit.h"
#include "Expr.h"
#include "optypes.h"
#include "ResizeCtrl.h"
#include "mpirxx.h"

// Just override CMFCButton so we can ceck the button colour
class CCalcButton : public CMFCButton
{
public:
	COLORREF GetTextColor() { return m_clrRegular; }
};

/////////////////////////////////////////////////////////////////////////////
// CCalcBits
//
// This is a child window of the calculator dialog that shows the current status
// of the 64 bits of the current calcualtor integer value.  It indicates if on/off
// or if disabled (depends on number of calculator bits in use).  It also
// allows (enabled) bits to be toggled just by clicking them.

class CCalcBits : public CWnd
{
	DECLARE_DYNAMIC(CCalcBits)

public:
	CCalcBits(CCalcDlg * pParent) : m_pParent(pParent) { }

protected:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP()

private:
	void calc_widths(CRect & rct);
	int spacing(int bnum);
	int pos2bit(CPoint point);

	CCalcDlg * m_pParent;   // owning calc dlg, so we can get bits etc

	int m_ww, m_nn, m_bb, m_cc;  // widths for size/spacing of bit display
	int m_hh;                    // height of each bit display
};

/////////////////////////////////////////////////////////////////////////////
// CCalcDlg dialog
class CMainFrame;

class CCalcDlg : public CDialog
{
	friend class CCalcEdit;             // Edit box where values/expressions are entered and results displayed
	friend class CCalcBits;             // Displays 64 boxes (for the current bits) underneath the edit box
	friend class CHexEditApp;           // Allows macros to call protected members

// Construction
public:
	CCalcDlg(CWnd* pParent = NULL);   // standard constructor

	virtual BOOL Create(CWnd* pParentWnd = NULL);   // Normal way to create the modeless dialog
	CSize m_sizeInitial;

	void FinishMacro() { save_value_to_macro(); }// Tidy up any pending ops if macro is finishing

	void StartEdit();            // Set focus to text control and move caret to end
	bool IsVisible() { return (GetStyle() & WS_VISIBLE) != 0; }
	void Set(mpz_class t) { current_ = t; state_ = CALCINTUNARY; edit_.put(); edit_.get(); set_right(); }
	void Set(unsigned __int64 v) { mpz_set_ui64(current_.get_mpz_t(), v); state_ = CALCINTUNARY; edit_.put(); edit_.get(); set_right(); }
	void SetStr(CString ss)  { current_str_ = ss; state_ = CALCOTHRES; edit_.put(); state_ = edit_.update_value(false); }
	void SetFromFile(FILE_ADDRESS addr);
	void set_right();

	void change_base(int base);    // set radix (2 to 36)
	void change_signed(bool s);    // set whether numbers are signed or unsigned
	void change_bits(int);         // change how many bits are used (0 = unlimited)

	int ByteSize() const { return (bits_-1)/8 + 1; }
	int get_bits() const { return bits_; }
	unsigned __int64 GetValue() const { return mpz_get_ui64(get_norm(current_).get_mpz_t()); }
	CString GetStringValue() const { char * ss = mpz_get_str(NULL, 10, get_norm(current_).get_mpz_t()); CString retval(ss); free(ss); return retval; }

// Dialog Data
	enum { IDD = IDD_CALC };

	CCalcComboBox ctl_edit_combo_;  // Combo box with main edit controls
	CCalcEdit edit_;                // Subclassed edit control (child of above combo)
	CCalcBits ctl_calc_bits_;       // Just for drawing the bits

	// Calculator buttons
	CCalcButton ctl_digit_0_;
	CCalcButton ctl_digit_1_;
	CCalcButton ctl_digit_2_;
	CCalcButton ctl_digit_3_;
	CCalcButton ctl_digit_4_;
	CCalcButton ctl_digit_5_;
	CCalcButton ctl_digit_6_;
	CCalcButton ctl_digit_7_;
	CCalcButton ctl_digit_8_;
	CCalcButton ctl_digit_9_;
	CCalcButton ctl_digit_a_;
	CCalcButton ctl_digit_b_;
	CCalcButton ctl_digit_c_;
	CCalcButton ctl_digit_d_;
	CCalcButton ctl_digit_e_;
	CCalcButton ctl_digit_f_;
	CCalcButton ctl_mem_get_;
	CCalcButton ctl_mark_get_;
	CCalcButton ctl_sel_get_;
	CCalcButton ctl_sel_at_;
	CCalcButton ctl_sel_len_;
	CCalcButton ctl_mark_at_;
	CCalcButton ctl_eof_get_;

	CCalcButton ctl_backspace_;
	CCalcButton ctl_clear_entry_;
	CCalcButton ctl_clear_;
	CCalcButton ctl_equals_;
	CCalcButton ctl_and_;
	CCalcButton ctl_asr_;
	CCalcButton ctl_divide_;
	CCalcButton ctl_lsl_;
	CCalcButton ctl_lsr_;
	CCalcButton ctl_rol_;
	CCalcButton ctl_ror_;
	CCalcButton ctl_xor_;
	CCalcButton ctl_mod_;
	CCalcButton ctl_multiply_;
	CCalcButton ctl_or_;
	CCalcButton ctl_unary_dec_;
	CCalcButton ctl_unary_factorial_;
	CCalcButton ctl_unary_flip_;
	CCalcButton ctl_unary_rev_;
	CCalcButton ctl_unary_inc_;
	CCalcButton ctl_unary_not_;
	CCalcButton ctl_unary_sign_;
	CCalcButton ctl_unary_square_;
	CCalcButton ctl_subtract_;
	CCalcButton ctl_mem_clear_;
	CCalcButton ctl_mem_add_;
	CCalcButton ctl_mem_subtract_;
	CCalcButton ctl_mark_add_;
	CCalcButton ctl_mark_subtract_;
	CCalcButton ctl_gtr_;
	CCalcButton ctl_less_;
	CCalcButton ctl_pow_;
	CCalcButton ctl_mark_at_store_;
	CCalcButton ctl_mark_clear_;
	CCalcButton ctl_mark_store_;
	CCalcButton ctl_mem_store_;
	CCalcButton ctl_sel_at_store_;
	CCalcButton ctl_sel_store_;
	CCalcButton ctl_unary_rol_;
	CCalcButton ctl_unary_ror_;
	CCalcButton ctl_unary_lsl_;
	CCalcButton ctl_unary_lsr_;
	CCalcButton ctl_unary_asr_;
	CCalcButton ctl_unary_cube_;
	CCalcButton ctl_unary_at_;
	CCalcButton ctl_unary_square_root_;
	CCalcButton ctl_sel_len_store_;
	CCalcButton ctl_addop_;
	CMFCMenuButton ctl_go_;

	// Menu buttons
	CMFCMenuButton ctl_hex_hist_;
	CMFCMenuButton ctl_dec_hist_;
	CMFCMenuButton ctl_vars_;
	CMFCMenuButton ctl_func_;

	CMenu func_menu_;  // For functions
	CMenu go_menu_;    // Mneu for go button (go address, sector etc)

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
	afx_msg void OnInfbit();
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
	afx_msg void OnSelHistory();
	//afx_msg void OnDeleteItem(int nIDCtl, DELETEITEMSTRUCT *pdis);
	afx_msg void OnChangeRadix();
	afx_msg void OnChangeBits();
	afx_msg void OnChangeSigned();

	DECLARE_MESSAGE_MAP()

private:
	bool inited_;
	// The calculator has several "states" depending on what has just happened (such as the "="
	// button having just been pressed to display a result) and what charcters have been typed.
	// The state is normally obvious to the user from what they have done, what is shown in the
	// edit box and the "status" (IDC_OP_DISPLAY) shown to the right of the edit box
	enum CALCSTATE state_;

	//bool invalid_expression();              // Check if current expression is valid and show error message if not

	CToolTipCtrl ttc_;                      // For button tooltips
	HWND help_hwnd_;                        // HWND of window for which context help is pending (usually 0)

	void save_value_to_macro(CString ss = CString());
	void do_binop(binop_type binop);        // Handle binary operators
	void do_unary(unary_type unary);        // Handle unary operators
	void do_digit(char digit);              // Handle "digit" button on calculator
	void calc_binary();                     // Do binary operation using previous_, current_ and op_
	void calc_unary(unary_type unary);      // Do unary operation of current_ (previous_ and op_ are unaffected)
	void add_hist();                        // Add to drop-down history list
	ExprStringType get_expr(bool no_paren = false); // Get an expression that represents current calc value (if = used) including all binary/unary operations
	void fix_values();                      // Update min, max etc based on latest settings (radix_, signed_)
	bool put_bytes(FILE_ADDRESS addr);      // Write calculator bytes to file at addr (or special locations for -ve values)
	bool get_bytes(FILE_ADDRESS addr);      // Load bytes into calculator from current file address (or special locations for -ve values)

	CHexEditApp *aa_;                       // Ptr to the app (mainly for macro handling)
	CMainFrame *mm_;                        // Ptr to the main window

	// The following is used to keep track of where the current calculator contents came from.
	// That is, was the number typed in by the user or obtained using one of the buttons on
	// the left side of the calculator (memory recall etc).  This info is needed for later
	// playback of macros that use the calculator.  The value of source_ should be one of
	// km_user_str, km_memget, km_markat/get, km_selat/get, km_selend, km_sellen, km_eofget.

	km_type source_;                        // One of km_user_str to km_eofget, or km_result

	// The following is used to keep track of what binary operator is in use.  For example,
	// if the user does 123 + 456 then it has value km_add.  If no binary operation is currently
	// active then is has value binop_none.
	binop_type op_;

	void inedit(km_type kk)
	{
		source_ = theApp.recording_ ? kk : km_result;   // where to get value from in playback
	}
#if 0
	BOOL in_edit_;                      // Can we edit the numbers in the edit box or should next edit clear it (eg. after "=" used)
	void inedit(km_type kk)
	{
		in_edit_ = TRUE;                // signal that we are editing
		source_ = theApp.recording_ ? kk : km_result;   // where to get value from in playback
		edit_.sel_ = (DWORD)-1;         // prevent selection being changed now
	}

	// We need to know if there is an expression or a simple integer constant currently
	// in the edit box.  This is necessary for backward compatibility (when expression
	// evaluation added to the calculator).  Integer constants may have separator
	// characters added.  Also we need to be able to set the current calculator value
	// if it has been changed in a macro (refresh off) at the end of the macro.
	BOOL current_const_;            // TRUE if the current "expression" is an integer constant
	expr_eval::type_t current_type_; // Type of current expression (TYPE_NONE if invalid)

	BOOL overflow_;                 // Has a numeric overflow occurred?
	BOOL error_;                    // Has a numeric error (eg. Div by 0) occurred?
#endif

	mpz_class get_norm(mpz_class v) const; // Get a value converted according to bits_ and signed_

	// Calculator values: current displayed value, 2nd value (for binop) and calc memory value
	// Notes: Internally mpz big integers are stored as a bit pattern (with implicit leading zeroes) and a separate sign.
	//        If signed_ == false then the mpz values should be +ve or zero (mpz_sgn >= 0)
	//        If signed_ == true then the mpz value can be -ve in which case we have to negate the value to get the 2's complement bit pattern
	mpz_class current_;            // Current value (always +ve) - affected by bits_, signed_ and radix_ when displayed in edit box
	mpz_class previous_;           // Previous value of edit control (if binary operator active)

	mpz_class memory_;             // Current contents of memory (used with Memory, MS, MC, etc buttons)

	ExprStringType current_str_;           // Contains the result of the expression or error message (if current_type_ == TYPE_NONE)

	// The following are used for checking the current value (eg for overflow) but are
	// only relevant when bits_ > 0.
	mpz_class min_val_, max_val_;  // Range of valid values according to bits_ and signed_
	mpz_class mask_;               // Mask for current value of bits_, typically: 0xFF, 0xFFFF, 0xffffFFFF etc

	// The following are used to display the current expression as the user uses binary/unary operations etc, and
	// also to write to the history list.  left_ represents the expression that was used to generate previous_ (the
	// left side of the active binary operation) and does not change until calc_binary is called (or Clear button used).
	// right_ represents the expression that was used to generate the current calculator value (current_/current_str_),
	// including any unary operations.  right_ may be completely changed by typing a new number, getting from memory etc.
	ExprStringType left_, right_;

	// Current state info
	int radix_;                     // Actual radix (base) in use (usually 2,8,10, or 16 but may be any value 2-36
	int orig_radix_;                // Radix at start of expression (used to generate an history expression all in the same radix for consistency)
	int bits_;                      // Numbers of bits in use: (usually 8, 16, 32 or 64), the special value of 0 means unlimited
	bool signed_;                   // For now only decimal (base 10) numbers are shown signed but later we will allow a signed checkbox
                                    // Note: for unlimited bits (bits_ == 0) -ve numbers are shown as such (ie leading minus sign) as we can't show an infinite number of FF's

	bool m_first;                   // Remember first call to OnKickIdle (we can't add the controls to the resizer till then)
	CResizeCtrl m_resizer;          // Used to move controls around when the window is resized
	static CBrush * m_pBrush;       // brush used for background
	CPen purple_pen;                // Just to draw square root symbol
	CFont small_fnt_, med_fnt_;     // Diff sized fonts used when window resized

	void check_for_error();         // Check for integer error/overflow conditions and inform the user
	void no_file_error();           // Display an error when a button is used that operates on a file but no file is open
	void not_int_error();           // Display error when value is not an int (eg expression), button used requires an int
	void make_noise(const char *ss);// Make Windows noise or beep on error
	void setup_tooltips();          // set up tooltips for buttons
	void setup_expr();
	void setup_static_buttons();    // Unchanging buttons
	void setup_resizer();           // set up resizing of controls when window is resized
	void update_digit_buttons();    // fix basic buttons (digits etc)
	void update_file_buttons();     // fix file related buttons
	void update_controls();         // set up all controls
	void update_modes();            // Radix, bits, signed controls
	void update_expr();             // Displays the "expression" that would generate the current calc value
	void update_op();               // Update the "operator" display to the right of the edit box

	void build_menus();             // Build menus for menu buttons
	void button_colour(CWnd *pp, bool enable, COLORREF normal); // Make button greyed/normal
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CALCDLG_H__DA8B4B10_D7C4_11D3_A23F_0020AFDC3196__INCLUDED_)
