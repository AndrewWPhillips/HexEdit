// CalcEdit.cpp : implements the subclassed edit control for the calculator
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
#include <locale.h>

#include "HexEdit.h"
#include "MainFrm.h"
#include "CalcEdit.h"
#include "CalcDlg.h"
#include "SystemSound.h"
#include "HexEditDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// NOTES
// 1. The current value in the edit control is continually calculated & stored 
// in "current_" in the CCalcDlg class (ie, via pp_).  This allows overflow to
// be detected as soon as the user types a digit. So for example, if bits is 8
// and radix is 10 and the user has already typed "25" then typing "6" will
// cause a beep due to overflow, but typing "5" will not.
//
// 2. An overflow sets overflow_ in the CCalcDlg class (ie, via pp_).
//
// 3. With the ability to evaluate expressions in the edit control the type of 
// the contents of the edit control are stored in current_type_ in CalcDlg. If
// the value is a simple integer constant then current_const_ is TRUE.
// If the contents of the edit box contains something more complex (not an int)
// then current_const_ is FALSE and current_type_ contains the type of the
// result of the expression (which is TYPE_INT for a simple integer).
// If it is not a valid expression current_type_ contains TYPE_NONE.
// Note that for invlaid expressions no error is indicated (beep, message etc)
// since the user has not finished it, but the error is stored in
// current_str_ so it can be displayed if the user tries to use it.

/////////////////////////////////////////////////////////////////////////////
// CCalcEdit

CCalcEdit::CCalcEdit()
{
    pp_ = NULL;

    // Work out how to display decimal numbers in this culture
    struct lconv *plconv = localeconv();
    if (strlen(plconv->thousands_sep) == 1)
    {
        dec_sep_char_ = *plconv->thousands_sep;
        dec_group_ = *plconv->grouping;
    }
    else
    {
        dec_sep_char_ = ',';
        dec_group_ = 3;
    }
}

CCalcEdit::~CCalcEdit()
{
}


BEGIN_MESSAGE_MAP(CCalcEdit, CEdit)
	//{{AFX_MSG_MAP(CCalcEdit)
	ON_WM_CHAR()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// Put current calculator value (pp_->current_) into edit control nicely formatted
void CCalcEdit::Put()
{
    ASSERT(pp_ != NULL);
    ASSERT(pp_->radix_ > 1 && pp_->radix_ <= 36);
    ASSERT(pp_->IsVisible());

    char buf[72];

    if (pp_->radix_ == 10)
    {
        signed char s8;
        signed short s16;
        signed long s32;

        switch (pp_->bits_)
        {
        case 8:
            s8 = (signed char)pp_->current_;
            itoa((int)s8, buf, pp_->radix_);
            break;
        case 16:
            s16 = (signed short)pp_->current_;
            itoa((int)s16, buf, pp_->radix_);
            break;
        case 32:
            s32 = (signed long)pp_->current_;
            _i64toa((__int64)s32, buf, pp_->radix_);
            break;
        case 64:
            _i64toa(pp_->current_, buf, pp_->radix_);
            break;
        default:
            ASSERT(0);
        }
    }
    else
        _ui64toa(pp_->current_ & pp_->mask_, buf, pp_->radix_);
	pp_->current_const_ = TRUE;
	pp_->current_type_ = CJumpExpr::TYPE_INT;

    SetWindowText(buf);
    SetSel(strlen(buf), strlen(buf), FALSE);

    add_sep();
}

#ifdef CALC_EXPR
// Check if string is a simple number (possible with separators) or an expression
bool CCalcEdit::is_number(LPCTSTR ss)
{
    char sep_char = ' ';
    if (pp_->radix_ == 10)
		sep_char = dec_sep_char_;

	bool digit_seen = false;
	bool last_sep = false;

    for (const char *ps = ss; *ps != '\0'; ++ps)
    {
        if (pp_->radix_ == 10 && !digit_seen && *ps == '-')
            continue;   // skip leading minus sign for decimal numbers

		last_sep = *ps == sep_char;
		if (last_sep)
			continue;                    // Skip separator

		// Check if valid digit
        unsigned int digval;
        if (isdigit(*ps))
            digval = *ps - '0';
        else if (isalpha(*ps))
            digval = toupper(*ps) - 'A' + 10;
		else
			return false;                // Not number character

		// Check if digit is in range of radix
        if (digval >= pp_->radix_)
			return false;               // Invalid digit for radix

        digit_seen = true;              // We've now seen a valid digit
    }

	return digit_seen && !last_sep;     // OK if we saw a digit and did not end on a separator
}

