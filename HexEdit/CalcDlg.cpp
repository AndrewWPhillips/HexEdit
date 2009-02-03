// CalcDlg.cpp : implements the Goto/Calculator dialog
//
// Copyright (c) 2003 by Andrew W. Phillips.
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
// CCalcDlg dialog

CCalcDlg::CCalcDlg(CWnd* pParent /*=NULL*/)
    : CHexDialogBar(), purple_pen(PS_SOLID, 0, RGB(0x80, 0, 0x80))
{
    aa_ = dynamic_cast<CHexEditApp *>(AfxGetApp());

	m_sizeInitial = CSize(-1, -1);
	help_hwnd_ = (HWND)0;

    op_ = binop_none;
    current_ = previous_ = 0;
    in_edit_ = FALSE;
	current_const_ = TRUE;
	current_type_ = CJumpExpr::TYPE_INT;

    bits_index_ = base_index_ = -1;  // Fixed later in Create()

    // Get last used settings from ini file/registry
	radix_ = aa_->GetProfileInt("Calculator", "Base", 16);
	bits_ = aa_->GetProfileInt("Calculator", "Bits", 32);
    current_ = _atoi64(aa_->GetProfileString("Calculator", "Current"));
    memory_ = _atoi64(aa_->GetProfileString("Calculator", "Memory"));

    edit_.pp_ = this;                   // Set parent of edit control
}

BOOL CCalcDlg::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
    return Create(pParentWnd);
}

BOOL CCalcDlg::Create(CWnd* pParentWnd /*=NULL*/) 
{
    mm_ = dynamic_cast<CMainFrame *>(AfxGetMainWnd());

	if (!CHexDialogBar::Create(CCalcDlg::IDD, pParentWnd, CBRS_LEFT | CBRS_SIZE_DYNAMIC))
        return FALSE;

    // We need to setup the resizer here to make sure it gets a WM_SIZE message for docked window
	resizer_.Create(GetSafeHwnd(), TRUE, 100, TRUE); // 4th param when dlg = child of resized window

    // Base min size on dialog resource width & 3/4 height
	CSize tmp_size;
	if (m_sizeInitial.cx == -1)
	{
		// Get window size (not client size) from BCGControlBar and adjust for edges + caption
		tmp_size = m_sizeDefault;
		tmp_size.cx -= 2*GetSystemMetrics(SM_CXFIXEDFRAME);
		tmp_size.cy -= 2*GetSystemMetrics(SM_CYFIXEDFRAME) + GetSystemMetrics(SM_CYCAPTION);
	}
	else
	{
		tmp_size = m_sizeInitial;
	}
	resizer_.SetInitialSize(tmp_size);

	tmp_size.cy = tmp_size.cy*3/4;
	resizer_.SetMinimumTrackingSize(tmp_size);
	//This has no effect for control bars - see m_wndCalc.SetMinSize/SetMaxSize
	//tmp_size.cx *= 2; tmp_size.cy *= 2;
	//resizer_.SetMaximumTrackingSize(tmp_size);
	//tmp_size.cx /= 2;
	//tmp_size.cy = tmp_size.cy*3/8;
	//resizer_.SetMinimumTrackingSize(tmp_size);

	// Add all the controls and proportional change to  LEFT, TOP, WIDTH, HEIGHT 
    resizer_.Add(IDC_EDIT, 0, 8, 100, 3);        // edit control resizes with width (moves/sizes slightly vert.)
    resizer_.Add(IDC_OP_DISPLAY, 100, 10, 0, 0);  // operator display sticks to right edge (moves vert)

	// Settings controls don't move/size horizontally but move vert. and size slightly too
    resizer_.Add(IDC_BIG_ENDIAN_FILE_ACCESS, 0, 13, 0, 13);
    resizer_.Add(IDC_BASE_GROUP, 0, 13, 0, 8);
    resizer_.Add(IDC_HEX, 0, 13, 0, 5);
    resizer_.Add(IDC_DECIMAL, 0, 13, 0, 5);
    resizer_.Add(IDC_OCTAL, 0, 13, 0, 5);
    resizer_.Add(IDC_BINARY, 0, 13, 0, 5);
    resizer_.Add(IDC_BITS_GROUP, 0, 13, 0, 8);
    resizer_.Add(IDC_64BIT, 0, 13, 0, 5);
    resizer_.Add(IDC_32BIT, 0, 13, 0, 5);
    resizer_.Add(IDC_16BIT, 0, 13, 0, 5);
    resizer_.Add(IDC_8BIT, 0, 13, 0, 5);

	// First row of buttons
    resizer_.Add(IDC_MEM_GET, 1, 27, 12, 10);
    resizer_.Add(IDC_MEM_STORE, 14, 27, 9, 10);
    resizer_.Add(IDC_MEM_CLEAR, 23, 27, 9, 10);
    resizer_.Add(IDC_MEM_ADD, 32, 27, 9, 10);
    resizer_.Add(IDC_MEM_SUBTRACT, 41, 27, 8, 10);

    resizer_.Add(IDC_BACKSPACE, 64, 26, 16, 10);
    resizer_.Add(IDC_CLEAR_ENTRY, 80, 26, 10, 10);
    resizer_.Add(IDC_CLEAR, 90, 26, 10, 10);

	// 2nd row of buttons
    resizer_.Add(IDC_MARK_GET, 1, 37, 12, 10);
    resizer_.Add(IDC_MARK_STORE, 14, 37, 9, 10);
    resizer_.Add(IDC_MARK_CLEAR, 23, 37, 9, 10);
    resizer_.Add(IDC_MARK_ADD, 32, 37, 9, 10);
    resizer_.Add(IDC_MARK_SUBTRACT, 41, 37, 8, 10);
	resizer_.Add(IDC_DIGIT_D, 50, 37, 8, 10);
	resizer_.Add(IDC_DIGIT_E, 58, 37, 8, 10);
	resizer_.Add(IDC_DIGIT_F, 66, 37, 8, 10);
    resizer_.Add(IDC_POW, 75, 37, 8, 10);
    resizer_.Add(IDC_GTR, 83, 37, 8, 10);
    resizer_.Add(IDC_ROL, 91, 37, 8, 10);

	// 3rd row of buttons
    resizer_.Add(IDC_MARK_AT, 1, 47, 12, 10);
    resizer_.Add(IDC_MARK_AT_STORE, 14, 47, 9, 10);
    resizer_.Add(IDC_UNARY_SQUARE, 23, 47, 9, 10);
    resizer_.Add(IDC_UNARY_ROL, 32, 47, 9, 10);
    resizer_.Add(IDC_UNARY_REV, 41, 47, 8, 10);
	resizer_.Add(IDC_DIGIT_A, 50, 47, 8, 10);
	resizer_.Add(IDC_DIGIT_B, 58, 47, 8, 10);
	resizer_.Add(IDC_DIGIT_C, 66, 47, 8, 10);
    resizer_.Add(IDC_MOD, 75, 47, 8, 10);
    resizer_.Add(IDC_LESS, 83, 47, 8, 10);
    resizer_.Add(IDC_ROR, 91, 47, 8, 10);

	// 4th row of buttons
    resizer_.Add(IDC_SEL_GET, 1, 57, 12, 10);
    resizer_.Add(IDC_SEL_STORE, 14, 57, 9, 10);
    resizer_.Add(IDC_UNARY_SQUARE_ROOT, 23, 57, 9, 10);
    resizer_.Add(IDC_UNARY_ROR, 32, 57, 9, 10);
    resizer_.Add(IDC_UNARY_NOT, 41, 57, 8, 10);
	resizer_.Add(IDC_DIGIT_7, 50, 57, 8, 10);
	resizer_.Add(IDC_DIGIT_8, 58, 57, 8, 10);
	resizer_.Add(IDC_DIGIT_9, 66, 57, 8, 10);
    resizer_.Add(IDC_DIVIDE, 75, 57, 8, 10);
    resizer_.Add(IDC_XOR, 83, 57, 8, 10);
    resizer_.Add(IDC_LSL, 91, 57, 8, 10);

	// 5th row of buttons
    resizer_.Add(IDC_SEL_AT, 1, 67, 12, 10);
    resizer_.Add(IDC_SEL_AT_STORE, 14, 67, 9, 10);
    resizer_.Add(IDC_UNARY_CUBE, 23, 67, 9, 10);
    resizer_.Add(IDC_UNARY_LSL, 32, 67, 9, 10);
    resizer_.Add(IDC_UNARY_INC, 41, 67, 8, 10);
	resizer_.Add(IDC_DIGIT_4, 50, 67, 8, 10);
	resizer_.Add(IDC_DIGIT_5, 58, 67, 8, 10);
	resizer_.Add(IDC_DIGIT_6, 66, 67, 8, 10);
    resizer_.Add(IDC_MULTIPLY, 75, 67, 8, 10);
    resizer_.Add(IDC_OR, 83, 67, 8, 10);
    resizer_.Add(IDC_LSR, 91, 67, 8, 10);

	// 6th row of buttons
    resizer_.Add(IDC_SEL_LEN, 1, 77, 12, 10);
    resizer_.Add(IDC_SEL_LEN_STORE, 14, 77, 9, 10);
    resizer_.Add(IDC_UNARY_FACTORIAL, 23, 77, 9, 10);
    resizer_.Add(IDC_UNARY_LSR, 32, 77, 9, 10);
    resizer_.Add(IDC_UNARY_DEC, 41, 77, 8, 10);
	resizer_.Add(IDC_DIGIT_1, 50, 77, 8, 10);
	resizer_.Add(IDC_DIGIT_2, 58, 77, 8, 10);
	resizer_.Add(IDC_DIGIT_3, 66, 77, 8, 10);
    resizer_.Add(IDC_SUBTRACT, 75, 77, 8, 10);
    resizer_.Add(IDC_AND, 83, 77, 8, 10);
    resizer_.Add(IDC_ASR, 91, 77, 8, 10);

	// 7th row of buttons
    resizer_.Add(IDC_EOF_GET, 1, 87, 12, 10);

    resizer_.Add(IDC_UNARY_AT, 23, 87, 9, 10);
    resizer_.Add(IDC_UNARY_ASR, 32, 87, 9, 10);
    resizer_.Add(IDC_UNARY_FLIP, 41, 87, 8, 10);
	resizer_.Add(IDC_DIGIT_0, 50, 87, 8, 10);
    resizer_.Add(IDC_UNARY_SIGN, 58, 87, 8, 10);
	resizer_.Add(IDC_EQUALS, 66, 87, 8, 10);
    resizer_.Add(IDC_ADDOP, 75, 87, 8, 10);
    resizer_.Add(IDC_GO, 83, 87, 16, 10);

	return TRUE;
}

BOOL CCalcDlg::OnInitDialog()
{
    CWnd *pwnd = ctl_edit_combo_.GetWindow(GW_CHILD);
	ASSERT(pwnd != NULL);
    VERIFY(edit_.SubclassWindow(pwnd->m_hWnd));
    LONG style = ::GetWindowLong(edit_.m_hWnd, GWL_STYLE);
    ::SetWindowLong(edit_.m_hWnd, GWL_STYLE, style | ES_RIGHT | ES_WANTRETURN);

	// Make sure radio buttons are up to date
    change_base(radix_);
    change_bits(bits_);

    // If saved memory value was restored then make it available
    if (memory_ != 0)
        GetDlgItem(IDC_MEM_GET)->EnableWindow(TRUE);

    // Enable/disable digits keys depending on base
    ASSERT(GetDlgItem(IDC_DIGIT_2) != NULL);
    ASSERT(GetDlgItem(IDC_DIGIT_3) != NULL);
    ASSERT(GetDlgItem(IDC_DIGIT_4) != NULL);
    ASSERT(GetDlgItem(IDC_DIGIT_5) != NULL);
    ASSERT(GetDlgItem(IDC_DIGIT_6) != NULL);
    ASSERT(GetDlgItem(IDC_DIGIT_7) != NULL);
    ASSERT(GetDlgItem(IDC_DIGIT_8) != NULL);
    ASSERT(GetDlgItem(IDC_DIGIT_9) != NULL);
    ASSERT(GetDlgItem(IDC_DIGIT_A) != NULL);
    ASSERT(GetDlgItem(IDC_DIGIT_B) != NULL);
    ASSERT(GetDlgItem(IDC_DIGIT_C) != NULL);
    ASSERT(GetDlgItem(IDC_DIGIT_D) != NULL);
    ASSERT(GetDlgItem(IDC_DIGIT_E) != NULL);
    ASSERT(GetDlgItem(IDC_DIGIT_F) != NULL);
    GetDlgItem(IDC_DIGIT_2)->EnableWindow(radix_ > 2);
    GetDlgItem(IDC_DIGIT_3)->EnableWindow(radix_ > 3);
    GetDlgItem(IDC_DIGIT_4)->EnableWindow(radix_ > 4);
    GetDlgItem(IDC_DIGIT_5)->EnableWindow(radix_ > 5);
    GetDlgItem(IDC_DIGIT_6)->EnableWindow(radix_ > 6);
    GetDlgItem(IDC_DIGIT_7)->EnableWindow(radix_ > 7);
    GetDlgItem(IDC_DIGIT_8)->EnableWindow(radix_ > 8);
    GetDlgItem(IDC_DIGIT_9)->EnableWindow(radix_ > 9);
    GetDlgItem(IDC_DIGIT_A)->EnableWindow(radix_ > 10);
    GetDlgItem(IDC_DIGIT_B)->EnableWindow(radix_ > 11);
    GetDlgItem(IDC_DIGIT_C)->EnableWindow(radix_ > 12);
    GetDlgItem(IDC_DIGIT_D)->EnableWindow(radix_ > 13);
    GetDlgItem(IDC_DIGIT_E)->EnableWindow(radix_ > 14);
    GetDlgItem(IDC_DIGIT_F)->EnableWindow(radix_ > 15);

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
        ttc_.AddTool(GetDlgItem(IDC_BINARY), "Use Binary Numbers (F8)");
        ttc_.AddTool(GetDlgItem(IDC_OCTAL), "USe Octal Numbers (F7)");
        ttc_.AddTool(GetDlgItem(IDC_DECIMAL), "Use (Signed) Decimal Numbers (F6)");
        ttc_.AddTool(GetDlgItem(IDC_HEX), "Use Hexadecimal Numbers (F5)");
        ttc_.AddTool(GetDlgItem(IDC_BIG_ENDIAN_FILE_ACCESS), "Read/Write to File in Big-Endian Format");
        ttc_.Activate(TRUE);
    }

	//FixFileButtons(); // No file open yet so what is the point?

    // We need this so that the resizer gets WM_SIZE event after the controls have been added.
    CRect cli;
    GetClientRect(&cli);
    PostMessage(WM_SIZE, SIZE_RESTORED, MAKELONG(cli.Width(), cli.Height()));

