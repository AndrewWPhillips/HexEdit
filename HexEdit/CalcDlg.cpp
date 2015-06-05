// CalcDlg.cpp : implements the Goto/Calculator dialog
//
// Copyright (c) 2000-2011 by Andrew W. Phillips.
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

// Notes
// The calculator dialog is handled here (CCalcDlg) but there are some related classes:
// CCalcEdit - handles the text box where values are entered and results displayed
// CCalcComboBox - combo that contains the edit box and provides the history list
// CCalcListBox - list part of combo (used for displaying a tip window with result value)
// CCalcBits - displays boxes showinh the bottom 64 bits (in an integer value)
// CCalcButton - base class for all calculator buttons
//
// The "current value" of the calculator is stored in this class (CCalcDlg) in:
//   state_ - says whether the value is an integer or other type of value, is a literal
//            or expression, is a result (including error/overflow) or is being edited
//   current_ - current value if it's a simple integer literal (ie, state_ <= CALCINTLIT)
//   current_str_ - current value as a string if not an integer or is an int expression
// Also if there is a pending binary operation then these members are also relevant:
//   previous_ - values of left side of operation
//   op_ - the binary operation
//
// The edit box is used to display results by setting current/current_str_ and state_
// and calling CCalcEdit::put.
//
#include "stdafx.h"
#include <cmath>
#include "HexEdit.h"
#include "MainFrm.h"
#include "HexEditDoc.h"
#include "HexEditView.h"
#include "CalcDlg.h"
#include "Misc.h"
#include "SystemSound.h"
#include "resource.hm"      // Help IDs

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCalcBits

IMPLEMENT_DYNAMIC(CCalcBits, CWnd)

BEGIN_MESSAGE_MAP(CCalcBits, CWnd)
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

void CCalcBits::OnSize(UINT nType, int cx, int cy)   // WM_SIZE
{
	CWnd::OnSize(nType, cx, cy);
	Invalidate();
}

// Draw the images for the bits in the erase background event
BOOL CCalcBits::OnEraseBkgnd(CDC* pDC)
{
	CRect rct;    // used for drawing/fills etc and for calculations
	GetClientRect(&rct);
	pDC->FillSolidRect(rct, afxGlobalData.clrBarFace);
	//pDC->FillRect(&rct, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));

	// Don't show anything if it's not an int
	if (m_pParent->state_ == CALCERROR || m_pParent->state_ >= CALCINTEXPR)
		return TRUE;

	int wndHeight = rct.Height();
	calc_widths(rct);

	COLORREF base_colour =  m_pParent->radix_ == 16 ? ::BestHexAddrCol() :
	                        m_pParent->radix_ == 10 ? ::BestDecAddrCol() :
							                          ::GetSysColor(COLOR_BTNTEXT);
	base_colour = ::same_hue(base_colour, 100, 35);
	COLORREF disabled_colour = ::tone_down(base_colour, ::afxGlobalData.clrBarFace, 0.6);
	COLORREF enabled_colour = ::add_contrast(base_colour,  ::afxGlobalData.clrBarFace);

	// Set up the graphics objects we need for drawing the bits
	//CPen penDisabled(PS_SOLID, 0, ::tone_down(::GetSysColor(COLOR_BTNTEXT), ::afxGlobalData.clrBarFace, 0.7));
	//CPen penEnabled (PS_SOLID, 0, ::GetSysColor(COLOR_BTNTEXT));
	CPen penDisabled(PS_SOLID, 0, disabled_colour);
	CPen penEnabled (PS_SOLID, 0, enabled_colour);

	CFont font;
	LOGFONT lf;
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = 10;
	strcpy(lf.lfFaceName, "Tahoma");  // Simple font for small digits
	font.CreateFontIndirect(&lf);
	CFont *pOldFont = (CFont *)pDC->SelectObject(&font);

	// Start off with disabled colours as we draw from left (disabled side first)
	COLORREF colour = ::tone_down(base_colour, ::afxGlobalData.clrBarFace, 0.8);
	CPen * pOldPen = (CPen*) pDC->SelectObject(&penDisabled);
	bool enabled = false;

	// We need this so Rectangle() does not fill
	CBrush * pOldBrush = pDC->SelectObject(CBrush::FromHandle((HBRUSH)GetStockObject(NULL_BRUSH)));

	// Draw bits from left to right
	int horz = 1;                        // current horizontal position
	// Work out the size of one bit's image and the position of the left (most sig.) bit
	rct.left = horz;
	rct.right = rct.left + m_ww - 1;  // Subtract 1 to leave (at least) one pixel between each bit
	rct.top = 1;
	rct.bottom = rct.top + m_hh;

	unsigned __int64 val;
	// If showing result after binop button pressed then previous_ has the displayed value
	if (m_pParent->state_ == CALCINTBINARY && m_pParent->op_ != binop_none)
		val = mpz_get_ui64(m_pParent->previous_.get_mpz_t());
	else
		val = mpz_get_ui64(m_pParent->current_.get_mpz_t());
	for (int bnum = 63; bnum >= 0; bnum--)
	{
		// When we hit the first enabled bit switch to the darker pen
		if (!enabled && (bnum < m_pParent->bits_ || m_pParent->bits_ == 0))
		{
			pDC->SelectObject(&penEnabled);
			colour = enabled_colour;
			enabled = true;
		}

		if ( (val & ((__int64)1<<bnum)) != 0)
			pDC->FillSolidRect(rct, colour);
		pDC->Rectangle(&rct);

		// Draw numbers underneath some digits
		if (m_hh < wndHeight - 10 && (bnum%8 == 0 || bnum == 63))
		{
			char buf[4];
			sprintf(buf, "%d", bnum);
			pDC->SetTextColor(colour);
			pDC->SetBkColor(afxGlobalData.clrBarFace);
			pDC->TextOut(horz, m_hh + 1, buf, strlen(buf));
		}

		// Move horizontally to next bit position
		horz += m_ww + spacing(bnum);

		rct.MoveToX(horz);
	}
	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldPen);
	pDC->SelectObject(pOldFont);

	return TRUE;
}

// Toggle a bit when clicked on (and enabled)
void CCalcBits::OnLButtonDown(UINT nFlags, CPoint point)
{
	int bnum;
	if (m_pParent->state_ > CALCERROR &&
		m_pParent->state_ <= CALCINTLIT &&   // currently displaying an int
		(bnum = pos2bit(point)) > -1 &&      // valid bit clicked on
		(m_pParent->bits_ == 0 || bnum < m_pParent->bits_)) // not a disabled bit
	{
		ASSERT(bnum < 64 && bnum >= 0);
		mpz_class val = m_pParent->current_;
		mpz_combit(val.get_mpz_t(), bnum);
		m_pParent->Set(val);
	}
	CWnd::OnLButtonDown(nFlags, point);
}

// Used to decide horizontal positions of the bits
int CCalcBits::spacing(int bnum)
{
	int retval = 0;
	if (bnum%4 == 0)
		retval += m_nn;
	if (bnum%8 == 0)
		retval += m_bb;
	if (bnum%16 == 0)
		retval += m_cc;
	return retval;
}

// Work out where the bit images are to be displayed based on the width of the control
void CCalcBits::calc_widths(CRect & rct)
{
	m_ww = rct.Width()/70;                 // width of display for one bit
	ASSERT(m_ww > 3);
	m_hh = 4 + (rct.Height() - 6)/2;
	if (m_hh < 7)
		m_hh = 7;
	else if (m_hh > 16)
		m_hh = 16;
	if (m_hh > rct.Height() - 1)
		m_hh = rct.Height() - 1;
	m_nn = (rct.Width()-m_ww*64)/22;             // sep between nybbles = half of rest split ~15 ways
	m_bb = (rct.Width()-m_ww*64-m_nn*16)/10;     // sep between bytes = half of rest split ~7 ways
	m_cc = (rct.Width()-m_ww*64-m_nn*16-m_bb*8)/3; // sep between words = rest of space split 3 ways
	ASSERT(m_nn > 0 && m_bb > 0 && m_cc > 0);      // sanity check
}

// Given a child window position (point) works out which bit is beneath it.
// Returns the bit (0 to 63) or -1 if none.
int CCalcBits::pos2bit(CPoint point)
{
	if (point.y <= 0 && point.y >= m_hh - 1)
		return -1;                  // above or below

	// We use an iterative algorithm rather than calculate the bit directly,
	// as it is simpler, more maintainable (re-uses spacing() func as used above)
	// and should be more than fast enough for a user initiated action.
	int horz = 1;
	for (int bnum = 63; bnum >= 0; bnum--)
	{
		if (point.x < horz)
			return -1;              // must be in a gap between two bits
		if (point.x < horz + m_ww - 1)
			return bnum;
		horz += m_ww + spacing(bnum);
	}
	return -1;
}

/////////////////////////////////////////////////////////////////////////////
// CCalcDlg dialog

CCalcDlg::CCalcDlg(CWnd* pParent /*=NULL*/)
	: CDialog(), 
	  ctl_calc_bits_(this),
	  purple_pen(PS_SOLID, 0, RGB(0x80, 0, 0x80))
{
	inited_ = false;
	aa_ = dynamic_cast<CHexEditApp *>(AfxGetApp());

	m_sizeInitial = CSize(-1, -1);
	help_hwnd_ = (HWND)0;

	source_ = km_result;
	op_ = binop_none;
	previous_unary_op_ = unary_none;
	current_ = previous_ = 0;
	state_ = CALCINTRES;

	radix_ = bits_ = -1;  // Set to -1 so they are set up correctly

	edit_.pp_ = this;                   // Set parent of edit control
	ctl_edit_combo_.pp_ = this;
	m_first = true;
}

BOOL CCalcDlg::Create(CWnd* pParentWnd /*=NULL*/)
{
	mm_ = dynamic_cast<CMainFrame *>(AfxGetMainWnd());

	if (!CDialog::Create(MAKEINTRESOURCE(IDD), pParentWnd)) // IDD_CALC
	{
		TRACE0("Failed to create Calculator dialog\n");
		return FALSE; // failed to create
	}

	// Subclass the main calculator edit box
	CWnd *pwnd = ctl_edit_combo_.GetWindow(GW_CHILD);
	ASSERT(pwnd != NULL);
	VERIFY(edit_.SubclassWindow(pwnd->m_hWnd));
	LONG style = ::GetWindowLong(edit_.m_hWnd, GWL_STYLE);
	::SetWindowLong(edit_.m_hWnd, GWL_STYLE, style | ES_RIGHT | ES_WANTRETURN);

	// Get last used settings from registry
	change_base(aa_->GetProfileInt("Calculator", "Base", 16));
	change_bits(aa_->GetProfileInt("Calculator", "Bits", 32));
	change_signed(aa_->GetProfileInt("Calculator", "signed", 0) ? true : false);
	current_.set_str(aa_->GetProfileString("Calculator", "Current", "0"), 10);
	state_ = CALCINTUNARY;
	edit_.put();
	edit_.get();
	set_right();
	memory_.set_str(aa_->GetProfileString("Calculator", "Memory", "0"), 10);

	// If saved memory value was restored then make it available
	if (memory_ != 0)
		button_colour(GetDlgItem(IDC_MEM_GET), true, RGB(0x40, 0x40, 0x40));

	// Special setup for "GO" button
	ctl_go_.SetMouseCursorHand();   	// Indicates that GO button takes user back to file
	ctl_go_.m_bDefaultClick = TRUE;     // Default click goes to address (also drop-down menu for go sector etc)
	ctl_go_.m_bOSMenu = FALSE;
	VERIFY(go_menu_.LoadMenu(IDR_CALC_GO));
	ctl_go_.m_hMenu = go_menu_.GetSubMenu(0)->GetSafeHmenu();  // Add drop-down menu (handled by checking m_nMenuResult in button click event - see OnGo)

	// Since functions menu doesn't change we only need to set it up once (load from resources)
	VERIFY(func_menu_.LoadMenu(IDR_FUNCTION));
	ctl_func_.m_hMenu = func_menu_.GetSubMenu(0)->GetSafeHmenu();

	setup_tooltips();
	setup_expr();
	setup_static_buttons();
	((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_RADIX))->SetRange(2, 36);
	((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_BITS))->SetRange(0, 32767);  // 0=Inf

	// ------------ Set up the "bits" child window ------------
	static CString bitsClass;
	if (bitsClass.IsEmpty())
	{
		// Register new window class
		bitsClass = AfxRegisterWndClass(0);
		ASSERT(!bitsClass.IsEmpty());
	}

	CRect rct;  // New window location
	ASSERT(GetDlgItem(IDC_BITS_PLACEHOLDER) != NULL);
	GetDlgItem(IDC_BITS_PLACEHOLDER)->GetWindowRect(&rct);
	ScreenToClient(&rct);

	DWORD bitsStyle = WS_CHILD | WS_VISIBLE;
	VERIFY(ctl_calc_bits_.Create(bitsClass, NULL, bitsStyle, rct, this, IDC_CALC_BITS));
	ctl_calc_bits_.SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOSIZE|SWP_NOOWNERZORDER);

	// ----------- Set up resizer control ----------------
	// (See also setup_resizer - called the first time OnKickIdle is called.)

	// We must set the 4th parameter true else we get a resize border
	// added to the dialog and this really stuffs things up inside a pane.
	m_resizer.Create(GetSafeHwnd(), TRUE, 100, TRUE);

	// It needs an initial size for it's calcs
	GetWindowRect(&rct);
	m_resizer.SetInitialSize(rct.Size());
	rct.bottom = rct.top + (rct.bottom - rct.top)*3/4;
	m_resizer.SetMinimumTrackingSize(rct.Size());

	check_for_error();   // check for overflow of restored value
	edit_.put();
	//edit_.get();    // current_str_ already contains the restored value
	set_right();
	inited_ = true;
	return TRUE;
}

LRESULT CCalcDlg::HandleInitDialog(WPARAM wParam, LPARAM lParam)
{
	CDialog::HandleInitDialog(wParam, lParam);

	return TRUE;
} // HandleInitDialog

// called from UpdateData
void CCalcDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT,  ctl_edit_combo_);

	DDX_Control(pDX, IDC_DIGIT_0, ctl_digit_0_);
	DDX_Control(pDX, IDC_DIGIT_1, ctl_digit_1_);
	DDX_Control(pDX, IDC_DIGIT_2, ctl_digit_2_);
	DDX_Control(pDX, IDC_DIGIT_3, ctl_digit_3_);
	DDX_Control(pDX, IDC_DIGIT_4, ctl_digit_4_);
	DDX_Control(pDX, IDC_DIGIT_5, ctl_digit_5_);
	DDX_Control(pDX, IDC_DIGIT_6, ctl_digit_6_);
	DDX_Control(pDX, IDC_DIGIT_7, ctl_digit_7_);
	DDX_Control(pDX, IDC_DIGIT_8, ctl_digit_8_);
	DDX_Control(pDX, IDC_DIGIT_9, ctl_digit_9_);
	DDX_Control(pDX, IDC_DIGIT_A, ctl_digit_a_);
	DDX_Control(pDX, IDC_DIGIT_B, ctl_digit_b_);
	DDX_Control(pDX, IDC_DIGIT_C, ctl_digit_c_);
	DDX_Control(pDX, IDC_DIGIT_D, ctl_digit_d_);
	DDX_Control(pDX, IDC_DIGIT_E, ctl_digit_e_);
	DDX_Control(pDX, IDC_DIGIT_F, ctl_digit_f_);
	DDX_Control(pDX, IDC_MEM_GET, ctl_mem_get_);
	DDX_Control(pDX, IDC_MARK_GET, ctl_mark_get_);
	DDX_Control(pDX, IDC_SEL_GET, ctl_sel_get_);
	DDX_Control(pDX, IDC_SEL_AT, ctl_sel_at_);
	DDX_Control(pDX, IDC_SEL_LEN, ctl_sel_len_);
	DDX_Control(pDX, IDC_MARK_AT, ctl_mark_at_);
	DDX_Control(pDX, IDC_EOF_GET, ctl_eof_get_);

	DDX_Control(pDX, IDC_BACKSPACE, ctl_backspace_);
	DDX_Control(pDX, IDC_CLEAR_ENTRY, ctl_clear_entry_);
	DDX_Control(pDX, IDC_CLEAR, ctl_clear_);
	DDX_Control(pDX, IDC_EQUALS, ctl_equals_);
	DDX_Control(pDX, IDC_AND, ctl_and_);
	DDX_Control(pDX, IDC_ASR, ctl_asr_);
	DDX_Control(pDX, IDC_DIVIDE, ctl_divide_);
	DDX_Control(pDX, IDC_LSL, ctl_lsl_);
	DDX_Control(pDX, IDC_LSR, ctl_lsr_);
	DDX_Control(pDX, IDC_ROL, ctl_rol_);
	DDX_Control(pDX, IDC_ROR, ctl_ror_);
	DDX_Control(pDX, IDC_XOR, ctl_xor_);
	DDX_Control(pDX, IDC_MOD, ctl_mod_);
	DDX_Control(pDX, IDC_MULTIPLY, ctl_multiply_);
	DDX_Control(pDX, IDC_OR, ctl_or_);
	DDX_Control(pDX, IDC_UNARY_DEC, ctl_unary_dec_);
	DDX_Control(pDX, IDC_UNARY_FACTORIAL, ctl_unary_factorial_);
	DDX_Control(pDX, IDC_UNARY_FLIP, ctl_unary_flip_);
	DDX_Control(pDX, IDC_UNARY_REV, ctl_unary_rev_);
	DDX_Control(pDX, IDC_UNARY_INC, ctl_unary_inc_);
	DDX_Control(pDX, IDC_UNARY_NOT, ctl_unary_not_);
	DDX_Control(pDX, IDC_UNARY_SIGN, ctl_unary_sign_);
	DDX_Control(pDX, IDC_UNARY_SQUARE, ctl_unary_square_);
	DDX_Control(pDX, IDC_SUBTRACT, ctl_subtract_);
	DDX_Control(pDX, IDC_MEM_CLEAR, ctl_mem_clear_);
	DDX_Control(pDX, IDC_MEM_ADD, ctl_mem_add_);
	DDX_Control(pDX, IDC_MEM_SUBTRACT, ctl_mem_subtract_);
	DDX_Control(pDX, IDC_MARK_ADD, ctl_mark_add_);
	DDX_Control(pDX, IDC_MARK_SUBTRACT, ctl_mark_subtract_);
	DDX_Control(pDX, IDC_GTR, ctl_gtr_);
	DDX_Control(pDX, IDC_LESS, ctl_less_);
	DDX_Control(pDX, IDC_POW, ctl_pow_);
	DDX_Control(pDX, IDC_MARK_AT_STORE, ctl_mark_at_store_);
	DDX_Control(pDX, IDC_MARK_CLEAR, ctl_mark_clear_);
	DDX_Control(pDX, IDC_MARK_STORE, ctl_mark_store_);
	DDX_Control(pDX, IDC_MEM_STORE, ctl_mem_store_);
	DDX_Control(pDX, IDC_SEL_AT_STORE, ctl_sel_at_store_);
	DDX_Control(pDX, IDC_SEL_STORE, ctl_sel_store_);
	DDX_Control(pDX, IDC_UNARY_ROL, ctl_unary_rol_);
	DDX_Control(pDX, IDC_UNARY_ROR, ctl_unary_ror_);
	DDX_Control(pDX, IDC_UNARY_LSL, ctl_unary_lsl_);
	DDX_Control(pDX, IDC_UNARY_LSR, ctl_unary_lsr_);
	DDX_Control(pDX, IDC_UNARY_ASR, ctl_unary_asr_);
	DDX_Control(pDX, IDC_UNARY_CUBE, ctl_unary_cube_);
	DDX_Control(pDX, IDC_UNARY_AT, ctl_unary_at_);
	DDX_Control(pDX, IDC_UNARY_SQUARE_ROOT, ctl_unary_square_root_);
	DDX_Control(pDX, IDC_SEL_LEN_STORE, ctl_sel_len_store_);
	DDX_Control(pDX, IDC_ADDOP, ctl_addop_);
	DDX_Control(pDX, IDC_GO, ctl_go_);

	DDX_Control(pDX, IDC_HEX_HIST, ctl_hex_hist_);
	DDX_Control(pDX, IDC_DEC_HIST, ctl_dec_hist_);
	DDX_Control(pDX, IDC_VARS, ctl_vars_);
	DDX_Control(pDX, IDC_FUNC, ctl_func_);
} // DoDataExchange

