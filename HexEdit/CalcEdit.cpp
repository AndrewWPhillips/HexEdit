// CalcEdit.cpp : implements the subclassed edit control for the calculator
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

#include "stdafx.h"
#include <locale.h>

#include "HexEdit.h"
#include "MainFrm.h"
#include "CalcEdit.h"
#include "SystemSound.h"
#include "HexEditDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// NOTES

// 1. The current calculator value if not necessarilly the string in the edit
// box but is determined by the CCalcDlg members state_, current_, current_str_.
// If state <= CALCINTLIT current_ is the value (inf. precision integer);
// otherwise current_str_ is an expression/literal containing the current value.
//
// 2. The current value continually calculated & stored in "current_" (if an
// integer) which is accessed via pp_.  This allows overflow to 
// be detected as soon as the user types a digit. So for example, if bits is 8 
// and radix is 10 and the user has already typed "25" then typing "6" will
// cause a beep due to overflow, but typing "5" will not.
//
// 3. An overflow sets state_ = CALCOVERFLOW in the CCalcDlg class.

/////////////////////////////////////////////////////////////////////////////
// CCalcEdit

CCalcEdit::CCalcEdit()
{
	pp_ = NULL;
	sel_ = (DWORD)-1;
}

CCalcEdit::~CCalcEdit()
{
}


BEGIN_MESSAGE_MAP(CCalcEdit, CEdit)
	//{{AFX_MSG_MAP(CCalcEdit)
	ON_WM_CHAR()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

#if 0
// Put current integer (current_) as literal text into the edit box
// Also checks for overflow etc and updates the bits display.
void CCalcEdit::Put()
{
	ASSERT(pp_ != NULL);
	ASSERT(pp_->radix_ > 1 && pp_->radix_ <= 36);
	ASSERT(pp_->IsVisible());

	mpz_class val = pp_->get_norm(pp_->current_);

	// Get a buffer large enough to hold the text (may be big)
	char *buf = new char[mpz_sizeinbase(val.get_mpz_t(), pp_->radix_) + 2];

	// Get the number as a string and add it to the text box
	mpz_get_str(buf, pp_->radix_, val.get_mpz_t());
	SetWindowText(buf);
	SetSel(strlen(buf), -1, FALSE);     // move caret to end

	delete[] buf;

	// Format the number nicely
	add_sep();

	// We know it is an integer (though the caller will probably set this to INTRES)
	if (pp_->bits_ > 0 && (pp_->current_ < pp_->min_val_ || pp_->current_ > pp_->max_val_))
		pp_->state_ = CALCOVERFLOW;

	if (pp_->ctl_calc_bits_.m_hWnd != 0)
		pp_->ctl_calc_bits_.RedrawWindow();
}

// Put general string (eg may be expression or non-integer value) into the edit box.
// Checks the type of expression, overflow etc and updates the bits display.
void CCalcEdit::PutStr()
{
	ASSERT(pp_ != NULL);
	ASSERT(pp_->radix_ > 1 && pp_->radix_ <= 36);
	ASSERT(pp_->IsVisible());

	// Put as Unicode as an expression can contain a Unicode string
	::SetWindowTextW(m_hWnd, (LPCWSTR)pp_->current_str_);

	pp_->state_ = update_value(false);

	// Force redisplay of bits in case it was an integer
	if (pp_->ctl_calc_bits_.m_hWnd != 0)
		pp_->ctl_calc_bits_.RedrawWindow();
}
#endif
// Put the current value into the edit box
// - from current_ if state_ <= CALCINTLIT,
// - else from current_str_
//   Note that state_/current_/current_str_ may be updated to reflect what current_str_ contained.
void CCalcEdit::put()
{
	ASSERT(pp_ != NULL);
	ASSERT(pp_->radix_ > 1 && pp_->radix_ <= 36);

	if (theApp.refresh_off_ || !pp_->IsVisible())
		return;

	if (pp_->state_ <= CALCINTLIT)
	{
		// Get value from current_ (integer)
		mpz_class val = pp_->get_norm(pp_->current_);

		// Get a buffer large enough to hold the text (may be big)
		char *buf = new char[mpz_sizeinbase(val.get_mpz_t(), pp_->radix_) + 2];

		// Get the number as a string and add it to the text box
		mpz_get_str(buf, pp_->radix_, val.get_mpz_t());
		SetWindowText(buf);
		SetSel(strlen(buf), -1, FALSE);     // move caret to end

		delete[] buf;
		TRACE("++++++ Put: int %s\r\n", buf);

		// Format the number nicely
		add_sep();
	}
	else
	{
		// Get value from current_str_ (expression (possibly integer), non-int literal, or something else)
		//SetWindowText(pp_->current_str_);
		// Put as Unicode as an expression can contain a Unicode string
		::SetWindowTextW(m_hWnd, (LPCWSTR)pp_->current_str_);
		SetSel(pp_->current_str_.GetLength(), -1, FALSE);     // move caret to end
		TRACE("++++++ Put: expr %s\r\n", pp_->current_str_);

		// Finally we need to make sure that state_ etc match what the string contains.
		// Eg current_str_ may actually be an integer literal, in which case we update state_/current_
		pp_->state_ = update_value(false);
	}

	if (pp_->state_ <= CALCINTLIT)
		add_sep();

	// xxx do this in idle processing???
	if (pp_->ctl_calc_bits_.m_hWnd != 0)
		pp_->ctl_calc_bits_.RedrawWindow();
}