#ifdef CALCULATOR_IMPROVEMENTS
#if 0
	// Things that set current operand are black
    ctl_digit_0_ .SetTextColor(RGB(0x0, 0x0, 0x0));
    ctl_digit_1_ .SetTextColor(RGB(0x0, 0x0, 0x0));
    ctl_digit_2_ .SetTextColor(RGB(0x0, 0x0, 0x0));
    ctl_digit_3_ .SetTextColor(RGB(0x0, 0x0, 0x0));
    ctl_digit_4_ .SetTextColor(RGB(0x0, 0x0, 0x0));
    ctl_digit_5_ .SetTextColor(RGB(0x0, 0x0, 0x0));
    ctl_digit_6_ .SetTextColor(RGB(0x0, 0x0, 0x0));
    ctl_digit_7_ .SetTextColor(RGB(0x0, 0x0, 0x0));
    ctl_digit_8_ .SetTextColor(RGB(0x0, 0x0, 0x0));
    ctl_digit_9_ .SetTextColor(RGB(0x0, 0x0, 0x0));
    ctl_digit_a_ .SetTextColor(RGB(0x0, 0x0, 0x0));
    ctl_digit_b_ .SetTextColor(RGB(0x0, 0x0, 0x0));
    ctl_digit_c_ .SetTextColor(RGB(0x0, 0x0, 0x0));
    ctl_digit_d_ .SetTextColor(RGB(0x0, 0x0, 0x0));
    ctl_digit_e_ .SetTextColor(RGB(0x0, 0x0, 0x0));
    ctl_digit_f_ .SetTextColor(RGB(0x0, 0x0, 0x0));

	ctl_mem_get_ .SetTextColor(RGB(0x0, 0x0, 0x0));
	ctl_mark_get_.SetTextColor(RGB(0x0, 0x0, 0x0));
	ctl_sel_get_ .SetTextColor(RGB(0x0, 0x0, 0x0));
	ctl_sel_len_ .SetTextColor(RGB(0x0, 0x0, 0x0));
	ctl_mark_at_ .SetTextColor(RGB(0x0, 0x0, 0x0));
	ctl_sel_at_  .SetTextColor(RGB(0x0, 0x0, 0x0));
	ctl_eof_get_ .SetTextColor(RGB(0x0, 0x0, 0x0));
#endif

	// Things that use the calculator value and other "admin" type buttons are red
	ctl_backspace_     .SetTextColor(RGB(0xC0, 0x0, 0x0));
	ctl_clear_entry_   .SetTextColor(RGB(0xC0, 0x0, 0x0));
	ctl_clear_         .SetTextColor(RGB(0xC0, 0x0, 0x0));
	ctl_equals_        .SetTextColor(RGB(0xC0, 0x0, 0x0));
	ctl_go_            .SetTextColor(RGB(0xC0, 0x0, 0x0));
	ctl_mem_store_     .SetTextColor(RGB(0xC0, 0x0, 0x0));
	ctl_mem_clear_     .SetTextColor(RGB(0xC0, 0x0, 0x0));
	ctl_mem_add_       .SetTextColor(RGB(0xC0, 0x0, 0x0));
	ctl_mem_subtract_  .SetTextColor(RGB(0xC0, 0x0, 0x0));
	ctl_mark_clear_    .SetTextColor(RGB(0xC0, 0x0, 0x0));
	ctl_mark_store_    .SetTextColor(RGB(0xC0, 0x0, 0x0));
	ctl_mark_add_      .SetTextColor(RGB(0xC0, 0x0, 0x0));
	ctl_mark_subtract_ .SetTextColor(RGB(0xC0, 0x0, 0x0));
	ctl_mark_at_store_ .SetTextColor(RGB(0xC0, 0x0, 0x0));
	ctl_sel_store_     .SetTextColor(RGB(0xC0, 0x0, 0x0));
	ctl_sel_at_store_  .SetTextColor(RGB(0xC0, 0x0, 0x0));
	ctl_sel_len_store_ .SetTextColor(RGB(0xC0, 0x0, 0x0));

	// Indicates that GO button takes user back to file
	ctl_go_.SetMouseCursorHand();

	// Binary operations are blue
	ctl_addop_.   SetTextColor(RGB(0x0, 0x0, 0xC0));
	ctl_addop_.   SetTextHotColor(RGB(0,0,0));
	ctl_subtract_.SetTextColor(RGB(0x0, 0x0, 0xC0));
	ctl_subtract_.SetTextHotColor(RGB(0,0,0));
	ctl_multiply_.SetTextColor(RGB(0x0, 0x0, 0xC0));
	ctl_multiply_.SetTextHotColor(RGB(0,0,0));
	ctl_divide_.  SetTextColor(RGB(0x0, 0x0, 0xC0));
	ctl_divide_.  SetTextHotColor(RGB(0,0,0));
	ctl_mod_.     SetTextColor(RGB(0x0, 0x0, 0xC0));
	ctl_mod_.     SetTextHotColor(RGB(0,0,0));
	ctl_pow_.     SetTextColor(RGB(0x0, 0x0, 0xC0));
	ctl_pow_.     SetTextHotColor(RGB(0,0,0));
	ctl_gtr_.     SetTextColor(RGB(0x0, 0x0, 0xC0));
	ctl_gtr_.     SetTextHotColor(RGB(0,0,0));
	ctl_less_.    SetTextColor(RGB(0x0, 0x0, 0xC0));
	ctl_less_.    SetTextHotColor(RGB(0,0,0));
	ctl_or_.      SetTextColor(RGB(0x0, 0x0, 0xC0));
	ctl_or_.      SetTextHotColor(RGB(0,0,0));
	ctl_and_.     SetTextColor(RGB(0x0, 0x0, 0xC0));
	ctl_and_.     SetTextHotColor(RGB(0,0,0));
	ctl_xor_.     SetTextColor(RGB(0x0, 0x0, 0xC0));
	ctl_xor_.     SetTextHotColor(RGB(0,0,0));
	ctl_asr_.     SetTextColor(RGB(0x0, 0x0, 0xC0));
	ctl_asr_.     SetTextHotColor(RGB(0,0,0));
	ctl_lsl_.     SetTextColor(RGB(0x0, 0x0, 0xC0));
	ctl_lsl_.     SetTextHotColor(RGB(0,0,0));
	ctl_lsr_.     SetTextColor(RGB(0x0, 0x0, 0xC0));
	ctl_lsr_.     SetTextHotColor(RGB(0,0,0));
	ctl_rol_.     SetTextColor(RGB(0x0, 0x0, 0xC0));
	ctl_rol_.     SetTextHotColor(RGB(0,0,0));
	ctl_ror_.     SetTextColor(RGB(0x0, 0x0, 0xC0));
	ctl_ror_.     SetTextHotColor(RGB(0,0,0));

	// Unary operations are purple
	ctl_unary_sign_.       SetTextColor(RGB(0x80, 0x0, 0x80));
	ctl_unary_sign_.       SetTextHotColor(RGB(0,0,0));
	ctl_unary_dec_.        SetTextColor(RGB(0x80, 0x0, 0x80));
	ctl_unary_dec_.        SetTextHotColor(RGB(0,0,0));
	ctl_unary_factorial_.  SetTextColor(RGB(0x80, 0x0, 0x80));
	ctl_unary_factorial_.  SetTextHotColor(RGB(0,0,0));
	ctl_unary_flip_.       SetTextColor(RGB(0x80, 0x0, 0x80));
	ctl_unary_flip_.       SetTextHotColor(RGB(0,0,0));
	ctl_unary_rev_.        SetTextColor(RGB(0x80, 0x0, 0x80));
	ctl_unary_rev_.        SetTextHotColor(RGB(0,0,0));
	ctl_unary_inc_.        SetTextColor(RGB(0x80, 0x0, 0x80));
	ctl_unary_inc_.        SetTextHotColor(RGB(0,0,0));
	ctl_unary_not_.        SetTextColor(RGB(0x80, 0x0, 0x80));
	ctl_unary_not_.        SetTextHotColor(RGB(0,0,0));
	ctl_unary_square_.     SetTextColor(RGB(0x80, 0x0, 0x80));
	ctl_unary_square_.     SetTextHotColor(RGB(0,0,0));
	ctl_unary_rol_.        SetTextColor(RGB(0x80, 0x0, 0x80));
	ctl_unary_rol_.        SetTextHotColor(RGB(0,0,0));
	ctl_unary_ror_.        SetTextColor(RGB(0x80, 0x0, 0x80));
	ctl_unary_ror_.        SetTextHotColor(RGB(0,0,0));
	ctl_unary_lsl_.        SetTextColor(RGB(0x80, 0x0, 0x80));
	ctl_unary_lsl_.        SetTextHotColor(RGB(0,0,0));
	ctl_unary_lsr_.        SetTextColor(RGB(0x80, 0x0, 0x80));
	ctl_unary_lsr_.        SetTextHotColor(RGB(0,0,0));
	ctl_unary_asr_.        SetTextColor(RGB(0x80, 0x0, 0x80));
	ctl_unary_asr_.        SetTextHotColor(RGB(0,0,0));
	ctl_unary_cube_.       SetTextColor(RGB(0x80, 0x0, 0x80));
	ctl_unary_cube_.       SetTextHotColor(RGB(0,0,0));
	ctl_unary_at_.         SetTextColor(RGB(0x80, 0x0, 0x80));
	ctl_unary_at_.         SetTextHotColor(RGB(0,0,0));
	ctl_unary_square_root_.SetTextColor(RGB(0x80, 0x0, 0x80));
	ctl_unary_square_root_.SetTextHotColor(RGB(0,0,0));

	build_menus();         // Setup initial menu button menus

	// Since functions menu doesn't change we only need to set it up once (load from resources)
    VERIFY(func_menu_.LoadMenu(IDR_FUNCTION));
    ctl_func_.m_hMenu = func_menu_.GetSubMenu(0)->GetSafeHmenu();
#endif

    return TRUE;
} // OnInitDialog

void CCalcDlg::DoDataExchange(CDataExchange* pDX)
{
	CHexDialogBar::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT,  ctl_edit_combo_);

#ifdef CALCULATOR_IMPROVEMENTS
#if 1
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
#endif
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
#endif
} // DoDataExchange

