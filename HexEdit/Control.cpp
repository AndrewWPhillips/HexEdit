// Control.cpp : implements control bars and controls thereon
//
// Copyright (c) 1999-2010 by Andrew W. Phillips.
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
#include <assert.h>

#include "HexEdit.h"
#include "MainFrm.h"
#include "HexEditDoc.h"
#include "HexEditView.h"
#include "bookmark.h"
#include "SystemSound.h"
#include "misc.h"
#include "resource.hm"          // For control help IDs
#include <afxpriv.h>            // for WM_COMMANDHELP and WM_HELPHITTEST

#include <HtmlHelp.h>

extern CHexEditApp theApp;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHexEdit - subclassed edit control that allows entry of hex bytes

IMPLEMENT_DYNAMIC(CHexEdit, CEdit)

BEGIN_MESSAGE_MAP(CHexEdit, CEdit)
	ON_WM_CHAR()
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHexEdit message handlers

void CHexEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CString ss;                     // Current text in control
	GetWindowText(ss);
	int start, end;                 // Range of selection of text in control
	GetSel(start, end);

	const char *test_str;
	if (allow_qmark_)
		test_str = "\b 0123456789abcdefABCDEF?";
	else
		test_str = "\b 0123456789abcdefABCDEF";

	if (nChar == '\b' && start > 1 && start == end && ss[start-1] == ' ')
	{
		// If deleting 1st nybble (hex mode) then also delete preceding space
		SetSel(start-2, end);
		// no return
	}
	else if (nChar == ' ')
	{
		// Just ignore spaces
		return;
	}
	else if (::isprint(nChar) && strchr(test_str, nChar) == NULL)
	{
#ifdef SYS_SOUNDS
		if (!CSystemSound::Play("Invalid Character"))
#endif
			::Beep(5000,200);
		return;
	}
	CEdit::OnChar(nChar, nRepCnt, nFlags);
	add_spaces();
}

void CHexEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CString ss;                     // Current text in control
	GetWindowText(ss);
	int start, end;                 // Range of selection of text in control
	GetSel(start, end);

	if (nChar == VK_DELETE && start == end && ss.GetLength() > start+1 &&
			 ss[start+1] == ' ')
		SetSel(start, start+2);
	else if (nChar == VK_LEFT && start == end && start > 0 && ss[start-1] == ' ')
		SetSel(start-1, start-1);

	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);

	// Tidy display if char(s) deleted or caret moved
	GetWindowText(ss);
	if ((nChar == VK_DELETE || nChar == VK_RIGHT || nChar == VK_LEFT) &&
		::GetKeyState(VK_SHIFT) >= 0 && 
		ss.GetLength() > 0)
	{
		add_spaces();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CHexEdit private methods

void CHexEdit::add_spaces()
{
	CString ss;                         // Current string
	GetWindowText(ss);
	int start, end;                     // Current selection (start==end if just caret)
	GetSel(start, end);

	const char *pp;                     // Ptr into orig. (hex digit) string
	int newstart, newend;               // New caret position/selection
	newstart = newend = 0;              // In case of empty string

	// Allocate enough space (allowing for space padding)
	char *out = new char[(ss.GetLength()*(group_by_+1))/group_by_ + 2]; // Where chars are stored
	size_t ii, jj;                      // Number of hex nybbles written/read
	char *dd = out;                     // Ptr to current output char
	int ndigits;                        // Numbers of chars that are part of number

	// If right aligned we need to know how many digits are to be used
	if (right_align_)
	{
		for (ndigits = 0, pp = ss.GetBuffer(0); *pp != '\0'; ++pp)
			if (isxdigit(*pp))
				++ndigits;
		ss.ReleaseBuffer();
	}

	for (pp = ss.GetBuffer(0), ii = jj = 0; /*forever*/; ++pp, ++ii)
	{
		if (ii == start)
			newstart = dd - out;        // save new caret position
		if (ii == end)
			newend = dd - out;

		if (*pp == '\0')
			break;

		// Ignore spaces (and anything else)
		if (!isxdigit(*pp) && *pp != '?')
			continue;                   // ignore all but digits and ?

		if (theApp.hex_ucase_)
			*dd++ = toupper(*pp);       // convert all to upper case
		else
			*dd++ = tolower(*pp);       // convert all to lower case

		++jj;
		if (right_align_)
		{
			if ((ndigits-jj) % group_by_ == 0)
				*dd++ = ' ';                // pad with space at start of next group
		}
		else
		{
			if (jj > 0 && (jj % group_by_) == 0)
				*dd++ = ' ';                // pad with space after every group_by_'th hex digit
		}
	}
	if (dd > out && *(dd-1) == ' ')
//    if (jj > 0 && (jj % group_by_) == 0)
		dd--;                           // Forget last space if added
	*dd = '\0';                         // Terminate string

	SetWindowText(out);
	SetSel(newstart, newend);
	delete[] out;
}

/////////////////////////////////////////////////////////////////////////////
// CDecEdit - subclassed edit control that allows entry of a decimal number

// TBD: this is similar but does less than CDecEditControl (below) - perhaps it should just be an option in CDecEditControl

IMPLEMENT_DYNAMIC(CDecEdit, CEdit)

BEGIN_MESSAGE_MAP(CDecEdit, CEdit)
	ON_WM_CHAR()
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()

CDecEdit::CDecEdit() : allow_neg_(false)
{
	sep_char_ = theApp.dec_sep_char_;
	group_by_ = theApp.dec_group_;
}

/////////////////////////////////////////////////////////////////////////////
// CDecEdit message handlers

void CDecEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CString ss;                     // Current text in control
	GetWindowText(ss);
	int start, end;                 // Range of selection of text in control
	GetSel(start, end);

	CString test_str = CString("\b 0123456789") + sep_char_;

	if (nChar == '\b' && start > 1 && start == end && ss[start-1] == sep_char_)
	{
		// If deleting 1st char of group then also delete preceding sep_char
		SetSel(start-2, end);
		// no return
	}
	else if (nChar == ' ' || nChar == sep_char_)
	{
		// Just ignore these characters
		return;
	}
	else if (allow_neg_ && nChar == '-')
	{
		/*nothing*/ ;
	}
	else if (::isprint(nChar) && strchr(test_str, nChar) == NULL)
	{
#ifdef SYS_SOUNDS
		if (!CSystemSound::Play("Invalid Character"))
#endif
			::Beep(5000,200);
		return;
	}
	CEdit::OnChar(nChar, nRepCnt, nFlags);
	add_commas();
}

void CDecEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CString ss;                     // Current text in control
	GetWindowText(ss);
	int start, end;                 // Range of selection of text in control
	GetSel(start, end);

	if (nChar == VK_DELETE && start == end && ss.GetLength() > start+1 &&
			 ss[start+1] == sep_char_)
		SetSel(start, start+2);
	else if (nChar == VK_LEFT && start == end && start > 0 && ss[start-1] == sep_char_)
		SetSel(start-1, start-1);

	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);

	// Tidy display if char(s) deleted or caret moved
	GetWindowText(ss);
	if ((nChar == VK_DELETE || nChar == VK_RIGHT || nChar == VK_LEFT) &&
		::GetKeyState(VK_SHIFT) >= 0 && 
		ss.GetLength() > 0)
	{
		add_commas();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDecEdit private methods

void CDecEdit::add_commas()
{
	CString ss;                         // Current string
	GetWindowText(ss);
	int start, end;                     // Current selection (start==end if just caret)
	GetSel(start, end);

	const char *pp;                     // Ptr into orig. (decimal number) string
	int newstart, newend;               // New caret position/selection
	newstart = newend = 0;              // In case of empty string

	// Allocate enough space (allowing for comma padding)
	char *out = new char[(ss.GetLength()*(group_by_+1))/group_by_ + 2]; // Where chars are stored
	size_t ii, jj;                      // Number of digits written/read
	char *dd = out;                     // Ptr to current output char
	int ndigits;                        // Numbers of chars that are part of number
	BOOL none_yet = TRUE;               // Have we seen any non-zero digits?

	for (pp = ss.GetBuffer(0), ndigits = 0; *pp != '\0'; ++pp)
		if (isdigit(*pp))
			++ndigits;
	ss.ReleaseBuffer();

	for (pp = ss.GetBuffer(0), ii = jj = 0; /*forever*/; ++pp, ++ii)
	{
		if (ii == start)
			newstart = dd - out;        // save new caret position
		if (ii == end)
			newend = dd - out;

		if (*pp == '\0')
			break;

		if (allow_neg_ && dd == out && *pp == '-')
		{
			*dd++ = '-';
			continue;
		}

		// Ignore spaces (and anything else)
		if (!isdigit(*pp))
			continue;                   // ignore all but digits

		if (++jj < ndigits && none_yet && *pp == '0')
			continue;                   // ignore leading zeroes

		none_yet = FALSE;
		*dd++ = *pp;

		if ((ndigits - jj) % group_by_ == 0)
			*dd++ = sep_char_;          // put in thousands separator
	}
	if (dd > out && ndigits > 0)
		dd--;                           // Forget last comma if added
	*dd = '\0';                         // Terminate string

	SetWindowText(out);
	SetSel(newstart, newend);
	delete[] out;
}


//===========================================================================
/////////////////////////////////////////////////////////////////////////////
// CSearchEditControl

IMPLEMENT_DYNAMIC(CSearchEditControl, CMFCToolBarComboBoxEdit)

CSearchEditControl::CSearchEditControl(CMFCToolBarComboBoxButton & combo) : 
	CMFCToolBarComboBoxEdit(combo)
{
	mode_ = mode_none;
}

BEGIN_MESSAGE_MAP(CSearchEditControl, CMFCToolBarComboBoxEdit)
		//{{AFX_MSG_MAP(CSearchEditControl)
		ON_WM_CHAR()
		ON_WM_SETFOCUS()
		ON_WM_KEYDOWN()
		ON_WM_KILLFOCUS()
	//}}AFX_MSG_MAP
		ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()

void CSearchEditControl::Redisplay()            // Make sure hex digits case OK etc
{
	CString ss;
	GetWindowText(ss);
	if (ss.GetLength() > 0 && ss[0] != sflag_char && ss[0] != iflag_char)
	{
		normalize_hex();
		// Make sure OnUpdate does not set search string back to what it was
		GetWindowText(ss);
//        ((CMainFrame *)AfxGetMainWnd())->SetSearch(ss);
		((CMainFrame *)AfxGetMainWnd())->SetSearchString(ss);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CSearchEditControl message handlers

void CSearchEditControl::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CString ss;
	if (process_char(nChar))
	{
		GetWindowText(ss);
		((CMainFrame *)AfxGetMainWnd())->SetSearch(ss);
		return;
	}

	CMFCToolBarComboBoxEdit::OnChar(nChar, nRepCnt, nFlags);

	// If in hex mode make sure it looks neat
	GetWindowText(ss);
	if (ss.GetLength() > 0 && ss[0] != sflag_char && ss[0] != iflag_char)
	{
		normalize_hex();
		GetWindowText(ss);
	}
	((CMainFrame *)AfxGetMainWnd())->SetSearch(ss);

#ifdef AUTO_COMPLETE_SEARCH
	// Now do search string completion using all strings in history
	int start, end;             // Range of current selection
	GetSel(start, end);         // (start == end means no selection just caret)

	if (::isprint(nChar) && start == ss.GetLength())
	{
		ASSERT(start == end);

		// Valid char typed at end of current text so do completion
		CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();

		// Find matching string from search history
		int count = 0;
		std::vector<CString>::iterator match;
		for (std::vector<CString>::iterator ps = mm->search_hist_.begin();
			 ps != mm->search_hist_.end(); ++ps)
		{
			 if (ss.GetLength() > 0 && ss[0] != sflag_char)
			 {
				 // Case-insensitive comparisons (doing hex or case-insensitive search)
				 if (ss.CompareNoCase(ps->Left(ss.GetLength())) == 0)
				 {
					 match = ps;
					 ++count;
				 }
			 }
			 else
			 {
				 // Case-sensitive comparisons
				 if (ss.Compare(ps->Left(ss.GetLength())) == 0)
				 {
					 match = ps;
					 ++count;
				 }
			 }
		}
		if (count == 1)
		{
			// We autocomplete if exactly one match found but not if the new string is
			// bigger than the edit control can display otherwise the SetSel() call
			// below makes the end of 

			// Work out new bit to add to end
			CString strEnd = match->Mid(ss.GetLength());

			// Work out size of new string in edit control
			CDC *pDC = GetDC();
			CSize str_size = pDC->GetTextExtent(strEnd);
			ReleaseDC(pDC);

			// Work out visible area of edit control
			CRect rct;
			GetRect(&rct);

			if (str_size.cx < rct.Width())
			{
				// The new string fits so complete it
				SetWindowText(ss + strEnd);

				// Select it so user doesn't have to do anything if they don't really want this one
				SetSel(start, -1);

				// Make sure OnUpdate handler doesn't set string back the way it was
				mm->SetSearch(ss + strEnd);
			}
		}
	}
#endif
}

void CSearchEditControl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CString ss;
	GetWindowText(ss);
	int start, end;
	GetSel(start, end);

	if (nChar == VK_DELETE && start == 0 && ss.GetLength() > 0 &&
		(ss[0] == sflag_char || ss[0] == iflag_char))
	{
		// Convert ASCII chars to equiv. hex digits
		if (end > 0)
		{
			SetSel(1, end);
			Clear();
		}
		convert2hex();
		GetWindowText(ss);
		((CMainFrame *)AfxGetMainWnd())->SetSearch(ss);
		return;
	}
	else if (nChar == VK_DELETE && start == end && ss.GetLength() > start+1 &&
			 ss[0] != sflag_char && ss[0] != iflag_char && ss[start+1] == ' ')
		SetSel(start, start+2);
	else if (nChar == VK_LEFT && start == end && start > 0 &&
				ss[0] != sflag_char && ss[0] != iflag_char && ss[start-1] == ' ')
		SetSel(start-1, start-1);

	CMFCToolBarComboBoxEdit::OnKeyDown(nChar, nRepCnt, nFlags);

	// Tidy display if hex and char(s) deleted or caret moved
	GetWindowText(ss);
	if ((nChar == VK_DELETE || nChar == VK_RIGHT || nChar == VK_LEFT) &&
		::GetKeyState(VK_SHIFT) >= 0 && ss.GetLength() > 0 &&
		ss[0] != sflag_char && ss[0] != iflag_char)
	{
		normalize_hex();
		GetWindowText(ss);
	}
	((CMainFrame *)AfxGetMainWnd())->SetSearch(ss);
}

// TBD: xxx OnSet/KillFocus could perhaps be removed and mode changes done more effectively elsewhere
void CSearchEditControl::OnSetFocus(CWnd* pOldWnd)
{
	CString strCurr;
	CMFCToolBarComboBoxEdit::OnSetFocus(pOldWnd);
	char new_text[2] = "?";

	GetWindowText(strCurr);

	if (strCurr.GetLength() > 0)
	{
		if (mode_ == mode_hex && (strCurr[0] == sflag_char || strCurr[0] == iflag_char))
		{
			convert2hex();              // Convert existing chars to hex digits
			SetSel(0,-1);               // Make sure all selected
		}
		else if (mode_ == mode_char && strCurr[0] != sflag_char && strCurr[0] != iflag_char)
			convert2chars();            // Convert hex digits to chars (as many as possible)
		else if (mode_ == mode_icase && strCurr[0] != sflag_char && strCurr[0] != iflag_char)
			convert2chars(iflag_char);  // Convert hex digits to chars (icase search)
		else if (mode_ == mode_char && strCurr[0] == iflag_char)
		{
			SetSel(0,1);
			new_text[0] = sflag_char;
			ReplaceSel(new_text);
			((CMainFrame *)AfxGetMainWnd())->
				SetSearch(CString(char(sflag_char)) + strCurr.Mid(1));
		}
		else if (mode_ == mode_icase && strCurr[0] == sflag_char)
		{
			SetSel(0,1);
			new_text[0] = iflag_char;
			ReplaceSel(new_text);
			((CMainFrame *)AfxGetMainWnd())->
				SetSearch(CString(char(iflag_char)) + strCurr.Mid(1));
		}
		else if (mode_ == mode_none && strCurr[0] == sflag_char)
			mode_ = mode_char;
		else if (mode_ == mode_none && strCurr[0] == iflag_char)
			mode_ = mode_icase;
	}
	else if (mode_ == mode_char)
	{
		// No chars but char search mode so add "="
		new_text[0] = sflag_char;
		SetWindowText(new_text);
		((CMainFrame *)AfxGetMainWnd())->SetSearch(new_text);
	}
	else if (mode_ == mode_icase)
	{
		// No chars but case-insensitive search mode so add "~"
		new_text[0] = iflag_char;
		SetWindowText(new_text);
		((CMainFrame *)AfxGetMainWnd())->SetSearch(new_text);
	}

	if (mode_ == mode_char || mode_ == mode_icase)
		SetSel(1,-1);                   // select all but ' so string can easily be overtyped
}

void CSearchEditControl::OnKillFocus(CWnd* pNewWnd)
{
	CMFCToolBarComboBoxEdit::OnKillFocus(pNewWnd);

	mode_ = mode_none;
}

LRESULT CSearchEditControl::OnCommandHelp(WPARAM, LPARAM lParam)
{
	if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_,
					 HH_HELP_CONTEXT, 0x10000 + ID_SEARCH_COMBO))
		AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
	return 1;                           // Indicate help launched
}