bool CCalcEdit::update_value()
{
    ASSERT(pp_ != NULL);
    ASSERT(pp_->IsVisible());
    ASSERT(pp_->radix_ > 1 && pp_->radix_ <= 36);

    CString ss;
    GetWindowText(ss);
	if (ss.IsEmpty())
		ss = "0";
    pp_->overflow_ = FALSE;

	// check if it is a simple numeric value
	pp_->current_const_ = is_number(ss);

	// evaluate the expression
    int ac;
    CHexExpr::value_t vv = pp_->mm_->expr_.evaluate(ss, 0 /*unused*/, ac /*unused*/, pp_->radix_);
	switch((expr_eval::type_t)vv.typ)
	{
	case CHexExpr::TYPE_NONE:
		pp_->current_str_ = pp_->mm_->expr_.get_error_message();
#ifdef UNICODE_TYPE_STRING
		if (pp_->current_str_.Left(8).CompareNoCase(L"Overflow") == 0)
#else
		if (pp_->current_str_.Left(8).CompareNoCase("Overflow") == 0)
#endif
		{
            pp_->overflow_ = TRUE;
			pp_->current_type_ = CHexExpr::TYPE_INT;  // xxx restore previous type since we are ignoring the change
		}
		else
		{
			pp_->current_ = 0;
		}
		ASSERT(!pp_->current_const_ || pp_->overflow_);
		break;

	case CHexExpr::TYPE_INT:
        // If we are using decimals check if the value is -ve (has high bit set)
        if (pp_->radix_ == 10 && (vv.int64 & ((pp_->mask_ + 1)>>1)) != 0)
        {
            // Make sure all the high bits are on
            if (~(vv.int64 | pp_->mask_) != 0)
                pp_->overflow_ = TRUE;
        }
        else
        {
            // Make sure all the high bits are off
            if ((vv.int64 & ~pp_->mask_) != 0)
                pp_->overflow_ = TRUE;
        }
		if (!pp_->overflow_)
		{
            //pp_->current_ = (pp_->current_ & ~pp_->mask_) | (vv.int64 & pp_->mask_);
            pp_->current_ = vv.int64;
			if (pp_->current_const_)
				add_sep();
		}
		break;

	case CHexExpr::TYPE_REAL:
		ASSERT(!pp_->current_const_);
		pp_->current_ = (pp_->current_ & ~pp_->mask_) | (__int64(vv.real64) & pp_->mask_);
#ifdef UNICODE_TYPE_STRING
		pp_->current_str_.Format(L"%g", vv.real64);
#else
		pp_->current_str_.Format("%g", vv.real64);
#endif
		break;

	case CHexExpr::TYPE_BOOLEAN:
		ASSERT(!pp_->current_const_);
		pp_->current_ = vv.boolean;
		pp_->current_str_ = vv.boolean ? "TRUE" : "FALSE";
		break;

	case CHexExpr::TYPE_STRING:
		ASSERT(!pp_->current_const_);
		pp_->current_ = 0;
		pp_->current_str_ = *vv.pstr;
		// Escape special characters
#ifdef UNICODE_TYPE_STRING
		pp_->current_str_.Replace(L"\a", L"\\a");
		pp_->current_str_.Replace(L"\b", L"\\b");
		pp_->current_str_.Replace(L"\t", L"\\t");
		pp_->current_str_.Replace(L"\n", L"\\n");
		pp_->current_str_.Replace(L"\v", L"\\n");
		pp_->current_str_.Replace(L"\f", L"\\n");
		pp_->current_str_.Replace(L"\r", L"\\n");
		pp_->current_str_.Replace(L"\0", L"\\0");
		pp_->current_str_.Replace(L"\"", L"\\\"");
		pp_->current_str_ = L"\"" + pp_->current_str_ + L"\"";
#else
		pp_->current_str_.Replace("\a", "\\a");
		pp_->current_str_.Replace("\b", "\\b");
		pp_->current_str_.Replace("\t", "\\t");
		pp_->current_str_.Replace("\n", "\\n");
		pp_->current_str_.Replace("\v", "\\n");
		pp_->current_str_.Replace("\f", "\\n");
		pp_->current_str_.Replace("\r", "\\n");
		pp_->current_str_.Replace("\0", "\\0");
		pp_->current_str_.Replace("\"", "\\\"");
		pp_->current_str_ = "\"" + pp_->current_str_ + "\"";
#endif
		break;

	case CHexExpr::TYPE_DATE:
		ASSERT(!pp_->current_const_);
        if (vv.date <= -1e30)
		{
			pp_->current_ = 0;
            pp_->current_str_ = "##Invalid date##";
		}
		else
		{
			pp_->current_ = (pp_->current_ & ~pp_->mask_) | (__int64(vv.date) & pp_->mask_);
			COleDateTime odt;
			odt.m_dt = vv.date;
			odt.m_status = COleDateTime::valid;
			double whole, frac = modf(vv.date, &whole);
			if (vv.date > 0.0 && vv.date < 1.0)
				pp_->current_str_ = "TIME(\"" + odt.Format("%X") + "\")";   // Just show time of day part (no date)
			else if (frac == 0.0)
				pp_->current_str_ = "DATE(\"" + odt.Format("%x") + "\")";   // Just show date part (no time of day)
			else
				pp_->current_str_ = "DATE(\"" + odt.Format("%x %X") + "\")"; // Date and time
		}
		break;

	default:
		ASSERT(0);
		pp_->current_type_ = CHexExpr::TYPE_NONE;  // signal as error until calc can handle expressions of any type
		pp_->current_str_ = "##Unexpected expression type##";
		break;
	}

	pp_->FixFileButtons();

    if (pp_->overflow_)
	{
		// Note: Since we are ignoring the typed character (returning false)
		//       we do not want to change pp_->current_type_.
#ifdef SYS_SOUNDS
        CSystemSound::Play("Invalid Character");
#else
        ::Beep(5000,200);
#endif
        return false;
    }
	else
	{
		pp_->current_type_ = (expr_eval::type_t)vv.typ;
        return true;
	}
}
#else
bool CCalcEdit::update_value()
{
    ASSERT(pp_ != NULL);
    ASSERT(pp_->IsVisible());
    ASSERT(pp_->radix_ > 1 && pp_->radix_ <= 36);

    CString ss;
    GetWindowText(ss);

    __int64 newval = 0;
    BOOL none_yet = TRUE;		        // Have we seen any digits yet?
    BOOL neg = FALSE;                   // Have we seen a leading minus sign?

    pp_->overflow_ = FALSE;             // Default to no overflow

    for (const char *src = ss.GetBuffer(0); *src != '\0'; ++src)
    {
        if (pp_->radix_ == 10 && none_yet && *src == '-')
        {
            neg = TRUE;
            continue;
        }

        // Ignore anything else except valid digits
        unsigned int digval;
        if (isdigit(*src))
            digval = *src - '0';
        else if (isalpha(*src))
            digval = toupper(*src) - 'A' + 10;
        else
            continue;                   // Ignore separators (or any other garbage)

        if (digval >= pp_->radix_)
        {
            ASSERT(0);                  // How did this happen?
            continue;                   // Ignore invalid digits
        }

        none_yet = FALSE;               // We've now seen a valid digit

        if (newval > pp_->mask_ / pp_->radix_ ||
            (newval == pp_->mask_ / pp_->radix_ && 
             digval > pp_->mask_ % pp_->radix_) )
        {
            pp_->overflow_ = TRUE;
#ifdef SYS_SOUNDS
            CSystemSound::Play("Invalid Character");
#else
            ::Beep(5000,200);
#endif
			return false;
        }
        newval = newval * pp_->radix_ + digval;
    }

    if (neg)
        newval = -newval;

    pp_->current_ = (pp_->current_ & ~pp_->mask_) | (newval & pp_->mask_);
	//pp_->expr_type_ = CCalcDlg::VALUE;
    add_sep();
	pp_->FixFileButtons();

	return true;
}
#endif