BEGIN_MESSAGE_MAP(CCalcDlg, CHexDialogBar)
	ON_WM_CREATE()
    ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
    //{{AFX_MSG_MAP(CCalcDlg)
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
    ON_BN_CLICKED(IDC_BINARY, OnBinary)
    ON_BN_CLICKED(IDC_OCTAL, OnOctal)
    ON_BN_CLICKED(IDC_DECIMAL, OnDecimal)
    ON_BN_CLICKED(IDC_HEX, OnHex)
    ON_BN_CLICKED(IDC_SEL_LEN, OnSelLen)
    ON_BN_CLICKED(IDC_SEL_LEN_STORE, OnSelLenStore)
    ON_BN_CLICKED(IDC_ADDOP, OnAdd)
    ON_WM_DESTROY()
	ON_WM_HELPINFO()
    ON_BN_DOUBLECLICKED(IDC_DIGIT_D, OnDigitD)
    ON_BN_DOUBLECLICKED(IDC_BACKSPACE, OnBackspace)
    ON_BN_DOUBLECLICKED(IDC_DIGIT_0, OnDigit0)
    ON_BN_DOUBLECLICKED(IDC_DIGIT_1, OnDigit1)
    ON_BN_DOUBLECLICKED(IDC_DIGIT_2, OnDigit2)
    ON_BN_DOUBLECLICKED(IDC_DIGIT_3, OnDigit3)
    ON_BN_DOUBLECLICKED(IDC_DIGIT_4, OnDigit4)
    ON_BN_DOUBLECLICKED(IDC_DIGIT_5, OnDigit5)
    ON_BN_DOUBLECLICKED(IDC_DIGIT_6, OnDigit6)
    ON_BN_DOUBLECLICKED(IDC_DIGIT_7, OnDigit7)
    ON_BN_DOUBLECLICKED(IDC_DIGIT_8, OnDigit8)
    ON_BN_DOUBLECLICKED(IDC_DIGIT_9, OnDigit9)
    ON_BN_DOUBLECLICKED(IDC_DIGIT_A, OnDigitA)
    ON_BN_DOUBLECLICKED(IDC_DIGIT_B, OnDigitB)
    ON_BN_DOUBLECLICKED(IDC_DIGIT_C, OnDigitC)
    ON_BN_DOUBLECLICKED(IDC_DIGIT_E, OnDigitE)
    ON_BN_DOUBLECLICKED(IDC_DIGIT_F, OnDigitF)
    ON_BN_DOUBLECLICKED(IDC_EQUALS, OnEquals)
    ON_BN_DOUBLECLICKED(IDC_MARK_ADD, OnMarkAdd)
    ON_BN_DOUBLECLICKED(IDC_MARK_SUBTRACT, OnMarkSubtract)
    ON_BN_DOUBLECLICKED(IDC_MEM_ADD, OnMemAdd)
    ON_BN_DOUBLECLICKED(IDC_MEM_SUBTRACT, OnMemSubtract)
    ON_BN_DOUBLECLICKED(IDC_UNARY_ASR, OnUnaryAsr)
    ON_BN_DOUBLECLICKED(IDC_UNARY_AT, OnUnaryAt)
    ON_BN_DOUBLECLICKED(IDC_UNARY_CUBE, OnUnaryCube)
    ON_BN_DOUBLECLICKED(IDC_UNARY_DEC, OnUnaryDec)
    ON_BN_DOUBLECLICKED(IDC_UNARY_FACTORIAL, OnUnaryFactorial)
    ON_BN_DOUBLECLICKED(IDC_UNARY_FLIP, OnUnaryFlip)
    ON_BN_DOUBLECLICKED(IDC_UNARY_INC, OnUnaryInc)
    ON_BN_DOUBLECLICKED(IDC_UNARY_LSL, OnUnaryLsl)
    ON_BN_DOUBLECLICKED(IDC_UNARY_LSR, OnUnaryLsr)
    ON_BN_DOUBLECLICKED(IDC_UNARY_NOT, OnUnaryNot)
    ON_BN_DOUBLECLICKED(IDC_UNARY_REV, OnUnaryRev)
    ON_BN_DOUBLECLICKED(IDC_UNARY_ROL, OnUnaryRol)
    ON_BN_DOUBLECLICKED(IDC_UNARY_ROR, OnUnaryRor)
    ON_BN_DOUBLECLICKED(IDC_UNARY_SIGN, OnUnarySign)
    ON_BN_DOUBLECLICKED(IDC_UNARY_SQUARE, OnUnarySquare)
    ON_BN_DOUBLECLICKED(IDC_UNARY_SQUARE_ROOT, OnUnarySquareRoot)
	ON_BN_CLICKED(IDC_BIG_ENDIAN_FILE_ACCESS, OnBigEndian)
	//}}AFX_MSG_MAP
#ifdef CALCULATOR_IMPROVEMENTS
    ON_BN_CLICKED(IDC_HEX_HIST, OnGetHexHist)
    ON_BN_CLICKED(IDC_DEC_HIST, OnGetDecHist)
    ON_BN_CLICKED(IDC_VARS, OnGetVar)
    ON_BN_CLICKED(IDC_FUNC, OnGetFunc)
#endif
	ON_WM_SIZE()
	ON_NOTIFY(TTN_SHOW, 0, OnTooltipsShow)
    ON_WM_CONTEXTMENU()
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

void CCalcDlg::Redisplay()
{
    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
    CString ss;

    DWORD sel = edit_.GetSel();
    edit_.GetWindowText(ss);
    if (aa->hex_ucase_)
        ss.MakeUpper();
    else
        ss.MakeLower();
    edit_.SetWindowText(ss);
    edit_.SetSel(sel);
}

// Tidy up any pending ops if macro is finishing
void CCalcDlg::FinishMacro()
{
    if (source_ != km_result)
        aa_->SaveToMacro(source_, current_&mask_);
    source_ = km_result;
}

// Check if the current expression in the edit box is a valid expression returning an integer
bool CCalcDlg::invalid_expression()
{
    CString ss = "Calculator buttons only operate on integer \r\n"
                 "values, but the current value is ";
	switch (current_type_)
	{
	case CJumpExpr::TYPE_INT:
		return false;           // This is OK!

	case CJumpExpr::TYPE_NONE:
        ::HMessageBox((LPCTSTR)CString(current_str_));
        edit_.SetFocus();
		edit_.SetSel(mm_->expr_.get_error_pos(), mm_->expr_.get_error_pos());
		return true;

	case CHexExpr::TYPE_REAL:
        ::HMessageBox((LPCTSTR)(ss + "real."));
		return true;

	case CHexExpr::TYPE_BOOLEAN:
        ::HMessageBox((LPCTSTR)(ss + "boolean."));
		return true;

	case CHexExpr::TYPE_STRING:
        ::HMessageBox((LPCTSTR)(ss + "a string."));
		return true;

	case CHexExpr::TYPE_DATE:
        ::HMessageBox((LPCTSTR)(ss + "a date."));
		return true;

	default:
		ASSERT(0);
		return true;
	}
}

void CCalcDlg::do_binop(binop_type binop)
{
	if (invalid_expression())
		return;

    if (in_edit_ || op_ == binop_none || aa_->playing_)
    {
        unsigned __int64 temp = current_;
        calc_previous();
        if (!aa_->refresh_off_ && IsVisible()) edit_.Put();

        // If the value in current_ is not there as a result of a calculation then
        // save operand (current_) and it's source (user-entered, cursor position etc)
        // Also don't save the value if this is the very first thing in the macro.
        if (source_ != km_result)
            aa_->SaveToMacro(source_, temp);

        // Get ready for new value to be entered
        previous_ = current_;
        current_ = 0;
        source_ = km_result;
    }
    aa_->SaveToMacro(km_binop, long(binop));
    op_ = binop;

    if (!aa_->refresh_off_ && IsVisible())
    {
        FixFileButtons();
        if (!overflow_ && !error_)
            ShowBinop(binop);
    }
    in_edit_ = FALSE;
}

void CCalcDlg::ShowStatus()
{
    if (!aa_->refresh_off_ && IsVisible())
    {
        ASSERT(GetDlgItem(IDC_OP_DISPLAY) != NULL);
        if (overflow_)
        {
            TRACE0("OVERFLOW!!");
            GetDlgItem(IDC_OP_DISPLAY)->SetWindowText("O");
#ifdef SYS_SOUNDS
            CSystemSound::Play("Calculator Error");
#else
            ::Beep(3000,400);
#endif
        }
        else if (error_)
        {
            TRACE0("ERROR!!");
            GetDlgItem(IDC_OP_DISPLAY)->SetWindowText("E");
#ifdef SYS_SOUNDS
            CSystemSound::Play("Calculator Error");
#else
            ::Beep(3000,400);
#endif
        }
        else
            GetDlgItem(IDC_OP_DISPLAY)->SetWindowText("");
    }
}

void CCalcDlg::ShowBinop(int ii /*=-1*/)
{
    binop_type binop;

    if (ii == -1)
        binop = op_;
    else
        binop = binop_type(ii);

    const char *op_disp = "";
    switch (binop)
    {
    case binop_add:
        op_disp = "+";
        break;
    case binop_subtract:
        op_disp = "-";
        break;
    case binop_multiply:
        op_disp = "*";
        break;
    case binop_divide:
        op_disp = "/";
        break;
    case binop_mod:
        op_disp = "%";
        break;
    case binop_pow:
        op_disp = "**";
        break;
    case binop_gtr:
        op_disp = ">";
        break;
    case binop_less:
        op_disp = "<";
        break;
    case binop_ror:
        op_disp = "=>";
        break;
    case binop_rol:
        op_disp = "<=";
        break;
    case binop_lsl:
        op_disp = "<-";
        break;
    case binop_lsr:
        op_disp = "->";
        break;
    case binop_asr:
        op_disp = "+>";
        break;

    case binop_and:
        op_disp = "&&";  // displays as a single &
        break;
    case binop_or:
        op_disp = "|";
        break;
    case binop_xor:
        op_disp = "^";
        break;
    }

    ASSERT(IsVisible());
    ASSERT(GetDlgItem(IDC_OP_DISPLAY) != NULL);
    GetDlgItem(IDC_OP_DISPLAY)->SetWindowText(op_disp);
}

// Values for detecting overflows in some unary operators
static unsigned __int64 fact_max[4] = { 5, 8, 12, 20};
static unsigned __int64 square_max[4] = { 0xF, 0xFF, 0xFFFF, 0xffffFFFF };
static unsigned __int64 cube_max[4] = { 6, 40, 1625, 2642244 };
static unsigned __int64 signed_fact_max[4] = { 5, 7, 12, 20};
static unsigned __int64 signed_square_max[4] = { 11, 181, 46340, 3037000499 };
static unsigned __int64 signed_cube_max[4] = { 5, 31, 1290, 2097151 };

