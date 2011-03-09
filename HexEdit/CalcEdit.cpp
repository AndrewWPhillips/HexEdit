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
#include "TipWnd.h"
#include "misc.h"

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
// An overflow sets state_ = CALCOVERFLOW in the CCalcDlg class.
//
// 3. To put something into the calculator edit box set state_ then either current_
// (for integer literal states) or current_str_ for expressions (or partial ones)
// and call CCalcEdit::put().  This is used for example after calculating a result
// or restoring a value (eg from the Calculator Memory).
//
// 4. To get the current calculator value (from edit box into current_str_) use
// CCalcEdit::get().  This is not exactly the inverse of CCalcEdit::put() as it
// does not set current_ and behaves the same irrespective of the type (state_).
//
// 5. The function CCalcDlg::update_value() takes current_str_ and works out what
// it is (setting state_) and also sets current_ if it is an integer.  Note that
// current_ is set even if it is an integer expressions however in this case the
// value may overflow 64-bits as expressions only use 64-bit ints at present.
//
// Since update_value() evaluates expressions it has an option to allow or disallow
// side-effects, such as assigning to variables.
//
// Calling get() then update_value() is the inverse operation of put().

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

	if (pp_->state_ <= CALCERROR)
	{
		if (pp_->current_str_.IsEmpty())
			SetWindowText("ERROR");
		else
			::SetWindowTextW(m_hWnd, (LPCWSTR)pp_->current_str_);
		TRACE("++++++ Put: error\r\n");
		return;
	}
	else if (pp_->state_ <= CALCINTLIT)
	{
		// Get value from current_ (integer)
		mpz_class val = pp_->get_norm(pp_->current_);

		// Get a buffer large enough to hold the text (may be big)
		int len = mpz_sizeinbase(val.get_mpz_t(), pp_->radix_) + 2 + 1;
		char *buf = new char[len];
		buf[len-1] = '\xCD';

		// Get the number as a string and add it to the text box
		mpz_get_str(buf, pp_->radix_, val.get_mpz_t());
		ASSERT(buf[len-1] == '\xCD');
		SetWindowText(buf);
		if (GetWindowTextLength() < len - 2)
		{
			ASSERT(0);
			SetWindowText("Result too big for edit box to display");
			// xxx Offer option to open in a file?
			pp_->state_ = CALCOVERFLOW;
		}
		else
			SetSel(len, -1, FALSE);     // move caret to end

		TRACE("++++++ Put: int %.100s\r\n", (const char *)buf);
		delete[] buf;
	}
	else
	{
		// Get value from current_str_ (expression (possibly integer), non-int literal, or something else)
		//SetWindowText(pp_->current_str_);
		// Put as Unicode as an expression can contain a Unicode string
		::SetWindowTextW(m_hWnd, (LPCWSTR)pp_->current_str_);
		SetSel(pp_->current_str_.GetLength(), -1, FALSE);     // move caret to end
		TRACE("++++++ Put: expr %s\r\n", (const char *)CString(pp_->current_str_));

#if 0 // I don't think this is necessary and makes other code more complicated
		// Finally we need to make sure that state_ etc match what the string contains.
		// Eg current_str_ may actually be an integer literal, in which case we update state_/current_
		pp_->state_ = update_value(false);
#endif
	}

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

// Takes the value in current_str_ and works out info about it:
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

	CString ss = get_number(pp_->current_str_.IsEmpty() ? "0" : CString(pp_->current_str_));

	if (!ss.IsEmpty())
	{
		// Get the literal integer from the string using current radix
		mpz_set_str(pp_->current_.get_mpz_t(), ss, pp_->radix_);
		return CALCINTLIT;
	}

	// evaluate the expression
	int ac;
	CHexExpr::value_t vv = pp_->mm_->expr_.evaluate(CString(pp_->current_str_), 0 /*unused*/, ac /*unused*/, pp_->radix_, side_effects);

	CALCSTATE retval = CALCERROR;
	pp_->current_ = 0;                 // default to zero in case of error etc

	// Set current_, current_str_ and retval depending on what we found
	switch((expr_eval::type_t)vv.typ)
	{
	case CHexExpr::TYPE_NONE:
		pp_->current_str_ = pp_->mm_->expr_.get_error_message();
		retval = CALCOTHER;     // Probably just an incomplete expression 
		break;

	case CHexExpr::TYPE_INT:
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
		retval = CALCERROR;
		break;
	}