#if 0 // not needed with BCG
UINT CSearchEditControl::OnGetDlgCode()
{
	// Get all keys so that we see CR and Escape
	return CMFCToolBarComboBoxEdit::OnGetDlgCode() | DLGC_WANTALLKEYS;
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CSearchEditControl private methods

BOOL CSearchEditControl::process_char(UINT nChar)
{
	CString ss;
	GetWindowText(ss);          // Current text in control (before key pressed)
	int len = ss.GetLength();   // Length of current text
	int start, end;             // Range of current selection
	GetSel(start, end);         // (start == end means no selection just caret)

	if (nChar == '\b' &&                                       // Backspace ...
		(start == 1 && end == 1 || start == 0 && end > 0) &&   // When after 1st char OR they are included in selection
		(ss[0] == sflag_char || ss[0] == iflag_char))          // And 1st char is '~' or '='
	{
		// Convert ASCII chars to equiv. hex digits
		if (start == 0)
		{
			SetSel(1, end);
			Clear();
		}
		convert2hex();
		return TRUE;
	}
	else if (nChar == '\b' && start > 1 && start == end && ss[start-1] == ' ' &&
			 ss[0] != sflag_char && ss[0] != iflag_char)
	{
		// If deleting 1st nybble (hex mode) then also delete preceding space
		SetSel(start-2, end);
	}
	else if (nChar == sflag_char && start == 0 && len > 0 &&        // '=' typed when at start
				(ss[0] == iflag_char || ss[0] == sflag_char))       // and doing char search
	{
		// Make search case-sensitive
		char new_text[2] = "?";
		new_text[0] = sflag_char;
		if (end == 0) SetSel(0,1);
		ReplaceSel(new_text);
		return TRUE;
	}
	else if (nChar == iflag_char && start == 0 && len > 0 &&        // '~' typed when at start
				(ss[0] == iflag_char || ss[0] == sflag_char))       // and doing char search
	{
		// Make search case-insensitive
		char new_text[2] = "?";
		new_text[0] = iflag_char;
		if (end == 0) SetSel(0,1);
		ReplaceSel(new_text);
		return TRUE;
	}
	else if ((nChar == sflag_char || nChar == iflag_char) &&
			 start == 0 && len > 0 &&
			 ss[0] != sflag_char && ss[0] != iflag_char )
	{
		// =/~ at start and not one there already
		Clear();
		convert2chars(nChar);
		SetSel(1,1);
		return TRUE;
	}
	else if (nChar == ' ' && len > 0 &&
			 ss[0] != sflag_char && ss[0] != iflag_char )
	{
		// Ignore spaces if in hex mode (tend to type them because of display)
		return TRUE;
	}
	else if (::isprint(nChar))
	{
		// Chars allowed in empty edit (ie start of text search or a hex digit)
		char valid_empty[] = "??0123456789ABCDEFabcdef";
		valid_empty[0] = sflag_char;
		valid_empty[1] = iflag_char;

		// Disallow the following (where ASCII mode is where ss[0] == iflag or sflag)
		// - any char at start (except sflag/iflag/DEL, handled above) if ASCII mode
		// - any char except sflag/iflag/hex-digit if field empty
		// - non-hexdigit (except BS) if not ASCII mode
		if ((start == 0 && len > 0 && (ss[0]==sflag_char||ss[0]==iflag_char)) ||
			(len == 0 && strchr(valid_empty, nChar) == NULL) ||
			(len > 0 && ss[0] != sflag_char && ss[0] != iflag_char &&
						strchr("\b0123456789ABCDEFabcdef", nChar) == NULL) )
		{
#ifdef SYS_SOUNDS
			if (!CSystemSound::Play("Invalid Character"))
#endif
				::Beep(5000,200);
			return TRUE;
		}
	}

	return FALSE;
}

void CSearchEditControl::normalize_hex()
{
	CString ss;
	GetWindowText(ss);
	int start, end;
	GetSel(start, end);

	const char *pp;                     // Ptr into orig. (hex digit) string
	int newstart, newend;               // New caret position/selection
	newstart = newend = 0;              // In case of empty text string

	// Allocate enough space (allowing for space padding)
	char *out = new char[(ss.GetLength()*3)/2+2]; // Where chars are stored
	size_t ii, jj;                      // Number of hex nybbles written/read
	char *dd = out;                     // Ptr to current output char

	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

	for (pp = ss.GetBuffer(0), ii = jj = 0; /*forever*/; ++pp, ++ii)
	{
		if (ii == start)
			newstart = dd - out;        // save new caret position
		if (ii == end)
			newend = dd - out;

		if (*pp == '\0')
			break;

		// Ignore spaces (and anything else)
		if (!isxdigit(*pp))
			continue;                   // ignore all but hex digits

		if (aa->hex_ucase_)
			*dd++ = toupper(*pp);       // convert all to upper case
		else
			*dd++ = tolower(*pp);       // convert all to lower case

		if ((++jj % 2) == 0)
			*dd++ = ' ';                // pad with space after 2 nybbles
	}
	if ((jj % 2) == 0 && jj > 0)
		dd--;                   // Remove trailing space
	*dd = '\0';                 // Terminate string

	SetWindowText(out);
	SetSel(newstart, newend);
	delete[] out;
}

void CSearchEditControl::convert2hex()
{
	BOOL bad_chars = FALSE;
	BOOL ebcdic = FALSE;
	CString ss;

	GetWindowText(ss);
	if (ss.GetLength() == 0)
		return;

	if (ss[0] != sflag_char && ss[0] != iflag_char)
	{
		convert2chars();                // -> chars then -> hex to normalize
		GetWindowText(ss);
	}

	const char *pp;                     // Ptr into input (ASCII char) string
	char *out = new char[ss.GetLength()*3+1]; // Where output chars are stored
	char *dd = out;                     // Ptr to current output char

	// Display hex in upper or lower case?
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	const char *hex_fmt;
	if (aa->hex_ucase_)
		hex_fmt = "%02X ";
	else
		hex_fmt = "%02x ";

	// If view is displaying EBCDIC chars then convert to hex assuming EBCDIC chars
	ASSERT(GetView() != NULL);
	if (GetView()->EbcdicMode())
		ebcdic = TRUE;

	for (pp = ss.GetBuffer(0) + 1; *pp != '\0'; ++pp)
	{
		if (ebcdic && *pp < 128 && *pp >= 0 && a2e_tab[*pp] != '\0')
		{
			sprintf(dd, hex_fmt, a2e_tab[(unsigned char)*pp]);
			dd += 3;
		}
		else if (ebcdic)
			bad_chars = TRUE;
		else
		{
			sprintf(dd, hex_fmt, (unsigned char)*pp);
			dd += 3;
		}
	}
	if (dd > out) dd--;                 // Forget trailing space
	*dd = '\0';                         // Terminate string

	SetWindowText(out);
	((CMainFrame *)AfxGetMainWnd())->SetSearch(out);
	delete[] out;

	if (bad_chars)
	{
		((CMainFrame *)AfxGetMainWnd())->
			StatusBarText("Hex search: invalid EBCDIC chars ignored");
	}
}

void CSearchEditControl::convert2chars(char c1)
{
	BOOL bad_chars = FALSE;
	BOOL ebcdic = FALSE;
	CString ss;

	GetWindowText(ss);
	if (ss.GetLength() == 0)
	{
		char new_text[2] = "?";
		new_text[0] = c1;
		SetWindowText(new_text);
		((CMainFrame *)AfxGetMainWnd())->SetSearch(new_text);
		return;
	}

	if (ss[0] == sflag_char || ss[0] == iflag_char)
	{
		convert2hex();                  // -> hex then -> chars to normalize
		GetWindowText(ss);
	}

	const char *pp;                     // Ptr to input (hex digit) string

	char *out = new char[ss.GetLength()/2+3]; // Where output chars are stored
	size_t length;                      // Number of hex output nybbles generated
	char *dd = out;                     // Ptr to current output char

	// If view is displaying EBCDIC chars then convert hex to EBCDIC
	ASSERT(GetView() != NULL);
	if (GetView()->EbcdicMode())
		ebcdic = TRUE;

	*(dd++) = c1;                       // Indicate char string (1st char == ' OR *)
	for (pp = ss.GetBuffer(0), length = 0, *dd = '\0'; *pp != '\0'; ++pp)
	{
		// Ignore spaces (and anything else)
		if (!isxdigit(*pp))
			continue;

		*dd = (*dd<<4) + (isdigit(*pp) ? *pp - '0' : toupper(*pp) - 'A' + 10);

		if ((++length % 2) == 0)
		{
			if (ebcdic && (*dd = e2a_tab[(unsigned char)*dd]) != '\0')
				++dd;                   // Valid EBCDIC so move to next byte
			else if (!ebcdic && (unsigned char)*dd > '\r')
				++dd;                   // Printable ASCII so move to next byte
			else
			{
				length -= 2;            // Forget byte (2 hex digits) just seen
				bad_chars = TRUE;
			}
			*(dd) = '\0';               // Zero next byte
		}
	}
	if ((length % 2) != 0  && *dd == '\0')  // WAS ... && !isprint(*dd))
		length--;                       // Solo last hex digit -> not printable char
	if (length > 0)
		length = (length-1)/2 + 1;      // Convert nybble count to byte count
	out[length+1] = '\0';

	SetWindowText(out);
	((CMainFrame *)AfxGetMainWnd())->SetSearch(out);
	delete[] out;

	if (ebcdic && bad_chars)
		((CMainFrame *)AfxGetMainWnd())->StatusBarText("EBCDIC search: invalid characters ignored");
	else if (bad_chars)
		((CMainFrame *)AfxGetMainWnd())->StatusBarText("ASCII search: control characters ignored");
}

/////////////////////////////////////////////////////////////////////////////
// CSearchEditControl class (static methods)

// This finds the first visible search tool (in toolbars), 
// changes to specified search mode and sets focus to it.
void CSearchEditControl::BeginSearch(enum mode_t mode)
{
	CObList listButtons;

	// Search all toolbars and jump to the first visible search tool found (can be more than one)
	if (CMFCToolBar::GetCommandButtons(ID_SEARCH_COMBO, listButtons) > 0)
	{
		for (POSITION posCombo = listButtons.GetHeadPosition(); posCombo != NULL; )
		{
			CFindComboButton* pCombo = 
				DYNAMIC_DOWNCAST(CFindComboButton, listButtons.GetNext(posCombo));
			ASSERT(pCombo != NULL);

			CSearchEditControl * pedit = dynamic_cast<CSearchEditControl *>(pCombo->GetEditCtrl());
			ASSERT(pedit != NULL);
			if (pedit->IsWindowVisible())
			{
				// We found (first) one so change its mode and set focus
				pedit->SetMode(mode);
				pedit->SetFocus();
				break;
			}
		}
	}
}

// Finds all search tools and forces them to redraw
void CSearchEditControl::RedisplayAll()
{
	CObList listButtons;
	if (CMFCToolBar::GetCommandButtons(ID_SEARCH_COMBO, listButtons) > 0)
	{
		for (POSITION posCombo = listButtons.GetHeadPosition ();
			posCombo != NULL; )
		{
			CFindComboButton* pCombo = 
				DYNAMIC_DOWNCAST(CFindComboButton, listButtons.GetNext(posCombo));
			ASSERT(pCombo != NULL);

			CSearchEditControl * pedit = dynamic_cast<CSearchEditControl *>(pCombo->GetEditCtrl());
			ASSERT(pedit != NULL);
			pedit->Redisplay();
		}
	}
}

//===========================================================================
/////////////////////////////////////////////////////////////////////////////
// CFindComboBox

IMPLEMENT_DYNCREATE(CFindComboBox, CComboBox)

BEGIN_MESSAGE_MAP(CFindComboBox, CComboBox)
	ON_CONTROL_REFLECT(CBN_SELCHANGE, OnSelchange)
END_MESSAGE_MAP()

void CFindComboBox::SetSearchString()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	CString ss;
	GetWindowText(ss);

	if (ss.GetLength() > 0 && 
		ss[0] != CSearchEditControl::sflag_char && 
		ss[0] != CSearchEditControl::iflag_char)
	{
		if (aa->hex_ucase_)
			ss.MakeUpper();
		else
			ss.MakeLower();
	}
	((CMainFrame *)AfxGetMainWnd())->SetSearch(ss);
}