void CCalcDlg::do_unary(unary_type unary)
{
	if (invalid_expression())
		return;

    unsigned char *byte;                // Pointer to bytes of current value
    unsigned char cc;
    int ii;                             // Loop variable
    __int64 temp;                       // Temp variables for calcs
    signed char s8;
    signed short s16;
    signed long s32;
    signed __int64 s64;
    CHexEditView *pview;                // Active view (or NULL if no views)

    // Save the value to macro before we do anything with it (unless it's a result of a previous op).
    // Also don't save the value if this (unary op) is the very first thing in the macro.
    if (source_ != km_result)
        aa_->SaveToMacro(source_, current_);

    overflow_ = error_ = FALSE;

    if (unary >= unary_mask)
    {
        // Mask off any high bits that might interfere with the calculations
        current_ &= mask_;
    }
    switch (unary)
    {
    case unary_inc:
        ++current_;
        if ((radix_ == 10 && current_ == ~(mask_>>1)) ||
            (radix_ != 10 && (current_ & mask_) == 0))
        {
            overflow_ = TRUE;
        }
        break;
    case unary_dec:
        if ((radix_ == 10 && current_ == ~(mask_>>1)) ||
            (radix_ != 10 && (current_ & mask_) == 0))
        {
            overflow_ = TRUE;
        }
        current_ --;
        break;
    case unary_sign:
        if (radix_ == 10 && current_ == ~(mask_>>1))
            overflow_ = TRUE;  // eg in 8 bit mode -128 => 128 (overflow)
        current_ = -(__int64)current_;
        break;
    case unary_square:
        ASSERT(bits_index_ > -1 && bits_index_ < 4);
        if ((radix_ == 10 && current_ > signed_square_max[3-bits_index_]) ||
            (radix_ != 10 && current_ > square_max[3-bits_index_]))
        {
            overflow_ = TRUE;
        }
        current_ = current_ * current_;
        break;
    case unary_squareroot:
        // Rounds down to the nearest int so before we take root add 0.5 to avoid
        // problems. eg. sqrt(4) -> 1.99999999... -> 1 BUT sqrt(4.5) -> 2.12 -> 2.
        if (radix_ == 10 && (current_&sign_mask_) != 0)
        {
            mm_->StatusBarText("Square root of -ve value is illegal");
            error_ = TRUE;
        }
        current_ = unsigned __int64(sqrt(double(__int64(current_)) + 0.5));
        break;
    case unary_cube:
        ASSERT(bits_index_ > -1 && bits_index_ < 4);
        if ((radix_ == 10 && current_ > signed_cube_max[3-bits_index_]) ||
            (radix_ != 10 && current_ > cube_max[3-bits_index_]))
        {
            overflow_ = TRUE;
        }
        current_ = current_ * current_ * current_;
        break;
    case unary_factorial:
        ASSERT(bits_index_ > -1 && bits_index_ < 4);
        if ((radix_ == 10 && current_ > signed_fact_max[3-bits_index_]) ||
            (radix_ != 10 && current_ > fact_max[3-bits_index_]))
        {
            overflow_ = TRUE;
        }
        // Only do the calcs if current_ is not too big (else calcs may take a long time)
        if (current_ < 100)
        {
            temp = 1;
            for (ii = 1; ii < current_; ++ii)
                temp *= ii + 1;
            current_ = temp;
        }
        else
        {
            ASSERT(overflow_);        // (Must have overflowed if current_ is this big.)
            current_ = 0;
        }
        break;
    case unary_not:
        current_ = ~current_;
        break;
    case unary_rol:
        current_ = ((current_ << 1) & mask_) | (current_ >> (bits_ - 1));
        break;
    case unary_ror:
        current_ = (current_ >> 1) | ((current_ << (bits_ - 1)) & mask_);
        break;
    case unary_lsl:
        current_ = (current_ & mask_) << 1;
        break;
    case unary_lsr:
        current_ = (current_ & mask_) >> 1;
        break;
    case unary_asr:
        // Use Compiler signed types to do sign extension
        if (bits_ == 8)
        {
            s8 = (signed char)current_;
            s8 >>= 1;
            current_ = s8;
        }
        else if (bits_ == 16)
        {
            s16 = (signed short)current_;
            s16 >>= 1;
            current_ = s16;
        }
        else if (bits_ == 32)
        {
            s32 = (signed long)current_;
            s32 >>= 1;
            current_ = s32;
        }
        else if (bits_ == 64)
        {
            s64 = current_;
            s64 >>= 1;
            current_ = s64;
        }
        else
            ASSERT(0);
        break;
    case unary_rev:  // Reverse all bits
        // This algorithm tests the current bits by shifting right and testing
        // bottom bit.  Any bits on sets the correspoding bit of the result as
        // it is shifted left.
        // This could be sped up using a table lookup but is probably OK for now.
        temp = 0;
        for (ii = 0; ii < bits_; ++ii)
        {
            temp <<= 1;                  // Make room for the next bit
            if (current_ & 0x1)
                temp |= 0x1;
            current_ >>= 1;              // Move bits down to test the next
        }
        current_ = temp;
        break;
    case unary_flip: // Flip byte order
        // This assumes (of course) that byte order is little-endian in memory
        byte = (unsigned char *)&current_;
        switch (bits_)
        {
        case 8:
            mm_->StatusBarText("Can't flip bytes in 8 bit mode");
            aa_->mac_error_ = 2;
            break;
        case 16:
            cc = byte[0]; byte[0] = byte[1]; byte[1] = cc;
            break;
        case 32:
            cc = byte[0]; byte[0] = byte[3]; byte[3] = cc;
            cc = byte[1]; byte[1] = byte[2]; byte[2] = cc;
            break;
        case 64:
            cc = byte[0]; byte[0] = byte[7]; byte[7] = cc;
            cc = byte[1]; byte[1] = byte[6]; byte[6] = cc;
            cc = byte[2]; byte[2] = byte[5]; byte[5] = cc;
            cc = byte[3]; byte[3] = byte[4]; byte[4] = cc;
            break;
        default:
            ASSERT(0);
        }
        break;
    case unary_at:
        pview = GetView();
		// Make sure view and calculator endianness are in sync
		ASSERT(pview == NULL || pview->BigEndian() == ((CButton*)GetDlgItem(IDC_BIG_ENDIAN_FILE_ACCESS))->GetCheck());
        if (pview == NULL || (current_&mask_) + bits_/8 > (unsigned __int64)pview->GetDocument()->length())
        {
            if (pview == NULL)
                mm_->StatusBarText("No window open");
            else
                mm_->StatusBarText("@ n: Not enough bytes to end of file to read");
            aa_->mac_error_ = 10;
            return;
        }

        temp = 0;
        pview->GetDocument()->GetData((unsigned char *)&temp, bits_/8, current_);
        if (pview->BigEndian())
        {
            // Reverse the byte order to match that used internally (Intel=little-endian)
            byte = (unsigned char *)&temp;
            switch (bits_)
            {
            case 8:
                /* nothing */
                break;
            case 16:
                cc = byte[0]; byte[0] = byte[1]; byte[1] = cc;
                break;
            case 32:
                cc = byte[0]; byte[0] = byte[3]; byte[3] = cc;
                cc = byte[1]; byte[1] = byte[2]; byte[2] = cc;
                break;
            case 64:
                cc = byte[0]; byte[0] = byte[7]; byte[7] = cc;
                cc = byte[1]; byte[1] = byte[6]; byte[6] = cc;
                cc = byte[2]; byte[2] = byte[5]; byte[5] = cc;
                cc = byte[3]; byte[3] = byte[4]; byte[4] = cc;
                break;
            default:
                ASSERT(0);
            }
        }
        current_ = temp;
        break;

    case unary_none:
    default:
        ASSERT(0);
    }

    ShowStatus();                       // Indicate overflow etc

    if (overflow_ || error_)
    {
        if (overflow_)
            mm_->StatusBarText("Result overflowed data size");
        else
            mm_->StatusBarText("Arithmetic error (root of -ve, etc)");
        aa_->mac_error_ = 2;
    }

    if (!aa_->refresh_off_ && IsVisible())
    {
        edit_.Put();
        FixFileButtons();
    }

    // Save the unary operation to the current macro (if any)
    aa_->SaveToMacro(km_unaryop, long(unary));

    // The user can't edit the result of a unary operation by pressing digits
    in_edit_ = FALSE;
    source_ = km_result;
}

void CCalcDlg::do_digit(char digit)
{
    if (!aa_->refresh_off_ && IsVisible())  // Always true but left in for consistency
    {
        //edit_.SetFocus();
        if (!in_edit_)
        {
            edit_.SetWindowText("");
            ShowBinop(op_);
        }
    }
    edit_.SendMessage(WM_CHAR, digit, 1);
    in_edit_ = TRUE;
    source_ = aa_->recording_ ? km_user : km_result;
}

void CCalcDlg::calc_previous()
{
    bool same_sign;                     // Used in signed (radix 10 only) tests
    int ii;
    __int64 temp;                       // Temp used in calcs
    signed char s8;                     // Signed int types used for ASR
    signed short s16;
    signed long s32;
    signed __int64 s64;

	ASSERT(current_type_ != CJumpExpr::TYPE_NONE);      // We can't use an invalid expression
    overflow_ = error_ = FALSE;

    if (op_ >= binop_mask)
    {
        // Mask off any high bits that might interfere with the calculations
        current_ &= mask_;
        previous_ &= mask_;
    }

    switch (op_)
    {
    case binop_add:
        if (radix_ == 10)
            same_sign = (current_ & sign_mask_) == (previous_ & sign_mask_);
        else if ((previous_&mask_) > mask_ - (current_&mask_))
            overflow_ = TRUE;
        current_ += previous_;

        // For signed numbers overflow if operands have same sign which is diff to sign of result
        if (radix_ == 10 && same_sign && (current_ & sign_mask_) != (previous_ & sign_mask_))
            overflow_ = TRUE;
        break;
    case binop_subtract:
        if (radix_ == 10)
            same_sign = (current_ & sign_mask_) == (previous_ & sign_mask_);
        else if ((previous_&mask_) < (current_&mask_))
            overflow_ = TRUE;
        current_ = previous_ - current_;

        if (radix_ == 10 && !same_sign && (current_ & sign_mask_) != (previous_ & sign_mask_))
            overflow_ = TRUE;
        break;
    case binop_multiply:
        if (current_ != 0 && radix_ == 10)
        {
            if ((bits_ ==  8 && mac_abs((signed char)previous_) > mac_abs((signed char)(mask_>>1)/(signed char)current_)) ||
                (bits_ == 16 && mac_abs((signed short)previous_) > mac_abs((signed short)(mask_>>1)/(signed short)current_)) ||
                (bits_ == 32 && mac_abs((signed long)previous_) > mac_abs((signed long)(mask_>>1)/(signed long)current_)) ||
                (bits_ == 64 && mac_abs((signed __int64)previous_) > mac_abs((signed __int64)(mask_>>1)/(signed __int64)current_)))
            {
                overflow_ = TRUE;
            }
        }
        else if (current_ != 0 && (previous_&mask_) > mask_/(current_&mask_))
            overflow_ = TRUE;
        current_ *= previous_;
        break;
    case binop_divide:
        if ((current_&mask_) == 0)
            current_ = 0, error_ = TRUE;
        else
            current_ = previous_ / current_;
        break;
    case binop_mod:
        if ((current_&mask_) == 0)
            current_ = 0, error_ = TRUE;
        else
            current_ = previous_ % current_;
        break;
    case binop_pow:
        if (previous_ == 1 || current_ == 0)
            current_ = 1;               // One to anything or anything to zero is one (assume 0^0 == 1)
        else if (previous_ == 0)
            current_ = 0;               // Zero to anything is zero
        else
        {
            double logmax = log(pow(2.0, double(bits_)) - 1.0);
            if ((double)(signed __int64)current_ > logmax/log((double)(signed __int64)previous_))
                overflow_ = TRUE;
            // Only do the calcs if current_ is not too big (else calcs may take a long time)
            if (current_ < 128)
            {
                temp = 1;
                // Using a short loop should not be too slow but could possibly be sped up if necessary.
                for (ii = 0; ii < current_; ++ii)
                    temp *= previous_;
                current_ = temp;
            }
            else
            {
                ASSERT(overflow_);        // (Must have overflowed if current_ is this big.)
                current_ = 0;
            }
        }
        break;
    case binop_gtr:
    case binop_gtr_old:
        if ((radix_ == 10 && bits_ ==  8 && (signed char)current_ <= (signed char)previous_) ||
            (radix_ == 10 && bits_ == 16 && (signed short)current_ <= (signed short)previous_) ||
            (radix_ == 10 && bits_ == 32 && (signed long)current_ <= (signed long)previous_) ||
            (radix_ == 10 && bits_ == 64 && (signed __int64)current_ <= (signed __int64)previous_) ||
            (radix_ != 10 && bits_ ==  8 && (unsigned char)current_ <= (unsigned char)previous_) ||
            (radix_ != 10 && bits_ == 16 && (unsigned short)current_ <= (unsigned short)previous_) ||
            (radix_ != 10 && bits_ == 32 && (unsigned long)current_ <= (unsigned long)previous_) ||
            (radix_ != 10 && bits_ == 64 && (unsigned __int64)current_ <= (unsigned __int64)previous_) )
        {
            current_ = previous_;
        }
        else
            aa_->mac_error_ = 1;        // Allow detection of max value
        break;
    case binop_less:
    case binop_less_old:
        if ((radix_ == 10 && bits_ ==  8 && (signed char)current_ >= (signed char)previous_) ||
            (radix_ == 10 && bits_ == 16 && (signed short)current_ >= (signed short)previous_) ||
            (radix_ == 10 && bits_ == 32 && (signed long)current_ >= (signed long)previous_) ||
            (radix_ == 10 && bits_ == 64 && (signed __int64)current_ >= (signed __int64)previous_) ||
            (radix_ != 10 && bits_ ==  8 && (unsigned char)current_ >= (unsigned char)previous_) ||
            (radix_ != 10 && bits_ == 16 && (unsigned short)current_ >= (unsigned short)previous_) ||
            (radix_ != 10 && bits_ == 32 && (unsigned long)current_ >= (unsigned long)previous_) ||
            (radix_ != 10 && bits_ == 64 && (unsigned __int64)current_ >= (unsigned __int64)previous_) )
        {
            current_ = previous_;
        }
        else
            aa_->mac_error_ = 1;        // Allow detection of min value
        break;
    case binop_ror:
        temp = current_ % bits_;
        current_ = ((previous_ << temp) & mask_) | (previous_ >> (bits_ - temp));
        break;
    case binop_rol:
        temp = current_ % bits_;
        current_ = (previous_ >> temp) | ((previous_ << (bits_ - temp)) & mask_);
        break;
    case binop_lsl:
        current_ = (previous_ << current_) & mask_;
        break;
    case binop_lsr:
        current_ = previous_ >> current_;
        break;
    case binop_asr:
        // Use Compiler signed types to do sign extension
        if (bits_ == 8)
        {
            s8 = (signed char)previous_;
            s8 >>= current_;
            current_ = s8;
        }
        else if (bits_ == 16)
        {
            s16 = (short)previous_;
            s16 >>= current_;
            current_ = s16;
        }
        else if (bits_ == 32)
        {
            s32 = (long)previous_;
            s32 >>= current_;
            current_ = s32;
        }
        else if (bits_ == 64)
        {
            s64 = previous_;
            s64 >>= current_;
            current_ = s64;
        }
        else
            ASSERT(0);
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
        /* fall through in release versions */
    case binop_none:
        // No operation but convert a +ve decimal that is too big to be
        // represented to the equivalent -ve value.
        if (radix_ == 10 && (current_ & ((mask_ + 1)>>1)) != 0)
        {
            current_ |= ~mask_;
        }
        break;
    }

	// Signal that current value is not an expression.  This allows correct refresh behaviour (eg,
	// refresh off during macros playback) so that edit box is redrawn correctly (see update_controls).
	current_const_ = TRUE;
	current_type_ = CJumpExpr::TYPE_INT;

    // Check if the result overflowed the number of bits in use.
    // This will not detect overflow for bits_ == 64, in which case
    // overflow should have been detected above.

    if (op_ >= binop_mask)
    {
        // If we are using decimals check if the value is -ve (has high bit set)
        if (radix_ == 10 && (current_ & ((mask_ + 1)>>1)) != 0)
        {
            // Make sure all the high bits are on
            if (~(current_ | mask_) != 0)
                overflow_ = TRUE;
        }
        else
        {
            // Make sure all the high bits are off
            if ((current_ & ~mask_) != 0)
                overflow_ = TRUE;
        }
    }

    ShowStatus();                       // Indicate overflow/error (if any)

    if (overflow_ || error_)
    {
        if (overflow_)
            mm_->StatusBarText("Result overflowed data size");
        else
            mm_->StatusBarText("Arithmetic error (divide by 0, etc)");
        aa_->mac_error_ = 2;
    }
}  // calc_previous