void CCalcEdit::add_sep()
{
    ASSERT(pp_ != NULL);
    ASSERT(pp_->IsVisible());
    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

    // Work out how to group the digits
    int group;
    if (pp_->radix_ == 10)
        group = dec_group_;             // Use decimal grouping from locale
    else if (pp_->radix_ == 16)
        group = 4;                      // For hex make groups of 4 nybbles (2 bytes)
    else
        group = 8;                      // Group = whole number of bytes (1 byte for binary, 3 for octal)

    char sep_char = ' ';
    if (pp_->radix_ == 10) sep_char = dec_sep_char_;

    CString ss;                         // Current string
    GetWindowText(ss);
    int start, end;                     // Current selection (start==end if just caret)
    GetSel(start, end);

    const char *src;                    // Ptr into orig. string
    int newstart, newend;               // New caret position/selection
    newstart = newend = 0;              // In case of empty string

    char out[72];                       // Largest output is 64 bit binary (64 bits + 7 spaces + nul)

    size_t ii, jj;                      // Number of digits written/read
    char *dst = out;                    // Ptr to current output char
    int ndigits;                        // Numbers of chars that are part of number

    BOOL none_yet = TRUE;		        // Have we seen any non-zero digits yet?

    // Work out how many digits we have
    for (src = ss.GetBuffer(0), ndigits = 0; *src != '\0'; ++src)
    {
        unsigned int val;
        if (isdigit(*src))
            val = *src - '0';
        else if (isalpha(*src))
            val = toupper(*src) - 'A' + 10;
        else
            continue;

        // Check for valid digits according to current radix
        if (val < pp_->radix_)
            ++ndigits;
    }

    for (src = ss.GetBuffer(0), ii = jj = 0; /*forever*/; ++src, ++ii)
    {
        if (ii == start)
            newstart = dst - out;       // save new caret position
        if (ii == end)
            newend = dst - out;

        if (*src == '\0')
            break;

        // Copy -ve sign
        if (jj < ndigits && pp_->radix_ == 10 && none_yet && *src == '-')
        {
            *dst++ = '-';
            continue;
        }

        // Ignore anything else except valid digits
        unsigned int val;
        if (isdigit(*src))
            val = *src - '0';
        else if (isalpha(*src))
            val = toupper(*src) - 'A' + 10;
        else
            continue;

        if (val >= pp_->radix_)
        {
            ASSERT(0);                  // Digits invalid for this base should never have got in there!
            continue;
        }

        ++jj;

        // Ignore leading zeroes
        if (jj < ndigits && none_yet && *src == '0')
            continue;

        none_yet = FALSE;

        if (aa->hex_ucase_)
            *dst++ = toupper(*src);     // convert all to upper case
        else
            *dst++ = tolower(*src);     // convert all to lower case


        // If at end of group insert pad char
        if ((ndigits - jj) % group == 0)
            *dst++ = sep_char;
    }
    if (dst > out)
        dst--;                          // Forget last sep_char if added
    *dst = '\0';                        // Terminate string

    SetWindowText(out);
    SetSel(newstart, newend);
}