BEGIN_MESSAGE_MAP(CCalcDlg, CDialog)
	ON_WM_CREATE()
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_MESSAGE(WM_INITDIALOG, HandleInitDialog)
	ON_BN_CLICKED(IDC_DIGIT_0, OnDigit0)
	ON_BN_CLICKED(IDC_DIGIT_1, OnDigit1)
	ON_BN_CLICKED(IDC_DIGIT_2, OnDigit2)
	ON_BN_CLICKED(IDC_DIGIT_3, OnDigit3)
	ON_BN_CLICKED(IDC_DIGIT_4, OnDigit4)
	ON_BN_CLICKED(IDC_DIGIT_5, OnDigit5)
	ON_BN_CLICKED(IDC_DIGIT_6, OnDigit6)
	ON_BN_CLICKED(IDC_DIGIT_7, OnDigit7)
	ON_BN_CLICKED(IDC_DIGIT_8, OnDigit8)
	ON_BN_CLICKED(IDC_DIGIT_9, OnDigit9)
	ON_BN_CLICKED(IDC_DIGIT_A, OnDigitA)
	ON_BN_CLICKED(IDC_DIGIT_B, OnDigitB)
	ON_BN_CLICKED(IDC_DIGIT_C, OnDigitC)
	ON_BN_CLICKED(IDC_DIGIT_D, OnDigitD)
	ON_BN_CLICKED(IDC_DIGIT_E, OnDigitE)
	ON_BN_CLICKED(IDC_DIGIT_F, OnDigitF)
	ON_BN_CLICKED(IDC_BACKSPACE, OnBackspace)
	ON_BN_CLICKED(IDC_CLEAR_ENTRY, OnClearEntry)
	ON_BN_CLICKED(IDC_CLEAR, OnClear)
	ON_BN_CLICKED(IDC_EQUALS, OnEquals)
	ON_BN_CLICKED(IDC_AND, OnAnd)
	ON_BN_CLICKED(IDC_ASR, OnAsr)
	ON_BN_CLICKED(IDC_DIVIDE, OnDivide)
	ON_BN_CLICKED(IDC_LSL, OnLsl)
	ON_BN_CLICKED(IDC_LSR, OnLsr)
	ON_BN_CLICKED(IDC_ROL, OnRol)
	ON_BN_CLICKED(IDC_ROR, OnRor)
	ON_BN_CLICKED(IDC_XOR, OnXor)
	ON_BN_CLICKED(IDC_MOD, OnMod)
	ON_BN_CLICKED(IDC_MULTIPLY, OnMultiply)
	ON_BN_CLICKED(IDC_OR, OnOr)
	ON_BN_CLICKED(IDC_UNARY_DEC, OnUnaryDec)
	ON_BN_CLICKED(IDC_UNARY_FACTORIAL, OnUnaryFactorial)
	ON_BN_CLICKED(IDC_UNARY_FLIP, OnUnaryFlip)
	ON_BN_CLICKED(IDC_UNARY_REV, OnUnaryRev)
	ON_BN_CLICKED(IDC_UNARY_INC, OnUnaryInc)
	ON_BN_CLICKED(IDC_UNARY_NOT, OnUnaryNot)
	ON_BN_CLICKED(IDC_UNARY_SIGN, OnUnarySign)
	ON_BN_CLICKED(IDC_UNARY_SQUARE, OnUnarySquare)
	ON_BN_CLICKED(IDC_SUBTRACT, OnSubtract)
	ON_BN_CLICKED(IDC_GO, OnGo)
	ON_BN_CLICKED(IDC_MEM_GET, OnMemGet)
	ON_BN_CLICKED(IDC_MEM_CLEAR, OnMemClear)
	ON_BN_CLICKED(IDC_MEM_ADD, OnMemAdd)
	ON_BN_CLICKED(IDC_MEM_SUBTRACT, OnMemSubtract)
	ON_BN_CLICKED(IDC_MARK_GET, OnMarkGet)
	ON_BN_CLICKED(IDC_MARK_AT, OnMarkAt)
	ON_BN_CLICKED(IDC_MARK_ADD, OnMarkAdd)
	ON_BN_CLICKED(IDC_MARK_SUBTRACT, OnMarkSubtract)
	ON_BN_CLICKED(IDC_SEL_GET, OnSelGet)
	ON_BN_CLICKED(IDC_SEL_AT, OnSelAt)
	ON_BN_CLICKED(IDC_EOF_GET, OnEofGet)
	ON_BN_CLICKED(IDC_GTR, OnGtr)
	ON_BN_CLICKED(IDC_LESS, OnLess)
	ON_BN_CLICKED(IDC_POW, OnPow)
	ON_BN_CLICKED(IDC_MARK_AT_STORE, OnMarkAtStore)
	ON_BN_CLICKED(IDC_MARK_CLEAR, OnMarkClear)
	ON_BN_CLICKED(IDC_MARK_STORE, OnMarkStore)
	ON_BN_CLICKED(IDC_MEM_STORE, OnMemStore)
	ON_BN_CLICKED(IDC_SEL_AT_STORE, OnSelAtStore)
	ON_BN_CLICKED(IDC_SEL_STORE, OnSelStore)
	ON_BN_CLICKED(IDC_UNARY_ROL, OnUnaryRol)
	ON_BN_CLICKED(IDC_UNARY_ROR, OnUnaryRor)
	ON_BN_CLICKED(IDC_UNARY_LSL, OnUnaryLsl)
	ON_BN_CLICKED(IDC_UNARY_LSR, OnUnaryLsr)
	ON_BN_CLICKED(IDC_UNARY_ASR, OnUnaryAsr)
	ON_BN_CLICKED(IDC_UNARY_CUBE, OnUnaryCube)
	ON_BN_CLICKED(IDC_UNARY_AT, OnUnaryAt)
	ON_BN_CLICKED(IDC_UNARY_SQUARE_ROOT, OnUnarySquareRoot)
	ON_WM_DRAWITEM()
	ON_BN_CLICKED(IDC_8BIT, On8bit)
	ON_BN_CLICKED(IDC_16BIT, On16bit)
	ON_BN_CLICKED(IDC_32BIT, On32bit)
	ON_BN_CLICKED(IDC_64BIT, On64bit)
	ON_BN_CLICKED(IDC_INFBIT, OnInfbit)
	ON_BN_CLICKED(IDC_BINARY, OnBinary)
	ON_BN_CLICKED(IDC_OCTAL, OnOctal)
	ON_BN_CLICKED(IDC_DECIMAL, OnDecimal)
	ON_BN_CLICKED(IDC_HEX, OnHex)
	ON_BN_CLICKED(IDC_SEL_LEN, OnSelLen)
	ON_BN_CLICKED(IDC_SEL_LEN_STORE, OnSelLenStore)
	ON_BN_CLICKED(IDC_ADDOP, OnAdd)
	ON_WM_DESTROY()
	ON_WM_HELPINFO()

	// Removed ON_BN_DOUBLECLICKED stuff as d-click now seems to send two
	// mouse down/up events (as well as a dclick event)
	//ON_BN_DOUBLECLICKED(IDC_BACKSPACE, OnBackspace) ...
	ON_BN_CLICKED(IDC_BIG_ENDIAN_FILE_ACCESS, OnBigEndian)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_HEX_HIST, OnGetHexHist)
	ON_BN_CLICKED(IDC_DEC_HIST, OnGetDecHist)
	ON_BN_CLICKED(IDC_VARS, OnGetVar)
	ON_BN_CLICKED(IDC_FUNC, OnGetFunc)
	ON_WM_SIZE()
	ON_NOTIFY(TTN_SHOW, 0, OnTooltipsShow)
	ON_WM_CONTEXTMENU()
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
	ON_CBN_SELCHANGE(IDC_EDIT, OnSelHistory)
	//ON_WM_DELETEITEM()   // OnDeleteItem - message only sent for owner draw list boxes
	ON_EN_CHANGE(IDC_RADIX, OnChangeRadix)
	ON_EN_CHANGE(IDC_BITS, OnChangeBits)
	ON_BN_CLICKED(IDC_CALC_SIGNED, OnChangeSigned)

	END_MESSAGE_MAP()

// Control ID and corresp help ID - used for popup context help
static DWORD id_pairs[] = { 
	IDC_EDIT, HIDC_EDIT,
	IDC_OP_DISPLAY, HIDC_OP_DISPLAY,
	IDC_BIG_ENDIAN_FILE_ACCESS, HIDC_BIG_ENDIAN_FILE_ACCESS,
	IDC_HEX, HIDC_HEX,
	IDC_DECIMAL, HIDC_DECIMAL,
	IDC_OCTAL, HIDC_OCTAL,
	IDC_BINARY, HIDC_BINARY,
	IDC_INFBIT, HIDC_INFBIT,
	IDC_64BIT, HIDC_64BIT,
	IDC_32BIT, HIDC_32BIT,
	IDC_16BIT, HIDC_16BIT,
	IDC_8BIT, HIDC_8BIT,
	IDC_MEM_GET, HIDC_MEM_GET,
	IDC_MEM_STORE, HIDC_MEM_STORE,
	IDC_MEM_CLEAR, HIDC_MEM_CLEAR,
	IDC_MEM_ADD, HIDC_MEM_ADD,
	IDC_MEM_SUBTRACT, HIDC_MEM_SUBTRACT,
	IDC_BACKSPACE, HIDC_BACKSPACE,
	IDC_CLEAR_ENTRY, HIDC_CLEAR_ENTRY,
	IDC_CLEAR, HIDC_CLEAR,
	IDC_MARK_GET, HIDC_MARK_GET,
	IDC_MARK_STORE, HIDC_MARK_STORE,
	IDC_MARK_CLEAR, HIDC_MARK_CLEAR,
	IDC_MARK_ADD, HIDC_MARK_ADD,
	IDC_MARK_SUBTRACT, HIDC_MARK_SUBTRACT,
	IDC_MARK_AT, HIDC_MARK_AT,
	IDC_MARK_AT_STORE, HIDC_MARK_AT_STORE,
	IDC_SEL_GET, HIDC_SEL_GET,
	IDC_SEL_STORE, HIDC_SEL_STORE,
	IDC_SEL_AT, HIDC_SEL_AT,
	IDC_SEL_AT_STORE, HIDC_SEL_AT_STORE,
	IDC_SEL_LEN, HIDC_SEL_LEN,
	IDC_SEL_LEN_STORE, HIDC_SEL_LEN_STORE,
	IDC_EOF_GET, HIDC_EOF_GET,
	IDC_UNARY_SQUARE, HIDC_UNARY_SQUARE,
	IDC_UNARY_SQUARE_ROOT, HIDC_UNARY_SQUARE_ROOT,
	IDC_UNARY_CUBE, HIDC_UNARY_CUBE,
	IDC_UNARY_FACTORIAL, HIDC_UNARY_FACTORIAL,
	IDC_UNARY_AT, HIDC_UNARY_AT,
	IDC_UNARY_ROL, HIDC_UNARY_ROL,
	IDC_UNARY_ROR, HIDC_UNARY_ROR,
	IDC_UNARY_LSL, HIDC_UNARY_LSL,
	IDC_UNARY_LSR, HIDC_UNARY_LSR,
	IDC_UNARY_ASR, HIDC_UNARY_ASR,
	IDC_UNARY_REV, HIDC_UNARY_REV,
	IDC_UNARY_NOT, HIDC_UNARY_NOT,
	IDC_UNARY_INC, HIDC_UNARY_INC,
	IDC_UNARY_DEC, HIDC_UNARY_DEC,
	IDC_UNARY_FLIP, HIDC_UNARY_FLIP,
	IDC_UNARY_SIGN, HIDC_UNARY_SIGN,
	IDC_EQUALS, HIDC_EQUALS,
	IDC_POW, HIDC_POW,
	IDC_MOD, HIDC_MOD,
	IDC_DIVIDE, HIDC_DIVIDE,
	IDC_MULTIPLY, HIDC_MULTIPLY,
	IDC_SUBTRACT, HIDC_SUBTRACT,
	IDC_ADDOP, HIDC_ADDOP,
	IDC_GTR, HIDC_GTR,
	IDC_LESS, HIDC_LESS,
	IDC_AND, HIDC_AND,
	IDC_OR, HIDC_OR,
	IDC_XOR, HIDC_XOR,
	IDC_ROL, HIDC_ROL,
	IDC_ROR, HIDC_ROR,
	IDC_LSL, HIDC_LSL,
	IDC_LSR, HIDC_LSR,
	IDC_ASR, HIDC_ASR,
	IDC_GO, HIDC_GO,
	IDC_DIGIT_0, HIDC_DIGIT_0,
	IDC_DIGIT_1, HIDC_DIGIT_0,
	IDC_DIGIT_2, HIDC_DIGIT_0,
	IDC_DIGIT_3, HIDC_DIGIT_0,
	IDC_DIGIT_4, HIDC_DIGIT_0,
	IDC_DIGIT_5, HIDC_DIGIT_0,
	IDC_DIGIT_6, HIDC_DIGIT_0,
	IDC_DIGIT_7, HIDC_DIGIT_0,
	IDC_DIGIT_8, HIDC_DIGIT_0,
	IDC_DIGIT_9, HIDC_DIGIT_0,
	IDC_DIGIT_A, HIDC_DIGIT_0,
	IDC_DIGIT_B, HIDC_DIGIT_0,
	IDC_DIGIT_C, HIDC_DIGIT_0,
	IDC_DIGIT_D, HIDC_DIGIT_0,
	IDC_DIGIT_E, HIDC_DIGIT_0,
	IDC_DIGIT_F, HIDC_DIGIT_0,
	0,0
};

// Set focus to text control and move caret to end
void CCalcDlg::StartEdit()
{
	edit_.SetFocus();
	//edit_.SetSel(edit_.GetWindowTextLength(), -1);  // move caret to end
	edit_.SetSel(0, -1);                              // select all
}

// Save a calculator value to the current macro
// If a value (ss) is specified save it else save the current calc value as a string
void CCalcDlg::save_value_to_macro(CString ss /*= CString()*/)
{
	if (ss.IsEmpty())
		ss = current_str_;

	if (source_ == km_user_str && state_ >= CALCINTEXPR)
		aa_->SaveToMacro(km_expression, ss);
	else if (source_ == km_user_str)
		aa_->SaveToMacro(km_user_str, ss);
	else if (source_ != km_result)
		aa_->SaveToMacro(source_);

	source_ = km_result;   // ensure it is not saved again
}

// Get value (current calc value, memory etc) according to current
// bits_ and signed_ settings.
mpz_class CCalcDlg::get_norm(mpz_class v, bool only_pos /*= false*/) const
{
	// If infinite precision (bits_ == 0) then there is no mask
	// Note: this means we can return a -ve value even when signed_ == false
	if (bits_ == 0) return v;

	mpz_class retval = v & mask_;           // mask off high bits

	// If high bit is on then make -ve.
	if (!only_pos && signed_ && mpz_tstbit(v.get_mpz_t(), bits_ - 1))
	{
		// Convert to the 2's complement (invert bits and add one within bits_ range)
		mpz_com(retval.get_mpz_t(), retval.get_mpz_t());
		retval &= mask_;
		++ retval;
		// Now say it is -ve
		mpz_neg(retval.get_mpz_t(), retval.get_mpz_t());
	}

	return retval;
}

void CCalcDlg::SetFromFile(FILE_ADDRESS addr)
{
	if (!get_bytes(addr))
	{
		state_ = CALCERROR;
		return;
	}
	state_ = CALCINTUNARY;
	edit_.put();
	edit_.get();
	set_right();
}

// Get data from the file into calculator, reversing byte order if necessary
// Returns false on error such as no file open or invalid address.
bool CCalcDlg::get_bytes(FILE_ADDRESS addr)
{
	if (bits_ == 0 || bits_%8 != 0)
	{
		current_str_ = "Bits must be divisible by 8";
		return false;
	}

	const char * err_mess = mpz_set_bytes(current_.get_mpz_t(), addr, bits_/8);
	if (err_mess != NULL)
	{
		if (GetView() == NULL)
			no_file_error();
		current_str_ = err_mess;
		return false;
	}

	state_ = CALCINTUNARY;
	return true;
}

// Put data to the file from the current calculator value
bool CCalcDlg::put_bytes(FILE_ADDRESS addr)
{
	if (bits_ == 0 || bits_%8 != 0)
	{
		TaskMessageBox("Can't write to file",
			"The calculator integer size (Bits) must be a multiple of 8.  "
			"Only a whole number of bytes can be written to a file.");
		return false;
	}

	CHexEditView *pview = GetView();
	if (pview == NULL)
	{
		no_file_error();
		return false;
	}

	if (pview->ReadOnly())
	{
		TaskMessageBox("File is read only",
			"The calculator cannot write to the current file when it is open in read only mode.");
		return false;
	}

	FILE_ADDRESS dummy;
	switch (addr)
	{
	case -2:   // address of mark
		addr = pview->GetMark();
		break;
	case -3:   // address of start of selection
		pview->GetSelAddr(addr, dummy);
		break;
	}

	if (addr > pview->GetDocument()->length())
	{
		TaskMessageBox("Can't write past EOF",
			"An attempt was made to write at an address past the end of file.  "
			"The calculator won't extend a file to allow the bytes to be written.");
		return false;
	}
	if (!pview->OverType() && addr + bits_/8 > pview->GetDocument()->length())
	{
		TaskMessageBox("Write extends past EOF",
			"The bytes to be written extends past the end of file.  "
			"The calculator cannot write past EOF when the file is open in OVR mode.");
		return false;
	}

	// Get buffer for bytes (size rounded up to multiple of mpz limb size)
	int units = (bits_/8 + 3)/4; // number of DWORDs required
	mp_limb_t * buf = new mp_limb_t[units];

	// Get all limbs from the big integer (little-endian)

	// Make sure view and calculator endianness are in sync
	ASSERT(pview->BigEndian() == ((CButton*)GetDlgItem(IDC_BIG_ENDIAN_FILE_ACCESS))->GetCheck());
	if (pview->BigEndian())
		flip_bytes((unsigned char *)buf, bits_/8);   // Reverse the byte order to match that used internally (Intel=little-endian)

	pview->GetDocument()->Change(pview->OverType() ? mod_replace : mod_insert,
								addr, bits_/8, (unsigned char *)buf, 0, pview);
	delete[] buf;

	return true;
}

void CCalcDlg::fix_values()
{
	// Adjust mask etc depending on bits_ and signed_ (assumes 2's-complement numbers)
	if (bits_ == 0) return;  // Indicates unlimited bits whence min/max/mask are not used

	mpz_class one(1);
	mask_ = (one << bits_) - 1;
	if (signed_)
	{
		// 2's complement: abs(min) == abs(max) + 1
		min_val_ = - one << (bits_ - 1);
		max_val_ = - min_val_ - 1;
	}
	else
	{
		min_val_ = 0;
		max_val_ = mask_;
	}
}

// Stores a binary operation  by:
// - evaluating any outstanding binary operation (saved in current_)
// - move current_ to previous_ ready for a new value to be entered
// - store the operation (input parameter binop) in op_
// On Entry state_ can have these values:
// CALCERROR, CALCOVERFLOW - causes an error
// CALCINTRES - current_ is final result (eg = button)
// CALCINTBINARY - binop (eg + button) but no next value yet entered
// CALCINTUNARY - unary op (eg INC button) or restored value (eg MR button)
// CALCINTLIT, CALCINTEXPR - combine current_ with previous_ (if op_ is valid)
// > CALCINTEXPR - error: can't perform an integer operation on current value

void CCalcDlg::do_binop(binop_type binop)
{
	// First check that we can perform a binary operation
	if (state_ == CALCERROR)
	{
		make_noise("Calculator Error");
		CAvoidableDialog::Show(IDS_CALC_ERROR,
				"You cannot perform this operation on an invalid value.");
		//mm_->StatusBarText("Operation on invalid value.");
		aa_->mac_error_ = 10;
		return;
	}
	else if (state_ > CALCINTEXPR)
	{
		not_int_error();
		return;
	}

	// If state_ == CALCINTBINARY we have just processed a binop and apparently
	// the user just wants to override.  In this case we don't want to save
	// to previous_ as this was just done.
	if (state_ != CALCINTBINARY)
	{
		// If there is an active binop (op_ != binop_none) we need to calculate
		// it before moving current_ to previous_
		CString temp = GetStringValue();
		calc_binary();
		check_for_error();   // check for overflow & display error messages
		if (state_ >= CALCINTRES)
			state_ = CALCINTBINARY;   // Indicate that (next) binary operation is still active
		edit_.put();                  // Displays left side of binop until user starts to enter a new value (for right side)

		save_value_to_macro(temp);

		// Get ready for new value to be entered
		previous_ = get_norm(current_, binop >= binop_rol && binop <= binop_asr);
		left_ = right_;
		current_ = 0;
		current_str_ = "0"; // don't use edit_.put()/get() here as we want top preserve the previous result until something is entered
		set_right();
	}
	aa_->SaveToMacro(km_binop, long(binop));
	op_ = binop;
	previous_unary_op_ = unary_none;
}

// Displays errors and aslo checks for overflow (integer literals).
// Should be called after a calculation that could generate and error or overflow
// (unary or binary operation) or when an integer literal has been changed (edited or 
// set from memory etc) or the way it is displayed could cause overflow (bits/signed).
void CCalcDlg::check_for_error()
{
	if (state_ > CALCINTLIT) return;

	// Check we have not overflowed bits (if we don't already have an error)
	if (state_ >= CALCINTRES &&
		bits_ > 0 &&                                // can't overflow infinite bits (bits_ == 0)
		(current_ < min_val_ || current_ > max_val_))
	{
		state_ = CALCOVERFLOW;
	}

	if (state_ == CALCERROR)
	{
		make_noise("Calculator Error");
		CAvoidableDialog::Show(IDS_CALC_ERROR, (CString)current_str_);
		//mm_->StatusBarText();
		aa_->mac_error_ = 10;
	}
	else if (state_ == CALCOVERFLOW)
	{
		make_noise("Calculator Overflow");
		CAvoidableDialog::Show(IDS_CALC_OVERFLOW, 
			"The operation overflowed the current number of bits.  "
			"This may mean the value is too large a positive or negative value or "
			"the value is negative and the current settings are for unsigned values.\n\n"
			"The displayed result has been adjusted to fit the current settings.  "
			"Please modify the \"Bits\" and/or \"Signed\" settings to see the full result or "
			"use \"Inf\" for infinite precision (Bits = 0) to display any value without overflow.");
		//mm_->StatusBarText("Overflow");
		aa_->mac_error_ = 2;
	}
}