// This will go when we remove km_endian (replaced by km_big_endian)
void CCalcDlg::toggle_endian()
{
	ASSERT(GetDlgItem(IDC_BIG_ENDIAN_FILE_ACCESS) != NULL);

	CHexEditView *pview = GetView();
	if (pview != NULL)
	{
		GetDlgItem(IDC_BIG_ENDIAN_FILE_ACCESS)->EnableWindow(TRUE);  // enable so we can change check
		//bool big_endian = !pview->BigEndian();
		//pview->SetBigEndian(big_endian);
		//if (!aa_->refresh_off_ && IsVisible())
		//	((CButton *)GetDlgItem(IDC_BIG_ENDIAN_FILE_ACCESS))->SetCheck(big_endian);

        // Do it this way so that macros and undo work
        if (pview->BigEndian())
            pview->OnLittleEndian();
        else
		    pview->OnBigEndian();
	}
}

void CCalcDlg::change_base(int base)
{
    radix_ = base;                      // Store new radix (affects edit control display)

    // Set up index for next radio button update
    switch (radix_)
    {
    case 2:
        base_index_ = 3;
        break;
    case 8:
        base_index_ = 2;
        break;
    case 10:
        base_index_ = 1;
        break;
    case 16:
        base_index_ = 0;
        break;
    default:
        ASSERT(0);
    }

    // check that it's an integer expression
	if (invalid_expression())
	{
		edit_.update_value(false);
		return;
	}

    if (!aa_->refresh_off_ && IsVisible())
	{
		update_controls();
		edit_.Put();
	}

    source_ = km_result;
    aa_->SaveToMacro(km_change_base, long(radix_));
}

void CCalcDlg::change_bits(int bits)
{
    __int64 one = 1;

    bits_ = bits;
    mask_ = (one << bits_) - 1;
    sign_mask_ = one << (bits - 1);

    // Set up index for next radio button update
    switch (bits_)
    {
    case 8:
        bits_index_ = 3;
        break;
    case 16:
        bits_index_ = 2;
        break;
    case 32:
        bits_index_ = 1;
        break;
    case 64:
        bits_index_ = 0;
        break;
    default:
        ASSERT(0);
    }

	if (invalid_expression())
	{
		edit_.update_value(false);
		return;
	}

    overflow_ = error_ = FALSE;
    if (radix_ == 10 &&
        ((current_&sign_mask_) == 0 && (current_&~mask_) != 0 ||
         (current_&sign_mask_) != 0 && ~(current_|mask_) != 0) ||
        radix_ != 10 && (current_ & ~mask_) != 0)
    {
        overflow_ = TRUE;
        mm_->StatusBarText("Current value overflowed new data size");
        aa_->mac_error_ = 1;
    }
    ShowStatus();                       // Set/clear overflow indicator

    if (!aa_->refresh_off_ && IsVisible())
	{
		update_controls();
		edit_.Put();
	}

    source_ = km_result;
    aa_->SaveToMacro(km_change_bits, long(bits_));
}

void CCalcDlg::update_controls()
{
    // Update radio buttons
    ASSERT(GetDlgItem(IDC_HEX) != NULL);
    ASSERT(GetDlgItem(IDC_DECIMAL) != NULL);
    ASSERT(GetDlgItem(IDC_OCTAL) != NULL);
    ASSERT(GetDlgItem(IDC_BINARY) != NULL);
	((CButton *)GetDlgItem(IDC_HEX    ))->SetCheck(base_index_ == 0);
	((CButton *)GetDlgItem(IDC_DECIMAL))->SetCheck(base_index_ == 1);
	((CButton *)GetDlgItem(IDC_OCTAL  ))->SetCheck(base_index_ == 2);
	((CButton *)GetDlgItem(IDC_BINARY ))->SetCheck(base_index_ == 3);

    // Enable/disable digits keys depending on base
    ASSERT(GetDlgItem(IDC_DIGIT_2) != NULL);
    ASSERT(GetDlgItem(IDC_DIGIT_3) != NULL);
    ASSERT(GetDlgItem(IDC_DIGIT_4) != NULL);
    ASSERT(GetDlgItem(IDC_DIGIT_5) != NULL);
    ASSERT(GetDlgItem(IDC_DIGIT_6) != NULL);
    ASSERT(GetDlgItem(IDC_DIGIT_7) != NULL);
    ASSERT(GetDlgItem(IDC_DIGIT_8) != NULL);
    ASSERT(GetDlgItem(IDC_DIGIT_9) != NULL);
    ASSERT(GetDlgItem(IDC_DIGIT_A) != NULL);
    ASSERT(GetDlgItem(IDC_DIGIT_B) != NULL);
    ASSERT(GetDlgItem(IDC_DIGIT_C) != NULL);
    ASSERT(GetDlgItem(IDC_DIGIT_D) != NULL);
    ASSERT(GetDlgItem(IDC_DIGIT_E) != NULL);
    ASSERT(GetDlgItem(IDC_DIGIT_F) != NULL);
    GetDlgItem(IDC_DIGIT_2)->EnableWindow(radix_ > 2);
    GetDlgItem(IDC_DIGIT_3)->EnableWindow(radix_ > 3);
    GetDlgItem(IDC_DIGIT_4)->EnableWindow(radix_ > 4);
    GetDlgItem(IDC_DIGIT_5)->EnableWindow(radix_ > 5);
    GetDlgItem(IDC_DIGIT_6)->EnableWindow(radix_ > 6);
    GetDlgItem(IDC_DIGIT_7)->EnableWindow(radix_ > 7);
    GetDlgItem(IDC_DIGIT_8)->EnableWindow(radix_ > 8);
    GetDlgItem(IDC_DIGIT_9)->EnableWindow(radix_ > 9);
    GetDlgItem(IDC_DIGIT_A)->EnableWindow(radix_ > 10);
    GetDlgItem(IDC_DIGIT_B)->EnableWindow(radix_ > 11);
    GetDlgItem(IDC_DIGIT_C)->EnableWindow(radix_ > 12);
    GetDlgItem(IDC_DIGIT_D)->EnableWindow(radix_ > 13);
    GetDlgItem(IDC_DIGIT_E)->EnableWindow(radix_ > 14);
    GetDlgItem(IDC_DIGIT_F)->EnableWindow(radix_ > 15);

    ASSERT(GetDlgItem(IDC_8BIT) != NULL);
    ASSERT(GetDlgItem(IDC_16BIT) != NULL);
    ASSERT(GetDlgItem(IDC_32BIT) != NULL);
    ASSERT(GetDlgItem(IDC_64BIT) != NULL);
	((CButton *)GetDlgItem(IDC_8BIT ))->SetCheck(bits_index_ == 3);
	((CButton *)GetDlgItem(IDC_16BIT))->SetCheck(bits_index_ == 2);
	((CButton *)GetDlgItem(IDC_32BIT))->SetCheck(bits_index_ == 1);
	((CButton *)GetDlgItem(IDC_64BIT))->SetCheck(bits_index_ == 0);

    // Disable flip bytes button if there is only one byte
    ASSERT(GetDlgItem(IDC_UNARY_FLIP) != NULL);
    GetDlgItem(IDC_UNARY_FLIP)->EnableWindow(bits_ > 8);
    ASSERT(GetDlgItem(IDC_BIG_ENDIAN_FILE_ACCESS) != NULL);
    GetDlgItem(IDC_BIG_ENDIAN_FILE_ACCESS)->EnableWindow(GetView() != NULL && bits_ > 8);

	if (current_const_ == TRUE)
        edit_.Put();
    FixFileButtons();
}