#ifdef _DEBUG
	if (pp_->state_ <= CALCINTLIT)
		TRACE("++++++ Got: int %d\r\n", mpz_get_ui(pp_->current_.get_mpz_t()));
	else
		TRACE("++++++ Got: <%.100s>\r\n", (const char *)CString(pp_->current_str_));
#endif

	return retval;
}

// Gets the value from the edit box if it is an integer literal, and reformats the digits
// nicely grouped with separators (spaces or commas/dots), preserving the cursor (caret) position.
void CCalcEdit::add_sep()
{
	if (pp_->state_ == CALCERROR || pp_->state_ > CALCINTLIT)
		return;

	ASSERT(pp_ != NULL);
	ASSERT(pp_->IsVisible());
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
	{
		dst--;                          // Forget last sep_char if added
		if (newstart > dst-out) newstart = dst-out;
		if (newend   > dst-out) newend   = dst-out;
	}
	*dst = '\0';                        // Terminate string

	SetWindowText(out);
	TRACE("++++++ Sep: <%.100s> to <%.100s>\r\n", (const char *)ss, (const char *)out);
	delete[] out;
	SetSel(newstart, newend);
}

// Check if string is a simple number (possibly with separators) and
// return string without separators or an empty string if not a number.
CString CCalcEdit::get_number(LPCTSTR ss)
{
	char * buf = new char[strlen(ss)+1];
	char * pbuf = buf;

	char sep_char = ' ';
	if (pp_->radix_ == 10)
		sep_char = theApp.dec_sep_char_;

	bool digit_seen = false;
	bool last_sep = false;

	for (const char *ps = ss; *ps != '\0'; ++ps)
	{
		if ((pp_->signed_ || pp_->bits_ == 0) && !digit_seen && *ps == '-')
		{
			*(pbuf++) = '-';
			continue;   // skip leading minus sign for signed numbers
		}

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
			return CString();           // Not number character

		// Check if digit is in range of radix
		if (digval >= pp_->radix_)
			return CString();           // Invalid digit for radix

		*(pbuf++) = *ps;
		digit_seen = true;              // We've now seen a valid digit
	}

	if (digit_seen && !last_sep)       // OK if we saw a digit and did not end on a separator
	{
		*pbuf = '\0';
		return CString(buf);
	}
	else
		return CString();
}

// When the edit box is displaying a result (eg, state_ == CALCINTRES etc)
// this changes the state_ to allow editing (eg CALCINTLIT) and also gives the 
// option to clear the edit box in preparation for a new value being entered.
void CCalcEdit::clear_result(bool clear /* = true */)
{
	if (pp_->state_ < CALCINTLIT || pp_->state_ >= CALCOTHRES)
	{
		if (clear)
		{
			TRACE("++++++ Clr: CLEARED RESULT FROM EDIT BOX\r\n");
			SetWindowText("");
		}

		TRACE("++++++ Now: editable <%s>\r\n", pp_->state_ < CALCINTLIT ? "int" : "");
		if (pp_->state_ < CALCINTLIT)
			pp_->state_ = CALCINTLIT;
		else if (pp_->state_ >= CALCOTHRES)
			pp_->state_ = CALCOTHER;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CCalcEdit message handlers

void CCalcEdit::OnSetFocus(CWnd* pOldWnd)
{
	TRACE("xxxx0 OnSetF: sel = %x\r\n", GetSel());
	// This clears the edit box after a binop button but allows editing after '=' button,
	// so the user can edit a result (eg to copy to clipboard).
	clear_result(pp_->op_ != binop_none);

	CEdit::OnSetFocus(pOldWnd);
	TRACE("xxxx1 OnSetF: sel = %x\r\n", GetSel());
}

void CCalcEdit::OnKillFocus(CWnd* pNewWnd)
{
	TRACE("xxxx0 OnKill: sel = %x\r\n", GetSel());
	sel_ = GetSel();        // base class seems to fiddle with selection so save
	CEdit::OnKillFocus(pNewWnd);
	SetSel(sel_, TRUE);

	TRACE("xxxx1 OnKill: sel = %x\r\n", GetSel());
}

void CCalcEdit::OnLButtonUp(UINT nFlags, CPoint point)
{
	CEdit::OnLButtonUp(nFlags, point);
}

void CCalcEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	ASSERT(pp_ != NULL && pp_->IsVisible());
	clear_result();

	TRACE("xxxx0 OnChar: sel = %x\r\n", GetSel());
	CEdit::OnChar(nChar, nRepCnt, nFlags);

	get();
	pp_->state_ = update_value(false);
	pp_->check_for_error();
	add_sep();
	get();
	pp_->set_right();
	pp_->inedit(km_user_str);
	pp_->ctl_calc_bits_.RedrawWindow();
}

void CCalcEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	ASSERT(pp_ != NULL && pp_->IsVisible());

	// Clear any result (unless arrows etc pressed)
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
	{
		// This is handled similarly to OnChar since Del key changes the text
		get();
		pp_->state_ = update_value(false);
		pp_->check_for_error();
		add_sep();
		get();
		pp_->set_right();
		pp_->inedit(km_user_str);
		pp_->ctl_calc_bits_.RedrawWindow();
	}
}