// Called when something is done that requires a file open but no file is open
void CCalcDlg::no_file_error()
{
	make_noise("Calculator Error");
	CAvoidableDialog::Show(IDS_CALC_NO_FILE,
		"The calculator cannot perform this operation when no file is open.");
	aa_->mac_error_ = 10;
}

void CCalcDlg::not_int_error()
{
	make_noise("Calculator Error");
	CAvoidableDialog::Show(IDS_CALC_NOT_INT,
			"Most calculator buttons are designed to only work with integers.  "
			"You cannot use this button when working with a non-integer expression or value.");
// xxx work out what it is and say so: real etc???
	//mm_->StatusBarText("Buttons only valid for integers.");
	aa_->mac_error_ = 10;
}

void CCalcDlg::make_noise(const char * ss)
{
#ifdef SYS_SOUNDS
	if (!CSystemSound::Play(ss))
#endif
		::Beep(3000,400);
}

void CCalcDlg::update_expr()
{
	ExprStringType disp = get_expr(true);
	if (orig_radix_ != radix_)
	{
		ExprStringType strRadix;
		strRadix.Format(EXPRSTR(" [%d]"), orig_radix_);
		disp += strRadix;
	}

	// This is the window (readonly text box) where we show the expression
	CWnd * pwnd = GetDlgItem(IDC_EXPR);
	ASSERT(pwnd != NULL);

	// Get the window's size
	CRect rct;
	pwnd->GetWindowRect(&rct);

	// Get the text's size
	CClientDC dc(pwnd);
	int nSave = dc.SaveDC();
	dc.SelectObject(pwnd->GetFont());
	CSize sz = dc.GetTextExtent(CString(disp));

	if (sz.cx > rct.Width())                       // string is too big to display
		disp.Replace(EXPRSTR(" "), EXPRSTR(""));   // remove spaces to make the string shorter

	// Get text from expression window to check if it is different from what we want
	int len = pwnd->GetWindowTextLength();
	wchar_t * pbuf = new wchar_t[len + 2];
	::GetWindowTextW(pwnd->m_hWnd, pbuf, len+1);

	// If different update with the new text
	if (pbuf != disp)
		::SetWindowTextW(pwnd->m_hWnd, (const wchar_t *)disp);
	delete[] pbuf;
	dc.RestoreDC(nSave);
}

void CCalcDlg::update_op()
{
	if (aa_->refresh_off_ ||! IsVisible())
		return;

	const char *disp = "";
	switch (state_)
	{
	case CALCERROR:
		disp = "E";
		break;
	case CALCOVERFLOW:
		disp = "O";
		break;

	case CALCREALEXPR:
	case CALCREALRES:
		disp = "R";
		break;
	case CALCDATEEXPR:
	case CALCDATERES:
		disp = "D";
		break;
	case CALCSTREXPR:
	case CALCSTRRES:
		disp = "S";
		break;
	case CALCBOOLEXPR:
	case CALCBOOLRES:
		disp = "B";
		break;

#ifdef _DEBUG  // Normally we don't display anything for these but this helps with debugging
	case CALCINTRES:
		disp = "=";
		break;
	case CALCOTHER:
		disp = "??";
		break;
#endif

	case CALCINTBINARY:
	case CALCINTUNARY:
	case CALCINTLIT:
	case CALCINTEXPR:
		switch (op_)
		{
		case binop_add:
			disp = "+";
			break;
		case binop_subtract:
			disp = "-";
			break;
		case binop_multiply:
			disp = "*";
			break;
		case binop_divide:
			disp = "/";
			break;
		case binop_mod:
			disp = "%";
			break;
		case binop_pow:
			disp = "**";
			break;
		case binop_gtr:
			disp = ">";
			break;
		case binop_less:
			disp = "<";
			break;
		case binop_ror:
			disp = "=>";
			break;
		case binop_rol:
			disp = "<=";
			break;
		case binop_lsl:
			disp = "<-";
			break;
		case binop_lsr:
			disp = "->";
			break;
		case binop_asr:
			disp = "+>";
			break;

		case binop_and:
			disp = "&&";  // displays as a single &
			break;
		case binop_or:
			disp = "|";
			break;
		case binop_xor:
			disp = "^";
			break;

		case binop_none:
			disp = "";
			break;

		default:
			ASSERT(0);  // make sure we add new binary ops here
		}
		break;
	}

	// Check current OP contents and update if different
	CString ss;
	ASSERT(GetDlgItem(IDC_OP_DISPLAY) != NULL);
	GetDlgItem(IDC_OP_DISPLAY)->GetWindowText(ss);
	if (ss != disp)
		GetDlgItem(IDC_OP_DISPLAY)->SetWindowText(disp);
}

void CCalcDlg::do_unary(unary_type unary)
{
	if (state_ == CALCERROR)
	{
		make_noise("Calculator Error");
		CAvoidableDialog::Show(IDS_CALC_ERROR,
				"You cannot perform this operation on an invalid value.");
		//mm_->StatusBarText("Operation on invalid value.");
		aa_->mac_error_ = 10;
		return;
	}
	else if (state_ > CALCINTEXPR)
	{
		not_int_error();
		return;
	}

	// Save the value to macro before we do anything with it (unless it's a result of a previous op).
	// Also don't save the value if this (unary op) is the very first thing in the macro.
	save_value_to_macro();

	calc_unary(unary);  // This does that actual operation

	check_for_error();   // check for overflow & display error messages
	edit_.put();

	// Save the unary operation to the current macro (if any)
	aa_->SaveToMacro(km_unaryop, long(unary));

	// The user can't edit the result of a unary operation by pressing digits
	source_ = km_result;
}

void CCalcDlg::calc_unary(unary_type unary)
{
	CWaitCursor wc;   // show an hourglass just in case it takes a long time
	current_ = get_norm(current_, unary >= unary_not && unary <= unary_rev);
	ExprStringType tmp = right_;

	state_ = CALCINTUNARY;  // set default (could also be error/overflow as set below)
	switch (unary)
	{
	case unary_inc:
		++current_;
		if (previous_unary_op_ == unary_inc)
			right_ = inc_right_operand(right_);
		else
			right_.Format(EXPRSTR("(%s + 1)"), (const wchar_t *)tmp);
		break;
	case unary_dec:
		current_ --;
		if (previous_unary_op_ == unary_dec)
			right_ = inc_right_operand(right_);
		else
			right_.Format(EXPRSTR("(%s - 1)"), (const wchar_t *)tmp);
		break;
	case unary_sign:
		current_ = -current_;
		// Ask the user if they want to switch to signed numbers
		if (!signed_ && 
			current_ < min_val_ && 
			CAvoidableDialog::Show(IDS_CALC_NEGATIVE_UNSIGNED,
			                       "You are changing the sign of a positive number. "
			                       "Since you are currently using unsigned numbers "
			                       "this will cause an OVERFLOW condition.\n\n"
			                       "Do you want to change to using signed numbers?",
			                       "Toggle Signed?", MLCBF_YES_BUTTON | MLCBF_NO_BUTTON) == IDYES)
		{
			change_signed(!signed_);
		}
		right_.Format(EXPRSTR("-%s"), (const wchar_t *)tmp);
		break;
	case unary_square:
		current_ = current_ * current_;
		right_.Format(EXPRSTR("(%s * %s)"), (const wchar_t *)tmp, (const wchar_t *)tmp);
		break;
	case unary_squareroot:
		if (mpz_sgn(current_.get_mpz_t()) < 0)
		{
			current_str_ = "Square root of negative value";
			state_ = CALCERROR;
		}
		else
		{
			mpz_sqrt(current_.get_mpz_t(), current_.get_mpz_t());
			right_.Format(EXPRSTR("sqrt(%s)"), (const wchar_t *)without_parens(tmp));
		}
		break;
	case unary_cube:
		current_ = current_ * current_ * current_;
		right_.Format(EXPRSTR("(%s*%s*%s)"), (const wchar_t *)tmp, (const wchar_t *)tmp, (const wchar_t *)tmp);
		break;
	case unary_factorial:
		{
			mpz_fac_ui(current_.get_mpz_t(), current_.get_ui());
			right_.Format(EXPRSTR("fact(%s)"), (const wchar_t *)without_parens(tmp));
		}
		break;
	case unary_not:
		current_ = ~current_;
		right_.Format(EXPRSTR("~%s"), (const wchar_t *)tmp);
		break;
	case unary_rol:    // ROL1 button
		if (bits_ == 0)
		{
			current_str_ = "Can't rotate left if bit count is unlimited";
			state_ = CALCERROR;
		}
		else
		{
//			if (mpz_sgn(current_.get_mpz_t()) < 0)
//				mpz_neg(current_.get_mpz_t(), current_.get_mpz_t());

			current_ = ((current_ << 1) | (current_ >> (bits_ - 1))) & mask_;
			right_.Format(EXPRSTR("rol(%s, 1, %s)"), (const wchar_t *)without_parens(tmp), bits_as_string());
		}
		break;
	case unary_ror:   // ROR1 button
		if (bits_ == 0)
		{
			current_str_ = "Can't rotate right if bit count is unlimited";
			state_ = CALCERROR;
		}
		else
		{
			current_ = ((current_ >> 1) | (current_ << (bits_ - 1))) & mask_;
			right_.Format(EXPRSTR("ror(%s, 1, %s)"), (const wchar_t *)without_parens(tmp), bits_as_string());
		}
		break;
	case unary_lsl:
		current_ <<= 1;
		if (previous_unary_op_ == unary_lsl)
			right_ = inc_right_operand(right_);
		else
			right_.Format(EXPRSTR("(%s << 1)"), (const wchar_t *)tmp);
		break;
	case unary_lsr:
		current_ >>= 1;
		if (previous_unary_op_ == unary_lsr)
			right_ = inc_right_operand(right_);
		else
			right_.Format(EXPRSTR("(%s >> 1)"), (const wchar_t *)tmp);
		break;
	case unary_asr:
		if (bits_ == 0)
		{
			current_ >>= 1;
		}
		else
		{
			int neg = mpz_tstbit(current_.get_mpz_t(), bits_-1);  // is high bit on?
			current_ >>= 1;
			if (neg)
				mpz_setbit(current_.get_mpz_t(), bits_-1);
			right_.Format(EXPRSTR("asr(%s, 1, %s)"), (const wchar_t *)without_parens(tmp), bits_as_string());
		}
		break;
	case unary_rev:  // Reverse all bits
		{
			if (bits_ == 0)
			{
				current_str_ = "Can't reverse bits if bit count is unlimited";
				state_ = CALCERROR;
				break;
			}
			// This algorithm tests the current bits by shifting right and testing
			// bottom bit.  Any bits on sets the correspoding bit of the result as
			// it is shifted left.
			// This could be sped up using a table lookup but is probably OK for now.
			mpz_class temp = 0;
			for (int ii = 0; ii < bits_; ++ii)
			{
				temp <<= 1;                  // Make room for the next bit
				if (mpz_tstbit(current_.get_mpz_t(), 0))
					mpz_setbit(temp.get_mpz_t(), 0);
				current_ >>= 1;              // Move bits down to test the next
			}
			current_ = temp;
			right_.Format(EXPRSTR("reverse(%s, %s)"), (const wchar_t *)without_parens(tmp), bits_as_string());
		}
		break;
	case unary_flip: // Flip byte order
		// This code assumes that byte order is little-endian in memory
		if (bits_ == 0 || bits_%8 != 0)
		{
			current_str_ = "Bits must be divisible by 8 to flip bytes";
			state_ = CALCERROR;
		}
		else
		{
			int bytes = bits_ / 8;
			int units = (bytes + 3)/4; // number of DWORDs required
			mp_limb_t * buf = new mp_limb_t[units];
			for (int ii = 0; ii < units; ++ii)
			{
				if (ii < mpz_size(current_.get_mpz_t()))
					buf[ii] = mpz_getlimbn(current_.get_mpz_t(), ii);
				else
					buf[ii] = 0;
			}
			flip_bytes((unsigned char *)buf, bytes);
			mpz_import(current_.get_mpz_t(), units, -1, sizeof(mp_limb_t), -1, 0, buf);
			delete[] buf;
			right_.Format(EXPRSTR("flip(%s, %s)"), (const wchar_t *)without_parens(tmp), int_as_string(bits_/8));
		}
		break;
	case unary_at:
		if (GetView() == NULL)
		{
			no_file_error();
			return;  // just ignore this situation by returning with no action
			//current_str_ = "No file open to get data from";
			//state_ = CALCERROR;
		}
		else
		{
			// Check that current_ is within file address range 0 <= x <= eof.
			// get)bytes does this but only on a possibly truncated (64-bit) value.
			mpz_class eof;
			mpz_set_ui64(eof.get_mpz_t(), GetView()->GetDocument()->length());

			if (current_ < 0)
			{
				current_str_ = "Can't read before start of file";
				state_ = CALCERROR;
			}
			else if (current_ >= eof)
			{
				current_str_ = "Can't read past end of file";
				state_ = CALCERROR;
			}
			else if (!get_bytes(GetValue()))
				state_ = CALCERROR;
			else
				right_.Format(EXPRSTR("get(%s, %s)"), (const wchar_t *)without_parens(tmp), bits_as_string());
		}
		break;

	case unary_none:
	default:
		ASSERT(0);
	}
	if (state_ == CALCERROR)
		right_ = "***";

	previous_unary_op_ = unary;
}

// Handle "digit" button click
void CCalcDlg::do_digit(char digit)
{
	// Since digit buttons are "enabled" (but greyed) even when invalid we need to check for invalid digits
	int val = isdigit(digit) ? (digit - '0') : digit - 'A' + 10;
	if (val >= radix_)
	{
#ifdef SYS_SOUNDS
		if (!CSystemSound::Play("Invalid Character"))
#endif
			::Beep(5000,200);
		return;
	}

	edit_.SendMessage(WM_CHAR, digit, 1);
}

// Calculates any pending binary operation and leaves the result in current_.  Ie:
//   current_  =  previous_  op_  current_
// Note that binary operations (from calc buttons) are only valid for integer values.
// It is called when we want the final result (OnEquals/OnGo) or when we need to
// calcualte the current binary operation before preparing for the next (do_binop).
void CCalcDlg::calc_binary()
{
	if (op_ == binop_none) return;  // Handles case where state_ > CALCINTEXPR without changing state_

	CWaitCursor wc;   // show an hourglass just in case it takes a long time

	ASSERT(state_ != CALCERROR && state_ <= CALCINTEXPR);
	current_ = get_norm(current_);

	state_ = CALCINTRES;  // default to integer result - may be overridden due to error/overflow or for CALCINTBINARY
	switch (op_)
	{
	case binop_add:
		current_ += previous_;
		break;
	case binop_subtract:
		current_ = previous_ - current_;
		break;
	case binop_multiply:
		current_ *= previous_;
		break;
	case binop_divide:
		if (current_ == 0)
		{
			current_str_ = "Divide by zero";
			state_ = CALCERROR;
		}
		else
			current_ = previous_ / current_;
		break;
	case binop_mod:
		if (current_ == 0)
		{
			current_str_ = "Divide (modulus) by zero";
			state_ = CALCERROR;
		}
		else
			current_ = previous_ % current_;
		break;
	case binop_pow:
		if (current_ > mpz_class(ULONG_MAX))
		{
			current_ = 0;
			state_ = CALCOVERFLOW;
		}
		else
			mpz_pow_ui(current_.get_mpz_t(), previous_.get_mpz_t(), current_.get_si()); 
		break;
	case binop_gtr:
	case binop_gtr_old:
		if (current_ < previous_)
			current_ = previous_;
		break;
	case binop_less:
	case binop_less_old:
		if (current_ > previous_)
			current_ = previous_;
		break;
	case binop_rol:
		if (bits_ == 0)
		{
			current_str_ = "Can't rotate right if bit count is unlimited";
			state_ = CALCERROR;
		}
		else
		{
			mpz_class t1, t2;
			current_ %= bits_;
			mpz_mul_2exp(t1.get_mpz_t(), previous_.get_mpz_t(), current_.get_ui());
			mpz_fdiv_q_2exp(t2.get_mpz_t(), previous_.get_mpz_t(), bits_ - current_.get_ui());
			current_ = (t1 | t2) & mask_;
		}
		break;
	case binop_ror:
		if (bits_ == 0)
		{
			current_str_ = "Can't rotate left if bit count is unlimited";
			state_ = CALCERROR;
		}
		else
		{
			mpz_class t1, t2;
			current_ %= bits_;
			mpz_fdiv_q_2exp(t1.get_mpz_t(), previous_.get_mpz_t(), current_.get_ui());
			mpz_mul_2exp(t2.get_mpz_t(), previous_.get_mpz_t(), bits_ - current_.get_ui());
			current_ = (t1 | t2) & mask_;
		}
		break;
	case binop_lsl:
		mpz_mul_2exp(current_.get_mpz_t(), previous_.get_mpz_t(), current_.get_ui());
		break;
	case binop_lsr:
		mpz_fdiv_q_2exp(current_.get_mpz_t(), previous_.get_mpz_t(), current_.get_ui());
		break;
	case binop_asr:
		if (bits_ == 0)
			mpz_fdiv_q_2exp(current_.get_mpz_t(), previous_.get_mpz_t(), current_.get_ui());
		else
		{
			int shift = current_.get_si();
			int neg = mpz_tstbit(previous_.get_mpz_t(), bits_-1);
			if (shift > bits_)
			{
				// All bits shifted out
				if (neg)
					current_ = -1 & mask_;
				else
					current_ = 0;
			}
			else
			{
				current_ = previous_;
				for (int ii = 0; ii < shift; ++ii)
				{
					current_ >>= 1;
					if (neg) mpz_setbit(current_.get_mpz_t(), bits_-1);  // propagate high bit
				}
			}
		}
		break;

	case binop_and:
		current_ &= previous_;
		break;
	case binop_or:
		current_ |= previous_;
		break;
	case binop_xor:
		current_ ^= previous_;
		break;

	default:
		ASSERT(0);
		break;
	}
	right_ = get_expr(false);  // we need parentheses in case there are further operations

	op_ = binop_none;
	previous_unary_op_ = unary_none;
}  // calc_binary

void CCalcDlg::change_base(int base)
{
	if (base == radix_) return;         // no change

	radix_ = base;                      // Store new radix (affects edit control display)
	if (left_.IsEmpty() && (right_.IsEmpty() || right_ == "0") && op_ == binop_none)
		orig_radix_ = radix_;

	// If an integer literal redisplay
	if (inited_)
	{
		if (state_ <= CALCINTLIT)
		{
			state_ = CALCINTUNARY;  // need to clear any previous overflow
			check_for_error();      // check for overflow
			edit_.put();
		}

		source_ = km_result;
		aa_->SaveToMacro(km_change_base, long(radix_));
	}
}

void CCalcDlg::change_signed(bool s)
{
	if (s == signed_) return; // no change

	signed_ = s;
	fix_values();   // signed numbers have a different range of values

	if (inited_)
	{
		// If an integer expression redisplay
		if (state_ <= CALCINTLIT)
		{
			state_ = CALCINTUNARY;  // need to clear any previous overflow
			check_for_error();      // check for overflow
			edit_.put();
		}

		source_ = km_result;
		aa_->SaveToMacro(km_change_signed);
	}
}

void CCalcDlg::change_bits(int bits)
{
	if (bits == bits_) return; // no change

	bits_ = bits;
	fix_values();   // adjust value range etc for new bits setting

	if (inited_)
	{
		// If an integer expression redisplay
		if (state_ <= CALCINTLIT)
		{
			state_ = CALCINTUNARY;  // need to clear any previous overflow
			check_for_error();   // check for overflow
			edit_.put();
		}

		source_ = km_result;
		aa_->SaveToMacro(km_change_bits, long(bits_));
	}
}