/////////////////////////////////////////////////////////////////////////////
// CFindComboBox message handlers

// CBN_SELCHANGE message reflected back to the control
void CFindComboBox::OnSelchange()
{
	SetSearchString();
}

//===========================================================================
/////////////////////////////////////////////////////////////////////////////
// CFindComboButton

IMPLEMENT_SERIAL(CFindComboButton, CMFCToolBarComboBoxButton, 1)

CComboBox *CFindComboButton::CreateCombo(CWnd* pWndParent, const CRect& rect)
{
	CFindComboBox *pcombo = new CFindComboBox;

	if (!pcombo->Create(m_dwStyle, rect, pWndParent, m_nID))
	{
		delete pcombo;
		return NULL;
	}

	//pcombo->AddString("");  // This stops BCG restoring combo since we do it ourselves in OnUpdate
	SetDropDownHeight(400);

	return pcombo;
}

CSearchEditControl * CFindComboButton::CreateEdit(CWnd* pWndParent, const CRect& rect, DWORD dwEditStyle)
{
	CSearchEditControl * pWndEdit = new CSearchEditControl(*this);

	if (!pWndEdit->Create(dwEditStyle, rect, pWndParent, m_nID))
	{
		delete pWndEdit;
		return NULL;
	}

	return pWndEdit;
}