void CCalcEdit::get()
{
	ASSERT(pp_ != NULL);
	ASSERT(pp_->radix_ > 1 && pp_->radix_ <= 36);

	int len = GetWindowTextLength();

	wchar_t * pbuf = new wchar_t[len + 2];

	::GetWindowTextW(m_hWnd, pbuf, len+1);
	pp_->current_str_ = pbuf;

	delete[] pbuf;
}

// Takes the value in the edit control and works out info about it:
//  - sets current_ if it is an integer value
//  - sets current_str_ if it is something else
//  - returns a new STATE to says what sort of value/expression is there
//
// The side_effects parameters says whether side effects in an expression
// have any effect.  For example, in the expression "q ? a=1 : b=2" the values
// of the variables 'a' and 'b' will not be changed if side_effects is false
// but when side_effects is true then 'a' or 'b' will be changed depending
// on the value of 'q'.
//
// Returns
//  - CALCERROR or CALCOVERFLOW when there is some problem with the integer result
//  - CALCINTLIT for a simple integer literal
//  - CALC*EXPR for a valid expression returning a result of any of the 5 types
//  - CALCOTHER for an invalid expression
CALCSTATE CCalcEdit::update_value(bool side_effects /* = true */)
{
	ASSERT(pp_ != NULL);
	ASSERT(pp_->radix_ > 1 && pp_->radix_ <= 36);

	CString ss;
	ss = pp_->current_str_;
	if (ss.IsEmpty())
		ss = "0";     // make zero if empty

	// xxx check if isnumber before evaluating as an expression???

	// evaluate the expression
	int ac;
	CHexExpr::value_t vv = pp_->mm_->expr_.evaluate(ss, 0 /*unused*/, ac /*unused*/, pp_->radix_, side_effects);

	CALCSTATE retval = CALCERROR;
	pp_->current_ = 0;                 // default to zero in case of error etc

	// Set current_, current_str_ and retval depending on what we found
	switch((expr_eval::type_t)vv.typ)
	{
	case CHexExpr::TYPE_NONE:
		pp_->current_str_ = pp_->mm_->expr_.get_error_message();
#ifdef UNICODE_TYPE_STRING
		if (pp_->current_str_.Left(8).CompareNoCase(L"Overflow") == 0)
#else
		if (pp_->current_str_.Left(8).CompareNoCase("Overflow") == 0)
#endif
			retval = CALCOVERFLOW;  // xxx does this ever happen?
		else
			retval = CALCOTHER;     // Probably just an incomplete expression 
		break;

	case CHexExpr::TYPE_INT:
		if (is_number(ss))    // it's a simple integer literal (perhaps with sign and separator chars)
		{
			// Remove separator chars from the string
			char sep_char[2];
			sep_char[1] = '\0';
			if (pp_->radix_ == 10)
				sep_char[0] = theApp.dec_sep_char_;
			else
				sep_char[0] = ' ';
			ss.Replace(sep_char, "");

			// Get the value from the string using current radix
			mpz_set_str(pp_->current_.get_mpz_t(), ss, pp_->radix_);

			// make sure current separators are correct
			add_sep(); // xxx move out of here
			retval = CALCINTLIT;
		}
		else
		{
			// If we are using decimals check if the value is -ve (has high bit set)
			mpz_class val;
			if ((pp_->signed_ || pp_->bits_ == 0) && vv.int64 < 0)
			{
				// Take 2's complement and set as -ve
				mpz_set_ui64(val.get_mpz_t(), ~vv.int64 + 1);
				mpz_neg(val.get_mpz_t(), val.get_mpz_t());
			}
			else
			{
				mpz_set_ui64(val.get_mpz_t(), vv.int64);
			}
			pp_->current_ = val;
			retval = CALCINTEXPR;
		}
		if (pp_->bits_ > 0 && (pp_->current_ < pp_->min_val_ || pp_->current_ > pp_->max_val_))
			retval = CALCOVERFLOW;
		break;

	case CHexExpr::TYPE_REAL:
#ifdef UNICODE_TYPE_STRING
		pp_->current_str_.Format(L"%g", vv.real64);
#else
		pp_->current_str_.Format("%g", vv.real64);
#endif
		retval = CALCREALEXPR;
		break;

	case CHexExpr::TYPE_BOOLEAN:
		pp_->current_str_ = vv.boolean ? "TRUE" : "FALSE";
		retval = CALCBOOLEXPR;
		break;

	case CHexExpr::TYPE_STRING:
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
		retval = CALCSTREXPR;
		break;

	case CHexExpr::TYPE_DATE:
		if (vv.date <= -1e30)
			pp_->current_str_ = "##Invalid date##";
		else
		{
			// Convert the date to an expression that evaluates to that date.  Since we don't have date
			// literals we need to use a function and a date string.  Thence we can put the expression 
			// back into the calc edit box and it will evaluates to the same date.
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
			retval = CALCDATEEXPR;
		}
		break;

	default:
		ASSERT(0);  // I believe this should not happen
		pp_->current_str_ = "##Unexpected expression type##";
		//retval = CALCERROR;
		break;
	}
	pp_->ShowStatus();                       // Indicate overflow etc

#if 0  // This is handled by ShowStatus() ??? xxx
	if (retval == CALCOVERFLOW || retval == CALCERROR)
	{
#ifdef SYS_SOUNDS
		if (!CSystemSound::Play("Invalid Character"))
#endif
			::Beep(5000,200);
	}
#endif
#ifdef _DEBUG
	if (pp_->state_ <= CALCINTLIT)
		TRACE("++++++ Got: int %d\r\n", mpz_get_ui(pp_->current_.get_mpz_t()));
	else
		TRACE("++++++ Got: <%s>\r\n", pp_->current_str_);
#endif

	return retval;
}