void CCalcDlg::FixFileButtons()
{
    ASSERT(IsVisible());      // Should only be called if dlg is visible
    ASSERT(GetDlgItem(IDC_MARK_GET) != NULL);
    ASSERT(GetDlgItem(IDC_MARK_STORE) != NULL);
    ASSERT(GetDlgItem(IDC_MARK_CLEAR) != NULL);
    ASSERT(GetDlgItem(IDC_MARK_ADD) != NULL);
    ASSERT(GetDlgItem(IDC_MARK_SUBTRACT) != NULL);
    ASSERT(GetDlgItem(IDC_MARK_AT) != NULL);
    ASSERT(GetDlgItem(IDC_MARK_AT_STORE) != NULL);
    ASSERT(GetDlgItem(IDC_SEL_GET) != NULL);
    ASSERT(GetDlgItem(IDC_SEL_STORE) != NULL);
    ASSERT(GetDlgItem(IDC_SEL_AT) != NULL);
    ASSERT(GetDlgItem(IDC_SEL_AT_STORE) != NULL);
    ASSERT(GetDlgItem(IDC_SEL_LEN) != NULL);
    ASSERT(GetDlgItem(IDC_SEL_LEN_STORE) != NULL);
    ASSERT(GetDlgItem(IDC_EOF_GET) != NULL);
    ASSERT(GetDlgItem(IDC_GO) != NULL);
    ASSERT(GetDlgItem(IDC_UNARY_AT) != NULL);
    ASSERT(GetDlgItem(IDC_BIG_ENDIAN_FILE_ACCESS) != NULL);

    CHexEditView *pview = GetView();
    FILE_ADDRESS start, end;                    // Current selection
    FILE_ADDRESS eof, mark;                     // Length of file and posn of mark

    if (pview != NULL)
    {
        pview->GetSelAddr(start, end);
        eof = pview->GetDocument()->length();
        mark = pview->GetMark();
    }

    GetDlgItem(IDC_MARK_GET)->EnableWindow(pview != NULL);
    GetDlgItem(IDC_MARK_CLEAR)->EnableWindow(pview != NULL);
    GetDlgItem(IDC_MARK_AT)->EnableWindow(pview != NULL && mark + bits_/8 <= eof);
    GetDlgItem(IDC_SEL_GET)->EnableWindow(pview != NULL);
    GetDlgItem(IDC_SEL_AT)->EnableWindow(pview != NULL && start + bits_/8 <= eof);
    GetDlgItem(IDC_SEL_LEN)->EnableWindow(pview != NULL);
    GetDlgItem(IDC_EOF_GET)->EnableWindow(pview != NULL);

	if (current_type_ != CJumpExpr::TYPE_INT)
	{
		GetDlgItem(IDC_MARK_STORE)->EnableWindow(FALSE);
		GetDlgItem(IDC_MARK_SUBTRACT)->EnableWindow(FALSE);
		GetDlgItem(IDC_MARK_ADD)->EnableWindow(FALSE);
		GetDlgItem(IDC_MARK_AT_STORE)->EnableWindow(FALSE);
		GetDlgItem(IDC_SEL_STORE)->EnableWindow(FALSE);
		GetDlgItem(IDC_SEL_AT_STORE)->EnableWindow(FALSE);
		GetDlgItem(IDC_SEL_LEN_STORE)->EnableWindow(FALSE);
		GetDlgItem(IDC_GO)->EnableWindow(FALSE);
		GetDlgItem(IDC_UNARY_AT)->EnableWindow(FALSE);
	}
	else
	{
		GetDlgItem(IDC_MARK_STORE)->EnableWindow(pview != NULL && (current_&mask_) <= (unsigned __int64)eof);
		if (pview != NULL && radix_ == 10)
		{
			signed __int64 mark_minus, mark_plus;
			switch (bits_)
			{
			case 8:
				mark_minus = mark - (signed char)current_;
				mark_plus  = mark + (signed char)current_;
				break;
			case 16:
				mark_minus = mark - (signed short)current_;
				mark_plus  = mark + (signed short)current_;
				break;
			case 32:
				mark_minus = mark - (signed long)current_;
				mark_plus  = mark + (signed long)current_;
				break;
			case 64:
				mark_minus = mark - (signed __int64)current_;
				mark_plus  = mark + (signed __int64)current_;
				break;
			}
			GetDlgItem(IDC_MARK_SUBTRACT)->EnableWindow(pview != NULL && mark_minus > 0 && mark_minus <= eof);
			GetDlgItem(IDC_MARK_ADD)->EnableWindow(pview != NULL && mark_plus > 0 && mark_plus <= eof);
		}
		else
		{
			GetDlgItem(IDC_MARK_SUBTRACT)->EnableWindow(pview != NULL && mark - (current_&mask_) <= (unsigned __int64)eof);
			GetDlgItem(IDC_MARK_ADD)->EnableWindow(pview != NULL && mark + (current_&mask_) <= (unsigned __int64)eof);
		}
		GetDlgItem(IDC_MARK_AT_STORE)->EnableWindow(pview != NULL && !pview->ReadOnly() && mark <= eof &&
													(mark + bits_/8 <= eof || !pview->OverType()));
		GetDlgItem(IDC_SEL_STORE)->EnableWindow(pview != NULL && (current_&mask_) <= (unsigned __int64)eof);
		GetDlgItem(IDC_SEL_AT_STORE)->EnableWindow(pview != NULL && !pview->ReadOnly() && start <= eof &&
												(start + bits_/8 <= eof || !pview->OverType()));
		GetDlgItem(IDC_SEL_LEN_STORE)->EnableWindow(pview != NULL && start + (current_&mask_) <= (unsigned __int64)eof);
		GetDlgItem(IDC_GO)->EnableWindow(pview != NULL && (current_&mask_) <= (unsigned __int64)eof);
		GetDlgItem(IDC_UNARY_AT)->EnableWindow(pview != NULL && (current_&mask_) + bits_/8 <= (unsigned __int64)eof);
	}

	if (pview != NULL)
	{
		GetDlgItem(IDC_BIG_ENDIAN_FILE_ACCESS)->EnableWindow(TRUE);  // enable so we can change check
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
	if (CHexDialogBar::OnCreate(lpCreateStruct) == -1)
		return -1;

    return 0;
}

void CCalcDlg::OnDestroy() 
{
    CHexDialogBar::OnDestroy();

    // Save some settings to the in file/registry
    aa_->WriteProfileInt("Calculator", "Base", radix_);
    aa_->WriteProfileInt("Calculator", "Bits", bits_);

    char buf[30];
    _i64toa(memory_, buf, 10);
    aa_->WriteProfileString("Calculator", "Memory", buf);
	if (current_type_ != CJumpExpr::TYPE_NONE)
		calc_previous();
    _i64toa((current_&mask_), buf, 10);
    aa_->WriteProfileString("Calculator", "Current", buf);
}

void CCalcDlg::OnSize(UINT nType, int cx, int cy)   // WM_SIZE
{
    if (cy < m_sizeInitial.cy*7/8)
    {
        SetDlgItemText(IDC_BASE_GROUP, "");
        SetDlgItemText(IDC_BITS_GROUP, "");
    }
    else
    {
        SetDlgItemText(IDC_BASE_GROUP, "Base");
        SetDlgItemText(IDC_BITS_GROUP, "Bits");
    }

	if (cx > 0 && m_sizeInitial.cx == -1)
		m_sizeInitial = CSize(cx, cy);
	CHexDialogBar::OnSize(nType, cx, cy);
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

LRESULT CCalcDlg::OnKickIdle(WPARAM, LPARAM)
{
#ifdef CALCULATOR_IMPROVEMENTS
    build_menus();
#endif
	// Display context help for ctrl set up in OnHelpInfo
	if (help_hwnd_ != (HWND)0)
	{
		theApp.HtmlHelpWmHelp(help_hwnd_, id_pairs);
		help_hwnd_ = (HWND)0;
	}
	return FALSE;
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
#ifndef CALC_EXPR  // These operators can now be entered in the text box directly
        static const char *bo_char = "+-*/%&|^";
        static binop_type bo_op[] =
        {
            binop_add,
            binop_subtract,
            binop_multiply,
            binop_divide,
            binop_mod,
            binop_and,
            binop_or,
            binop_xor,
        };
        const char *pbo;                    // Pointer into bo_char string

        if ((pbo = strchr(bo_char, pMsg->wParam)) != NULL)
        {
            // Binary operator character typed
            ASSERT(pbo - bo_char < sizeof(bo_op)/sizeof(*bo_op)); // Bounds check
            do_binop(bo_op[pbo - bo_char]);
            return TRUE;
        }
        else if (pMsg->wParam == '!')
        {
            do_unary(unary_factorial);
            return TRUE;
        }
        else if (pMsg->wParam == '~')
        {
            do_unary(unary_not);
            return TRUE;
        }
        else if (pMsg->wParam == '=')
        {
            OnEquals();
            return TRUE;
        }
        else
#endif
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

    return CHexDialogBar::PreTranslateMessage(pMsg);
}

#define IDC_FIRST_BLUE   IDC_DIGIT_0
#define IDC_FIRST_RED    IDC_BACKSPACE
#define IDC_FIRST_BLACK  IDC_MARK_STORE
#define IDC_FIRST_PURPLE IDC_POW

void CCalcDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
#ifndef CALCULATOR_IMPROVEMENTS
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

//    CBrush *pBrush = new CBrush(GetSysColor(COLOR_BTNFACE));
//    pDC->FillRect(&rcButton, pBrush);
//    delete pBrush;
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

    TRACE3("Draw button %ld, action %d, state %d\n",
           long(nIDCtl), 
           lpDrawItemStruct->itemAction, 
           lpDrawItemStruct->itemState);

    rcButton = lpDrawItemStruct->rcItem;
    ::Draw3DButtonFrame(pDC, rcButton,
                        (lpDrawItemStruct->itemState & ODS_FOCUS) != 0);

    // Restore text colour
    ::SetTextColor(lpDrawItemStruct->hDC, cr);
#endif
    CHexDialogBar::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

// This actually goes to a calculated address.  Unlike the "Store Cursor" button,
// the result is calculated (saves pressing "=" button) and the view is given focus
// after the current cursor position has been moved.
void CCalcDlg::OnGo()                   // Move cursor to current value
{
	edit_.update_value(true);           // eval the epsression allowing side-effects now

	if (current_type_ != CJumpExpr::TYPE_INT)
	{
		(void)invalid_expression();
		return;
	}

    add_hist();

    unsigned __int64 temp = current_;
    calc_previous();

    // If the value in current_ is not there as a result of a calculation then
    // save it before it is lost.
    if (source_ != km_result)
        aa_->SaveToMacro(source_, temp);

    op_ = binop_none;

    if (!aa_->refresh_off_ && IsVisible())
    {
        edit_.Put();
        FixFileButtons();
        if (!overflow_ && !error_)
            ShowBinop(binop_none);
    }
    in_edit_ = FALSE;
    //source_ = aa_->recording_ ? km_user : km_result;
    source_ = km_result;

    // Make sure we can go to the address
    CHexEditView *pview = GetView();
    if (pview == NULL || (current_&mask_) > (unsigned __int64)pview->GetDocument()->length())
    {
        mm_->StatusBarText("Error: new cursor address past end of file");
        aa_->mac_error_ = 10;
        return;
    }

	CString ss;
	edit_.GetWindowText(ss);
	if (radix_ == 16)
		ss = "Go To (hex) " + ss;
	else
		ss = "Go To " + ss;

    pview->MoveWithDesc(ss, current_);

    // Give view the focus
    if (pview != pview->GetFocus())
        pview->SetFocus();

    aa_->SaveToMacro(km_go);
}

void CCalcDlg::OnBigEndian() 
{
	toggle_endian();
    aa_->SaveToMacro(km_big_endian, (__int64)-1);
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

void CCalcDlg::OnBinary() 
{
    change_base(2);
}

void CCalcDlg::OnOctal() 
{
    change_base(8);
}

void CCalcDlg::OnDecimal() 
{
    change_base(10);
}

void CCalcDlg::OnHex() 
{
    change_base(16);
}

void CCalcDlg::OnBackspace()            // Delete back one digit
{
    ASSERT(IsVisible());
    edit_.SetFocus();

    edit_.SendMessage(WM_CHAR, '\b', 1);

    ShowBinop();
    in_edit_ = TRUE;                    // Allow edit of current value
    source_ = aa_->recording_ ? km_user : km_result;
}

void CCalcDlg::OnClearEntry()           // Zero current value
{
    current_ = 0;
	current_const_ = TRUE;
	current_type_ = CJumpExpr::TYPE_INT;

    if (!aa_->refresh_off_ && IsVisible())
    {
        edit_.SetFocus();
        edit_.Put();
        FixFileButtons();
    }

    ShowBinop();
    in_edit_ = FALSE;                   // Necessary?
    source_ = aa_->recording_ ? km_user : km_result;
    aa_->SaveToMacro(km_clear_entry);
}

void CCalcDlg::OnClear()                // Zero current and remove any operators/brackets
{
    op_ = binop_none;
    current_ = 0;
	current_const_ = TRUE;
	current_type_ = CJumpExpr::TYPE_INT;

    if (!aa_->refresh_off_ && IsVisible())
    {
        edit_.SetFocus();
        edit_.Put();
        FixFileButtons();
        ShowBinop(binop_none);
    }

    in_edit_ = FALSE;
    source_ = aa_->recording_ ? km_user : km_result;
    aa_->SaveToMacro(km_clear);
}

void CCalcDlg::OnEquals()               // Calculate result
{
	edit_.update_value(true);           // eval the epsression allowing side-effects now

	if (current_type_ == CJumpExpr::TYPE_NONE)
	{
		(void)invalid_expression();
		return;
	}

    add_hist();

	if (current_type_ != CJumpExpr::TYPE_INT)
	{
#ifdef UNICODE_TYPE_STRING
		// This is the way to put Unicode text into control with ANSI window procedure
		::CallWindowProcW(*edit_.GetSuperWndProcAddr(), edit_.m_hWnd, WM_SETTEXT, 0, (LPARAM)(LPCWSTR)current_str_);
#else
		edit_.SetWindowText(current_str_);
#endif
#ifdef CALCULATOR_IMPROVEMENTS
        switch (current_type_)
        {
        case CJumpExpr::TYPE_REAL:
            SetDlgItemText(IDC_OP_DISPLAY, "R");
            break;
        case CJumpExpr::TYPE_STRING:
            SetDlgItemText(IDC_OP_DISPLAY, "S");
            break;
        case CJumpExpr::TYPE_BOOLEAN:
            SetDlgItemText(IDC_OP_DISPLAY, "B");
            break;
        case CJumpExpr::TYPE_DATE:
            SetDlgItemText(IDC_OP_DISPLAY, "D");
            break;
        }
        in_edit_ = FALSE;
#endif
        edit_.SetFocus();
		return;
	}

    unsigned __int64 temp = current_;
    calc_previous();

    // If the value in current_ is not there as a result of a calculation then
    // save it before it is lost.
    if (source_ != km_result)
        aa_->SaveToMacro(source_, temp);

    op_ = binop_none;
    if (!aa_->refresh_off_ && IsVisible())
    {
        edit_.Put();
        FixFileButtons();
        if (!overflow_ && !error_)
            ShowBinop(binop_none);
    }

    in_edit_ = FALSE;
    edit_.SetFocus();
    //source_ = aa_->recording_ ? km_user : km_result;
    source_ = km_result;
    aa_->SaveToMacro(km_equals);
}

#ifdef CALCULATOR_IMPROVEMENTS
void CCalcDlg::add_hist()
{
    // We don't add to the history if there is an active calculator button, but
    // only if there is an expression in the calculator text box to be evaluated.
    if (op_ == binop_none)
    {
        // Get the current value or expression
        CString toadd;
        edit_.GetWindowText(toadd);

        for (int ii = 0; ii < ctl_edit_combo_.GetCount(); ++ii)
        {
            CString ss;
            ctl_edit_combo_.GetLBText(ii, ss);
            if (ss == toadd)                            // should we compare ignoring case?
            {
                ctl_edit_combo_.DeleteString(ii);       // remove existing entry with same text
                break;
            }
        }
        ctl_edit_combo_.InsertString(0, toadd);
    }
}

// ---------- Menu button handlers ---------
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

void CCalcDlg::OnGetHexHist()
{
    if (ctl_hex_hist_.m_nMenuResult != 0)
	{
        // Get the text of the menu item selected
		CMenu menu;
		menu.Attach(ctl_hex_hist_.m_hMenu);
        CString ss = get_menu_text(&menu, ctl_hex_hist_.m_nMenuResult);
		menu.Detach();

        // If just calculated a result then clear it
		if (!in_edit_)
			edit_.SetWindowText("");
	    in_edit_ = FALSE;

        if (radix_ != 16)
        {
            // We need to convert the hex digits to digits in the current radix
            __int64 ii = ::_strtoui64(ss, NULL, 16);
            char buf[72];
            ::_i64toa(ii, buf, radix_);
            ss = buf;
        }

        for (int ii = 0; ii < ss.GetLength (); ii++)
            edit_.SendMessage(WM_CHAR, (TCHAR)ss[ii]);
        edit_.SetFocus();

        SetDlgItemText(IDC_OP_DISPLAY, "");
		in_edit_ = TRUE;
		source_ = aa_->recording_ ? km_user : km_result;
	}
}

void CCalcDlg::OnGetDecHist()
{
    if (ctl_dec_hist_.m_nMenuResult != 0)
	{
		CMenu menu;
		menu.Attach(ctl_dec_hist_.m_hMenu);
        CString ss = get_menu_text(&menu, ctl_dec_hist_.m_nMenuResult);
		menu.Detach();
		if (!in_edit_)
			edit_.SetWindowText("");
	    in_edit_ = FALSE;

        if (radix_ != 10)
        {
            // We need to convert decimal to the current radix
            __int64 ii = ::_strtoi64(ss, NULL, 10);
            char buf[72];
            ::_i64toa(ii, buf, radix_);
            ss = buf;
        }

        for (int ii = 0; ii < ss.GetLength (); ii++)
            edit_.SendMessage(WM_CHAR, (TCHAR)ss[ii]);
        edit_.SetFocus();

        SetDlgItemText(IDC_OP_DISPLAY, "");
		in_edit_ = TRUE;
		source_ = aa_->recording_ ? km_user : km_result;
	}
}

void CCalcDlg::OnGetVar()
{
    if (ctl_vars_.m_nMenuResult == ID_VARS_CLEAR)
    {
        if (AfxMessageBox("Are you sure you want to delete all variables?", MB_YESNO|MB_ICONSTOP) == IDYES)
            mm_->expr_.DeleteVars();
    }
    else if (ctl_vars_.m_nMenuResult != 0)
	{
		CMenu menu;
		menu.Attach(ctl_vars_.m_hMenu);
        CString ss = get_menu_text(&menu, ctl_vars_.m_nMenuResult);
		menu.Detach();
		if (!in_edit_)
			edit_.SetWindowText("");
	    in_edit_ = FALSE;

        if (!ss.IsEmpty() && isalpha(ss[0]) && toupper(ss[0]) - 'A' + 10 < radix_)
            edit_.SendMessage(WM_CHAR, (TCHAR)'@');     // Prefix ID with @ so it's not treated as an integer literal
        for (int ii = 0; ii < ss.GetLength (); ii++)
            edit_.SendMessage(WM_CHAR, (TCHAR)ss[ii]);
        edit_.SetFocus();

        SetDlgItemText(IDC_OP_DISPLAY, "");
		in_edit_ = TRUE;
		source_ = aa_->recording_ ? km_user : km_result;
	}
}

void CCalcDlg::OnGetFunc()
{
    if (ctl_func_.m_nMenuResult != 0)
	{
		CMenu menu;
		menu.Attach(ctl_func_.m_hMenu);
        CString ss = get_menu_text(&menu, ctl_func_.m_nMenuResult);
        ss.Replace("&&", "&");  // Double-ampersand is needed in menus to show one &, but now we need to reverse that
		menu.Detach();
		if (!in_edit_)
			edit_.SetWindowText("");
	    in_edit_ = FALSE;

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
        edit_.SetFocus();
		edit_.SetSel(start, end);

        SetDlgItemText(IDC_OP_DISPLAY, "");
		in_edit_ = TRUE;
		source_ = aa_->recording_ ? km_user : km_result;
	}
}
#endif

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
	current_const_ = TRUE;
	current_type_ = CJumpExpr::TYPE_INT;

    overflow_ = error_ = FALSE;
    if ((current_ & ~mask_) != 0)
    {
        overflow_ = TRUE;
        mm_->StatusBarText("Memory overflowed data size");
        aa_->mac_error_ = 2;
    }
    ShowStatus();                       // Clear/set overflow indicator

    if (!aa_->refresh_off_ && IsVisible())
    {
        edit_.SetFocus();
        edit_.Put();
        FixFileButtons();
    }

    in_edit_ = TRUE;                    // The user can edit the value edit control
    source_ = aa_->recording_ ? km_memget : km_result;
}

void CCalcDlg::OnMemStore() 
{
	if (invalid_expression())
		return;

    memory_ = current_&mask_;
    if (!aa_->refresh_off_ && IsVisible())
    {
        edit_.SetFocus();
        edit_.Put();
        FixFileButtons();
        GetDlgItem(IDC_MEM_GET)->EnableWindow(TRUE);

		// xxx This works great but we have to also update tooltip after MC, M+, M- buttons
		// which means we can't just use the current text from the edit box.
		// Plus we should also display using the current radix and bits AND update after macros with refresh off.
		//// Update tooltip
		//CString ss;
		//edit_.GetWindowText(ss);
		//ttc_.UpdateTipText("Memory [" + ss + "] (Ctrl+R)", GetDlgItem(IDC_MEM_GET));
    }

    if (source_ != km_result)
        aa_->SaveToMacro(source_, current_&mask_);
    source_ = km_result;
    aa_->SaveToMacro(km_memstore);

}

void CCalcDlg::OnMemClear() 
{
    memory_ = 0;
    if (!aa_->refresh_off_ && IsVisible())
    {
        edit_.SetFocus();
        GetDlgItem(IDC_MEM_GET)->EnableWindow(FALSE);
    }

    aa_->SaveToMacro(km_memclear);
}

void CCalcDlg::OnMemAdd() 
{
	if (invalid_expression())
		return;

    memory_ += current_&mask_;
    if (!aa_->refresh_off_ && IsVisible())
    {
        edit_.SetFocus();
        edit_.Put();
        FixFileButtons();
        GetDlgItem(IDC_MEM_GET)->EnableWindow(TRUE);
    }

    if (source_ != km_result)
        aa_->SaveToMacro(source_, current_&mask_);
    source_ = km_result;
    aa_->SaveToMacro(km_memadd);
}

void CCalcDlg::OnMemSubtract() 
{
	if (invalid_expression())
		return;

    memory_ -= current_&mask_;
    if (!aa_->refresh_off_ && IsVisible())
    {
        edit_.SetFocus();
        edit_.Put();
        FixFileButtons();
        GetDlgItem(IDC_MEM_GET)->EnableWindow(TRUE);
    }

    if (source_ != km_result)
        aa_->SaveToMacro(source_, current_&mask_);
    source_ = km_result;
    aa_->SaveToMacro(km_memsubtract);
}

// ----------- Mark functions --------------
void CCalcDlg::OnMarkGet()              // Position of mark in the file
{
    CHexEditView *pview = GetView();
    if (pview == NULL)
    {
        mm_->StatusBarText("No file open");
        aa_->mac_error_ = 10;
        return;
    }

    current_ = pview->GetMark();
	current_const_ = TRUE;
	current_type_ = CJumpExpr::TYPE_INT;

    overflow_ = error_ = FALSE;
    if ((current_ & ~mask_) != 0)
    {
        overflow_ = TRUE;
        mm_->StatusBarText("Mark overflowed data size");
        aa_->mac_error_ = 2;
    }
    ShowStatus();                       // Clear/set overflow indicator

    if (!aa_->refresh_off_ && IsVisible())
    {
        edit_.SetFocus();
        edit_.Put();
        FixFileButtons();
    }

    in_edit_ = TRUE;                    // The user can edit the new value in edit control
    source_ = aa_->recording_ ? km_markget : km_result;
}

void CCalcDlg::OnMarkStore() 
{
	if (invalid_expression())
		return;

    CHexEditView *pview = GetView();
    if (pview == NULL || (current_&mask_) > (unsigned __int64)pview->GetDocument()->length())
    {
        mm_->StatusBarText("New mark address past end of file");
        aa_->mac_error_ = 10;
        return;
    }

    pview->SetMark(current_&mask_);

    if (!aa_->refresh_off_ && IsVisible())
    {
        edit_.SetFocus();
        edit_.Put();
        FixFileButtons();
    }

    if (source_ != km_result)
        aa_->SaveToMacro(source_, current_&mask_);
    source_ = km_result;
    aa_->SaveToMacro(km_markstore);
}

void CCalcDlg::OnMarkClear() 
{
    CHexEditView *pview = GetView();
    if (pview == NULL)
    {
        aa_->mac_error_ = 10;
        return;
    }

    pview->SetMark(0);
    if (!aa_->refresh_off_ && IsVisible())
    {
        edit_.SetFocus();
        FixFileButtons();
    }

    aa_->SaveToMacro(km_markclear);
}

void CCalcDlg::OnMarkAdd()              // Add current value to mark
{
	if (invalid_expression())
		return;

    CHexEditView *pview = GetView();
    if (pview == NULL)
    {
        aa_->mac_error_ = 10;
        return;
    }
    FILE_ADDRESS eof = pview->GetDocument()->length();
    FILE_ADDRESS mark = pview->GetMark();

    if (radix_ == 10)
    {
        signed __int64 new_mark;
        switch (bits_)
        {
        case 8:
            new_mark = mark + (signed char)current_;
            break;
        case 16:
            new_mark = mark + (signed short)current_;
            break;
        case 32:
            new_mark = mark + (signed long)current_;
            break;
        case 64:
            new_mark = mark + (signed __int64)current_;
            break;
        }
        if (new_mark < 0 || new_mark > eof)
        {
            if (new_mark < 0)
                mm_->StatusBarText("New mark address is -ve");
            else
                mm_->StatusBarText("New mark address past end of file");
            aa_->mac_error_ = 10;
            return;
        }
        pview->SetMark(new_mark);
    }
    else
    {
        if (mark + (current_&mask_) > (unsigned __int64)eof)
        {
            mm_->StatusBarText("New mark address past end of file");
            aa_->mac_error_ = 10;
            return;
        }
        pview->SetMark(mark + (current_&mask_));
    }

    if (!aa_->refresh_off_ && IsVisible())
    {
        edit_.SetFocus();
        edit_.Put();
        FixFileButtons();
    }

    if (source_ != km_result)
        aa_->SaveToMacro(source_, current_&mask_);
    source_ = km_result;
    aa_->SaveToMacro(km_markadd);
}

void CCalcDlg::OnMarkSubtract() 
{
	if (invalid_expression())
		return;

    CHexEditView *pview = GetView();
    if (pview == NULL)
    {
        aa_->mac_error_ = 10;
        return;
    }
    FILE_ADDRESS eof = pview->GetDocument()->length();
    FILE_ADDRESS mark = pview->GetMark();

    if (radix_ == 10)
    {
        signed __int64 new_mark;
        switch (bits_)
        {
        case 8:
            new_mark = mark - (signed char)current_;
            break;
        case 16:
            new_mark = mark - (signed short)current_;
            break;
        case 32:
            new_mark = mark - (signed long)current_;
            break;
        case 64:
            new_mark = mark - (signed __int64)current_;
            break;
        }
        if (new_mark < 0 || new_mark > eof)
        {
            if (new_mark < 0)
                mm_->StatusBarText("New mark address is -ve");
            else
                mm_->StatusBarText("New mark address past end of file");
            aa_->mac_error_ = 10;
            return;
        }
        pview->SetMark(new_mark);
    }
    else
    {
        if (mark - (current_&mask_) > (unsigned __int64)eof)
        {
            mm_->StatusBarText("New mark address past end of file");
            aa_->mac_error_ = 10;
            return;
        }
        pview->SetMark(mark - (current_&mask_));
    }
    if (!aa_->refresh_off_ && IsVisible())
    {
        edit_.SetFocus();
        edit_.Put();
        FixFileButtons();
    }

    if (source_ != km_result)
        aa_->SaveToMacro(source_, current_&mask_);
    source_ = km_result;
    aa_->SaveToMacro(km_marksubtract);
}

// ----------- Other file get funcs -------------
void CCalcDlg::OnMarkAt()               // Get value from file at mark
{
    CHexEditView *pview = GetView();
	// Make sure view and calculator endianness are in sync
	ASSERT(pview == NULL || pview->BigEndian() == ((CButton*)GetDlgItem(IDC_BIG_ENDIAN_FILE_ACCESS))->GetCheck());
    if (pview == NULL)
    {
        aa_->mac_error_ = 10;
        return;
    }
    FILE_ADDRESS eof = pview->GetDocument()->length();
    FILE_ADDRESS mark = pview->GetMark();
    if (mark + bits_/8 > eof)
    {
        mm_->StatusBarText("Mark address too close to end of file to get data");
        aa_->mac_error_ = 10;
        return;
    }

    __int64 temp = 0;
    pview->GetDocument()->GetData((unsigned char *)&temp, bits_/8, mark);
    if (pview->BigEndian())
    {
        // Reverse the byte order to match that used internally (Intel=little-endian)
        unsigned char cc, *byte = (unsigned char *)&temp;
        switch (bits_)
        {
        case 8:
            /* nothing */
            break;
        case 16:
            cc = byte[0]; byte[0] = byte[1]; byte[1] = cc;
            break;
        case 32:
            cc = byte[0]; byte[0] = byte[3]; byte[3] = cc;
            cc = byte[1]; byte[1] = byte[2]; byte[2] = cc;
            break;
        case 64:
            cc = byte[0]; byte[0] = byte[7]; byte[7] = cc;
            cc = byte[1]; byte[1] = byte[6]; byte[6] = cc;
            cc = byte[2]; byte[2] = byte[5]; byte[5] = cc;
            cc = byte[3]; byte[3] = byte[4]; byte[4] = cc;
            break;
        default:
            ASSERT(0);
        }
    }
    current_ = temp;
	current_const_ = TRUE;
	current_type_ = CJumpExpr::TYPE_INT;

    if (!aa_->refresh_off_ && IsVisible())
    {
        edit_.SetFocus();
        edit_.Put();
        FixFileButtons();
    }
    
    in_edit_ = TRUE;                    // The user can edit the new value in edit control
    source_ = aa_->recording_ ? km_markat : km_result;
}

void CCalcDlg::OnSelGet()              // Position of cursor (or start of selection)
{
    CHexEditView *pview = GetView();
    if (pview == NULL)
    {
        aa_->mac_error_ = 10;
        return;
    }
    FILE_ADDRESS start, end;
    pview->GetSelAddr(start, end);

    current_ = start;
	current_const_ = TRUE;
	current_type_ = CJumpExpr::TYPE_INT;

    overflow_ = error_ = FALSE;
    if ((current_ & ~mask_) != 0)
    {
        overflow_ = TRUE;
        mm_->StatusBarText("Cursor position overflowed data size");
        aa_->mac_error_ = 2;
    }
    ShowStatus();                       // Clear/set overflow indicator

    if (!aa_->refresh_off_ && IsVisible())
    {
        edit_.SetFocus();
        edit_.Put();
        FixFileButtons();
    }

    in_edit_ = TRUE;                    // The user can edit the new value in edit control
    source_ = aa_->recording_ ? km_selget : km_result;
}

void CCalcDlg::OnSelAt()                // Value in file at cursor
{
    CHexEditView *pview = GetView();
	// Make sure view and calculator endianness are in sync
	ASSERT(pview == NULL || pview->BigEndian() == ((CButton*)GetDlgItem(IDC_BIG_ENDIAN_FILE_ACCESS))->GetCheck());
    if (pview == NULL)
    {
        aa_->mac_error_ = 10;
        return;
    }

    FILE_ADDRESS eof = pview->GetDocument()->length();
    FILE_ADDRESS start, end;
    pview->GetSelAddr(start, end);

    if (start + bits_/8 > eof)
    {
        mm_->StatusBarText("Cursor too close to end of file to get data");
        aa_->mac_error_ = 10;
        return;
    }

    __int64 temp = 0;
    pview->GetDocument()->GetData((unsigned char *)&temp, bits_/8, start);
    if (pview->BigEndian())
    {
        // Reverse the byte order to match that used internally (Intel=little-endian)
        unsigned char cc, *byte = (unsigned char *)&temp;
        switch (bits_)
        {
        case 8:
            /* nothing */
            break;
        case 16:
            cc = byte[0]; byte[0] = byte[1]; byte[1] = cc;
            break;
        case 32:
            cc = byte[0]; byte[0] = byte[3]; byte[3] = cc;
            cc = byte[1]; byte[1] = byte[2]; byte[2] = cc;
            break;
        case 64:
            cc = byte[0]; byte[0] = byte[7]; byte[7] = cc;
            cc = byte[1]; byte[1] = byte[6]; byte[6] = cc;
            cc = byte[2]; byte[2] = byte[5]; byte[5] = cc;
            cc = byte[3]; byte[3] = byte[4]; byte[4] = cc;
            break;
        default:
            ASSERT(0);
        }
    }
    current_ = temp;
	current_const_ = TRUE;
	current_type_ = CJumpExpr::TYPE_INT;

    if (!aa_->refresh_off_ && IsVisible())
    {
        edit_.SetFocus();
        edit_.Put();
        FixFileButtons();
    }

    in_edit_ = TRUE;                    // The user can edit the new value in edit control
    source_ = aa_->recording_ ? km_selat : km_result;
}

void CCalcDlg::OnSelLen() 
{
    CHexEditView *pview = GetView();
    if (pview == NULL)
    {
        aa_->mac_error_ = 10;
        return;
    }

    FILE_ADDRESS start, end;
    pview->GetSelAddr(start, end);

    current_ = end - start;
	current_const_ = TRUE;
	current_type_ = CJumpExpr::TYPE_INT;

    if (!aa_->refresh_off_ && IsVisible())
    {
        edit_.SetFocus();
        edit_.Put();
        FixFileButtons();
    }

    in_edit_ = TRUE;                    // The user can edit the new value in edit control
    source_ = aa_->recording_ ? km_sellen : km_result;
}

void CCalcDlg::OnEofGet()               // Length of file
{
    CHexEditView *pview = GetView();
    if (pview == NULL)
    {
        aa_->mac_error_ = 10;
        return;
    }

    current_ = pview->GetDocument()->length();
	current_const_ = TRUE;
	current_type_ = CJumpExpr::TYPE_INT;

    if (!aa_->refresh_off_ && IsVisible())
    {
        edit_.SetFocus();
        edit_.Put();
        FixFileButtons();
    }

    in_edit_ = TRUE;                    // The user can edit the new value in edit control
    source_ = aa_->recording_ ? km_eofget : km_result;
}

// ----------- File change funcs -------------

void CCalcDlg::OnMarkAtStore() 
{
	if (invalid_expression())
		return;

    CHexEditView *pview = GetView();
	// Make sure view and calculator endianness are in sync
	ASSERT(pview == NULL || pview->BigEndian() == ((CButton*)GetDlgItem(IDC_BIG_ENDIAN_FILE_ACCESS))->GetCheck());
    if (pview == NULL || pview->ReadOnly())
    {
        mm_->StatusBarText("Can't write at mark: file is read only");
        aa_->mac_error_ = 10;
        return;
    }

    FILE_ADDRESS eof = pview->GetDocument()->length();
    FILE_ADDRESS mark = pview->GetMark();
    if (mark > eof || (mark + bits_/8 > eof && pview->OverType()))
    {
        mm_->StatusBarText("Can't write at mark: too close to EOF (OVR mode) or past EOF");
        aa_->mac_error_ = 10;
        return;
    }

    __int64 temp = (current_&mask_);
    if (pview->BigEndian())
    {
        // Reverse the byte order to match that used internally (Intel=little-endian)
        unsigned char cc, *byte = (unsigned char *)&temp;
        switch (bits_)
        {
        case 8:
            /* nothing */
            break;
        case 16:
            cc = byte[0]; byte[0] = byte[1]; byte[1] = cc;
            break;
        case 32:
            cc = byte[0]; byte[0] = byte[3]; byte[3] = cc;
            cc = byte[1]; byte[1] = byte[2]; byte[2] = cc;
            break;
        case 64:
            cc = byte[0]; byte[0] = byte[7]; byte[7] = cc;
            cc = byte[1]; byte[1] = byte[6]; byte[6] = cc;
            cc = byte[2]; byte[2] = byte[5]; byte[5] = cc;
            cc = byte[3]; byte[3] = byte[4]; byte[4] = cc;
            break;
        default:
            ASSERT(0);
        }
    }
    pview->GetDocument()->Change(pview->OverType() ? mod_replace : mod_insert,
                                mark, bits_/8, (unsigned char *)&temp, 0, pview);

    if (!pview->OverType())
        pview->SetMark(mark);   // Inserting at mark would have move it forward

    if (!aa_->refresh_off_ && IsVisible())
    {
        edit_.SetFocus();
        edit_.Put();
        FixFileButtons();
    }

    if (source_ != km_result)
        aa_->SaveToMacro(source_, current_&mask_);
    source_ = km_result;
    aa_->SaveToMacro(km_markatstore);
}

void CCalcDlg::OnSelStore() 
{
	if (invalid_expression())
		return;

    CHexEditView *pview = GetView();
    if (pview == NULL || (current_&mask_) > (unsigned __int64)pview->GetDocument()->length())
    {
        mm_->StatusBarText("Can't move cursor past end of file");
        aa_->mac_error_ = 10;
        return;
    }
    pview->MoveWithDesc("Set Cursor in Calculator", current_&mask_);
    if (!aa_->refresh_off_ && IsVisible())
    {
        edit_.SetFocus();
        edit_.Put();
        // FixFileButtons(); // done indirectly through MoveToAddress
    }

    if (source_ != km_result)
        aa_->SaveToMacro(source_, current_&mask_);
    source_ = km_result;
    aa_->SaveToMacro(km_selstore);
}

void CCalcDlg::OnSelAtStore() 
{
	if (invalid_expression())
		return;

    CHexEditView *pview = GetView();
	// Make sure view and calculator endianness are in sync
	ASSERT(pview == NULL || pview->BigEndian() == ((CButton*)GetDlgItem(IDC_BIG_ENDIAN_FILE_ACCESS))->GetCheck());
    if (pview == NULL || pview->ReadOnly())
    {
        mm_->StatusBarText("Can't store at cursor: file is read-only");
        aa_->mac_error_ = 10;
        return;
    }

    FILE_ADDRESS eof = pview->GetDocument()->length();
    FILE_ADDRESS start, end;
    pview->GetSelAddr(start, end);

    if (start > eof || (start + bits_/8 > eof && pview->OverType()))
    {
        mm_->StatusBarText("Can't store at cursor: too close to EOF (OVR mode) or past EOF");
        aa_->mac_error_ = 10;
        return;
    }

    __int64 temp = (current_&mask_);
    if (pview->BigEndian())
    {
        // Reverse the byte order to match that used internally (Intel=little-endian)
        unsigned char cc, *byte = (unsigned char *)&temp;
        switch (bits_)
        {
        case 8:
            /* nothing */
            break;
        case 16:
            cc = byte[0]; byte[0] = byte[1]; byte[1] = cc;
            break;
        case 32:
            cc = byte[0]; byte[0] = byte[3]; byte[3] = cc;
            cc = byte[1]; byte[1] = byte[2]; byte[2] = cc;
            break;
        case 64:
            cc = byte[0]; byte[0] = byte[7]; byte[7] = cc;
            cc = byte[1]; byte[1] = byte[6]; byte[6] = cc;
            cc = byte[2]; byte[2] = byte[5]; byte[5] = cc;
            cc = byte[3]; byte[3] = byte[4]; byte[4] = cc;
            break;
        default:
            ASSERT(0);
        }
    }
    pview->GetDocument()->Change(pview->OverType() ? mod_replace : mod_insert,
                                start, bits_/8, (unsigned char *)&temp, 0, pview);
    if (!aa_->refresh_off_ && IsVisible())
    {
        edit_.SetFocus();
        edit_.Put();
        FixFileButtons();
    }

    if (source_ != km_result)
        aa_->SaveToMacro(source_, current_&mask_);
    source_ = km_result;
    aa_->SaveToMacro(km_selatstore);
}

void CCalcDlg::OnSelLenStore() 
{
	if (invalid_expression())
		return;

    CHexEditView *pview = GetView();
    if (pview == NULL)
    {
        aa_->mac_error_ = 10;
        return;
    }

    FILE_ADDRESS eof = pview->GetDocument()->length();
    FILE_ADDRESS start, end;
    pview->GetSelAddr(start, end);
    if (start + (current_ & mask_) > (unsigned __int64)eof)
    {
        mm_->StatusBarText("New selection length would be past EOF");
        aa_->mac_error_ = 10;
        return;
    }

    pview->MoveToAddress(start, start + current_);
    if (!aa_->refresh_off_ && IsVisible())
    {
        edit_.SetFocus();
        edit_.Put();
        // FixFileButtons(); // done indirectly through MoveToAddress
    }

    if (source_ != km_result)
        aa_->SaveToMacro(source_, current_&mask_);
    source_ = km_result;
    aa_->SaveToMacro(km_sellenstore);
}