BOOL CFindComboButton::NotifyCommand(int iNotifyCode)
{
	if (m_pWndCombo->GetSafeHwnd() == 0)
		return FALSE;

	if (iNotifyCode == CBN_EDITCHANGE)
		return TRUE;
	else
		return CMFCToolBarComboBoxButton::NotifyCommand(iNotifyCode);
}

//===========================================================================
/////////////////////////////////////////////////////////////////////////////
// CHexEditControl

IMPLEMENT_DYNAMIC(CHexEditControl, CMFCToolBarComboBoxEdit)

CHexEditControl::CHexEditControl(CMFCToolBarComboBoxButton & combo) : 
	CMFCToolBarComboBoxEdit(combo)
{
	m_brush.CreateSolidBrush(RGB(255,255,255));
}

BEGIN_MESSAGE_MAP(CHexEditControl, CMFCToolBarComboBoxEdit)
		//{{AFX_MSG_MAP(CHexEditControl)
		ON_WM_CHAR()
		ON_WM_KEYDOWN()
	ON_WM_CTLCOLOR_REFLECT()
	//}}AFX_MSG_MAP
		ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()

void CHexEditControl::Redisplay()            // Make sure hex digits case OK etc
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	CString ss;
	GetWindowText(ss);
	DWORD sel = GetSel();
	if (aa->hex_ucase_)
		ss.MakeUpper();
	else
		ss.MakeLower();
	SetWindowText(ss);
	SetSel(sel);
}

/////////////////////////////////////////////////////////////////////////////
// CHexEditControl message handlers

void CHexEditControl::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CString ss;                     // Current text in control
	GetWindowText(ss);
	int start, end;                 // Range of selection of text in control
	GetSel(start, end);

	bool is_num(false);             // Is this a simple number or an expression?
	const char *test_str = "\b 0123456789abcdefABCDEF";

	// Check if text (current text and char just entered) is just an hex int
	if (strspn(ss, test_str) == ss.GetLength() && strchr(test_str, nChar) != NULL)
	{
		// Just a hex number (possibly with spaces)
		is_num = true;
		if (nChar == '\b' && start > 1 && start == end && ss[start-1] == ' ')
		{
			// If deleting 1st nybble (hex mode) then also delete preceding space
			SetSel(start-2, end);
			// no return
		}
		else if (nChar == ' ')
		{
			// Just ignore spaces
			return;
		}
		else if (::isprint(nChar) && strchr(test_str, nChar) == NULL)
		{
#ifdef SYS_SOUNDS
			if (!CSystemSound::Play("Invalid Character"))
#endif
				::Beep(5000,200);
			return;
		}
	}
	CMFCToolBarComboBoxEdit::OnChar(nChar, nRepCnt, nFlags);
	if (is_num)
		add_spaces();

	GetWindowText(ss);
	((CMainFrame *)AfxGetMainWnd())->SetHexAddress(ss);
}

void CHexEditControl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CString ss;                     // Current text in control
	GetWindowText(ss);
	int start, end;                 // Range of selection of text in control
	GetSel(start, end);

	bool is_num(false);             // Is this a simple number or an expression?
	const char *test_str = " 0123456789abcdefABCDEF";

	// Check if text (current text and char just entered) is just an hex int
	if (strspn(ss, test_str) == ss.GetLength())
	{
		// Just a hex number (possibly with spaces)
		is_num = true;

		if (nChar == VK_DELETE && start == end && ss.GetLength() > start+1 &&
				 ss[start+1] == ' ')
			SetSel(start, start+2);
		else if (nChar == VK_LEFT && start == end && start > 0 && ss[start-1] == ' ')
			SetSel(start-1, start-1);
	}

	CMFCToolBarComboBoxEdit::OnKeyDown(nChar, nRepCnt, nFlags);

	// Tidy display if char(s) deleted or caret moved
	GetWindowText(ss);
	if (is_num &&
		(nChar == VK_DELETE || nChar == VK_RIGHT || nChar == VK_LEFT) &&
		::GetKeyState(VK_SHIFT) >= 0 && 
		ss.GetLength() > 0)
	{
		add_spaces();
		GetWindowText(ss);
	}
	((CMainFrame *)AfxGetMainWnd())->SetHexAddress(ss);
}

HBRUSH CHexEditControl::CtlColor(CDC* pDC, UINT nCtlColor)
{
	if (nCtlColor == CTLCOLOR_EDIT)
		pDC->SetTextColor(::GetHexAddrCol());

	return m_brush;
}

LRESULT CHexEditControl::OnCommandHelp(WPARAM, LPARAM lParam)
{
	// Since there is only one control of this type just call help with its help ID
	if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_,
					 HH_HELP_CONTEXT, 0x10000 + ID_JUMP_HEX_COMBO))
		AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
	return 1;                           // Indicate help launched
}

/////////////////////////////////////////////////////////////////////////////
// CHexEditControl private methods

void CHexEditControl::add_spaces()
{
	CString ss;                         // Current string
	GetWindowText(ss);
	int start, end;                     // Current selection (start==end if just caret)
	GetSel(start, end);

	const char *pp;                     // Ptr into orig. (hex digit) string
	int newstart, newend;               // New caret position/selection
	newstart = newend = 0;              // In case of empty string

	// Allocate enough space (allowing for space padding)
	char *out = new char[(ss.GetLength()*3)/2+1]; // Where chars are stored
	size_t ii, jj;                      // Number of hex nybbles written/read
	char *dd = out;                     // Ptr to current output char
	int ndigits;                        // Numbers of chars that are part of number

	for (pp = ss.GetBuffer(0), ndigits = 0; *pp != '\0'; ++pp)
		if (isxdigit(*pp))
			++ndigits;
	ss.ReleaseBuffer();

	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

	for (pp = ss.GetBuffer(0), ii = jj = 0; /*forever*/; ++pp, ++ii)
	{
		if (ii == start)
			newstart = dd - out;        // save new caret position
		if (ii == end)
			newend = dd - out;

		if (*pp == '\0')
			break;

		// Ignore spaces (and anything else)
		if (!isxdigit(*pp))
			continue;                   // ignore all but digits

		if (aa->hex_ucase_)
			*dd++ = toupper(*pp);       // convert all to upper case
		else
			*dd++ = tolower(*pp);       // convert all to lower case

		++jj;
		if ((ndigits - jj) % 4 == 0)
			*dd++ = ' ';                // pad with space after every 4th hex digit
	}
	if (dd > out)
		dd--;                           // Forget last comma if added
	*dd = '\0';                         // Terminate string

	SetWindowText(out);
	SetSel(newstart, newend);
	delete[] out;
}

/////////////////////////////////////////////////////////////////////////////
// CHexEditControl class (static) methods

void CHexEditControl::BeginJump()
{
	CObList listButtons;

	// Search all toolbars and jump to the first visible hex address tool found (can be more than one)
	if (CMFCToolBar::GetCommandButtons(ID_JUMP_HEX_COMBO, listButtons) > 0)
	{
		for (POSITION posCombo = listButtons.GetHeadPosition();
			posCombo != NULL; )
		{
			CHexComboButton* pCombo =
				DYNAMIC_DOWNCAST(CHexComboButton, listButtons.GetNext(posCombo));
			ASSERT(pCombo != NULL);
			CHexEditControl * pedit = dynamic_cast<CHexEditControl *>(pCombo->GetEditCtrl());
			// In MFC9 the visible edit box is the sibling of the parent combo box!??!
			CWnd *pwnd = pedit->GetParent();
			ASSERT(pwnd != NULL);
			pedit = DYNAMIC_DOWNCAST(CHexEditControl, pwnd->GetWindow(GW_HWNDNEXT));
			ASSERT(pedit != NULL);
			if (pedit->IsWindowVisible())
			{
				pedit->SetFocus();
				break;
			}
		}
	}
}