// Gets the value from the edit box assuming it is an integer literal, and reformats the digits
// nicely grouped with separators (spaces or commas/dots), preserving the cursor (caret) position.
void CCalcEdit::add_sep()
{
	ASSERT(pp_ != NULL);
	ASSERT(pp_->IsVisible());
	ASSERT(pp_->state_ <= CALCINTLIT);
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

	// Work out how to group the digits
	int group;
	if (pp_->radix_ == 10)
		group = theApp.dec_group_;      // Use decimal grouping from locale
	else if (pp_->radix_ == 16)
		group = 4;                      // For hex make groups of 4 nybbles (2 bytes)
	else
		group = 8;                      // Group = whole number of bytes (1 byte for binary, 3 for octal)

	char sep_char = ' ';
	if (pp_->radix_ == 10) sep_char = theApp.dec_sep_char_;

	CString ss;                         // Current string
	GetWindowText(ss);
	int start, end;                     // Current selection in string (start==end if just caret)
	GetSel(start, end);

	const char *src;                    // Ptr into orig. string
	int ndigits;                        // Numbers of chars that are part of number

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

	char * out = new char[(ndigits * (group+1))/group + 4];
	char *dst = out;                    // Ptr to current output char

	int newstart, newend;               // New caret position/selection
	newstart = newend = 0;              // In case of empty string
	BOOL none_yet = TRUE;		        // Have we seen any non-zero digits yet?

	size_t ii, jj;                      // Number of digits written/read

	for (src = ss.GetBuffer(0), ii = jj = 0; /*forever*/; ++src, ++ii)
	{
		if (ii == start)
			newstart = dst - out;       // save new caret position
		if (ii == end)
			newend = dst - out;

		if (*src == '\0')
			break;

		// Copy -ve sign
		if (jj < ndigits && (pp_->signed_ || pp_->bits_ == 0) && none_yet && *src == '-')
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

	TRACE("++++++ Sep: <%s> to <%s>\r\n", ss, out);
	SetWindowText(out);
	delete[] out;
	SetSel(newstart, newend);
}

// Check if string is a simple number (possibly with separators) or an expression
bool CCalcEdit::is_number(LPCTSTR ss)
{
	char sep_char = ' ';
	if (pp_->radix_ == 10)
		sep_char = theApp.dec_sep_char_;

	bool digit_seen = false;
	bool last_sep = false;

	for (const char *ps = ss; *ps != '\0'; ++ps)
	{
		if ((pp_->signed_ || pp_->bits_ == 0) && !digit_seen && *ps == '-')
			continue;   // skip leading minus sign for signed numbers

		last_sep = *ps == sep_char;
		if (last_sep)
			continue;                    // Skip separator

		// Check if/get valid digit
		unsigned int digval;
		if (*ps < 0)
			return false;                // is* macros can't handle -ve values
		else if (isdigit(*ps))
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

// When the edit box is displaying a result (eg, state_ == CALCINTRES etc)
// this changes the state_ to allow editing (eg CALCINTLIT) and also gives the 
// option to clear the edit box in preparation for a new value being entered.
void CCalcEdit::clear_result(bool clear /* = true */)
{
	if (pp_->state_ < CALCINTLIT || pp_->state_ == CALCOTHRES)
	{
		if (clear)
		{
			TRACE("++++++ Clr: CLEARED RESULT FROM EDIT BOX\r\n");
			SetWindowText("");
		}

		TRACE("++++++ Now: editable <%s>\r\n", pp_->state_ < CALCINTLIT ? "int" : "");
		if (pp_->state_ < CALCINTLIT)
			pp_->state_ = CALCINTLIT;
		else if (pp_->state_ == CALCOTHRES)
			pp_->state_ = CALCOTHER;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CCalcEdit message handlers

void CCalcEdit::OnSetFocus(CWnd* pOldWnd)
{
	// This clears the edit box after a binop button but allows editing after '=' button,
	// so the user can edit a result (eg to copy to clipboard).
	clear_result(pp_->op_ != binop_none);

	CEdit::OnSetFocus(pOldWnd);
	if (sel_ != (DWORD)-1)
	{
		SetSel(sel_);
		sel_ = (DWORD)-1;
	}
}

void CCalcEdit::OnKillFocus(CWnd* pNewWnd)
{
	sel_ = GetSel();
	CEdit::OnKillFocus(pNewWnd);
}

void CCalcEdit::OnLButtonUp(UINT nFlags, CPoint point)
{
	CEdit::OnLButtonUp(nFlags, point);
}

void CCalcEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	ASSERT(pp_ != NULL && pp_->IsVisible());
	clear_result();

	CEdit::OnChar(nChar, nRepCnt, nFlags);

	pp_->state_ = update_value(false);
	pp_->inedit(km_user_str);
	pp_->ctl_calc_bits_.RedrawWindow();
}

void CCalcEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	ASSERT(pp_ != NULL && pp_->IsVisible());

	// Clear any result (unless arrow, Del etc key pressed)
	clear_result((nFlags & 0x100) == 0 && isprint(nChar));

	// If editing an integer then make allowances for separator char
	if (pp_->state_ == CALCINTLIT)
	{
		CString ss;                         // Text from the window (edit control)
		int start, end;                     // Current selection in the edit control

		// Set selection so that arrows/Del work OK in presence of separator chars
		GetWindowText(ss);
		GetSel(start, end);

		char sep_char = ' ';
		if (pp_->radix_ == 10) sep_char = theApp.dec_sep_char_;

		// If no selection and character to delete is separator ...
		if (nChar == VK_DELETE && start == end && start < ss.GetLength() - 1 && ss[start+1] == sep_char)
		{
			// Set selection so that the separator and following digit is deleted
			SetSel(start, start+2);
		}
		else if (nChar == VK_LEFT && start == end && start > 0 && ss[start-1] == sep_char)
		{
			// Move cursor back one so we skip over the separator (else add_sep below makes the caret stuck)
			SetSel(start-1, start-1);
		}
		else if (nChar == VK_RIGHT && start == end && start < ss.GetLength() - 1 && ss[start+1] == sep_char)
		{
			// Move cursor back one so we skip over the separator (else add_sep below makes the caret stuck)
			SetSel(start+1, start+1);
		}
	}

	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);

	if (nChar == VK_DELETE)
		(void)update_value(false);

	pp_->source_ = theApp.recording_ ? km_user_str : km_result;
	pp_->ctl_calc_bits_.RedrawWindow();
}

BOOL CCalcEdit::PreTranslateMessage(MSG* pMsg)
{
	CString ss;

	// The default behaviour for Enter key is GO or "=" button
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	{
		// Can only used GO button if it's enabled and we have an integer value
		if (pp_->ctl_go_.IsWindowEnabled() && pp_->state_ <= CALCINTEXPR)
			pp_->OnGo();
		else
			pp_->OnEquals();
		return TRUE;
	}

	return CEdit::PreTranslateMessage(pMsg);
}

/////////////////////////////////////////////////////////////////////////////
// CCalcListBox
#define CCALCLISTBOX_TIMER_ID 1

BEGIN_MESSAGE_MAP(CCalcListBox, CListBox)
	ON_WM_MOUSEMOVE()
	ON_WM_TIMER()
END_MESSAGE_MAP()

void CCalcListBox::OnMouseMove(UINT nFlags, CPoint point) 
{

	CRect rectClient;
	GetClientRect(&rectClient);

	if (rectClient.PtInRect(point))
	{
		CPoint pointScreen;
		::GetCursorPos(&pointScreen);
		BOOL bOutside = FALSE;
		int nItem = ItemFromPoint(point, bOutside);  // calculate listbox item number (if any)

		if (!bOutside && (nItem >= 0))
		{
			CString strText;
			GetText(nItem, strText);

			CRect rect;
			GetItemRect(nItem, &rect);
			ClientToScreen(&rect);

			HDC hDC = ::GetDC(m_hWnd);

			SIZE size;
			::GetTextExtentPoint32(hDC, strText, strText.GetLength(), &size);
			::ReleaseDC(m_hWnd, hDC);

		}
		else
		{
		}
	}
	else
	{
	}

	CListBox::OnMouseMove(nFlags, point);
}

void CCalcListBox::OnTimer(UINT id) 
{
	if (id == CCALCLISTBOX_TIMER_ID)
	{
		CPoint point;
		::GetCursorPos(&point);
		ScreenToClient(&point);

		CRect rectClient;
		GetClientRect(&rectClient);

		DWORD dwStyle = GetStyle();
		if ((!rectClient.PtInRect(point)) || ((dwStyle & WS_VISIBLE) == 0))
		{
			KillTimer(CCALCLISTBOX_TIMER_ID);
		}
	}
	else
		CListBox::OnTimer(id);
}

/////////////////////////////////////////////////////////////////////////////
// CCalcComboBox - just used to "subclass" the list box (see CCalcListBox)

BEGIN_MESSAGE_MAP(CCalcComboBox, CComboBox)
	ON_WM_CTLCOLOR()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

HBRUSH CCalcComboBox::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	if (nCtlColor == CTLCOLOR_LISTBOX)
	{
		if (listbox_.GetSafeHwnd() == NULL)
			listbox_.SubclassWindow(pWnd->GetSafeHwnd());
	}
	return CComboBox::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CCalcComboBox::OnDestroy() 
{
	if (listbox_.GetSafeHwnd() != NULL)
		listbox_.UnsubclassWindow();

	CComboBox::OnDestroy();
}