BOOL CCalcEdit::PreTranslateMessage(MSG* pMsg)
{
	CString ss;

	// The default behaviour for Enter key is GO or "=" button
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	{
		// Can only used GO button if it's enabled and we have an integer value
		//if (pp_->ctl_go_.IsWindowEnabled() && pp_->state_ <= CALCINTEXPR)
		CCalcButton *pbut = DYNAMIC_DOWNCAST(CCalcButton,& pp_->ctl_go_);
		if (pbut->GetTextColor() != RGB(0xC0, 0xC0, 0xC0) && pp_->state_ <= CALCINTEXPR)
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
		BOOL bOutside = FALSE;
		int nItem = ItemFromPoint(point, bOutside);  // listbox item number we're over (if any)

		if (!bOutside && (nItem >= 0))
		{
			// If we are now hovering over a different item
			if (nItem != curr_)
			{
				// Display expression and/or result string in the tip window
				tip_.Clear();

				CString ss;
				GetText(nItem, ss);   // This assumes combo box strings are not long (ie truncated)

				CString * ps = (CString *)GetItemDataPtr(nItem);
				if (ps == NULL)
				{
					// Expression only
					tip_.AddString(ss);
				}
				else if (ps->GetLength() < 100)
				{
					// "Expression = result"
					ss += " = " + *ps;
					tip_.AddString(ss);
				}
				else
				{
					// Add "Expression ="  then multiple lines of result
					tip_.AddString(ss + " =");

					// Work out the width of the result string in pixels
					CSize sz;
					{
						CClientDC dc(&tip_);
						int nSave = dc.SaveDC();
						dc.SelectObject(tip_.GetFont());
						sz = dc.GetTextExtent(*ps);
						dc.RestoreDC(nSave);
					}

					// Check if string fits within this monitor - and work out how many times to wrap if not
					int wrap = 0;
					CRect rctMon = MonitorMouse();   // rect of current monitor
					if (sz.cx > rctMon.Width())
						wrap = sz.cx / (rctMon.Width()*3/4);

					// Add the text to the tip window
					if (wrap > 0)
					{
						// First work out roughly how many characters to put on each line
						int len = ps->GetLength() / (wrap+1);
						char * start = ps->GetBuffer();
						char * end = start + ps->GetLength();

						while (start + len < end)
						{
							char * pp;
							for (pp = start + len*3/4; pp > start + len/2; pp--)
							{
								if (*pp == ' ')
									break;
							}
							char tmp = *pp;
							*pp = '\0';
							tip_.AddString(start);
							*pp = tmp;
							start = pp;
						}
						tip_.AddString(start);  // remaining bit
					}
					else
						tip_.AddString(*ps);
				}

				// Move the tip window just below the mouse and make visible
				CRect rct;
				tip_.GetWindowRect(&rct);
				CPoint pt;
				::GetCursorPos(&pt);
				pt.x -= rct.Width()/2;
				pt.y += rct.Height()/2;
				tip_.Move(pt, false);
				tip_.Show();

				SetTimer(CCALCLISTBOX_TIMER_ID, 100, NULL);  // allows check after window closed/moved off
				curr_ = nItem;  // remember which item we are now showing tip for
			}
			CListBox::OnMouseMove(nFlags, point);
			return;
		}
	}

	// If we didn't return above then we are not over an item
	// Close any tip window that is open
	if (tip_.IsWindowVisible() && !tip_.FadingOut())
		tip_.Hide(300);
	curr_ = -1;

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
			if (tip_.IsWindowVisible() && !tip_.FadingOut())
				tip_.Hide(300);
			curr_ = -1;
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