void CHexEditControl::RedisplayAll()
{
	CObList listButtons;
	if (CMFCToolBar::GetCommandButtons(ID_JUMP_HEX_COMBO, listButtons) > 0)
	{
		for (POSITION posCombo = listButtons.GetHeadPosition ();
			posCombo != NULL; )
		{
			CHexComboButton* pCombo = 
				DYNAMIC_DOWNCAST(CHexComboButton, listButtons.GetNext(posCombo));
			ASSERT(pCombo != NULL);

			CHexEditControl * pedit = dynamic_cast<CHexEditControl *>(pCombo->GetEditCtrl());
			ASSERT(pedit != NULL);
			pedit->Redisplay();
		}
	}
}

//===========================================================================
/////////////////////////////////////////////////////////////////////////////
// CHexComboBox

IMPLEMENT_DYNCREATE(CHexComboBox, CComboBox)

BEGIN_MESSAGE_MAP(CHexComboBox, CComboBox)
	ON_CONTROL_REFLECT(CBN_SELENDOK, OnSelendok)
	ON_CONTROL_REFLECT(CBN_SELCHANGE, OnSelchange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHexComboBox message handlers

// CBN_SELCHANGE message reflected back to the control
void CHexComboBox::OnSelchange()
{
	CString ss;
	GetWindowText(ss);
	((CMainFrame *)AfxGetMainWnd())->SetHexAddress(ss);
}

// xxx remove this?
// For some reason CBN_SELCHANGE does not seem to be reflected
// so also do it for CBN_SELENDOK
void CHexComboBox::OnSelendok()
{
	CString ss;
	GetWindowText(ss);
	((CMainFrame *)AfxGetMainWnd())->SetHexAddress(ss);
}

//===========================================================================
/////////////////////////////////////////////////////////////////////////////
// CHexComboButton

IMPLEMENT_SERIAL(CHexComboButton, CMFCToolBarComboBoxButton, 1)

CComboBox *CHexComboButton::CreateCombo(CWnd* pWndParent, const CRect& rect)
{
	CHexComboBox *pcombo = new CHexComboBox;

	if (!pcombo->Create(m_dwStyle, rect, pWndParent, m_nID))
	{
		delete pcombo;
		return NULL;
	}

	SetDropDownHeight(400);

	return pcombo;
}

CHexEditControl * CHexComboButton::CreateEdit(CWnd* pWndParent, const CRect& rect, DWORD dwEditStyle)
{
	CHexEditControl * pWndEdit = new CHexEditControl(*this);

	if (!pWndEdit->Create(dwEditStyle, rect, pWndParent, m_nID))
	{
		delete pWndEdit;
		return NULL;
	}

	return pWndEdit;
}

BOOL CHexComboButton::NotifyCommand(int iNotifyCode)
{
	if (m_pWndCombo->GetSafeHwnd() == NULL)
		return FALSE;

	if (iNotifyCode == CBN_EDITCHANGE)
		return TRUE;
	else
		return CMFCToolBarComboBoxButton::NotifyCommand(iNotifyCode);
}

//===========================================================================
/////////////////////////////////////////////////////////////////////////////
// CDecEditControl - similar to CDecEdit but if the string is not a number it does nothing

IMPLEMENT_DYNAMIC(CDecEditControl, CMFCToolBarComboBoxEdit)

CDecEditControl::CDecEditControl(CMFCToolBarComboBoxButton& combo) :
	CMFCToolBarComboBoxEdit(combo)
{
	sep_char_ = theApp.dec_sep_char_;
	group_ = theApp.dec_group_;

	allow_neg_ = false;

	m_brush.CreateSolidBrush(RGB(255,255,255));
}

BEGIN_MESSAGE_MAP(CDecEditControl, CMFCToolBarComboBoxEdit)
		//{{AFX_MSG_MAP(CDecEditControl)
		ON_WM_CHAR()
		ON_WM_KEYDOWN()
	ON_WM_CTLCOLOR_REFLECT()
	//}}AFX_MSG_MAP
		ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDecEditControl message handlers

void CDecEditControl::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CString ss;                     // Current text in control
	GetWindowText(ss);
	int start, end;                 // Range of selection of text in control
	GetSel(start, end);

	bool is_num(false);             // Is this a simple number or an expression?
	CString test_str = CString("\b 0123456789") + sep_char_;

	// Check if text (current text and char just entered) is just a number
	if (strspn(ss, test_str) == ss.GetLength() && strchr(test_str, nChar) != NULL)
	{
		// Just a (decimal) number possibly with separators (commas)
		is_num = true;
		if (nChar == '\b' && start > 1 && start == end && ss[start-1] == sep_char_)
		{
			// If deleting 1st char of group then also delete preceding sep_char
			SetSel(start-2, end);
			// no return
		}
		else if (nChar == ' ' || nChar == sep_char_)
		{
			// Just ignore these characters
			return;
		}
		else if (allow_neg_ && nChar == '-')
		{
			/*nothing*/ ;
		}
		else if (::isprint(nChar) && strchr(test_str, nChar) == NULL)
		{
#ifdef SYS_SOUNDS
			 if (!CSystemSound::Play("Invalid Character"))
#endif
				::Beep(5000,200);
			return;
		}
	}
	CMFCToolBarComboBoxEdit::OnChar(nChar, nRepCnt, nFlags);
	if (is_num)
		add_commas();

	GetWindowText(ss);
	((CMainFrame *)AfxGetMainWnd())->SetDecAddress(ss);
}

void CDecEditControl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CString ss;                     // Current text in control
	GetWindowText(ss);
	int start, end;                 // Range of selection of text in control
	GetSel(start, end);

	bool is_num(false);             // Is this a simple number or an expression?
	CString test_str = CString(" 0123456789") + sep_char_;

	// Check if text (current text and char just entered) is entirely a decimal int
	if (strspn(ss, test_str) == ss.GetLength())
	{
		is_num = true;
		if (nChar == VK_DELETE && start == end && ss.GetLength() > start+1 &&
				 ss[start+1] == sep_char_)
			SetSel(start, start+2);
		else if (nChar == VK_LEFT && start == end && start > 0 && ss[start-1] == sep_char_)
			SetSel(start-1, start-1);
	}

	CMFCToolBarComboBoxEdit::OnKeyDown(nChar, nRepCnt, nFlags);

	// Tidy display if char(s) deleted or caret moved
	GetWindowText(ss);
	if (is_num &&
		(nChar == VK_DELETE || nChar == VK_RIGHT || nChar == VK_LEFT) &&
		::GetKeyState(VK_SHIFT) >= 0 && 
		ss.GetLength() > 0)
	{
		add_commas();
		GetWindowText(ss);
	}
	((CMainFrame *)AfxGetMainWnd())->SetDecAddress(ss);
}

HBRUSH CDecEditControl::CtlColor(CDC* pDC, UINT nCtlColor)
{
	if (nCtlColor == CTLCOLOR_EDIT)
		pDC->SetTextColor(::GetDecAddrCol());

	return m_brush;
}

LRESULT CDecEditControl::OnCommandHelp(WPARAM, LPARAM lParam)
{
	// Since there is only one control of this type just call help with its help ID
	if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT,
					 0x10000 + ID_JUMP_DEC_COMBO))
		AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
	return 1;                           // Indicate help launched
}

/////////////////////////////////////////////////////////////////////////////
// CDecEditControl private methods