void CCalcDlg::setup_resizer()
{
	// Add all the controls and proportional change to  LEFT, TOP, WIDTH, HEIGHT 
	// NOTE: Don't allow resize vertically of IDC_EDIT as it stuffs up the combo drop down list window size
	m_resizer.Add(IDC_EDIT, 0, 0, 100, 0);        // edit control resizes with width
	m_resizer.Add(IDC_EXPR, 0, 0, 100, 0);        // displays the expression that produced the current calculator value
	m_resizer.Add(IDC_OP_DISPLAY, 100, 0, 0, 0);  // operator display sticks to right edge (moves vert)
	m_resizer.Add(IDC_CALC_BITS, 0, 0, 100, 13);  // where "bits" are drawn

	// Settings controls don't move/size horizontally but move vert. and size slightly too
	m_resizer.Add(IDC_BIG_ENDIAN_FILE_ACCESS, 0, 13, 0, 13);
	m_resizer.Add(IDC_RADIX_GROUP, 0, 13, 0, 13);
	m_resizer.Add(IDC_DESC_RADIX, 0, 14, 0, 0);
	m_resizer.Add(IDC_RADIX, 0, 14, 0, 4);
	m_resizer.Add(IDC_SPIN_RADIX, 0, 14, 0, 4);
	m_resizer.Add(IDC_HEX, 0, 21, 0, 5);
	m_resizer.Add(IDC_DECIMAL, 0, 21, 0, 5);
	m_resizer.Add(IDC_OCTAL, 0, 21, 0, 5);
	m_resizer.Add(IDC_BINARY, 0, 21, 0, 5);

	m_resizer.Add(IDC_BITS_GROUP, 0, 13, 0, 13);
	m_resizer.Add(IDC_DESC_BITS, 0, 14, 0, 0);
	m_resizer.Add(IDC_BITS, 0, 14, 0, 4);
	m_resizer.Add(IDC_SPIN_BITS, 0, 14, 0, 4);
	m_resizer.Add(IDC_CALC_SIGNED, 0, 14, 0, 5);
	m_resizer.Add(IDC_INFBIT, 0, 21, 0, 5);
	m_resizer.Add(IDC_64BIT, 0, 21, 0, 5);
	m_resizer.Add(IDC_32BIT, 0, 21, 0, 5);
	m_resizer.Add(IDC_16BIT, 0, 21, 0, 5);
	m_resizer.Add(IDC_8BIT, 0, 21, 0, 5);

	// First row of buttons
	m_resizer.Add(IDC_MEM_GET, 1, 27, 12, 10);
	m_resizer.Add(IDC_MEM_STORE, 14, 27, 9, 10);
	m_resizer.Add(IDC_MEM_CLEAR, 23, 27, 9, 10);
	m_resizer.Add(IDC_MEM_ADD, 32, 27, 9, 10);
	m_resizer.Add(IDC_MEM_SUBTRACT, 41, 27, 8, 10);

	m_resizer.Add(IDC_BACKSPACE, 64, 26, 16, 10);
	m_resizer.Add(IDC_CLEAR_ENTRY, 80, 26, 10, 10);
	m_resizer.Add(IDC_CLEAR, 90, 26, 10, 10);

	// 2nd row of buttons
	m_resizer.Add(IDC_MARK_GET, 1, 37, 12, 10);
	m_resizer.Add(IDC_MARK_STORE, 14, 37, 9, 10);
	m_resizer.Add(IDC_MARK_CLEAR, 23, 37, 9, 10);
	m_resizer.Add(IDC_MARK_ADD, 32, 37, 9, 10);
	m_resizer.Add(IDC_MARK_SUBTRACT, 41, 37, 8, 10);
	m_resizer.Add(IDC_DIGIT_D, 50, 37, 8, 10);
	m_resizer.Add(IDC_DIGIT_E, 58, 37, 8, 10);
	m_resizer.Add(IDC_DIGIT_F, 66, 37, 8, 10);
	m_resizer.Add(IDC_POW, 75, 37, 8, 10);
	m_resizer.Add(IDC_GTR, 83, 37, 8, 10);
	m_resizer.Add(IDC_ROL, 91, 37, 8, 10);

	// 3rd row of buttons
	m_resizer.Add(IDC_MARK_AT, 1, 47, 12, 10);
	m_resizer.Add(IDC_MARK_AT_STORE, 14, 47, 9, 10);
	m_resizer.Add(IDC_UNARY_SQUARE, 23, 47, 9, 10);
	m_resizer.Add(IDC_UNARY_ROL, 32, 47, 9, 10);
	m_resizer.Add(IDC_UNARY_REV, 41, 47, 8, 10);
	m_resizer.Add(IDC_DIGIT_A, 50, 47, 8, 10);
	m_resizer.Add(IDC_DIGIT_B, 58, 47, 8, 10);
	m_resizer.Add(IDC_DIGIT_C, 66, 47, 8, 10);
	m_resizer.Add(IDC_MOD, 75, 47, 8, 10);
	m_resizer.Add(IDC_LESS, 83, 47, 8, 10);
	m_resizer.Add(IDC_ROR, 91, 47, 8, 10);

	// 4th row of buttons
	m_resizer.Add(IDC_SEL_GET, 1, 57, 12, 10);
	m_resizer.Add(IDC_SEL_STORE, 14, 57, 9, 10);
	m_resizer.Add(IDC_UNARY_SQUARE_ROOT, 23, 57, 9, 10);
	m_resizer.Add(IDC_UNARY_ROR, 32, 57, 9, 10);
	m_resizer.Add(IDC_UNARY_NOT, 41, 57, 8, 10);
	m_resizer.Add(IDC_DIGIT_7, 50, 57, 8, 10);
	m_resizer.Add(IDC_DIGIT_8, 58, 57, 8, 10);
	m_resizer.Add(IDC_DIGIT_9, 66, 57, 8, 10);
	m_resizer.Add(IDC_DIVIDE, 75, 57, 8, 10);
	m_resizer.Add(IDC_XOR, 83, 57, 8, 10);
	m_resizer.Add(IDC_LSL, 91, 57, 8, 10);

	// 5th row of buttons
	m_resizer.Add(IDC_SEL_AT, 1, 67, 12, 10);
	m_resizer.Add(IDC_SEL_AT_STORE, 14, 67, 9, 10);
	m_resizer.Add(IDC_UNARY_CUBE, 23, 67, 9, 10);
	m_resizer.Add(IDC_UNARY_LSL, 32, 67, 9, 10);
	m_resizer.Add(IDC_UNARY_INC, 41, 67, 8, 10);
	m_resizer.Add(IDC_DIGIT_4, 50, 67, 8, 10);
	m_resizer.Add(IDC_DIGIT_5, 58, 67, 8, 10);
	m_resizer.Add(IDC_DIGIT_6, 66, 67, 8, 10);
	m_resizer.Add(IDC_MULTIPLY, 75, 67, 8, 10);
	m_resizer.Add(IDC_OR, 83, 67, 8, 10);
	m_resizer.Add(IDC_LSR, 91, 67, 8, 10);

	// 6th row of buttons
	m_resizer.Add(IDC_SEL_LEN, 1, 77, 12, 10);
	m_resizer.Add(IDC_SEL_LEN_STORE, 14, 77, 9, 10);
	m_resizer.Add(IDC_UNARY_FACTORIAL, 23, 77, 9, 10);
	m_resizer.Add(IDC_UNARY_LSR, 32, 77, 9, 10);
	m_resizer.Add(IDC_UNARY_DEC, 41, 77, 8, 10);
	m_resizer.Add(IDC_DIGIT_1, 50, 77, 8, 10);
	m_resizer.Add(IDC_DIGIT_2, 58, 77, 8, 10);
	m_resizer.Add(IDC_DIGIT_3, 66, 77, 8, 10);
	m_resizer.Add(IDC_SUBTRACT, 75, 77, 8, 10);
	m_resizer.Add(IDC_AND, 83, 77, 8, 10);
	m_resizer.Add(IDC_ASR, 91, 77, 8, 10);

	// 7th row of buttons
	m_resizer.Add(IDC_EOF_GET, 1, 87, 12, 10);

	m_resizer.Add(IDC_UNARY_AT, 23, 87, 9, 10);
	m_resizer.Add(IDC_UNARY_ASR, 32, 87, 9, 10);
	m_resizer.Add(IDC_UNARY_FLIP, 41, 87, 8, 10);
	m_resizer.Add(IDC_DIGIT_0, 50, 87, 8, 10);
	m_resizer.Add(IDC_UNARY_SIGN, 58, 87, 8, 10);
	m_resizer.Add(IDC_EQUALS, 66, 87, 8, 10);
	m_resizer.Add(IDC_ADDOP, 75, 87, 8, 10);
	m_resizer.Add(IDC_GO, 83, 87, 16, 10);

	// We need this so that the resizer gets WM_SIZE event after the controls have been added.
	CRect cli;
	GetClientRect(&cli);
	PostMessage(WM_SIZE, SIZE_RESTORED, MAKELONG(cli.Width(), cli.Height()));
}

// Customize expr display (IDC_EXPR)
void CCalcDlg::setup_expr()
{
	// Make a smaller font for use with expression display etc
	LOGFONT lf;
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = 11;
	strcpy(lf.lfFaceName, "Tahoma");  // Simple font for small digits
	small_fnt_.CreateFontIndirect(&lf);

	lf.lfHeight = 16;
	med_fnt_.CreateFontIndirect(&lf);

	GetDlgItem(IDC_EXPR)->SetFont(&small_fnt_);
}

void CCalcDlg::setup_tooltips()
{
	// Set up tool tips
	if (ttc_.Create(this))
	{
		ttc_.ModifyStyleEx(0, WS_EX_TOPMOST);

		ASSERT(GetDlgItem(IDC_BACKSPACE) != NULL);
		ASSERT(GetDlgItem(IDC_CLEAR_ENTRY) != NULL);
		ASSERT(GetDlgItem(IDC_CLEAR) != NULL);
		ASSERT(GetDlgItem(IDC_EQUALS) != NULL);
		ASSERT(GetDlgItem(IDC_AND) != NULL);
		ASSERT(GetDlgItem(IDC_ASR) != NULL);
		ASSERT(GetDlgItem(IDC_DIVIDE) != NULL);
		ASSERT(GetDlgItem(IDC_LSL) != NULL);
		ASSERT(GetDlgItem(IDC_LSR) != NULL);
		ASSERT(GetDlgItem(IDC_ROL) != NULL);
		ASSERT(GetDlgItem(IDC_ROR) != NULL);
		ASSERT(GetDlgItem(IDC_XOR) != NULL);
		ASSERT(GetDlgItem(IDC_MOD) != NULL);
		ASSERT(GetDlgItem(IDC_MULTIPLY) != NULL);
		ASSERT(GetDlgItem(IDC_OR) != NULL);
		ASSERT(GetDlgItem(IDC_UNARY_DEC) != NULL);
		ASSERT(GetDlgItem(IDC_UNARY_FACTORIAL) != NULL);
		ASSERT(GetDlgItem(IDC_UNARY_FLIP) != NULL);
		ASSERT(GetDlgItem(IDC_UNARY_REV) != NULL);
		ASSERT(GetDlgItem(IDC_UNARY_INC) != NULL);
		ASSERT(GetDlgItem(IDC_UNARY_NOT) != NULL);
		ASSERT(GetDlgItem(IDC_UNARY_SIGN) != NULL);
		ASSERT(GetDlgItem(IDC_UNARY_SQUARE) != NULL);
		ASSERT(GetDlgItem(IDC_SUBTRACT) != NULL);
		ASSERT(GetDlgItem(IDC_MEM_GET) != NULL);
		ASSERT(GetDlgItem(IDC_MEM_CLEAR) != NULL);
		ASSERT(GetDlgItem(IDC_MEM_ADD) != NULL);
		ASSERT(GetDlgItem(IDC_MEM_SUBTRACT) != NULL);
		ASSERT(GetDlgItem(IDC_MARK_GET) != NULL);
		ASSERT(GetDlgItem(IDC_MARK_AT) != NULL);
		ASSERT(GetDlgItem(IDC_MARK_ADD) != NULL);
		ASSERT(GetDlgItem(IDC_MARK_SUBTRACT) != NULL);
		ASSERT(GetDlgItem(IDC_SEL_GET) != NULL);
		ASSERT(GetDlgItem(IDC_SEL_AT) != NULL);
		ASSERT(GetDlgItem(IDC_EOF_GET) != NULL);
		ASSERT(GetDlgItem(IDC_GTR) != NULL);
		ASSERT(GetDlgItem(IDC_LESS) != NULL);
		ASSERT(GetDlgItem(IDC_POW) != NULL);
		ASSERT(GetDlgItem(IDC_MARK_AT_STORE) != NULL);
		ASSERT(GetDlgItem(IDC_MARK_CLEAR) != NULL);
		ASSERT(GetDlgItem(IDC_MARK_STORE) != NULL);
		ASSERT(GetDlgItem(IDC_MEM_STORE) != NULL);
		ASSERT(GetDlgItem(IDC_SEL_AT_STORE) != NULL);
		ASSERT(GetDlgItem(IDC_SEL_STORE) != NULL);
		ASSERT(GetDlgItem(IDC_UNARY_ROL) != NULL);
		ASSERT(GetDlgItem(IDC_UNARY_ROR) != NULL);
		ASSERT(GetDlgItem(IDC_UNARY_LSL) != NULL);
		ASSERT(GetDlgItem(IDC_UNARY_LSR) != NULL);
		ASSERT(GetDlgItem(IDC_UNARY_ASR) != NULL);
		ASSERT(GetDlgItem(IDC_UNARY_CUBE) != NULL);
		ASSERT(GetDlgItem(IDC_UNARY_AT) != NULL);
		ASSERT(GetDlgItem(IDC_UNARY_SQUARE_ROOT) != NULL);
		ASSERT(GetDlgItem(IDC_SEL_LEN) != NULL);
		ASSERT(GetDlgItem(IDC_SEL_LEN_STORE) != NULL);
		ASSERT(GetDlgItem(IDC_ADDOP) != NULL);

		ASSERT(GetDlgItem(IDC_GO) != NULL);
		ASSERT(GetDlgItem(IDC_8BIT) != NULL);
		ASSERT(GetDlgItem(IDC_16BIT) != NULL);
		ASSERT(GetDlgItem(IDC_32BIT) != NULL);
		ASSERT(GetDlgItem(IDC_64BIT) != NULL);
		ASSERT(GetDlgItem(IDC_INFBIT) != NULL);
		ASSERT(GetDlgItem(IDC_BINARY) != NULL);
		ASSERT(GetDlgItem(IDC_OCTAL) != NULL);
		ASSERT(GetDlgItem(IDC_DECIMAL) != NULL);
		ASSERT(GetDlgItem(IDC_HEX) != NULL);
		ASSERT(GetDlgItem(IDC_BIG_ENDIAN_FILE_ACCESS) != NULL);

		ttc_.AddTool(GetDlgItem(IDC_MEM_GET), "Memory Recall (Ctrl+R)");
		ttc_.AddTool(GetDlgItem(IDC_MEM_STORE), "Memory Store (Ctrl+M)");
		ttc_.AddTool(GetDlgItem(IDC_MEM_CLEAR), "Memory Clear (Ctrl+L)");
		ttc_.AddTool(GetDlgItem(IDC_MEM_ADD), "Memory Add (Ctrl+P)");
		ttc_.AddTool(GetDlgItem(IDC_MEM_SUBTRACT), "Memory Subtract");
		ttc_.AddTool(GetDlgItem(IDC_BACKSPACE), "Delete Last Digit");
		ttc_.AddTool(GetDlgItem(IDC_CLEAR_ENTRY), "Clear Entry");
		ttc_.AddTool(GetDlgItem(IDC_CLEAR), "Clear All");
		ttc_.AddTool(GetDlgItem(IDC_EQUALS), "Calculate Result");
		ttc_.AddTool(GetDlgItem(IDC_GO), "Jump To Resultant Address");

		ttc_.AddTool(GetDlgItem(IDC_ADDOP), "Add");
		ttc_.AddTool(GetDlgItem(IDC_SUBTRACT), "Subtract");
		ttc_.AddTool(GetDlgItem(IDC_MULTIPLY), "Multiply");
		ttc_.AddTool(GetDlgItem(IDC_DIVIDE), "Integer Divide");
		ttc_.AddTool(GetDlgItem(IDC_MOD), "Remainder");
		ttc_.AddTool(GetDlgItem(IDC_POW), "Integer Power");
		ttc_.AddTool(GetDlgItem(IDC_GTR), "Greater of Two Values");
		ttc_.AddTool(GetDlgItem(IDC_LESS), "Smaller of Two Values");
		ttc_.AddTool(GetDlgItem(IDC_AND), "AND");
		ttc_.AddTool(GetDlgItem(IDC_OR), "OR");
		ttc_.AddTool(GetDlgItem(IDC_XOR), "Exclusive Or");
		ttc_.AddTool(GetDlgItem(IDC_LSL), "Logical Shift Left");
		ttc_.AddTool(GetDlgItem(IDC_LSR), "Logical Shift Right");
		ttc_.AddTool(GetDlgItem(IDC_ASR), "Arithmetic Shift Right");
		ttc_.AddTool(GetDlgItem(IDC_ROL), "Rotate Left");
		ttc_.AddTool(GetDlgItem(IDC_ROR), "Rotate Right");

		ttc_.AddTool(GetDlgItem(IDC_UNARY_FLIP), "Reverse Byte Order");
		ttc_.AddTool(GetDlgItem(IDC_UNARY_REV), "Reverse Bit Order");
		ttc_.AddTool(GetDlgItem(IDC_UNARY_DEC), "Subtract One");
		ttc_.AddTool(GetDlgItem(IDC_UNARY_INC), "Add One");
		ttc_.AddTool(GetDlgItem(IDC_UNARY_SIGN), "Change Sign");
		ttc_.AddTool(GetDlgItem(IDC_UNARY_SQUARE), "Square");
		ttc_.AddTool(GetDlgItem(IDC_UNARY_CUBE), "Cube");
		ttc_.AddTool(GetDlgItem(IDC_UNARY_SQUARE_ROOT), "Integer Square Root");
		ttc_.AddTool(GetDlgItem(IDC_UNARY_FACTORIAL), "Factorial");
		ttc_.AddTool(GetDlgItem(IDC_UNARY_NOT), "NOT");
		ttc_.AddTool(GetDlgItem(IDC_UNARY_ROL), "Rotate Left by One Bit");
		ttc_.AddTool(GetDlgItem(IDC_UNARY_ROR), "Rotate Right by One Bit");
		ttc_.AddTool(GetDlgItem(IDC_UNARY_LSL), "Logical Shift Left by One Bit");
		ttc_.AddTool(GetDlgItem(IDC_UNARY_LSR), "Logical Shift Right by One Bit");
		ttc_.AddTool(GetDlgItem(IDC_UNARY_ASR), "Arithmetic Shift Right by One Bit");
		ttc_.AddTool(GetDlgItem(IDC_UNARY_AT), "Retrieve the Value from File");

		ttc_.AddTool(GetDlgItem(IDC_MARK_GET), "Address of Mark");
		ttc_.AddTool(GetDlgItem(IDC_MARK_STORE), "Move the Mark");
		ttc_.AddTool(GetDlgItem(IDC_MARK_CLEAR), "Set the Mark to Start of File");
		ttc_.AddTool(GetDlgItem(IDC_MARK_ADD), "Add to Mark");
		ttc_.AddTool(GetDlgItem(IDC_MARK_SUBTRACT), "Subtract from Mark");
		ttc_.AddTool(GetDlgItem(IDC_MARK_AT), "Get the Value at the Mark");
		ttc_.AddTool(GetDlgItem(IDC_MARK_AT_STORE), "Store at Mark");
		ttc_.AddTool(GetDlgItem(IDC_SEL_GET), "Cursor Location");
		ttc_.AddTool(GetDlgItem(IDC_SEL_STORE), "Move the Cursor");
		ttc_.AddTool(GetDlgItem(IDC_SEL_AT), "Get the Value at the Cursor");
		ttc_.AddTool(GetDlgItem(IDC_SEL_AT_STORE), "Store to File at Cursor");
		ttc_.AddTool(GetDlgItem(IDC_SEL_LEN), "Get the Length of the Selection");
		ttc_.AddTool(GetDlgItem(IDC_SEL_LEN_STORE), "Change the Length of the Selection");
		ttc_.AddTool(GetDlgItem(IDC_EOF_GET), "Get the Length of File");
		ttc_.AddTool(GetDlgItem(IDC_8BIT), "Use Bytes (F12)");
		ttc_.AddTool(GetDlgItem(IDC_16BIT), "Use Words (F11)");
		ttc_.AddTool(GetDlgItem(IDC_32BIT), "Use Double Words (F10)");
		ttc_.AddTool(GetDlgItem(IDC_64BIT), "Use Quad Words (F9)");
		ttc_.AddTool(GetDlgItem(IDC_INFBIT), "Unlimited integer precision");
		ttc_.AddTool(GetDlgItem(IDC_BINARY), "Use Binary Numbers (F8)");
		ttc_.AddTool(GetDlgItem(IDC_OCTAL), "USe Octal Numbers (F7)");
		ttc_.AddTool(GetDlgItem(IDC_DECIMAL), "Use (Signed) Decimal Numbers (F6)");
		ttc_.AddTool(GetDlgItem(IDC_HEX), "Use Hexadecimal Numbers (F5)");
		ttc_.AddTool(GetDlgItem(IDC_BIG_ENDIAN_FILE_ACCESS), "Read/Write to File in Big-Endian Format");
		ttc_.Activate(TRUE);
	}
}