/////////////////////////////////////////////////////////////////////////////
// CCalcEdit message handlers

void CCalcEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    ASSERT(pp_ != NULL);
    ASSERT(pp_->IsVisible());
    CString ss;
    GetWindowText(ss);
    int start, end;
    GetSel(start, end);

#ifndef CALC_EXPR
    char sep_char = ' ';
    if (pp_->radix_ == 10) sep_char = dec_sep_char_;

    if (nChar == sep_char)
    {
        // Just ignore separator chars (user has tendency to type them even though not required)
        return;
    }
    else if (nChar == '\b')
    {
        // If backspace deletes 1st digit of a group then also delete preceding separator
        // (If we don't do this the add_sep call below stops the backspace from working.)
        if (start > 1 && start == end && ss[start-1] == sep_char)
            SetSel(start-2, end);
    }
    else if (isalnum(nChar))
    {
        // Calculate digit value and beep if it is invalid for the current radix
        unsigned int val;
        if (isdigit(nChar))
            val = nChar - '0';
        else
            val = toupper(nChar) - 'A' + 10;

        if (val >= pp_->radix_)
        {
#ifdef SYS_SOUNDS
            CSystemSound::Play("Invalid Character");
#else
            ::Beep(5000,200);
#endif
            return;
        }
    }
#endif

    CEdit::OnChar(nChar, nRepCnt, nFlags);

	if (!update_value())
	{
        SetWindowText(ss);
        SetSel(start, end, FALSE);
    }
}

void CCalcEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    ASSERT(pp_ != NULL);
    ASSERT(pp_->IsVisible());
    CString ss;                         // Text from the window (edit control)
    int start, end;                     // Current selection in the edit control

    pp_->ShowBinop(pp_->op_);           // Show current binop (if any) and clear overflow/error status

    // If we need to clear edit control (eg. binary op just entered) ...
    if (!pp_->in_edit_)
    {
        // Clear control ready for new number unless (arrow, Del etc key pressed)
        if (isprint(nChar))
            SetWindowText("");
        pp_->in_edit_ = TRUE;           // and go into edit mode
    }
    else
    {
        // Fiddle with the selection so that arrows/Del work OK in presence of separator chars
        GetWindowText(ss);
        GetSel(start, end);

        char sep_char = ' ';
        if (pp_->radix_ == 10) sep_char = dec_sep_char_;

        // If no selection and character to delete is separator ...
        if (nChar == VK_DELETE && start == end && ss.GetLength() > start+1 && ss[start+1] == sep_char)
        {
            // Set selection so that the separator and following digit is deleted
            SetSel(start, start+2);
        }
        else if (nChar == VK_LEFT && start == end && start > 0 && ss[start-1] == sep_char)
        {
            // Move cursor back one so we skip over the separator (else add_sep below makes the caret stuck)
            SetSel(start-1, start-1);
        }
    }

    CEdit::OnKeyDown(nChar, nRepCnt, nFlags);

    // Tidy display if hex and char(s) deleted or caret moved xxx Is this necessary?
    GetWindowText(ss);
    if ((nChar == VK_DELETE || nChar == VK_RIGHT || nChar == VK_LEFT) &&
#ifdef CALC_EXPR
        is_number(ss) &&
#endif
        ::GetKeyState(VK_SHIFT) >= 0 && ss.GetLength() > 0)
    {
        add_sep();
    }

    if (nChar == VK_DELETE)
		(void)update_value();   // should always return true for Del key

    pp_->source_ = pp_->aa_->recording_ ? km_user : km_result;
}