void CDecEditControl::add_commas()
{
	CString ss;                         // Current string
	GetWindowText(ss);
	int start, end;                     // Current selection (start==end if just caret)
	GetSel(start, end);

	const char *pp;                     // Ptr into orig. (decimal number) string
	int newstart, newend;               // New caret position/selection
	newstart = newend = 0;              // In case of empty string

	// Allocate enough space (allowing for comma padding)
	char *out = new char[(ss.GetLength()*(group_+1))/group_ + 2]; // Where chars are stored
	size_t ii, jj;                      // Number of digits written/read
	char *dd = out;                     // Ptr to current output char
	int ndigits;                        // Numbers of chars that are part of number
	BOOL none_yet = TRUE;               // Have we seen any non-zero digits?

	for (pp = ss.GetBuffer(0), ndigits = 0; *pp != '\0'; ++pp)
		if (isdigit(*pp))
			++ndigits;
	ss.ReleaseBuffer();

	for (pp = ss.GetBuffer(0), ii = jj = 0; /*forever*/; ++pp, ++ii)
	{
		if (ii == start)
			newstart = dd - out;        // save new caret position
		if (ii == end)
			newend = dd - out;

		if (*pp == '\0')
			break;

		if (allow_neg_ && dd == out && *pp == '-')
		{
			*dd++ = '-';
			continue;
		}

		// Ignore spaces (and anything else)
		if (!isdigit(*pp))
			continue;                   // ignore all but digits

		if (++jj < ndigits && none_yet && *pp == '0')
			continue;                   // ignore leading zeroes

		none_yet = FALSE;
		*dd++ = *pp;

		if ((ndigits - jj) % group_ == 0)
			*dd++ = sep_char_;          // put in thousands separator
	}
	if (dd > out && ndigits > 0)
		dd--;                           // Forget last comma if added
	*dd = '\0';                         // Terminate string

	SetWindowText(out);
	SetSel(newstart, newend);
	delete[] out;
}

//===========================================================================
/////////////////////////////////////////////////////////////////////////////
// CDecComboBox

IMPLEMENT_DYNCREATE(CDecComboBox, CComboBox)

BEGIN_MESSAGE_MAP(CDecComboBox, CComboBox)
	ON_CONTROL_REFLECT(CBN_SELENDOK, OnSelendok)
	ON_CONTROL_REFLECT(CBN_SELCHANGE, OnSelchange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDecComboBox message handlers

// CBN_SELCHANGE message reflected back to the control
void CDecComboBox::OnSelchange()
{
	CString ss;
	GetWindowText(ss);
	((CMainFrame *)AfxGetMainWnd())->SetDecAddress(ss);
}

// xxx remove this?
// For some reason CBN_SELCHANGE does not seem to be reflected
// so also do it for CBN_SELENDOK
void CDecComboBox::OnSelendok()
{
	CString ss;
	GetWindowText(ss);
	((CMainFrame *)AfxGetMainWnd())->SetDecAddress(ss);
}

//===========================================================================
/////////////////////////////////////////////////////////////////////////////
// CDecComboButton

IMPLEMENT_SERIAL(CDecComboButton, CMFCToolBarComboBoxButton, 1)

CComboBox *CDecComboButton::CreateCombo(CWnd* pWndParent, const CRect& rect)
{
	CDecComboBox *pcombo = new CDecComboBox;

	if (!pcombo->Create(m_dwStyle, rect, pWndParent, m_nID))
	{
		delete pcombo;
		return NULL;
	}
//    pcombo->AddString("");  // This stops BCG restoring combo since we do it ourselves in OnUpdate
	SetDropDownHeight(400);

	return pcombo;
}

CDecEditControl * CDecComboButton::CreateEdit(CWnd* pWndParent, const CRect& rect, DWORD dwEditStyle)
{
	CDecEditControl * pWndEdit = new CDecEditControl(*this);

	if (!pWndEdit->Create(dwEditStyle, rect, pWndParent, m_nID))
	{
		delete pWndEdit;
		return NULL;
	}

	return pWndEdit;
}

BOOL CDecComboButton::NotifyCommand(int iNotifyCode)
{
	if (m_pWndCombo->GetSafeHwnd() == NULL)
		return FALSE;

	if (iNotifyCode == CBN_EDITCHANGE)
		return TRUE;
	else
		return CMFCToolBarComboBoxButton::NotifyCommand(iNotifyCode);
}

//===========================================================================
/////////////////////////////////////////////////////////////////////////////
// CBookmarksComboBox

IMPLEMENT_DYNCREATE(CBookmarksComboBox, CComboBox)

BEGIN_MESSAGE_MAP(CBookmarksComboBox, CComboBox)
	ON_CONTROL_REFLECT(CBN_SELCHANGE, OnSelchange)
//	ON_WM_DRAWITEM()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBookmarksComboBox message handlers

void CBookmarksComboBox::OnSelchange()
{
	CHexEditView *pview = GetView();
	if (pview == NULL)
		return;

	//pbl->GoTo(GetItemData(GetCurSel()));  // no longer storing bookmark index in the list item data

	// Get the name of the selected bookmark
	CString ss;
	GetLBText(GetCurSel(), ss);

	// Now we need to search the bookmarks of the active file to get the absolute bookmark index
	CHexEditDoc * pdoc = (CHexEditDoc *)pview->GetDocument();
	CBookmarkList * pbl = theApp.GetBookmarkList();
	ASSERT(pdoc != NULL && pbl != NULL);
	for (int ii = 0; ii < (int)pdoc->bm_index_.size(); ++ii)
		if (pbl->name_[pdoc->bm_index_[ii]] == ss)
		{
			// Found - so jump to it
			pbl->GoTo(pdoc->bm_index_[ii]);
			pview->SetFocus();
			break;
		}
}

#if 0
void CBookmarksComboBox::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	// TODO: Add your message handler code here and/or call default

	CComboBox::OnDrawItem(nIDCtl, lpDrawItemStruct);
}
#endif

//===========================================================================
/////////////////////////////////////////////////////////////////////////////
IMPLEMENT_SERIAL(CBookmarksComboButton, CMFCToolBarComboBoxButton, 1)

CComboBox *CBookmarksComboButton::CreateCombo(CWnd* pWndParent, const CRect& rect)
{
	CBookmarksComboBox *pcombo = new CBookmarksComboBox;

	if (!pcombo->Create(m_dwStyle, rect, pWndParent, m_nID))
	{
		delete pcombo;
		return NULL;
	}

	pcombo->AddString("");  // This stops BCG restoring combo since we do it ourselves in OnUpdate
	SetDropDownHeight(400);

	return pcombo;
}

//===========================================================================
/////////////////////////////////////////////////////////////////////////////
// CStatBar

#if 0
BOOL CStatBar::PreTranslateMessage(MSG* pMsg)
{
	if (ttip_.m_hWnd != NULL)
	{
		switch(pMsg->message)
		{
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
			// this will reactivate the tooltip
			ttip_.Activate(TRUE);
			ttip_.RelayEvent(pMsg);
			break;
		}
	}

	return CMFCStatusBar::PreTranslateMessage(pMsg);
}
#endif

BEGIN_MESSAGE_MAP(CStatBar, CMFCStatusBar)
		//{{AFX_MSG_MAP(CStatBar)
		ON_WM_LBUTTONDBLCLK()
		ON_WM_RBUTTONDOWN()
		ON_WM_CONTEXTMENU()
		ON_WM_CREATE()
		ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

#if 0
void CStatBar::add_tooltip(UINT id, const char *ss)
{
	int index;                  // Index of status bar pane
	CRect rect;                 // Rectangle (within status bar) of pane

	index = CommandToIndex(id);
	ASSERT(index > 0);
	GetItemRect(index, &rect);

	// rect may be empty if the window is too small to show this pane but
	// AddTool seems to handle this OK (ie. never shows the tip).
	ttip_.AddTool(this, ss, rect, id);
}

void CStatBar::move_tooltip(UINT id)
{
	int index;                  // Index of status bar pane
	CRect rect;                 // Rectangle (within status bar) of pane

	index = CommandToIndex(id);
	ASSERT(index > 0);
	GetItemRect(index, &rect);
	ttip_.SetToolRect(this, id, rect);
}
#endif