void CCalcDlg::setup_static_buttons()
{
	// Things that use the calculator value and other "admin" type buttons are red
	button_colour(GetDlgItem(IDC_BACKSPACE), true, RGB(0xC0, 0x0, 0x0));
	button_colour(GetDlgItem(IDC_CLEAR_ENTRY), true, RGB(0xC0, 0x0, 0x0));
	button_colour(GetDlgItem(IDC_CLEAR), true, RGB(0xC0, 0x0, 0x0));
	button_colour(GetDlgItem(IDC_MEM_STORE), true, RGB(0xC0, 0x0, 0x0));
	button_colour(GetDlgItem(IDC_MEM_CLEAR), true, RGB(0xC0, 0x0, 0x0));
	button_colour(GetDlgItem(IDC_MEM_ADD), true, RGB(0xC0, 0x0, 0x0));
	button_colour(GetDlgItem(IDC_MEM_SUBTRACT), true, RGB(0xC0, 0x0, 0x0));
	button_colour(GetDlgItem(IDC_EQUALS), true, RGB(0xC0, 0x0, 0x0));

	// Binary operations are blue
	button_colour(GetDlgItem(IDC_ADDOP), true, RGB(0x0, 0x0, 0xC0));
	button_colour(GetDlgItem(IDC_SUBTRACT), true, RGB(0x0, 0x0, 0xC0));
	button_colour(GetDlgItem(IDC_MULTIPLY), true, RGB(0x0, 0x0, 0xC0));
	button_colour(GetDlgItem(IDC_DIVIDE), true, RGB(0x0, 0x0, 0xC0));
	button_colour(GetDlgItem(IDC_MOD), true, RGB(0x0, 0x0, 0xC0));
	button_colour(GetDlgItem(IDC_POW), true, RGB(0x0, 0x0, 0xC0));
	button_colour(GetDlgItem(IDC_GTR), true, RGB(0x0, 0x0, 0xC0));
	button_colour(GetDlgItem(IDC_LESS), true, RGB(0x0, 0x0, 0xC0));
	button_colour(GetDlgItem(IDC_XOR), true, RGB(0x0, 0x0, 0xC0));
	button_colour(GetDlgItem(IDC_OR), true, RGB(0x0, 0x0, 0xC0));
	button_colour(GetDlgItem(IDC_AND), true, RGB(0x0, 0x0, 0xC0));
	button_colour(GetDlgItem(IDC_LSL), true, RGB(0x0, 0x0, 0xC0));
	button_colour(GetDlgItem(IDC_LSR), true, RGB(0x0, 0x0, 0xC0));
	button_colour(GetDlgItem(IDC_ASR), true, RGB(0x0, 0x0, 0xC0));

	// Unary operations are purple
	button_colour(GetDlgItem(IDC_UNARY_SIGN), true, RGB(0x80, 0x0, 0x80));
	button_colour(GetDlgItem(IDC_UNARY_NOT), true, RGB(0x80, 0x0, 0x80));
	button_colour(GetDlgItem(IDC_UNARY_INC), true, RGB(0x80, 0x0, 0x80));
	button_colour(GetDlgItem(IDC_UNARY_DEC), true, RGB(0x80, 0x0, 0x80));
	button_colour(GetDlgItem(IDC_UNARY_LSL), true, RGB(0x80, 0x0, 0x80));
	button_colour(GetDlgItem(IDC_UNARY_LSR), true, RGB(0x80, 0x0, 0x80));
	button_colour(GetDlgItem(IDC_UNARY_ASR), true, RGB(0x80, 0x0, 0x80));
	button_colour(GetDlgItem(IDC_UNARY_SQUARE), true, RGB(0x80, 0x0, 0x80));
	button_colour(GetDlgItem(IDC_UNARY_SQUARE_ROOT), true, RGB(0x80, 0x0, 0x80));
	button_colour(GetDlgItem(IDC_UNARY_CUBE), true, RGB(0x80, 0x0, 0x80));
	button_colour(GetDlgItem(IDC_UNARY_FACTORIAL), true, RGB(0x80, 0x0, 0x80));

	// Digit buttons (0 and 1) are always "enabled" (dark grey)
	button_colour(GetDlgItem(IDC_DIGIT_0), true, RGB(0x40, 0x40, 0x40));
	button_colour(GetDlgItem(IDC_DIGIT_1), true, RGB(0x40, 0x40, 0x40));
}

void CCalcDlg::update_controls()
{
	// Check if case has changed for edit box
	if (radix_  > 10 && state_ <= CALCINTLIT)
	{
		CString ss, new_ss;

		DWORD sel = edit_.GetSel();
		edit_.GetWindowText(ss);

		new_ss = ss;
		if (theApp.hex_ucase_)
			new_ss.MakeUpper();
		else
			new_ss.MakeLower();

		if (new_ss != ss)
		{
			edit_.SetWindowText(ss);
			edit_.SetSel(sel);
		}
	}
	update_op();
	update_expr();
	update_modes();  // Radix, bits, signed controls

	update_digit_buttons();
	update_file_buttons();

	// Update misc buttons
	button_colour(GetDlgItem(IDC_UNARY_FLIP), bits_ > 8 && bits_%8 == 0, RGB(0x80, 0x0, 0x80)); // can't flip single byte or individual bits
	button_colour(GetDlgItem(IDC_UNARY_REV), bits_ > 0, RGB(0x80, 0x0, 0x80));   // Can't reverse an inf. number of bits

	// Can't wrap around with unlimited precision
	button_colour(GetDlgItem(IDC_UNARY_ROL), bits_ > 0, RGB(0x80, 0x0, 0x80));
	button_colour(GetDlgItem(IDC_UNARY_ROR), bits_ > 0, RGB(0x80, 0x0, 0x80));
	button_colour(GetDlgItem(IDC_ROL), bits_ > 0, RGB(0x0, 0x0, 0xC0));
	button_colour(GetDlgItem(IDC_ROR), bits_ > 0, RGB(0x0, 0x0, 0xC0));

	// Disable big-endian checkbox if there is only one byte or not using whole bytes, or no active file
	ASSERT(GetDlgItem(IDC_BIG_ENDIAN_FILE_ACCESS) != NULL);
	BOOL enable = GetView() != NULL && bits_ > 8 && bits_%8 == 0;
	if (enable != GetDlgItem(IDC_BIG_ENDIAN_FILE_ACCESS)->IsWindowEnabled())
		GetDlgItem(IDC_BIG_ENDIAN_FILE_ACCESS)->EnableWindow(enable);

	build_menus();
}

void CCalcDlg::update_modes()
{
	CString ss;
	char buf[6];

	// Update controls showing radix
	ASSERT(GetDlgItem(IDC_RADIX) != NULL);
	GetDlgItemText(IDC_RADIX, ss);
	sprintf(buf, "%d", radix_);
	if (ss != buf)
	{
		ss = buf;
		SetDlgItemText(IDC_RADIX, ss);
	}
	// Update radio buttons
	int radix_index = -1;
	switch (radix_)
	{
	case 2:
		radix_index = 3;
		break;
	case 8:
		radix_index = 2;
		break;
	case 10:
		radix_index = 1;
		break;
	case 16:
		radix_index = 0;
		break;
	}
	((CButton *)GetDlgItem(IDC_HEX    ))->SetCheck(radix_index == 0);
	((CButton *)GetDlgItem(IDC_DECIMAL))->SetCheck(radix_index == 1);
	((CButton *)GetDlgItem(IDC_OCTAL  ))->SetCheck(radix_index == 2);
	((CButton *)GetDlgItem(IDC_BINARY ))->SetCheck(radix_index == 3);

	// Update controls showing number of bits
	ASSERT(GetDlgItem(IDC_BITS) != NULL);
	GetDlgItemText(IDC_BITS, ss);
	sprintf(buf, "%d", bits_);
	if (ss != buf)
	{
		ss = buf;
		SetDlgItemText(IDC_BITS, ss);
	}
	// Update radio buttons
	int bits_index = -1;
	switch (bits_)
	{
	case 8:
		bits_index = 4;
		break;
	case 16:
		bits_index = 3;
		break;
	case 32:
		bits_index = 2;
		break;
	case 64:
		bits_index = 1;
		break;
	case 0:
		bits_index = 0;
		break;
	}
	((CButton *)GetDlgItem(IDC_8BIT ))->SetCheck(bits_index == 4);
	((CButton *)GetDlgItem(IDC_16BIT))->SetCheck(bits_index == 3);
	((CButton *)GetDlgItem(IDC_32BIT))->SetCheck(bits_index == 2);
	((CButton *)GetDlgItem(IDC_64BIT))->SetCheck(bits_index == 1);
	((CButton *)GetDlgItem(IDC_INFBIT))->SetCheck(bits_index == 0);

	// Update checkbox showing signedness
	ASSERT(GetDlgItem(IDC_CALC_SIGNED) != NULL);
	CButton *pb = (CButton *)GetDlgItem(IDC_CALC_SIGNED);
	BOOL enabled = pb->IsWindowEnabled();

	// If we have infinite bits (bits_ == 0) then we must allow signed numbers (eg all bits on = -1)
	BOOL en = bits_ > 0;
	pb->SetCheck(!en || signed_);  // turn on tick when disabled to indicate that inf bits uses signed numbers
	if (en != enabled)
		pb->EnableWindow(en);
}

// Fix digit buttons depending on the radix
void CCalcDlg::update_digit_buttons()
{
	// Enable/disable digit keys depending on base
	button_colour(GetDlgItem(IDC_DIGIT_2), radix_ > 2, RGB(0x40, 0x40, 0x40));
	button_colour(GetDlgItem(IDC_DIGIT_3), radix_ > 3, RGB(0x40, 0x40, 0x40));
	button_colour(GetDlgItem(IDC_DIGIT_4), radix_ > 4, RGB(0x40, 0x40, 0x40));
	button_colour(GetDlgItem(IDC_DIGIT_5), radix_ > 5, RGB(0x40, 0x40, 0x40));
	button_colour(GetDlgItem(IDC_DIGIT_6), radix_ > 6, RGB(0x40, 0x40, 0x40));
	button_colour(GetDlgItem(IDC_DIGIT_7), radix_ > 7, RGB(0x40, 0x40, 0x40));
	button_colour(GetDlgItem(IDC_DIGIT_8), radix_ > 8, RGB(0x40, 0x40, 0x40));
	button_colour(GetDlgItem(IDC_DIGIT_9), radix_ > 9, RGB(0x40, 0x40, 0x40));
	button_colour(GetDlgItem(IDC_DIGIT_A), radix_ > 10, RGB(0x40, 0x40, 0x40));
	button_colour(GetDlgItem(IDC_DIGIT_B), radix_ > 11, RGB(0x40, 0x40, 0x40));
	button_colour(GetDlgItem(IDC_DIGIT_C), radix_ > 12, RGB(0x40, 0x40, 0x40));
	button_colour(GetDlgItem(IDC_DIGIT_D), radix_ > 13, RGB(0x40, 0x40, 0x40));
	button_colour(GetDlgItem(IDC_DIGIT_E), radix_ > 14, RGB(0x40, 0x40, 0x40));
	button_colour(GetDlgItem(IDC_DIGIT_F), radix_ > 15, RGB(0x40, 0x40, 0x40));
}

void CCalcDlg::update_file_buttons()
{
	ASSERT(IsVisible());      // Should only be called if dlg is visible
	ASSERT(GetDlgItem(IDC_BIG_ENDIAN_FILE_ACCESS) != NULL);

	CHexEditView *pview = GetView();
	int bytes = bits_ == 0 ? INT_MAX : bits_/8;   // # of bytes when reading/writing file

	mpz_class start, eof, mark, sel_len;

	if (pview != NULL)
	{
		mpz_set_ui64(eof.get_mpz_t(), pview->GetDocument()->length());
		mpz_set_ui64(mark.get_mpz_t(), pview->GetMark());
		FILE_ADDRESS s, e;                    // Current selection
		pview->GetSelAddr(s, e);
		mpz_set_ui64(start.get_mpz_t(), s);
		mpz_set_ui64(sel_len.get_mpz_t(), e - s);
	}

	// Most file buttons are only enabled if there is an active file open and calc has an INT value in it
	bool basic = pview != NULL && state_ <= CALCINTEXPR;

	// Red buttons
	button_colour(GetDlgItem(IDC_MARK_STORE),    basic && get_norm(current_) >= 0 &&
	                                                      get_norm(current_) <= eof,             RGB(0xC0, 0x0, 0x0));
	button_colour(GetDlgItem(IDC_MARK_CLEAR),    pview != NULL,                                  RGB(0xC0, 0x0, 0x0));
	button_colour(GetDlgItem(IDC_MARK_ADD),      basic && mark + get_norm(current_) >= 0 &&
	                                                      mark + get_norm(current_) <= eof,      RGB(0xC0, 0x0, 0x0));
	button_colour(GetDlgItem(IDC_MARK_SUBTRACT), basic && mark - get_norm(current_) >= 0 &&
	                                                      mark - get_norm(current_) <= eof,      RGB(0xC0, 0x0, 0x0));
	button_colour(GetDlgItem(IDC_MARK_AT_STORE), basic && !pview->ReadOnly() && mark <= eof &&
	                                             (mark + bytes <= eof || !pview->OverType()),  RGB(0xC0, 0x0, 0x0));
	button_colour(GetDlgItem(IDC_SEL_STORE),     basic && get_norm(current_) >= 0 &&
	                                                      get_norm(current_) <= eof,             RGB(0xC0, 0x0, 0x0));
	button_colour(GetDlgItem(IDC_SEL_AT_STORE),  basic && !pview->ReadOnly() && start <= eof &&
	                                             (start + bytes <= eof || !pview->OverType()), RGB(0xC0, 0x0, 0x0));
	button_colour(GetDlgItem(IDC_SEL_LEN_STORE), basic && get_norm(current_) >= 0 &&
	                                                      start + get_norm(current_) <= eof,     RGB(0xC0, 0x0, 0x0));
	button_colour(GetDlgItem(IDC_GO),            basic && get_norm(current_) <= eof,             RGB(0xC0, 0x0, 0x0));

	// Dark grey buttons
	button_colour(GetDlgItem(IDC_MARK_GET),      pview != NULL && mark <= max_val_,              RGB(0x40, 0x40, 0x40));
	button_colour(GetDlgItem(IDC_MARK_AT),       pview != NULL && mark + bytes <= eof,         RGB(0x40, 0x40, 0x40));
	button_colour(GetDlgItem(IDC_SEL_GET),       pview != NULL && start <= max_val_,             RGB(0x40, 0x40, 0x40));
	button_colour(GetDlgItem(IDC_SEL_AT),        pview != NULL && start + bytes <= eof,        RGB(0x40, 0x40, 0x40));
	button_colour(GetDlgItem(IDC_SEL_LEN),       pview != NULL && sel_len <= max_val_,           RGB(0x40, 0x40, 0x40));
	button_colour(GetDlgItem(IDC_EOF_GET),       pview != NULL && eof <= max_val_,               RGB(0x40, 0x40, 0x40));

	// Purple
	button_colour(GetDlgItem(IDC_UNARY_AT),      basic && get_norm(current_) + bytes <= eof,   RGB(0x80, 0x0, 0x80));

	if (pview != NULL)
	{
		GetDlgItem(IDC_BIG_ENDIAN_FILE_ACCESS)->EnableWindow(TRUE);  // enable so we can change it
		CButton *pb = (CButton *)GetDlgItem(IDC_BIG_ENDIAN_FILE_ACCESS);
		pb->SetCheck(pview->BigEndian());
	}
	GetDlgItem(IDC_BIG_ENDIAN_FILE_ACCESS)->EnableWindow(pview != NULL && bits_ > 8);
}

// Button drawing funcs for OnDrawItem below
static void Draw3DFrame(CDC *pDC, CRect rcBox, COLORREF colBottomRight, COLORREF colTopLeft)
{
	CPen *pPen2, *pPen, *pOldPen;

	pPen = new CPen(PS_SOLID, 1, colBottomRight);
	pOldPen = pDC->SelectObject(pPen);
	pDC->MoveTo(rcBox.right-1, rcBox.top);
	pDC->LineTo(rcBox.right-1, rcBox.bottom-1);
	pDC->LineTo(rcBox.left-1, rcBox.bottom-1);

	pPen2 = new CPen(PS_SOLID, 1, colTopLeft);
	pDC->SelectObject(pPen2);
	delete pPen;

	pDC->MoveTo(rcBox.left, rcBox.bottom-2);
	pDC->LineTo(rcBox.left, rcBox.top);
	pDC->LineTo(rcBox.right-1, rcBox.top);

	pDC->SelectObject(pOldPen);
	delete pPen2;
}

static void Draw3DButtonFrame(CDC *pDC, CRect rcButton, BOOL bFocus)
{
	CPen *pPen, *pOldPen;
	CBrush GrayBrush(GetSysColor(COLOR_BTNSHADOW));
	CBrush BlackBrush(GetSysColor(COLOR_BTNFACE));

	pPen = new CPen(PS_SOLID, 1, GetSysColor(COLOR_WINDOWFRAME));
	pOldPen = pDC->SelectObject(pPen);

	// Draw gray outside
	pDC->FrameRect(&rcButton, &BlackBrush);
	rcButton.InflateRect(-1, -1);

	pDC->MoveTo(rcButton.left+1, rcButton.top);
	pDC->LineTo(rcButton.right-1, rcButton.top);
	pDC->MoveTo(rcButton.left+1, rcButton.bottom-1);
	pDC->LineTo(rcButton.right-1, rcButton.bottom-1);
	pDC->MoveTo(rcButton.left, rcButton.top+1);
	pDC->LineTo(rcButton.left, rcButton.bottom-1);
	pDC->MoveTo(rcButton.right-1, rcButton.top+1);
	pDC->LineTo(rcButton.right-1, rcButton.bottom-1);

	if (bFocus)
	{
		rcButton.InflateRect(-3, -3);
		pDC->FrameRect(&rcButton, &GrayBrush);
	}

	pDC->SelectObject(pOldPen);
	delete pPen;
}

/////////////////////////////////////////////////////////////////////////////
// CCalcDlg message handlers

int CCalcDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}

void CCalcDlg::OnDestroy()
{
	// All history items have a CString on the heap associated with them.  We delete
	// the strings here (as WM_DELETEITEM is only saent for owner-draw combos).
	for (int ii = 0; ii < ctl_edit_combo_.GetCount(); ++ii)
	{
		CString * ps = (CString *)ctl_edit_combo_.GetItemDataPtr(ii);
		if (ps != NULL)
		{
			delete ps;
			ctl_edit_combo_.SetItemDataPtr(ii, NULL);  // set it to NULL to be safe
		}
	}

	CDialog::OnDestroy();

	small_fnt_.DeleteObject();
	med_fnt_.DeleteObject();

	if (m_pBrush != NULL)
	{
		delete m_pBrush;
		m_pBrush = NULL;
	}

	// Save some settings to the in file/registry
	aa_->WriteProfileInt("Calculator", "Base", radix_);
	aa_->WriteProfileInt("Calculator", "Bits", bits_);
	aa_->WriteProfileInt("Calculator", "signed", signed_ ? 1 : 0);

	std::string ss;
	ss = memory_.get_str(10);
	aa_->WriteProfileString("Calculator", "Memory", ss.c_str());

	if (state_ <= CALCINTEXPR)  // This test avoids errors being displayed during shutdown
		calc_binary();
	ss = current_.get_str(10);
	aa_->WriteProfileString("Calculator", "Current", ss.c_str());
}

void CCalcDlg::OnSize(UINT nType, int cx, int cy)   // WM_SIZE
{
	if (!inited_) return;

	if (cy < m_sizeInitial.cy*3/2)
	{
		//SetDlgItemText(IDC_RADIX_GROUP, "");
		//SetDlgItemText(IDC_BITS_GROUP, "");
		GetDlgItem(IDC_RADIX)->SetFont(&small_fnt_);
		GetDlgItem(IDC_BITS)->SetFont(&small_fnt_);
	}
	else
	{
		//SetDlgItemText(IDC_RADIX_GROUP, "Base");
		//SetDlgItemText(IDC_BITS_GROUP, "Bits");
		GetDlgItem(IDC_RADIX)->SetFont(&med_fnt_);
		GetDlgItem(IDC_BITS)->SetFont(&med_fnt_);
	}

	if (cx > 0 && m_sizeInitial.cx == -1)
		m_sizeInitial = CSize(cx, cy);
	CDialog::OnSize(nType, cx, cy);
}

BOOL CCalcDlg::OnHelpInfo(HELPINFO* pHelpInfo)
{
	// Note calling theApp.HtmlHelpWmHelp here seems to make the window go behind 
	// and then disappear when mouse up event is seen.  The only soln I could
	// find after a lot of experimenetation is to do it later (in OnKickIdle).
//	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs);
	help_hwnd_ = (HWND)pHelpInfo->hItemHandle;
	return TRUE;
}

void CCalcDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.HtmlHelpContextMenu(pWnd, id_pairs);
}

CBrush * CCalcDlg::m_pBrush = NULL;

BOOL CCalcDlg::OnEraseBkgnd(CDC *pDC)
{
	// We check for changed look in erase background event as it's done
	// before other drawing.  This is necessary (to update m_pBrush etc)
	// because there is no message sent when the look changes.
	static UINT saved_look = 0;
	if (theApp.m_nAppLook != saved_look)
	{
		// Create new brush used for background of static controls
		if (m_pBrush != NULL)
			delete m_pBrush;
		m_pBrush = new CBrush(afxGlobalData.clrBarFace);

		saved_look = theApp.m_nAppLook;
	}

	CRect rct;
	GetClientRect(&rct);

	// Fill the background with a colour that matches the current BCG theme (and hence sometimes with the Windows Theme)
	pDC->FillSolidRect(rct, afxGlobalData.clrBarFace);
	return TRUE;
}

HBRUSH CCalcDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (nCtlColor == CTLCOLOR_STATIC)
	{
		pDC->SetBkMode(TRANSPARENT);                            // Make sure text has no background
		if (m_pBrush != NULL)
			return (HBRUSH)*m_pBrush;
	}

	return CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
}