void CStatBar::SetToolTips()
{
	ASSERT(m_hWnd != 0);
	SetTipText(CommandToIndex(ID_INDICATOR_COMPARES), "Differences found (or compare progress)");
	SetTipText(CommandToIndex(ID_INDICATOR_OCCURRENCES), "Occurrences of search item (or search progress)");
	SetTipText(CommandToIndex(ID_INDICATOR_VALUES), "Hex,Dec,Octal,Binary,Char values of byte at cursor");
	SetTipText(CommandToIndex(ID_INDICATOR_HEX_ADDR), "Distance from mark to cursor (in hex)");
	SetTipText(CommandToIndex(ID_INDICATOR_DEC_ADDR), "Distance from mark to cursor (in decimal)");
	SetTipText(CommandToIndex(ID_INDICATOR_FILE_LENGTH), "File length [+ bytes inserted]");
	SetTipText(CommandToIndex(ID_INDICATOR_BIG_ENDIAN), "Little/Big Endian mode");
	SetTipText(CommandToIndex(ID_INDICATOR_READONLY), "Read-Only/Read-Write mode");
	SetTipText(CommandToIndex(ID_INDICATOR_OVR), "Overtype/Insert mode");
	SetTipText(CommandToIndex(ID_INDICATOR_REC), "Macro Recording");
	SetTipText(CommandToIndex(ID_INDICATOR_CAPS), "Caps Lock");
	SetTipText(CommandToIndex(ID_INDICATOR_NUM), "Num Lock");
}

/////////////////////////////////////////////////////////////////////////////
// CStatBar message handlers

int CStatBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMFCStatusBar::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}

void CStatBar::OnSize(UINT nType, int cx, int cy)
{
	CMFCStatusBar::OnSize(nType, cx, cy);

#if 0
	if (cx > 0 && cy > 0 && ttip_.m_hWnd == NULL)
	{
		VERIFY(ttip_.Create(this));

		add_tooltip(ID_INDICATOR_OCCURRENCES, "Occurrences of search item");
		add_tooltip(ID_INDICATOR_VALUES, "Hex,Dec,Octal,Binary,char values");
		add_tooltip(ID_INDICATOR_HEX_ADDR, "Distance from mark (hex)");
		add_tooltip(ID_INDICATOR_DEC_ADDR, "Distance from mark (decimal)");
		add_tooltip(ID_INDICATOR_READONLY, "Read-Only/Read-Write");
		add_tooltip(ID_INDICATOR_OVR, "Overtype/Insert");
		add_tooltip(ID_INDICATOR_REC, "Macro Recording");

		ttip_.Activate(TRUE);
		ttip_.SetDelayTime(1000);
	}
	else if (cx > 0 && cy > 0)
	{
		move_tooltip(ID_INDICATOR_OCCURRENCES);
		move_tooltip(ID_INDICATOR_VALUES);
		move_tooltip(ID_INDICATOR_HEX_ADDR);
		move_tooltip(ID_INDICATOR_DEC_ADDR);
		move_tooltip(ID_INDICATOR_READONLY);
		move_tooltip(ID_INDICATOR_OVR);
		move_tooltip(ID_INDICATOR_REC);
	}
#endif
}

void CStatBar::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	int index;                   // Index (0 based) of current pane we're checking
	CRect pane_rect;             // Bounds (device coords) of pane
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();

	// Check if mouse clicked on hex/dec/octal/binary/char pane
	index = CommandToIndex(ID_INDICATOR_VALUES);
	ASSERT(index > 0);
	GetItemRect(index, &pane_rect);
	if (pane_rect.PtInRect(point))
	{
		// If there is currently a view and no properties dlg exists create it
		if (GetView() != NULL)
		{
			CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
			mm->m_paneProp.ShowAndUnroll();
			// If prev prop page was 0 (file info) change to 1 (character info)
			mm->m_wndProp.SetActivePage(aa->prop_page_ == 0 ? 1 : aa->prop_page_);
		}
		else
		{
			mm->m_paneProp.ShowAndUnroll();
		}
		return;
	}

	// Check if mouse clicked in "search occurrences" pane
	index = CommandToIndex(ID_INDICATOR_OCCURRENCES);
	ASSERT(index > 0);
	GetItemRect(index, &pane_rect);
	if (pane_rect.PtInRect(point))
	{
		// Display the find dialog
		mm->OnEditFind();
		return;
	}

	// Check if mouse clicked in a "distance to" pane
	index = CommandToIndex(ID_INDICATOR_HEX_ADDR);
	ASSERT(index > 0);
	GetItemRect(index, &pane_rect);
	if (pane_rect.PtInRect(point))
	{
		// Display the calculator in hex mode
		mm->OnEditGoto(2);
		return;
	}

	index = CommandToIndex(ID_INDICATOR_DEC_ADDR);
	ASSERT(index > 0);
	GetItemRect(index, &pane_rect);
	if (pane_rect.PtInRect(point))
	{
		// Display the calculator in decimal mode
		mm->OnEditGoto(1);
		return;
	}

	// Check if mouse was clicked in big-endian pane
	index = CommandToIndex(ID_INDICATOR_BIG_ENDIAN);
	ASSERT(index > 0);
	GetItemRect(index, &pane_rect);
	if (pane_rect.PtInRect(point))
	{
		CHexEditView *pview = GetView();
		if (pview != NULL)
		{
			if (pview->BigEndian())
				pview->OnLittleEndian();
			else
				pview->OnBigEndian();
			return;
		}
	}

	// Check if mouse clicked on READONLY pane
	index = CommandToIndex(ID_INDICATOR_READONLY);
	ASSERT(index > 0);
	GetItemRect(index, &pane_rect);
	if (pane_rect.PtInRect(point))
	{
		CHexEditView *pview = GetView();
		if (pview != NULL)
			GetView()->AllowMods();
		return;
	}

	// Check if mouse clicked on OVR pane
	index = CommandToIndex(ID_INDICATOR_OVR);
	ASSERT(index > 0);
	GetItemRect(index, &pane_rect);
	if (pane_rect.PtInRect(point))
	{
		CHexEditView *pview = GetView();
		if (pview != NULL && !pview->ReadOnly())
			pview->ToggleInsert();
		return;
	}

	// Check if mouse clicked on OVR pane
	index = CommandToIndex(ID_INDICATOR_REC);
	ASSERT(index > 0);
	GetItemRect(index, &pane_rect);
	if (pane_rect.PtInRect(point))
	{
		CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
		aa->OnMacroRecord();
		return;
	}

	index = CommandToIndex(ID_INDICATOR_CAPS);
	ASSERT(index > 0);
	GetItemRect(index, &pane_rect);
	if (pane_rect.PtInRect(point))
	{
		// Simulate Caps Lock key being depressed then released
		keybd_event(VK_CAPITAL, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
		keybd_event(VK_CAPITAL, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
		return;
	}

	// This doesn't work under Windows 95 (indicator changes but not light/kb behaviour)
	// but we no longer support Windows 9X
	index = CommandToIndex(ID_INDICATOR_NUM);
	ASSERT(index > 0);
	GetItemRect(index, &pane_rect);
	if (pane_rect.PtInRect(point))
	{
	  // Simulate Num Lock key being depressed then released
	  keybd_event(VK_NUMLOCK, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
	  keybd_event(VK_NUMLOCK, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
	  return;
	}

	CMFCStatusBar::OnLButtonDblClk(nFlags, point);
}

void CStatBar::OnRButtonDown(UINT nFlags, CPoint point)
{
	CPoint scr_point(point);
	ClientToScreen(&scr_point);
	((CMainFrame *)AfxGetMainWnd())->bar_context(scr_point);
//	CMFCStatusBar::OnRButtonDown(nFlags, point);
}

void CStatBar::OnContextMenu(CWnd* pWnd, CPoint point)
{
	((CMainFrame *)AfxGetMainWnd())->bar_context(point);
}

//===========================================================================