LRESULT CCalcDlg::OnKickIdle(WPARAM, LPARAM)
{
	if (m_first)
	{
		setup_resizer();
		m_first = false;
	}

	update_controls();

	// If help_hwnd_ is not NULL then help has been requested for that window
	if (help_hwnd_ != (HWND)0)
	{
		theApp.HtmlHelpWmHelp(help_hwnd_, id_pairs);
		help_hwnd_ = (HWND)0;  // signal that it was handled
	}
	return FALSE;
}

void CCalcDlg::OnSelHistory()
{
	CString ss;
	int nn = ctl_edit_combo_.GetCurSel();
	if (nn == -1) return;   // why does this happen?

	// If the history list has a result string with a radix value in it then 
	// restore the radix before we restore the expression
	CString * ps = (CString *)ctl_edit_combo_.GetItemDataPtr(nn);
	if (ps != NULL && ps->GetLength() > 0 && (*ps)[0] != ' ')
	{
		char buf[2];
		buf[0] = (*ps)[0];
		buf[1] = '\0';
		long rr = strtol(buf, NULL, 36);
		if (rr != radix_)
			change_base(rr);
		orig_radix_ = radix_;
	}

	ctl_edit_combo_.GetLBText(nn, ss);
	SetStr(ss);
	op_ = binop_none;
	previous_unary_op_ = unary_none;

	left_= "";
	right_ = "(" + ss + ")";
}

void CCalcDlg::OnChangeRadix()
{
	if (!inited_) return;   // no point in doing anything yet

	// Get value from radix edit box and convert to an integer
	CString ss;
	GetDlgItemText(IDC_RADIX, ss);
	int radix = strtol(ss, NULL, 10);
	if (radix > 1 && radix <= 36)      // If new value is within out valid range then
		change_base(radix);            // set the new radix value
}

void CCalcDlg::OnChangeBits()
{
	if (!inited_) return;   // no point in doing anything yet

	CString ss;
	GetDlgItemText(IDC_BITS, ss);
	int bits = strtol(ss, NULL, 10);
	if (bits >= 0 && bits <= 32767)   // Use SHORT_MAX as max bits allowed as this is the biggest spin control value (and allows for huge numbers)
		change_bits(bits);
}

void CCalcDlg::OnChangeSigned()
{
	if (!inited_) return;   // no point in doing anything yet

	CButton *pb = (CButton *)GetDlgItem(IDC_CALC_SIGNED);
	change_signed(pb->GetCheck() == BST_CHECKED);
}

void CCalcDlg::OnTooltipsShow(NMHDR* pNMHDR, LRESULT* pResult)
{
	// Without this tooltips come up behind the calculator when it's floating
	SetWindowPos(&wndTopMost, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE);
	*pResult = 0;
	return;
}

BOOL CCalcDlg::PreTranslateMessage(MSG* pMsg)
{
	if (ttc_.m_hWnd != 0)
	{
		ttc_.Activate(TRUE);
		ttc_.RelayEvent(pMsg);
	}

	if (pMsg->message == WM_SYSKEYDOWN && pMsg->wParam == VK_F10)
	{
		// Handle F10 press
		On32bit();
		return TRUE;
	}
	else if (pMsg->message == WM_CHAR)
	{
		if (pMsg->wParam == '\r')
		{
			OnEquals();    // Carriage Return
			return TRUE;
		}
		else if (pMsg->wParam == '\x0C')
		{
			OnMemClear();  // ^L
			return TRUE;
		}
		else if (pMsg->wParam == '\x12')
		{
			OnMemGet();   // ^R
			return TRUE;
		}
		else if (pMsg->wParam == '\x0D')
		{
			OnMemStore(); // ^M
			return TRUE;
		}
		else if (pMsg->wParam == '\x10')
		{
			OnMemAdd();  // ^P
			return TRUE;
		}
	}
	else if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam > VK_F1 && pMsg->wParam <= VK_F12)
		{
			switch(pMsg->wParam)
			{
			case VK_F5:
				OnHex();
				break;
			case VK_F6:
				OnDecimal();
				break;
			case VK_F7:
				OnOctal();
				break;
			case VK_F8:
				OnBinary();
				break;
			case VK_F9:
				On64bit();
				break;
// F10 is a system key (see WM_SYSKEYDOWN above)
//            case VK_F10:
//                On32bit();
//                break;
			case VK_F11:
				On16bit();
				break;
			case VK_F12:
				On8bit();
				break;
		   }
		   return TRUE;
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}

#define IDC_FIRST_BLUE   IDC_DIGIT_0
#define IDC_FIRST_RED    IDC_BACKSPACE
#define IDC_FIRST_BLACK  IDC_MARK_STORE
#define IDC_FIRST_PURPLE IDC_POW

void CCalcDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (!IsVisible()) return;
	ASSERT(lpDrawItemStruct->CtlType == ODT_BUTTON);

	CDC *pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	COLORREF cr = pDC->GetTextColor();          // Save text colour to restore later

	if (lpDrawItemStruct->itemState & ODS_DISABLED)
		pDC->SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
	else if (nIDCtl >= IDC_FIRST_BLUE && nIDCtl < IDC_FIRST_RED)
		pDC->SetTextColor(RGB(0, 0, 0xC0));
	else if (nIDCtl >= IDC_FIRST_RED && nIDCtl < IDC_FIRST_BLACK)
		pDC->SetTextColor(RGB(0xC0, 0, 0));
	else if (nIDCtl >= IDC_FIRST_BLACK && nIDCtl < IDC_FIRST_PURPLE)
		pDC->SetTextColor(RGB(0x0, 0, 0));
	else if (nIDCtl >= IDC_FIRST_PURPLE)
		pDC->SetTextColor(RGB(0x80, 0, 0x80));

	CRect rcButton(lpDrawItemStruct->rcItem);
	rcButton.InflateRect(-2, -2);
	if ((lpDrawItemStruct->itemState & ODS_SELECTED) == 0)
	{
		::Draw3DFrame(pDC, rcButton, GetSysColor(COLOR_BTNSHADOW), GetSysColor(COLOR_BTNHIGHLIGHT));
		rcButton.InflateRect(-1, -1);
		::Draw3DFrame(pDC, rcButton, GetSysColor(COLOR_BTNSHADOW), GetSysColor(COLOR_BTNHIGHLIGHT));
		rcButton.InflateRect(-1, -1);
	}

	pDC->FillSolidRect(&rcButton, GetSysColor(COLOR_BTNFACE));

	if (nIDCtl == IDC_UNARY_SQUARE)
	{
		RECT rct = lpDrawItemStruct->rcItem;
		rct.right -= (rct.right - rct.left)/5;
		rct.top += (rct.bottom - rct.top)/10;
		pDC->DrawText("n", &rct,
					  DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
		rct = lpDrawItemStruct->rcItem;
		rct.left += (rct.right - rct.left)/5;
		rct.bottom -= (rct.bottom - rct.top)/5;
		pDC->DrawText("2", &rct,
					  DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
	}
	else if (nIDCtl == IDC_UNARY_SQUARE_ROOT)
	{
		RECT rct = lpDrawItemStruct->rcItem;
		rct.left += (rct.right - rct.left)/5;
		rct.top += (rct.bottom - rct.top)/10;
		pDC->DrawText("n", &rct,
					  DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);

		// Now draw the square root symbol
		CPen *ppen = pDC->SelectObject(&purple_pen);
		rct = lpDrawItemStruct->rcItem;
		pDC->MoveTo(rct.left + 3*(rct.right - rct.left)/10, rct.top + (rct.bottom - rct.top)/2);
		pDC->LineTo(rct.left + (rct.right - rct.left)/3, rct.top + 2*(rct.bottom - rct.top)/3);
		pDC->LineTo(rct.left + (rct.right - rct.left)/2, rct.top + (rct.bottom - rct.top)/3);
		pDC->LineTo(rct.left + 3*(rct.right - rct.left)/4, rct.top + (rct.bottom - rct.top)/3);
		pDC->SelectObject(ppen);
	}
	else if (nIDCtl == IDC_UNARY_CUBE)
	{
		RECT rct = lpDrawItemStruct->rcItem;
		rct.right -= (rct.right - rct.left)/5;
		rct.top += (rct.bottom - rct.top)/10;
		pDC->DrawText("n", &rct,
					  DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
		rct = lpDrawItemStruct->rcItem;
		rct.left += (rct.right - rct.left)/5;
		rct.bottom -= (rct.bottom - rct.top)/5;
		pDC->DrawText("3", &rct,
					  DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
	}
	else if (nIDCtl == IDC_POW)
	{
		RECT rct = lpDrawItemStruct->rcItem;
		rct.right -= (rct.right - rct.left)/4;
		pDC->DrawText("m", &rct,
					  DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
		rct = lpDrawItemStruct->rcItem;
		rct.left += (rct.right - rct.left)/4;
		rct.bottom -= (rct.bottom - rct.top)/4;
		pDC->DrawText("n", &rct,
					  DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
	}
	else
	{
		CString str;
		GetDlgItem(nIDCtl)->GetWindowText(str);
		pDC->DrawText(str, &lpDrawItemStruct->rcItem,
					  DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
	}

	rcButton = lpDrawItemStruct->rcItem;
	::Draw3DButtonFrame(pDC, rcButton,
						(lpDrawItemStruct->itemState & ODS_FOCUS) != 0);

	// Restore text colour
	::SetTextColor(lpDrawItemStruct->hDC, cr);

	CDialog::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

// This actually goes to a calculated address.  Unlike the "Store Cursor" button,
// the result is calculated (saves pressing "=" button) and the view is given focus
// after the current cursor position has been moved.
void CCalcDlg::OnGo()                   // Move cursor to current value
{
	CHexEditView *pview = GetView();
	if (pview == NULL)
	{
		// This should not happen unless we are playing back a macro
		no_file_error();
		return;
	}

	if (state_ == CALCERROR)
	{
		make_noise("Calculator Error");
		CAvoidableDialog::Show(IDS_CALC_ERROR,
				"You cannot perform a jump with an invalid value.");
		//mm_->StatusBarText("Jump with invalid value.");
		aa_->mac_error_ = 10;
		return;
	}
	else if (state_ > CALCINTEXPR)
	{
		not_int_error();
		return;
	}
	else if (state_ == CALCINTEXPR)
	{
		state_ = edit_.update_value(true);           // re-eval the expression allowing side-effects now
		ASSERT(state_ == CALCINTEXPR);
		state_ = CALCINTLIT;   // This is so that edit_.put() [below] will use current_ instead of current_str_
	}

	// First work out if we are jumping by address or sector
	bool sector = ctl_go_.m_nMenuResult == ID_GOSECTOR;

	// If the value in current_ is not there as a result of a calculation then
	// save it before it is lost.
	save_value_to_macro();

	calc_binary();
	check_for_error();   // check for overflow/error
	edit_.put();

	add_hist();
	// xxx make sure it also gets added to hex or dec jump list
	edit_.put();  

	CString ss;
	edit_.GetWindowText(ss);
	if (sector)
	{
		ss = "Go To sector " + ss + " ";
	}
	else if (radix_ == 16)
	{
		mm_->AddHexHistory(ss);
		ss = "Go To (hex) " + ss + " ";
	}
	else if (radix_ == 10)
	{
		mm_->AddDecHistory(ss);
		ss = "Go To (decimal) " + ss + " ";
	}
	else
	{
		ss = "Go To (calc) " + ss + " ";
	}

	mpz_class addr, eof, sector_size;
	addr = get_norm(current_);
	if (addr < 0)
	{
		TaskMessageBox("Jump to negative address",
			"The calculator cannot jump to an address before the start of file.");
		aa_->mac_error_ = 10;
		return;
	}

	mpz_set_ui64(eof.get_mpz_t(), pview->GetDocument()->length());
	mpz_set_ui64(sector_size.get_mpz_t(), pview->GetDocument()->GetSectorSize());

	if (sector && sector_size > 0)
		addr *= sector_size;

	if (addr > eof)
	{
		TaskMessageBox("Jump past EOF",
			"The calculator cannot jump to an address past the end of file.");
		aa_->mac_error_ = 10;
		return;
	}

	pview->MoveWithDesc(ss, mpz_get_ui64(addr.get_mpz_t()));

	// Give view the focus
	if (pview != pview->GetFocus())
		pview->SetFocus();

	state_ = CALCINTRES;
	//orig_radix_ = radix_;    // remember starting radix

	source_ = km_result;
	aa_->SaveToMacro(km_go);
}

void CCalcDlg::OnBigEndian()
{
	// toggle_endian();
	ASSERT(GetView() != NULL);
	if (GetView() != NULL)
	{
		GetView()->OnToggleEndian();
		aa_->SaveToMacro(km_big_endian, (__int64)-1);
	}
}

void CCalcDlg::On8bit()
{
	change_bits(8);
}

void CCalcDlg::On16bit()
{
	change_bits(16);
}

void CCalcDlg::On32bit()
{
	change_bits(32);
}

void CCalcDlg::On64bit()
{
	change_bits(64);
}

void CCalcDlg::OnInfbit()
{
	change_bits(0);
}

void CCalcDlg::OnBinary()
{
	change_base(2);
	change_signed(false);
}

void CCalcDlg::OnOctal()
{
	change_base(8);
	change_signed(false);
}

void CCalcDlg::OnDecimal()
{
	change_base(10);
	change_signed(true);
}

void CCalcDlg::OnHex()
{
	if (!inited_) return;  // Can't work out why we get this message during dialog creation
	change_base(16);
	change_signed(false);
}

void CCalcDlg::OnBackspace()            // Delete back one digit
{
	edit_.SendMessage(WM_CHAR, '\b', 1);
}

void CCalcDlg::OnClearEntry()           // Zero current value
{
	current_ = 0;
	state_ = CALCINTUNARY;
	edit_.put();
	edit_.get();
	set_right();
	source_ = aa_->recording_ ? km_user_str : km_result;
	aa_->SaveToMacro(km_clear_entry);
}

void CCalcDlg::OnClear()                // Zero current and remove any operators/brackets
{
	op_ = binop_none;
	previous_unary_op_ = unary_none;
	current_ = 0;
	state_ = CALCINTRES;
	edit_.put();
	edit_.get();
	left_.Empty(); set_right();
	source_ = aa_->recording_ ? km_user_str : km_result;
	aa_->SaveToMacro(km_clear);
}

void CCalcDlg::OnEquals()               // Calculate result
{
	TRACE("CALCULATOR EQUALS: expr:%.200s\r\n", (const char *)CString(get_expr(true)));
	// Check if we are in error state or we don't have a valid expression
	if (state_ == CALCERROR)
	{
		make_noise("Calculator Error");
		CAvoidableDialog::Show(IDS_CALC_ERROR,
				"You cannot perform this operation on an invalid value.");
		//mm_->StatusBarText("Operation on invalid value.");
		aa_->mac_error_ = 10;
		return;
	}
	else if (state_ == CALCOTHER)
	{
		make_noise("Calculator Error");
		CAvoidableDialog::Show(IDS_CALC_INVALID,
				"The current value or expression is invalid.");
		//mm_->StatusBarText("Invalid expression");
		return;
	}
	else if (op_ != binop_none && state_ > CALCINTEXPR)
	{
		not_int_error();
		return;
	}
	else if (state_ >= CALCINTEXPR)
	{
		state_ = edit_.update_value(true);           // re-eval the expression allowing side-effects now
		if (state_ == CALCINTEXPR)
			state_ = CALCINTLIT;   // This is so that edit_.put() [below] will use current_ instead of current_str_
	}

	// If the value in current_ is not there as a result of a calculation then
	// save it before it is lost.
	save_value_to_macro();

	calc_binary();
	check_for_error();   // check for overflow/error
	edit_.put();

	// We have a valid result so add it to the history list (even if just added)
	add_hist();
	edit_.put();    // removing from hist list (in add_hist) seems to clear the edit box sometimes so put it back

	switch (state_)
	{
	case CALCINTRES:
	case CALCINTUNARY:
	case CALCINTBINARY:
	case CALCINTLIT:
		// Result is an integer so we can do special integer things (like enable GO button)
		state_ = CALCINTRES;
		break;

	case CALCREALEXPR:
	case CALCDATEEXPR:
	case CALCSTREXPR:
	case CALCBOOLEXPR:
		// Result is a valid value of a non-integer type
		state_ = CALCSTATE(state_ + 20);  // convert valid "expression" to "result" (eg CALCDATEXEPR -> CALCDATERES)
		break;

	case CALCOVERFLOW:  // it happens
		break;

	case CALCINTEXPR:   // should have been converted to CALCINTLIT
	default:
		ASSERT(0);  // these cases should not occur
		return;
	}
	//orig_radix_ = radix_;    // remember starting radix

	source_ = km_result;
	aa_->SaveToMacro(km_equals);
}

// ---------- History and Expression Display ---------

// add_hist() adds to the history list of previous calculations by adding to the
// drop-down list (in which the main edit box resides) when a result is generated.
// The expression that created the result is added as a string to the drop-list, and
// the data-item associated with that contains the result (as a string).
// The result is displayed in atip window when the user holds the mouse over an
// expression in the history list.
// Duplicates are removed from the list but since the same expression can generate
// different result (eg if it use a variable name) then a duplicate is only one
// where both the expression and the result match.
void CCalcDlg::add_hist()
{
	// Get the expression that generated the result
	CString strRoll = CString(get_expr(true));

	CString strDrop;
	if (strRoll.GetLength() <= 100)
		strDrop = strRoll;
	else
		strDrop = strRoll.Left(100) + "...";

	// The first char of the string stores the radix as a char ('2'-'9', 'A'-'Z')
	char buf[64];
	if (state_ > CALCINTEXPR)
		buf[0] = ' ';               // space indicates we don't care about the radix because it's not an int
	else if (orig_radix_ < 10)
		buf[0] = orig_radix_ + '0';      // 0-9
	else
		buf[0] = orig_radix_ - 10 + 'A'; // A-Z
	buf[1] = '\0';

	CString * pResult = new CString(buf);   // Create the string to attach to the new drop-down hist list element

	// Put the result string on the heap so we can store it in the drop list item data
	edit_.get();   // Get the result string into current_str_
	int len = current_str_.GetLength();

	mm_->m_wndCalcHist.Add(strRoll + " = " + CString(current_str_));

	if (len < 2000 && state_ > CALCINTLIT)
	{
		// Short non-int result
		*pResult += current_str_;
	}
	else if (state_ > CALCINTLIT)      // not integer - probably a very long string
	{
		// Long non-int result
		*pResult += current_str_.Left(2000);
		*pResult += " ...";
	}
	else if (len < 2000 && radix_ == orig_radix_)
	{
		// Short int result in original radix
		*pResult += current_str_;
	}
	else if (len < 2000)
	{
		// Short int result that needs conversion to original radix
		int numlen = mpz_sizeinbase(current_.get_mpz_t(), orig_radix_) + 3;
		char *numbuf = new char[numlen];
		numbuf[numlen-1] = '\xCD';

		// Get the number as a string
		mpz_get_str(numbuf, theApp.hex_ucase_? -orig_radix_ : orig_radix_, current_.get_mpz_t());
		ASSERT(numbuf[numlen-1] == '\xCD');

		*pResult += numbuf;
		delete[] numbuf;
	}
	else
	{
		// Large integer result (displayed in radix 10)
		const int extra = 2 + 1;  // Need room for possible sign, EOS, and dummy (\xCD) byte
		int numlen = mpz_sizeinbase(current_.get_mpz_t(), 10) + extra;
		char *numbuf = new char[numlen];
		numbuf[numlen-1] = '\xCD';

		// Get the number as a string
		mpz_get_str(numbuf, 10, current_.get_mpz_t());
		ASSERT(numbuf[numlen-1] == '\xCD');

		// Copy the most sig. digits copying sign if present, including decimal point after 1st digit
		const char * pin = numbuf;
		char * pout = buf;
		if (*pin == '-')
		{
			*pout = '-';
			++pin, ++pout;
			//numlen--;         // mpz_sizeinbase does not return room for sign
		}
		*pout++ = *pin++;
		*pout++ = theApp.dec_point_;
		for (int ii = 0; ii < 40; ++ii)
		{
			if (isdigit(*pin) || isalpha(*pin))
				*pout++ = *pin;
			++pin;
		}
		delete[] numbuf;

		// Add the exponent (decimal) and add initial 'A' to indicate that this number uses base 10
		sprintf(pout, " E %d", numlen - 1 - extra);
		*pResult = CString("A") + buf;
	}

	// Find any duplicate entry in the history (expression and result) and remove it
	for (int ii = 0; ii < ctl_edit_combo_.GetCount(); ++ii)
	{
		// Get expression and result for this item
		CString ss, * ps;
		ctl_edit_combo_.GetLBText(ii, ss);
		ps = (CString *)ctl_edit_combo_.GetItemDataPtr(ii);

		if (ss == strDrop && ps != NULL && *ps == *pResult)
		{
			// We also have to delete the string (here and when combo is destroyed)
			// It would have been easy just to handle WM_DELETEITEM but 
			// this is only sent for owner-draw combo boxes apparently.
			delete ps;

			ctl_edit_combo_.DeleteString(ii);       // remove this entry

			break;  // exit loop since the same value could not appear again
		}
	}

	// Add the new entry (at the top of the drop-down list)
	ctl_edit_combo_.InsertString(0, strDrop);
	ctl_edit_combo_.SetItemDataPtr(0, pResult);
}

// Combine the current left and right values with active binary operator to get the
// expression that will re-create the value in the calculator.
// Parentheses are placed around expressions if there would be a precedence issue
// when combined with further operations.  To avoid the parenthese (eg when just
// displaying the result) pass true as the first parameter.
ExprStringType CCalcDlg::get_expr(bool no_paren /* = false */)
{
	ExprStringType retval;

	switch (op_)
	{
	case binop_none:
		if (state_ < CALCINTEXPR || right_[0] == '(' && right_[right_.GetLength()-1] == ')')
			retval = right_;
		else
			retval.Format(EXPRSTR("(%s)"), (const wchar_t *)right_);
		break;
	case binop_add:
		retval.Format(EXPRSTR("(%s + %s)"), (const wchar_t *)left_, (const wchar_t *)right_);
		break;
	case binop_subtract:
		retval.Format(EXPRSTR("(%s - %s)"), (const wchar_t *)left_, (const wchar_t *)right_);
		break;
	case binop_multiply:
		retval.Format(EXPRSTR("(%s * %s)"), (const wchar_t *)left_, (const wchar_t *)right_);
		break;
	case binop_divide:
		retval.Format(EXPRSTR("(%s / %s)"), (const wchar_t *)left_, (const wchar_t *)right_);
		break;
	case binop_mod:
		retval.Format(EXPRSTR("(%s %% %s)"), (const wchar_t *)left_, (const wchar_t *)right_);
		break;
	case binop_pow:
		retval.Format(EXPRSTR("pow(%s, %s)"), (const wchar_t *)left_, (const wchar_t *)right_);
		break;
	case binop_gtr:
	case binop_gtr_old:
		retval.Format(EXPRSTR("max(%s, %s)"), (const wchar_t *)left_, (const wchar_t *)right_);
		break;
	case binop_less:
	case binop_less_old:
		retval.Format(EXPRSTR("min(%s, %s)"), (const wchar_t *)left_, (const wchar_t *)right_);
		break;
	case binop_rol:
		retval.Format(EXPRSTR("rol(%s, %s, %s)"), (const wchar_t *)left_, (const wchar_t *)right_, bits_as_string());
		break;
	case binop_ror:
		retval.Format(EXPRSTR("ror(%s, %s, %s)"), (const wchar_t *)left_, (const wchar_t *)right_, bits_as_string());
		break;
	case binop_lsl:
		retval.Format(EXPRSTR("(%s << %s)"), (const wchar_t *)left_, (const wchar_t *)right_);
		break;
	case binop_lsr:
		retval.Format(EXPRSTR("(%s >> %s)"), (const wchar_t *)left_, (const wchar_t *)right_);
		break;
	case binop_asr:
		retval.Format(EXPRSTR("asr(%s, %s, %s)"), (const wchar_t *)left_, (const wchar_t *)right_, bits_as_string());
		break;

	case binop_and:
		retval.Format(EXPRSTR("(%s & %s)"), (const wchar_t *)left_, (const wchar_t *)right_);
		break;
	case binop_or:
		retval.Format(EXPRSTR("(%s | %s)"), (const wchar_t *)left_, (const wchar_t *)right_);
		break;
	case binop_xor:
		retval.Format(EXPRSTR("(%s ^ %s)"), (const wchar_t *)left_, (const wchar_t *)right_);
		break;

	default:
		ASSERT(0);
		break;
	}

	int len = retval.GetLength();
	if (no_paren)
		return without_parens(retval);
	else
		return retval;
}
// Checks if an expression (in a string) is enclosed in parentheses and if so removes them
// and returns the expression (without parentheses) else just returns the input string.
ExprStringType CCalcDlg::without_parens(const ExprStringType &ss)
{
	int len = ss.GetLength();

	if (len > 0 && ss[0] == '(')
	{
		while (--len > 0)
		{
			if (ss[len] == ')')
				return ss.Mid(1, len-1);
			else if (ss[len] != ' ')
				break;
		}
	}
	return ss;
}

// Set right_ (right side of current binary operation) from edit box.
// This is done after a value is added or typed into the edit box.
// An important consideration is to make sure that the value is evaluated 
// correctly in the original radix.
void CCalcDlg::set_right()
{
	// If there is nothing in the left operand yet we can change the expression radix
	if (left_.IsEmpty())
		orig_radix_ = radix_;
	previous_unary_op_ = unary_none;

	if (state_ == CALCERROR)
		right_ = "***";
	else if (radix_ == orig_radix_ || state_ > CALCINTLIT)
		right_ = current_str_;
	else if (orig_radix_ == 10)
	{
		// We started in decimal so convert right_ to decimal (so all lietrals are the same radix)
		int numlen = mpz_sizeinbase(current_.get_mpz_t(), 10) + 3;
		char *numbuf = new char[numlen];
		numbuf[numlen-1] = '\xCD';

		// Get the number as a string
		mpz_get_str(numbuf, 10, current_.get_mpz_t());
		ASSERT(numbuf[numlen-1] == '\xCD');

		right_ = numbuf;
		delete[] numbuf;
	}
	else
	{
		// Store other values as hex with leading "0x"
		right_ = "0x";
		int numlen = mpz_sizeinbase(current_.get_mpz_t(), 16) + 3;
		char *numbuf = new char[numlen];
		numbuf[numlen-1] = '\xCD';

		// Get the number as a string
		mpz_get_str(numbuf, theApp.hex_ucase_? -16 : 16, current_.get_mpz_t());
		ASSERT(numbuf[numlen-1] == '\xCD');

		right_ += numbuf;
		delete[] numbuf;
	}
}

// Increments the value (right operand) in an expression of the form "(EXPR OP VALUE)".
// For example "(N + 3)" becomes "(N + 4)".
ExprStringType CCalcDlg::inc_right_operand(const ExprStringType &expr)
{
	// First get the location of the right operand  in the string
	int pos = expr.GetLength() - 1;
	ASSERT(pos > 0 && expr[pos] == ')');
	pos--;
	while (isalnum(expr[pos]))
		pos--;
	++pos;     // move to first digit
	int replace_pos = pos;    // where we add the new value

	// Workout what radix to use
	int radix = orig_radix_;       // default to current expression radix
	if (expr[pos] == '0')
	{
		++pos;
		radix = 8;
		if (expr[pos] == 'x' || expr[pos] == 'X')
		{
			++pos;
			radix = 16;
		}
	}

	// Get the current value of the right operand
#ifdef UNICODE_TYPE_STRING
	int operand = wcstol((const wchar_t *)expr + pos, NULL, radix);
#else
	int operand = strtol((const char *)expr + pos, NULL, radix);
#endif
	// Re-create the expression string with incremented operand
	ExprStringType retval;
	retval.Format(EXPRSTR("%.*s%s)"), replace_pos, expr, int_as_string(operand + 1));
	return retval;
}

ExprStringType CCalcDlg::bits_as_string()
{
	return int_as_string(bits_);
}

ExprStringType CCalcDlg::int_as_string(int ii)
{
	ExprStringType retval;
	int radix = orig_radix_;
	if (ii < 10 && ii < radix)
		radix = 10;   // no point in doing anything fancy if it is just a single decimal digit

	switch(radix)
	{
	case 8:
		retval.Format(EXPRSTR("0%o"), ii);
		break;
	case 10:
		retval.Format(EXPRSTR("%d"), ii);
		break;
	default:
		// Just use hex for all other bases (but prefix with 0x to get right value if expression re-evaluated)
		if (theApp.hex_ucase_)
			retval.Format(EXPRSTR("0x%X"), ii);
		else
			retval.Format(EXPRSTR("0X%x"), ii);
		break;
	case 16:
		if (theApp.hex_ucase_)
			retval.Format(EXPRSTR("%X"), ii);
		else
			retval.Format(EXPRSTR("%x"), ii);
		break;
	}
	return retval;
}

// ---------- Menu button handlers ---------

// Change text colour within a button to grey (disabled) or normal
//  pp = pointer to CCalcButton
//  dis = true if to display greyed out else false to display as normal
//  normal = normal text colour for the button
void CCalcDlg::button_colour(CWnd *pp, bool enable, COLORREF normal)
{
	ASSERT(pp != NULL);

	CCalcButton *pbut = DYNAMIC_DOWNCAST(CCalcButton, pp);
	ASSERT(pbut != NULL);
	if (enable)
	{
		if (pbut->GetTextColor() != normal)
		{
			pbut->SetTextColor(normal);
			pbut->SetTextHotColor(RGB(0,0,0));   // black
			pp->Invalidate();
		}
	}
	else
	{
		if (pbut->GetTextColor() != RGB(0xC0, 0xC0, 0xC0))
		{
			pbut->SetTextColor(RGB(0xC0, 0xC0, 0xC0)); // light grey
			pbut->SetTextHotColor(RGB(0xC0, 0xC0, 0xC0));
			pp->Invalidate();
		}
	}
}

// Rebuild the menus for the menu buttons at the top of the calculator
void CCalcDlg::build_menus()
{
	static clock_t last_hex_hist_build = (clock_t)0;
	static clock_t last_dec_hist_build = (clock_t)0;
	static clock_t last_var_build = (clock_t)0;
	CMenu mm, msub;

	if (last_hex_hist_build < mm_->hex_hist_changed_)
	{
		// Hex jump tool history menu
		mm.CreatePopupMenu();
		if (mm_->hex_hist_.size() == 0)
		{
			// Display a single disabled menu item, since
			// disabling the button itself looks ugly
			mm.AppendMenu(MF_STRING | MF_GRAYED, 0, "(none)");
		}
		else
		{
			for (size_t ii = 0; ii < mm_->hex_hist_.size(); ++ii)
			{
				// Store filter and use index as menu item ID (but add 1 since 0 means no ID used).
				mm.AppendMenu(MF_STRING, ii + 1, mm_->hex_hist_[ii]);
			}
		}
		if (ctl_hex_hist_.m_hMenu != (HMENU)0)
		{
			::DestroyMenu(ctl_hex_hist_.m_hMenu);
			ctl_hex_hist_.m_hMenu = (HMENU)0;
		}
		ctl_hex_hist_.m_hMenu = mm.GetSafeHmenu();
		mm.Detach();

		last_hex_hist_build = mm_->hex_hist_changed_;
	}

	if (last_dec_hist_build < mm_->dec_hist_changed_)
	{
		// Decimal jump tool history
		mm.CreatePopupMenu();
		if (mm_->dec_hist_.size() == 0)
		{
			// Display a single disabled menu item, since
			// disabling the button itself looks ugly
			mm.AppendMenu(MF_STRING | MF_GRAYED, 0, "(none)");
		}
		else
		{
			for (size_t ii = 0; ii < mm_->dec_hist_.size(); ++ii)
			{
				// Store filter and use index as menu item ID (but add 1 since 0 means no ID used).
				mm.AppendMenu(MF_STRING, ii + 1, mm_->dec_hist_[ii]);
			}
		}
		if (ctl_dec_hist_.m_hMenu != (HMENU)0)
		{
			::DestroyMenu(ctl_dec_hist_.m_hMenu);
			ctl_dec_hist_.m_hMenu = (HMENU)0;
		}
		ctl_dec_hist_.m_hMenu = mm.GetSafeHmenu();
		mm.Detach();

		last_dec_hist_build = mm_->dec_hist_changed_;
	}

	if (last_var_build < mm_->expr_.VarChanged())
	{
		int ii = 1;                // Just used to make sure each menu item has a unique ID
		vector <CString> varNames;
		mm.CreatePopupMenu();

		varNames = mm_->expr_.GetVarNames(CJumpExpr::TYPE_INT);
		if (!varNames.empty())
		{
			msub.CreatePopupMenu();
			for (vector<CString>::const_iterator ps = varNames.begin(); ps != varNames.end(); ++ps)
			{
				msub.AppendMenu(MF_STRING, ii++, *ps);
			}
			if (msub.GetMenuItemCount() > 0)
				mm.AppendMenu(MF_POPUP, (UINT)msub.m_hMenu, "&Integer");
			msub.DestroyMenu();
		}

		varNames = mm_->expr_.GetVarNames(CJumpExpr::TYPE_REAL);
		if (!varNames.empty())
		{
			msub.CreatePopupMenu();
			for (vector<CString>::const_iterator ps = varNames.begin(); ps != varNames.end(); ++ps)
			{
				msub.AppendMenu(MF_STRING, ii++, *ps);
			}
			if (msub.GetMenuItemCount() > 0)
				mm.AppendMenu(MF_POPUP, (UINT)msub.m_hMenu, "&Real");
			msub.DestroyMenu();
		}

		varNames = mm_->expr_.GetVarNames(CJumpExpr::TYPE_STRING);
		if (!varNames.empty())
		{
			msub.CreatePopupMenu();
			for (vector<CString>::const_iterator ps = varNames.begin(); ps != varNames.end(); ++ps)
			{
				msub.AppendMenu(MF_STRING, ii++, *ps);
			}
			if (msub.GetMenuItemCount() > 0)
				mm.AppendMenu(MF_POPUP, (UINT)msub.m_hMenu, "&String");
			msub.DestroyMenu();
		}

		varNames = mm_->expr_.GetVarNames(CJumpExpr::TYPE_BOOLEAN);
		if (!varNames.empty())
		{
			msub.CreatePopupMenu();
			for (vector<CString>::const_iterator ps = varNames.begin(); ps != varNames.end(); ++ps)
			{
				msub.AppendMenu(MF_STRING, ii++, *ps);
			}
			if (msub.GetMenuItemCount() > 0)
				mm.AppendMenu(MF_POPUP, (UINT)msub.m_hMenu, "&Boolean");
			msub.DestroyMenu();
		}

		varNames = mm_->expr_.GetVarNames(CJumpExpr::TYPE_DATE);
		if (!varNames.empty())
		{
			msub.CreatePopupMenu();
			for (vector<CString>::const_iterator ps = varNames.begin(); ps != varNames.end(); ++ps)
			{
				msub.AppendMenu(MF_STRING, ii++, *ps);
			}
			if (msub.GetMenuItemCount() > 0)
				mm.AppendMenu(MF_POPUP, (UINT)msub.m_hMenu, "&Date");
			msub.DestroyMenu();
		}

		if (mm.GetMenuItemCount() == 0)
		{
			// If there are no vars then display a single disabled
			// menu item, since disabling the button itself looks ugly.
			mm.AppendMenu(MF_STRING | MF_GRAYED, 0, "(none)");
		}
		else
		{
			// Add menu item to allow all varibales to be deleted
			mm.AppendMenu(MF_STRING, ID_VARS_CLEAR, "&Clear variables...");
		}
		if (ctl_vars_.m_hMenu != (HMENU)0)
		{
			::DestroyMenu(ctl_vars_.m_hMenu);
			ctl_vars_.m_hMenu = (HMENU)0;
		}
		ctl_vars_.m_hMenu = mm.GetSafeHmenu();
		mm.Detach();

		last_var_build = mm_->expr_.VarChanged();
	}
}

// Handle selection from hex history menu
void CCalcDlg::OnGetHexHist()
{
	if (ctl_hex_hist_.m_nMenuResult != 0)
	{
		// Get the text of the menu item selected
		CMenu menu;
		menu.Attach(ctl_hex_hist_.m_hMenu);
		CString ss = get_menu_text(&menu, ctl_hex_hist_.m_nMenuResult);
		menu.Detach();
		ss.Replace(" ", "");    // get rid of padding

		// We clear the current edit box before adding the chars in 2 cases:
		// 1. Displaying result - probably want to start a new calculation
		// 2. Currently just displaying a numeric value - replace it as adding extra digits is just confusing
		//    and otherwise adding more digits may cause an overflow which would be really confusing
		//if (state_ != CALCOTHER)
		if (state_ <= CALCINTLIT || state_>= CALCOTHRES)
		{
			edit_.SetWindowText("");
			state_ = CALCINTUNARY;
		}

		if (radix_ != 16)
		{
			// We need to convert the hex digits to digits in the current radix
			__int64 ii = ::_strtoui64(ss, NULL, 16);
			char buf[72];
			::_i64toa(ii, buf, radix_);
			ss = buf;
		}

		CString strCurr;
		edit_.GetWindowText(strCurr);

		edit_.SetFocus();
		edit_.SetSel(edit_.GetWindowTextLength(), -1);

		if (!strCurr.IsEmpty() && !isspace(strCurr[strCurr.GetLength()-1]))
			edit_.SendMessage(WM_CHAR, (TCHAR)' ');
		for (int ii = 0; ii < ss.GetLength (); ii++)
			edit_.SendMessage(WM_CHAR, (TCHAR)ss[ii]);

		//SetDlgItemText(IDC_OP_DISPLAY, "");
		inedit(km_user_str);
	}
}

// Handle decimal history menu
void CCalcDlg::OnGetDecHist()
{
	if (ctl_dec_hist_.m_nMenuResult != 0)
	{
		CMenu menu;
		menu.Attach(ctl_dec_hist_.m_hMenu);
		CString ss = get_menu_text(&menu, ctl_dec_hist_.m_nMenuResult);
		menu.Detach();

		ss.Replace(",", "");ss.Replace(".", ""); ss.Replace(" ", "");    // get rid of padding

		// We clear the current edit box text in 2 cases:
		// 1. Currently displaying a result of previous calculation - probably want to start a new calculation
		// 2. Currently just displaying a numeric value - replace it by adding extra digits is just confusing
		//    + otherwise adding more digits may cause an overflow which would be really confusing
		//if (state_ != CALCOTHER)
		if (state_ <= CALCINTLIT || state_>= CALCOTHRES)
		{
			edit_.SetWindowText("");
			state_ = CALCINTUNARY;
		}

		if (radix_ != 10)
		{
			// We need to convert decimal to the current radix
			__int64 ii = ::_strtoi64(ss, NULL, 10);
			char buf[72];
			::_i64toa(ii, buf, radix_);
			ss = buf;
		}

		CString strCurr;
		edit_.GetWindowText(strCurr);

		edit_.SetFocus();
		edit_.SetSel(edit_.GetWindowTextLength(), -1);

		if (!strCurr.IsEmpty() && !isspace(strCurr[strCurr.GetLength()-1]))
			edit_.SendMessage(WM_CHAR, (TCHAR)' ');
		for (int ii = 0; ii < ss.GetLength (); ii++)
			edit_.SendMessage(WM_CHAR, (TCHAR)ss[ii]);

		//SetDlgItemText(IDC_OP_DISPLAY, "");
		inedit(km_user_str);
	}
}

// Handle vars menu button
void CCalcDlg::OnGetVar()
{
	if (ctl_vars_.m_nMenuResult == ID_VARS_CLEAR)
	{
		if (CAvoidableDialog::Show(IDS_VARS_CLEAR, 
			                       "Are you sure you want to delete all variables?", "", 
								   MLCBF_YES_BUTTON | MLCBF_NO_BUTTON) == IDYES)
			mm_->expr_.DeleteVars();
	}
	else if (ctl_vars_.m_nMenuResult != 0)
	{
		CMenu menu;
		menu.Attach(ctl_vars_.m_hMenu);
		CString ss = get_menu_text(&menu, ctl_vars_.m_nMenuResult);
		menu.Detach();

		if (state_ <= CALCINTLIT || state_>= CALCOTHRES)
		{
			edit_.SetWindowText("");
			state_ = CALCOTHRES;
		}

		edit_.SetFocus();
		edit_.SetSel(edit_.GetWindowTextLength(), -1);

		if (!ss.IsEmpty() && isalpha(ss[0]) && toupper(ss[0]) - 'A' + 10 < radix_)
			edit_.SendMessage(WM_CHAR, (TCHAR)'@');     // Prefix ID with @ so it's not treated as an integer literal
		for (int ii = 0; ii < ss.GetLength (); ii++)
			edit_.SendMessage(WM_CHAR, (TCHAR)ss[ii]);

		//SetDlgItemText(IDC_OP_DISPLAY, "");
		inedit(km_user_str);
	}
}

// Handle function selection
void CCalcDlg::OnGetFunc()
{
	if (ctl_func_.m_nMenuResult != 0)
	{
		CMenu menu;
		menu.Attach(ctl_func_.m_hMenu);
		CString ss = get_menu_text(&menu, ctl_func_.m_nMenuResult);
		ss.Replace("&&", "&");  // Double-ampersand is needed in menus to show one &, but now we need to reverse that
		menu.Detach();

		if (state_ <= CALCINTLIT || state_>= CALCOTHRES)
		{
			edit_.SetWindowText("");
			state_ = CALCOTHER;   // must be (incomplete) expression
		}

		edit_.SetFocus();
		edit_.SetSel(edit_.GetWindowTextLength(), -1);

		// Remember current location so we can later wipe out the #
		int start, end;
		edit_.GetSel(start, end);

		// First invalidate number conversions (eg case conversions in hex mode, separator removal) by adding non-numeric char
		edit_.SendMessage(WM_CHAR, (TCHAR)'#');
		for (int ii = 0; ii < ss.GetLength (); ii++)
			edit_.SendMessage(WM_CHAR, (TCHAR)ss[ii]);

		// Now replace the above non-numeric char (#) with a space ( )
		edit_.SetSel(start, start + 1);
		edit_.SendMessage(WM_CHAR, (TCHAR)' ');

		// Select everything between brackets
		end = start + ss.Find(')') + 1;
		start += ss.Find('(') + 2;
		edit_.SetSel(start, end);

		//SetDlgItemText(IDC_OP_DISPLAY, "");
		inedit(km_user_str);
	}
}

// ---------- Digits for entering numbers ----------
void CCalcDlg::OnDigit0()
{
	do_digit('0');
}

void CCalcDlg::OnDigit1()
{
	do_digit('1');
}

void CCalcDlg::OnDigit2()
{
	do_digit('2');
}

void CCalcDlg::OnDigit3()
{
	do_digit('3');
}

void CCalcDlg::OnDigit4()
{
	do_digit('4');
}

void CCalcDlg::OnDigit5()
{
	do_digit('5');
}

void CCalcDlg::OnDigit6()
{
	do_digit('6');
}

void CCalcDlg::OnDigit7()
{
	do_digit('7');
}

void CCalcDlg::OnDigit8()
{
	do_digit('8');
}

void CCalcDlg::OnDigit9()
{
	do_digit('9');
}

void CCalcDlg::OnDigitA()
{
	do_digit('A');
}

void CCalcDlg::OnDigitB()
{
	do_digit('B');
}

void CCalcDlg::OnDigitC()
{
	do_digit('C');
}

void CCalcDlg::OnDigitD()
{
	do_digit('D');
}

void CCalcDlg::OnDigitE()
{
	do_digit('E');
}

void CCalcDlg::OnDigitF()
{
	do_digit('F');
}

// ----- Binary operators ----------
void CCalcDlg::OnAdd()
{
	do_binop(binop_add);
}

void CCalcDlg::OnSubtract()
{
	do_binop(binop_subtract);
}

void CCalcDlg::OnMultiply()
{
	do_binop(binop_multiply);
}

void CCalcDlg::OnDivide()
{
	do_binop(binop_divide);
}

void CCalcDlg::OnMod()
{
	do_binop(binop_mod);
}

void CCalcDlg::OnPow()
{
	do_binop(binop_pow);
}

void CCalcDlg::OnAnd()
{
	do_binop(binop_and);
}

void CCalcDlg::OnOr()
{
	do_binop(binop_or);
}

void CCalcDlg::OnXor()
{
	do_binop(binop_xor);
}

void CCalcDlg::OnLsl()
{
	do_binop(binop_lsl);
}

void CCalcDlg::OnLsr()
{
	do_binop(binop_lsr);
}

void CCalcDlg::OnAsr()
{
	do_binop(binop_asr);
}

void CCalcDlg::OnRol()
{
	do_binop(binop_rol);
}

void CCalcDlg::OnRor()
{
	do_binop(binop_ror);
}

void CCalcDlg::OnGtr()                  // Returns the larger of it's 2 operands
{
	do_binop(binop_gtr);
}

void CCalcDlg::OnLess()                 // Returns the smaller of it's 2 operands
{
	do_binop(binop_less);
}

// --------- Unary operators -----------

void CCalcDlg::OnUnaryAt()
{
	do_unary(unary_at);
}

void CCalcDlg::OnUnaryRol()
{
	do_unary(unary_rol);
}

void CCalcDlg::OnUnaryRor()
{
	do_unary(unary_ror);
}

void CCalcDlg::OnUnaryLsl()
{
	do_unary(unary_lsl);
}

void CCalcDlg::OnUnaryLsr()
{
	do_unary(unary_lsr);
}

void CCalcDlg::OnUnaryAsr()
{
	do_unary(unary_asr);
}

void CCalcDlg::OnUnaryNot()
{
	do_unary(unary_not);
}

void CCalcDlg::OnUnaryFlip()            // Flip bytes
{
	do_unary(unary_flip);
}

void CCalcDlg::OnUnaryRev()             // Reverse bits
{
	do_unary(unary_rev);
}

void CCalcDlg::OnUnarySign()
{
	do_unary(unary_sign);
}

void CCalcDlg::OnUnaryInc()
{
	do_unary(unary_inc);
}

void CCalcDlg::OnUnaryDec()
{
	do_unary(unary_dec);
}

void CCalcDlg::OnUnarySquare()
{
	do_unary(unary_square);
}

void CCalcDlg::OnUnarySquareRoot()
{
	do_unary(unary_squareroot);
}

void CCalcDlg::OnUnaryCube()
{
	do_unary(unary_cube);
}

void CCalcDlg::OnUnaryFactorial()
{
	do_unary(unary_factorial);
}

// ------- Calculator memory funcs ----------
void CCalcDlg::OnMemGet()
{
	current_ = memory_;
	state_ = CALCINTUNARY;

	check_for_error();   // check for overflow & display error messages
	edit_.put();
	edit_.get();
	set_right();
	inedit(km_memget);
}

void CCalcDlg::OnMemStore()
{
	// First check that we can perform an integer operation
	if (state_ == CALCERROR)
	{
		make_noise("Calculator Error");
		CAvoidableDialog::Show(IDS_CALC_ERROR,
				"You cannot perform this operation on an invalid value.");
		//mm_->StatusBarText("Operation on invalid value.");
		aa_->mac_error_ = 10;
		return;
	}
	else if (state_ > CALCINTEXPR)
	{
		not_int_error();
		return;
	}

	memory_ = get_norm(current_);
	if (!aa_->refresh_off_ && IsVisible())
	{
		button_colour(GetDlgItem(IDC_MEM_GET), true, RGB(0x40, 0x40, 0x40));

		// TODO This works but we have to also update tooltip after MC, M+, M- buttons, and perhaps also after radix changes
		// which means we can't just use the current text from the edit box.
		// Plus we should also display using the current radix and bits AND update after macros with refresh off.
		//// Update tooltip
		//CString ss;
		//edit_.GetWindowText(ss);
		//ttc_.UpdateTipText("Memory [" + ss + "] (Ctrl+R)", GetDlgItem(IDC_MEM_GET));
	}

	save_value_to_macro();
	aa_->SaveToMacro(km_memstore);
}

void CCalcDlg::OnMemClear()
{
	memory_ = 0;
	if (!aa_->refresh_off_ && IsVisible())
	{
		button_colour(GetDlgItem(IDC_MEM_GET), false, RGB(0x40, 0x40, 0x40));
	}

	aa_->SaveToMacro(km_memclear);
}

void CCalcDlg::OnMemAdd()
{
	// First check that we can perform an integer operation
	if (state_ == CALCERROR)
	{
		make_noise("Calculator Error");
		CAvoidableDialog::Show(IDS_CALC_ERROR,
				"You cannot perform this operation on an invalid value.");
		//mm_->StatusBarText("Operation on invalid value.");
		aa_->mac_error_ = 10;
		return;
	}
	else if (state_ > CALCINTEXPR)
	{
		not_int_error();
		return;
	}

	memory_ += get_norm(current_);
	if (!aa_->refresh_off_ && IsVisible())
	{
		button_colour(GetDlgItem(IDC_MEM_GET), true, RGB(0x40, 0x40, 0x40));
	}

	save_value_to_macro();
	aa_->SaveToMacro(km_memadd);
}

void CCalcDlg::OnMemSubtract()
{
	// First check that we can perform an integer operation
	if (state_ == CALCERROR)
	{
		make_noise("Calculator Error");
		CAvoidableDialog::Show(IDS_CALC_ERROR,
				"You cannot perform this operation on an invalid value.");
		//mm_->StatusBarText("Operation on invalid value.");
		aa_->mac_error_ = 10;
		return;
	}
	else if (state_ > CALCINTEXPR)
	{
		not_int_error();
		return;
	}

	memory_ -= get_norm(current_);
	if (!aa_->refresh_off_ && IsVisible())
	{
		//update_file_buttons();
		button_colour(GetDlgItem(IDC_MEM_GET), true, RGB(0x40, 0x40, 0x40));
	}

	save_value_to_macro();
	aa_->SaveToMacro(km_memsubtract);
}

// ----------- Mark functions --------------
void CCalcDlg::OnMarkGet()              // Position of mark in the file
{
	CHexEditView *pview = GetView();
	if (pview == NULL)
	{
		no_file_error();
		return;
	}

	mpz_class val;
	mpz_set_ui64(val.get_mpz_t(), pview->GetMark());

	current_ = val;
	state_ = CALCINTUNARY;

	check_for_error();   // check for overflow & display error messages
	edit_.put();
	edit_.get();
	set_right();
	inedit(km_markget);
}

void CCalcDlg::OnMarkStore()
{
	// First check that we can perform an integer operation
	if (state_ == CALCERROR)
	{
		make_noise("Calculator Error");
		CAvoidableDialog::Show(IDS_CALC_ERROR,
				"You cannot perform this operation on an invalid value.");
		//mm_->StatusBarText("Operation on invalid value.");
		aa_->mac_error_ = 10;
		return;
	}
	else if (state_ > CALCINTEXPR)
	{
		not_int_error();
		return;
	}

	CHexEditView *pview = GetView();
	if (pview == NULL)
	{
		no_file_error();
		return;
	}

	mpz_class eof, new_mark;
	mpz_set_ui64(eof.get_mpz_t(), pview->GetDocument()->length());   // current eof
	new_mark = get_norm(current_);
	if (new_mark < 0 || new_mark > eof)
	{
		if (new_mark < 0)
			TaskMessageBox("Address is negative",
				"The calculator cannot move the mark to before the start of file.");
		else
			TaskMessageBox("Address is past EOF",
				"The calculator cannot move the mark past the end of file.");
		aa_->mac_error_ = 10;
		return;
	}
	pview->SetMark(mpz_get_ui64(new_mark.get_mpz_t()));

	//if (!aa_->refresh_off_ && IsVisible())
	//{
	//	edit_.SetFocus();
	//	edit_.Put();
	//	//update_file_buttons();
	//}

	save_value_to_macro();
	aa_->SaveToMacro(km_markstore);
}

void CCalcDlg::OnMarkClear()
{
	CHexEditView *pview = GetView();
	if (pview == NULL)
	{
		no_file_error();
		return;
	}

	pview->SetMark(0);
	//if (!aa_->refresh_off_ && IsVisible())
	//{
	//	edit_.SetFocus();
	//	//update_file_buttons();
	//}

	aa_->SaveToMacro(km_markclear);
}

void CCalcDlg::OnMarkAdd()              // Add current value to mark
{
	// First check that we can perform an integer operation
	if (state_ == CALCERROR)
	{
		make_noise("Calculator Error");
		CAvoidableDialog::Show(IDS_CALC_ERROR,
				"You cannot perform this operation on an invalid value.");
		//mm_->StatusBarText("Operation on invalid value.");
		aa_->mac_error_ = 10;
		return;
	}
	else if (state_ > CALCINTEXPR)
	{
		not_int_error();
		return;
	}

	CHexEditView *pview = GetView();
	if (pview == NULL)
	{
		no_file_error();
		return;
	}

	mpz_class eof, mark, new_mark;
	mpz_set_ui64(eof.get_mpz_t(), pview->GetDocument()->length());   // current eof
	mpz_set_ui64(mark.get_mpz_t(), pview->GetMark());                // current mark
	new_mark = mark + get_norm(current_);
	if (new_mark < 0 || new_mark > eof)
	{
		if (new_mark < 0)
			TaskMessageBox("Address is negative",
				"The calculator cannot move the mark to before the start of file.");
		else
			TaskMessageBox("Address is past EOF",
				"The calculator cannot move the mark past the end of file.");
		aa_->mac_error_ = 10;
		return;
	}
	pview->SetMark(mpz_get_ui64(new_mark.get_mpz_t()));

	//if (!aa_->refresh_off_ && IsVisible())
	//{
	//	edit_.SetFocus();
	//	edit_.Put();
	//	//update_file_buttons();
	//}

	save_value_to_macro();
	aa_->SaveToMacro(km_markadd);
}

void CCalcDlg::OnMarkSubtract()
{
	// First check that we can perform an integer operation
	if (state_ == CALCERROR)
	{
		make_noise("Calculator Error");
		CAvoidableDialog::Show(IDS_CALC_ERROR,
				"You cannot perform this operation on an invalid value.");
		//mm_->StatusBarText("Operation on invalid value.");
		aa_->mac_error_ = 10;
		return;
	}
	else if (state_ > CALCINTEXPR)
	{
		not_int_error();
		return;
	}

	CHexEditView *pview = GetView();
	if (pview == NULL)
	{
		no_file_error();
		return;
	}

	mpz_class eof, mark, new_mark;
	mpz_set_ui64(eof.get_mpz_t(), pview->GetDocument()->length());   // current eof
	mpz_set_ui64(mark.get_mpz_t(), pview->GetMark());                // current mark
	new_mark = mark - get_norm(current_);
	if (new_mark < 0 || new_mark > eof)
	{
		if (new_mark < 0)
			TaskMessageBox("Address is negative",
				"The calculator cannot move the mark to before the start of file.");
		else
			TaskMessageBox("Address is past EOF",
				"The calculator cannot move the mark past the end of file.");
		aa_->mac_error_ = 10;
		return;
	}
	pview->SetMark(mpz_get_ui64(new_mark.get_mpz_t()));
	//if (!aa_->refresh_off_ && IsVisible())
	//{
	//	edit_.SetFocus();
	//	edit_.Put();
	//	//update_file_buttons();
	//}

	save_value_to_macro();
	aa_->SaveToMacro(km_marksubtract);
}

// ----------- Other file get funcs -------------
// Get bytes at the mark into calculator
//  - the number of bytes is determined by bits_
//  - byte order is determined by active file endian-ness (display_.big_endian)
void CCalcDlg::OnMarkAt()
{
	if (!get_bytes(-2))  // get data at mark
	{
		state_ = CALCERROR;
		return;
	}
	ASSERT(state_ == CALCINTUNARY);

	//check_for_error();   // overflow should not happen?
	edit_.put();
	edit_.get();
	set_right();
	inedit(km_markat);
}

// Get address of caret (or start of selection)
void CCalcDlg::OnSelGet()
{
	CHexEditView *pview = GetView();
	if (pview == NULL)
	{
		no_file_error();
		return;
	}

	FILE_ADDRESS start, end;
	pview->GetSelAddr(start, end);
	mpz_class val;
	mpz_set_ui64(val.get_mpz_t(), start);

	current_ = val;
	state_ = CALCINTUNARY;

	check_for_error();   // check for overflow & display error messages
	edit_.put();
	edit_.get();
	set_right();
	inedit(km_selget);
}

// Get byte(s) at the caret position - # of bytes is determined by bits_
void CCalcDlg::OnSelAt()                // Value in file at cursor
{
	if (!get_bytes(-3))                // get data at cursor/selection
	{
		state_ = CALCERROR;
		return;
	}
	ASSERT(state_ == CALCINTUNARY);

	//check_for_error();   // overflow should not occur?
	edit_.put();
	edit_.get();
	set_right();
	inedit(km_selat);
}

// Get the selection length
void CCalcDlg::OnSelLen()
{
	CHexEditView *pview = GetView();
	if (pview == NULL)
	{
		no_file_error();
		return;
	}

	FILE_ADDRESS start, end;
	pview->GetSelAddr(start, end);
	mpz_class val;
	mpz_set_ui64(val.get_mpz_t(), end - start);

	current_ = val;
	state_ = CALCINTUNARY;

	check_for_error();   // check for overflow & display error messages
	edit_.put();
	edit_.get();
	set_right();
	inedit(km_sellen);
}

// Get the length of file
void CCalcDlg::OnEofGet()               // Length of file
{
	CHexEditView *pview = GetView();
	if (pview == NULL)
	{
		no_file_error();
		return;
	}

	mpz_class val;
	mpz_set_ui64(val.get_mpz_t(), pview->GetDocument()->length());

	current_ = val;
	state_ = CALCINTUNARY;

	check_for_error();   // check for overflow & display error messages
	edit_.put();
	edit_.get();
	set_right();
	inedit(km_eofget);
}

// ----------- File change funcs -------------

// Change the byte(s) at (and after) the "mark"
void CCalcDlg::OnMarkAtStore()
{
	// First check that we can perform an integer operation
	if (state_ == CALCERROR)
	{
		make_noise("Calculator Error");
		CAvoidableDialog::Show(IDS_CALC_ERROR,
				"You cannot perform this operation on an invalid value.");
		//mm_->StatusBarText("Operation on invalid value.");
		aa_->mac_error_ = 10;
		return;
	}
	else if (state_ > CALCINTEXPR)
	{
		not_int_error();
		return;
	}

	FILE_ADDRESS mark = 0;
	if (GetView() != NULL)
		mark = GetView()->GetMark(); // save mark (NULL view error message displayed in put_bytes)

	if (!put_bytes(-2))
	{
		aa_->mac_error_ = 10;
		return;
	}
	if (!GetView()->OverType())
		GetView()->SetMark(mark);   // Inserting at mark would have move it forward

	//if (!aa_->refresh_off_ && IsVisible())
	//{
	//	edit_.SetFocus();
	//	edit_.Put();
	//	//update_file_buttons();
	//}

	save_value_to_macro();
	aa_->SaveToMacro(km_markatstore);
}

// Set the caret position (or selection start)
void CCalcDlg::OnSelStore()
{
	// First check that we can perform an integer operation
	if (state_ == CALCERROR)
	{
		make_noise("Calculator Error");
		CAvoidableDialog::Show(IDS_CALC_ERROR,
				"You cannot perform this operation on an invalid value.");
		//mm_->StatusBarText("Operation on invalid value.");
		aa_->mac_error_ = 10;
		return;
	}
	else if (state_ > CALCINTEXPR)
	{
		not_int_error();
		return;
	}

	CHexEditView *pview = GetView();
	if (pview == NULL)
	{
		no_file_error();
		return;
	}

	mpz_class eof, val;
	mpz_set_ui64(eof.get_mpz_t(), pview->GetDocument()->length());   // current eof
	val = get_norm(current_);
	if (val < 0)
	{
		TaskMessageBox("Negative address",
			"The calculator cannot move the current address before the start of file.");
		aa_->mac_error_ = 10;
		return;
	}
	if (val > eof)
	{
		TaskMessageBox("Address past EOF",
			"The calculator cannot move the current address past the end of file.");
		aa_->mac_error_ = 10;
		return;
	}
	pview->MoveWithDesc("Set Cursor in Calculator", GetValue());

	//if (!aa_->refresh_off_ && IsVisible())
	//{
	//	edit_.SetFocus();
	//	edit_.Put();
	//	// update_file_buttons(); // done indirectly through MoveToAddress via MoveWithDesc
	//}

	save_value_to_macro();
	aa_->SaveToMacro(km_selstore);
}

// Change the byte(s) at (and after) the caret (start of selection)
void CCalcDlg::OnSelAtStore()
{
	// First check that we can perform an integer operation
	if (state_ == CALCERROR)
	{
		make_noise("Calculator Error");
		CAvoidableDialog::Show(IDS_CALC_ERROR,
				"You cannot perform this operation on an invalid value.");
		//mm_->StatusBarText("Operation on invalid value.");
		aa_->mac_error_ = 10;
		return;
	}
	else if (state_ > CALCINTEXPR)
	{
		not_int_error();
		return;
	}

	if (!put_bytes(-3))
	{
		aa_->mac_error_ = 10;
		return;
	}
	//if (!aa_->refresh_off_ && IsVisible())
	//{
	//	edit_.SetFocus();
	//	edit_.Put();
	//	//update_file_buttons();
	//}

	save_value_to_macro();
	aa_->SaveToMacro(km_selatstore);
}

// Set the selection length from the calculator
void CCalcDlg::OnSelLenStore()
{
	// First check that we can perform an integer operation
	if (state_ == CALCERROR)
	{
		make_noise("Calculator Error");
		CAvoidableDialog::Show(IDS_CALC_ERROR,
				"You cannot perform this operation on an invalid value.");
		//mm_->StatusBarText("Operation on invalid value.");
		aa_->mac_error_ = 10;
		return;
	}
	else if (state_ > CALCINTEXPR)
	{
		not_int_error();
		return;
	}

	CHexEditView *pview = GetView();
	if (pview == NULL)
	{
		no_file_error();
		return;
	}

	FILE_ADDRESS start, end;
	pview->GetSelAddr(start, end);

	mpz_class eof, addr, len;
	mpz_set_ui64(eof.get_mpz_t(), pview->GetDocument()->length());   // current eof
	mpz_set_ui64(addr.get_mpz_t(), start);
	len = get_norm(current_);

	if (len < 0)
	{
		TaskMessageBox("Negative selection length",
			"The calculator cannot make a selection with length less than zero.");
		aa_->mac_error_ = 10;
		return;
	}
	if (addr + len > eof)
	{
		TaskMessageBox("Selection past EOF",
			"The calculator cannot make a selection which extends past the end of file.");
		aa_->mac_error_ = 10;
		return;
	}
	len += addr;
	pview->MoveToAddress(mpz_get_ui64(addr.get_mpz_t()), mpz_get_ui64(len.get_mpz_t()));

	//if (!aa_->refresh_off_ && IsVisible())
	//{
	//	edit_.SetFocus();
	//	edit_.Put();
	//	// update_file_buttons(); // done indirectly through MoveToAddress
	//}

	save_value_to_macro();
	aa_->SaveToMacro(km_sellenstore);
}
