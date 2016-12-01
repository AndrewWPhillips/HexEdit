// Prop.cpp : implements the Properties tabbed dialog box
//
// Copyright (c) 2015 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//


#include "stdafx.h"
#include <MultiMon.h>

#include <mbstring.h>

#include "HexEdit.h"
#include "MainFrm.h"
#include "HexFileList.h"
#include "HexEditDoc.h"
#include "HexEditView.h"
#include "SystemSound.h"
#include "SimpleGraph.h"
#include "SpecialList.h"
#include "resource.hm"          // For control help IDs
#include "prop.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
static CBrush *pBrush = NULL;     // brush used for backgrounds of read-only text controls
static wchar_t char_unicode[8];   // current Unicode character to display
static wchar_t char_multibyte[8]; // current multibyte char converted to Unicode (using current code page)

#ifdef SUBCLASS_UNICODE_CONTROL
/////////////////////////////////////////////////////////////////////////////
// CUnicodeControl

BEGIN_MESSAGE_MAP(CUnicodeControl, CEdit)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

void CUnicodeControl::OnPaint()
{
	CPaintDC dc(this);                  // Device context for painting

	// Get rectangle where text is drawn
	CRect rct;
	GetClientRect(&rct);

	dc.SetTextAlign(TA_BASELINE | TA_CENTER);
	dc.SetBkMode(TRANSPARENT);
	::TextOutW(dc.GetSafeHdc(), rct.right/2, rct.bottom*3/4, char_unicode, wcslen(char_unicode));
}

BOOL CUnicodeControl::OnEraseBkgnd(CDC* pDC)
{
	CRect rct;
	GetClientRect(rct);

	// Fill background with bg_col_
	pDC->FillRect(rct, pBrush);

	return TRUE;
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropEditControl - used for (non-read-only) edit controls

BEGIN_MESSAGE_MAP(CPropEditControl, CEdit)
	ON_WM_GETDLGCODE()
END_MESSAGE_MAP()

UINT CPropEditControl::OnGetDlgCode()
{
	// Get all keys so that we see CR and Escape
	return CEdit::OnGetDlgCode() | DLGC_WANTALLKEYS;
}

/////////////////////////////////////////////////////////////////////////////
// CCommentEditControl - used for comment (non-read-only) edit controls

BEGIN_MESSAGE_MAP(CCommentEditControl, CEdit)
	ON_WM_CHAR()
	ON_WM_GETDLGCODE()
END_MESSAGE_MAP()

UINT CCommentEditControl::OnGetDlgCode()
{
	// Get all keys so that we see CR and Escape
	return CEdit::OnGetDlgCode() | DLGC_WANTALLKEYS;
}

void CCommentEditControl::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// Just filter out vertical bars (|) since these are used as the field
	// separator when comment fields are saved to the recently opened file file.
	if (nChar == '|')
	{
#ifdef SYS_SOUNDS
		if (!CSystemSound::Play("Invalid Character"))
#endif
			::Beep(5000,200);
		return;
	}
	CEdit::OnChar(nChar, nRepCnt, nFlags);
}

/////////////////////////////////////////////////////////////////////////////
// CBinEditControl

CBinEditControl::CBinEditControl()
{
}

CBinEditControl::~CBinEditControl()
{
}

BEGIN_MESSAGE_MAP(CBinEditControl, CEdit)
	//{{AFX_MSG_MAP(CBinEditControl)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_GETDLGCODE()
	ON_WM_SETCURSOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBinEditControl message handlers

void CBinEditControl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CHexEditView *pview = GetView();

	int ii = CharFromPos(point);
	if (pview != NULL && ii <= 8)
	{
		FILE_ADDRESS start_addr, end_addr;          // Start and end of selection
		pview->GetSelAddr(start_addr, end_addr);

		CString ss;
		GetWindowText(ss);

		if (ss.GetLength() > 0 && !pview->ReadOnly() && start_addr < pview->GetDocument()->length())
		{
			// Work out best character clicked on
			if (ii >= ss.GetLength())
			{
				ii = ss.GetLength() - 1;
			}
			else if (ii > 0)
			{
				CPoint pt = PosFromChar(ii);
				if (point.x < pt.x)
					ii--;
			}

			if (ss[ii] == '0')
				ss.SetAt(ii, '1');
			else
				ss.SetAt(ii, '0');
			ss.TrimLeft();

			char *endptr;
			unsigned char binary_val = (unsigned char)strtoul(ss, &endptr, 2);
			pview->GetDocument()->Change(mod_replace, start_addr, 1, 
										 &binary_val, 0, pview);
			if (end_addr - start_addr != 1)
				pview->MoveToAddress(start_addr, start_addr + 1, -1, -1, TRUE);

			SetWindowText(ss);
			SetSel(ii, ii+1);
			return;
		}
	}
	CEdit::OnLButtonDblClk(nFlags, point);
}

UINT CBinEditControl::OnGetDlgCode()
{
	// Get all keys so that we see CR and Escape
	return CEdit::OnGetDlgCode() | DLGC_WANTALLKEYS;
}

BOOL CBinEditControl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (nHitTest == HTCLIENT)
	{
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
		return TRUE;
	}
	else
		return CEdit::OnSetCursor(pWnd, nHitTest, message);
}

/////////////////////////////////////////////////////////////////////////////
// CPropDecEditControl - used for edit control where decimal are entered

// TODO Perhaps merge this with CDecEdit

CPropDecEditControl::CPropDecEditControl()
{
	//struct lconv *plconv = localeconv(); - now read in theApp.InitInstance
	sep_char_ = theApp.dec_sep_char_;
	group_ = theApp.dec_group_;
	allow_neg_ = true;
}


BEGIN_MESSAGE_MAP(CPropDecEditControl, CEdit)
		ON_WM_CHAR()
		ON_WM_KEYDOWN()
		ON_WM_GETDLGCODE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropDecEditControl message handlers

void CPropDecEditControl::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CString ss;
	GetWindowText(ss);
	int start, end;
	GetSel(start, end);

	if (nChar == '\b' && start > 1 && start == end && ss[start-1] == sep_char_)
	{
		// If deleting 1st nybble (hex mode) then also delete preceding space
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
	else if (::isprint(nChar) && strchr("0123456789\b", nChar) == NULL)
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

void CPropDecEditControl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CString ss;
	GetWindowText(ss);
	int start, end;
	GetSel(start, end);

	if (nChar == VK_DELETE && start == end && ss.GetLength() > start+1 &&
			 ss[start+1] == sep_char_)
		SetSel(start, start+2);
	else if (nChar == VK_LEFT && start == end && start > 0 && ss[start-1] == sep_char_)
		SetSel(start-1, start-1);

	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);

	// Tidy display if hex and char(s) deleted or caret moved
	GetWindowText(ss);
	if ((nChar == VK_DELETE || nChar == VK_RIGHT || nChar == VK_LEFT) &&
		::GetKeyState(VK_SHIFT) >= 0 && ss.GetLength() > 0)
	{
		add_commas();
	}
}

void CPropDecEditControl::add_commas()
{
	CString ss;                         // Current string
	GetWindowText(ss);
	int start, end;                     // Current selection (start==end if just caret)
	GetSel(start, end);

	const char *pp;                     // Ptr into orig. (hex digit) string
	int newstart, newend;               // New caret position/selection
	newstart = newend = 0;              // In case of empty string

	// Allocate enough space (allowing for comma padding)
	char *out = new char[(ss.GetLength()*(group_+1))/group_ + 2]; // Where chars are stored
	size_t ii, jj;                      // Number of hex nybbles written/read
	char *dd = out;                     // Ptr to current output char
	int ndigits;                        // Numbers of chars that are part of number
	BOOL none_yet = TRUE;               // Have we seen any non-zero digits?

	for (pp = ss.GetBuffer(0), ndigits = 0; *pp != '\0'; ++pp)
		if (::isdigit(*pp))
			++ndigits;

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
		if (!::isdigit(*pp))
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

UINT CPropDecEditControl::OnGetDlgCode()
{
	// Get all keys so that we see CR and Escape
	return CEdit::OnGetDlgCode() | DLGC_WANTALLKEYS;
}


/////////////////////////////////////////////////////////////////////////////
// CPropSheet

IMPLEMENT_DYNAMIC(CPropSheet, CPropertySheet)

CPropSheet::CPropSheet()
#ifdef PROP_INFO
		:CPropertySheet(_T("Properties"), AfxGetMainWnd(), 1)  // make file window first active one (see below)
#else
		:CPropertySheet(_T("Properties"), AfxGetMainWnd(), 0)
#endif
{
	help_hwnd_ = (HWND)0;
#ifdef PROP_INFO
	// Info page is the first but not active page - we need to get category list in
	// CPropInfoPage::OnSetActive but the recent file list has not been read yet.
	AddPage(&prop_info);
#endif
	AddPage(&prop_file);
	AddPage(&prop_char);
	AddPage(&prop_dec);
	AddPage(&prop_real);
	AddPage(&prop_date);
	AddPage(&prop_graph);
	AddPage(&prop_stats);
	AddPage(&prop_bitmap);

	// Create the brush used for background of read-only edit control
	ASSERT(pBrush == NULL);
	int hue, luminance, saturation;
	get_hls(::GetSysColor(COLOR_BTNFACE), hue, luminance, saturation);
	ASSERT(hue != -1 || saturation == 0);  // grey if saturation == 0 (hue == -1)
	// Make it lighter than normal button/dialog colour
	luminance += (100 - luminance)/2;
	if (luminance > 85) luminance = 85; // ... but not too bright/white
	pBrush = new CBrush(get_rgb(hue, luminance, saturation));
}

BOOL CPropSheet::Create(CWnd* pParentWnd, DWORD dwStyle)
{
	if (!CPropertySheet::Create(pParentWnd, dwStyle))
		return FALSE;

	return TRUE;
}

CPropSheet::~CPropSheet()
{
	delete pBrush;
	pBrush = NULL;
}

void CPropSheet::Update(CHexEditView *pv /*=NULL*/, FILE_ADDRESS address /*=-1*/)
{
	// Get currently active property page
	CPropUpdatePage *pp = dynamic_cast<CPropUpdatePage *>(GetActivePage());

	// Set pages members from view and update in displayed page
	pp->Update(pv, address);
}

BEGIN_MESSAGE_MAP(CPropSheet, CPropertySheet)
		ON_WM_NCCREATE()
		ON_WM_CLOSE()
		ON_WM_SIZE()
		ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
		ON_WM_ERASEBKGND()
		ON_MESSAGE(WM_RESIZEPAGE, OnResizePage)
		ON_COMMAND(ID_APPLY_NOW, OnApplyNow)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropSheet message handlers

BOOL CPropSheet::OnInitDialog()
{
	BOOL bResult = __super::OnInitDialog();
	GetClientRect(&m_rctPrev);

	return bResult;
}

BOOL CPropSheet::OnNcCreate(LPCREATESTRUCT lpCreateStruct)
{
		if (!CPropertySheet::OnNcCreate(lpCreateStruct))
				return FALSE;

		ModifyStyleEx(0, WS_EX_CONTEXTHELP);

		return TRUE;
}

void CPropSheet::OnClose()
{
	theApp.SaveToMacro(km_prop_close);
	CPropertySheet::OnClose();
}

void CPropSheet::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	CTabCtrl * pTab = GetTabControl();
	CPropertyPage * pPage = GetActivePage();

	if (m_rctPrev.Width() != 0 && pTab != NULL && pPage != NULL)
	{
		// Get difference between current and previous window sizes
		CSize diff;
		CRect rctCurr;
		GetClientRect(&rctCurr);

		diff.cx = m_rctPrev.Width()  - rctCurr.Width();
		diff.cy = m_rctPrev.Height() - rctCurr.Height();

		m_rctPrev = rctCurr;             // Save current window rect for next time

		// Resize the CTabCtrl
		pTab->GetWindowRect(&rctCurr);
		ScreenToClient(&rctCurr);
		rctCurr.right -= diff.cx;
		rctCurr.bottom -= diff.cy;
		pTab->MoveWindow(&rctCurr);

		// Store page size so we can resize each page as necessary
		pPage->GetWindowRect(&m_rctPage);
		ScreenToClient(&m_rctPage);
		m_rctPage.right -= diff.cx;
		m_rctPage.bottom -= diff.cy;

		// Resize the current page (others done in OnResizePage when required)
		pPage->MoveWindow(&m_rctPage);
	}
}

LONG CPropSheet::OnResizePage(UINT, LONG)
{
	// Resize the previously calculated page size
	CPropertyPage* pPage = GetActivePage();
	ASSERT(pPage != NULL && m_rctPage.Width() > 0);
	pPage->MoveWindow(&m_rctPage);

	return 0;
}

void CPropSheet::OnApplyNow()
{
	// The sheet resizes the page whenever the apply button is clicked.
	// So we need to resize it to what we want.
	PostMessage(WM_RESIZEPAGE);
}

BOOL CPropSheet::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	NMHDR* pNMHDR = (LPNMHDR) lParam;

	// The sheet resizes the page whenever it is activated
	// so we need to resize it to what we want
	if (pNMHDR->code == TCN_SELCHANGE)
		// user-defined message needs to be posted, not sent, because
		// page must be resized after TCN_SELCHANGE has been processed.
		PostMessage(WM_RESIZEPAGE);

	return CPropertySheet::OnNotify(wParam, lParam, pResult);
}

static DWORD id_pairs[] = {
	IDC_INFO_CATEGORY, HIDC_INFO_CATEGORY,
	IDC_INFO_CATEGORY_SELECT, HIDC_INFO_CATEGORY_SELECT,
	IDC_INFO_KEYWORDS, HIDC_INFO_KEYWORDS,
	IDC_INFO_COMMENTS, HIDC_INFO_COMMENTS,
//	IDC_FILE_SIZE, HIDC_FILE_SIZE,
//	IDC_INFO_DISK_SIZE, HIDC_INFO_DISK_SIZE,
	IDC_INFO_VIEW_TIME, HIDC_INFO_VIEW_TIME,
	IDC_INFO_EDIT_TIME, HIDC_INFO_EDIT_TIME,
	IDC_FILE_NAME, HIDC_FILE_NAME,
	IDC_FILE_TYPE, HIDC_FILE_TYPE,
	IDC_FILE_PATH, HIDC_FILE_PATH,
	IDC_FILE_CREATED, HIDC_FILE_CREATED,
	IDC_FILE_MODIFIED, HIDC_FILE_MODIFIED,
	IDC_FILE_ACCESSED, HIDC_FILE_ACCESSED,
	IDC_FILE_READONLY, HIDC_FILE_READONLY,
	IDC_FILE_READONLY_DESC, HIDC_FILE_READONLY,
	IDC_FILE_HIDDEN, HIDC_FILE_HIDDEN,
	IDC_FILE_HIDDEN_DESC, HIDC_FILE_HIDDEN,
	IDC_FILE_SYSTEM, HIDC_FILE_SYSTEM,
	IDC_FILE_SYSTEM_DESC, HIDC_FILE_SYSTEM,
	IDC_FILE_ARCHIVED, HIDC_FILE_ARCHIVED,
	IDC_FILE_ARCHIVED_DESC, HIDC_FILE_ARCHIVED,
	IDC_CHAR_HEX, HIDC_CHAR_HEX,
	IDC_CHAR_DEC, HIDC_CHAR_DEC,
	IDC_CHAR_OCTAL, HIDC_CHAR_OCTAL,
	IDC_CHAR_BINARY, HIDC_CHAR_BINARY,
	IDC_CHAR_ASCII, HIDC_CHAR_ASCII,
	IDC_UNICODE_DESC, HIDC_PLACE_HOLDER,
	IDC_CHAR_UNICODE, HIDC_PLACE_HOLDER,
	IDC_PLACE_HOLDER, HIDC_PLACE_HOLDER,
	IDC_SELECT_FONT, HIDC_SELECT_FONT,
#ifdef SHOW_CODE_PAGE
	IDC_MULTIBYTE_DESC, HIDC_MULTIBYTE_HOLDER,
	IDC_CHAR_MULTIBYTE, HIDC_MULTIBYTE_HOLDER,
	IDC_MULTIBYTE_HOLDER, HIDC_MULTIBYTE_HOLDER,
	IDC_CODE_PAGE, HIDC_CODE_PAGE,
#else
	IDC_CHAR_EBCDIC, HIDC_CHAR_EBCDIC,
#endif
	IDC_DEC_8BIT, HIDC_DEC_8BIT,
	IDC_DEC_16BIT, HIDC_DEC_16BIT,
	IDC_DEC_32BIT, HIDC_DEC_32BIT,
	IDC_DEC_64BIT, HIDC_DEC_64BIT,
//    IDC_DEC_LITTLE_ENDIAN, HIDC_DEC_LITTLE_ENDIAN,
//    IDC_DEC_BIG_ENDIAN, HIDC_DEC_BIG_ENDIAN,
	IDC_BIG_ENDIAN, HIDC_BIG_ENDIAN,
	IDC_DEC_UNSIGNED, HIDC_DEC_UNSIGNED,
//    IDC_DEC_SIGNED, HIDC_DEC_SIGNED,
	IDC_DEC_2COMP, HIDC_DEC_2COMP,
	IDC_DEC_1COMP, HIDC_DEC_1COMP,
	IDC_DEC_SIGN_MAG, HIDC_DEC_SIGN_MAG,
	IDC_FP_VAL, HIDC_FP_VAL,
	IDC_FP_MANT, HIDC_FP_MANT,
	IDC_FP_EXP, HIDC_FP_EXP,
	IDC_BIG_ENDIAN, HIDC_BIG_ENDIAN,
	IDC_FP_FORMAT, HIDC_FP_FORMAT,
	IDC_DATE_FORMAT, HIDC_DATE_FORMAT,
	IDC_DATE_FORMAT_DESC, HIDC_DATE_FORMAT_DESC,
	IDC_DATE_BIG_ENDIAN, HIDC_DATE_BIG_ENDIAN,
	IDC_DATE_LOCAL_TIME, HIDC_DATE_LOCAL_TIME,
	IDC_DATE_DISPLAY, HIDC_DATE_DISPLAY,
	IDC_DATE_DATE, HIDC_DATE_DATE,
	IDC_DATE_TIME, HIDC_DATE_TIME,
	IDC_DATE_FIRST, HIDC_DATE_FIRST,
	IDC_DATE_LAST, HIDC_DATE_LAST,
	IDC_DATE_NOW, HIDC_DATE_NOW,
	IDC_DATE_NULL, HIDC_DATE_NULL,
	IDC_STATS_GRAPH, HIDC_STATS_GRAPH,
	IDC_PLACEHOLDER, HIDC_STATS_GRAPH,
	IDC_STATS_CRC32, HIDC_STATS_CRC32, 
	IDC_STATS_MD5, HIDC_STATS_MD5, 
	IDC_STATS_SHA1, HIDC_STATS_SHA1, 
	IDC_STATS_SHA256, HIDC_STATS_SHA256, 
	IDC_STATS_SHA512, HIDC_STATS_SHA512, 
	IDC_BMP_BPP, HIDC_BMP_BPP,
	IDC_BMP_BPP_DISK, HIDC_BMP_BPP_DISK, 
	IDC_BMP_FORMAT, HIDC_BMP_FORMAT, 
	IDC_BMP_FORMAT_DISK, HIDC_BMP_FORMAT_DISK,
	IDC_BMP_HEIGHT, HIDC_BMP_HEIGHT,
	IDC_BMP_HEIGHT_DISK, HIDC_BMP_HEIGHT_DISK, 
	IDC_BMP_WIDTH, HIDC_BMP_WIDTH,
	IDC_BMP_WIDTH_DISK, HIDC_BMP_WIDTH_DISK,
	0,0
};

BOOL CPropSheet::OnEraseBkgnd(CDC *pDC)
{
	CRect rct;
	GetClientRect(&rct);

	// Fill the background with a colour that matches the current BCG theme (and hence sometimes with the Windows Theme)
	pDC->FillSolidRect(rct, afxGlobalData.clrBarFace);
	return TRUE;
}

LRESULT CPropSheet::OnKickIdle(WPARAM, LPARAM lCount)
{
	// Display context help for ctrl set up in call to a page's OnHelpInfo
	if (help_hwnd_ != (HWND)0)
	{
		theApp.HtmlHelpWmHelp(help_hwnd_, id_pairs);
		help_hwnd_ = (HWND)0;
	}
	CPropUpdatePage *pp = dynamic_cast<CPropUpdatePage *>(GetActivePage());
	if (pp == &prop_graph)
	{
		pp->Update(GetView());
	}

	return FALSE;
}

//===========================================================================
/////////////////////////////////////////////////////////////////////////////
// CPropUpdatePage stuff (most is inline in Prop.h)
IMPLEMENT_DYNAMIC(CPropUpdatePage, CPropertyPage)

BEGIN_MESSAGE_MAP(CPropUpdatePage, CPropertyPage)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

// This is necessary for XP where if the new theme is used the background colour
// of property pages becomes white and hence any static controls within the page
// (including read-only edit controls).  Since normal edit controls also have a
// white background there is no way to distinguish read-only from normal edit boxes.
// Since we use a lot of read-only text controls in the Properties dialog pages,
// doing this was easier than changing them all to static text with a border.
HBRUSH CPropUpdatePage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	ASSERT(pBrush != NULL);

	// Read-only edit controls have nCtlColor of CTLCOLOR_STATIC
	//if (nCtlColor != CTLCOLOR_EDIT)
	if (nCtlColor == CTLCOLOR_STATIC && (::GetWindowLong(pWnd->m_hWnd, GWL_STYLE) & ES_READONLY))
	{
		pDC->SetBkMode(TRANSPARENT);  // So the text has the same background as rest of control
		return (HBRUSH)*pBrush;
	}

	return CPropertyPage::OnCtlColor(pDC, pWnd, nCtlColor);
}

//===========================================================================
/////////////////////////////////////////////////////////////////////////////
// CPropFilePage property page

IMPLEMENT_DYNCREATE(CPropFilePage, CPropUpdatePage)

CPropFilePage::CPropFilePage() : CPropUpdatePage(CPropFilePage::IDD)
{
	//{{AFX_DATA_INIT(CPropFilePage)
	file_name_ = _T("");
	file_path_ = _T("");
	file_created_ = _T("");
	file_modified_ = _T("");
	file_accessed_ = _T("");
	file_size_ = _T("");
	file_type_ = _T("");
	file_readonly_ = FALSE;
	file_hidden_ = FALSE;
	file_system_ = FALSE;
	file_archived_ = FALSE;
	//}}AFX_DATA_INIT
}

CPropFilePage::~CPropFilePage()
{
}

void CPropFilePage::DoDataExchange(CDataExchange* pDX)
{
	CPropUpdatePage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropFilePage)
	DDX_Text(pDX, IDC_FILE_NAME, file_name_);
	DDX_Text(pDX, IDC_FILE_PATH, file_path_);
	DDX_Text(pDX, IDC_FILE_MODIFIED, file_modified_);
	DDX_Text(pDX, IDC_FILE_TYPE, file_type_);
	DDX_Text(pDX, IDC_FILE_CREATED, file_created_);
	DDX_Text(pDX, IDC_FILE_ACCESSED, file_accessed_);
	DDX_Text(pDX, IDC_FILE_SIZE, file_size_);
	DDX_Check(pDX, IDC_FILE_HIDDEN, file_hidden_);
	DDX_Check(pDX, IDC_FILE_READONLY, file_readonly_);
	DDX_Check(pDX, IDC_FILE_SYSTEM, file_system_);
	DDX_Check(pDX, IDC_FILE_ARCHIVED, file_archived_);
	//}}AFX_DATA_MAP
}

void CPropFilePage::Update(CHexEditView *pv, FILE_ADDRESS address)
{
	// Clear fields
	file_name_ = "";
	file_path_ = "";
	file_type_ = "";
	file_created_ = "";
	file_modified_ = "";
	file_accessed_ = "";
	file_size_ = "";
	file_readonly_ = 0;
	file_hidden_ = 0;
	file_system_ = 0;
	file_archived_ = 0;

	if (pv == NULL)
	{
		UpdateData(FALSE);
		return;
	}

	CHexEditDoc *pDoc = dynamic_cast<CHexEditDoc *>(pv->GetDocument());
	BOOL is_device = pDoc->IsDevice();
	if (address == -1)
		address = pv->GetPos();

	// hide windows (controls) that are not used for device
	GetDlgItem(IDC_FILE_HIDDEN)  ->ShowWindow(is_device ? SW_HIDE : SW_SHOW);
	//GetDlgItem(IDC_FILE_READONLY)->ShowWindow(is_device ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_FILE_SYSTEM)  ->ShowWindow(is_device ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_FILE_ARCHIVED)->ShowWindow(is_device ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_FILE_HIDDEN_DESC)  ->ShowWindow(is_device ? SW_HIDE : SW_SHOW);
	//GetDlgItem(IDC_FILE_READONLY_DESC)->ShowWindow(is_device ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_FILE_SYSTEM_DESC)  ->ShowWindow(is_device ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_FILE_ARCHIVED_DESC)->ShowWindow(is_device ? SW_HIDE : SW_SHOW);

	// Fix descriptions (for non-devices) in case pfile_ == NULL and we return straight away (below)
	if (!is_device)
	{
		GetDlgItem(IDC_FILE_CREATED_DESC) ->SetWindowText("Created:");
		GetDlgItem(IDC_FILE_MODIFIED_DESC)->SetWindowText("Modified:");
		GetDlgItem(IDC_FILE_ACCESSED_DESC)->SetWindowText("Access:");
	}

	CFile64 *pf = pDoc->pfile1_;

	// Can show length of file (in memory) even if there is no associated disk file
	if (!is_device)
	{
		// First get the abbreviated length
		FILE_ADDRESS len_mem = pDoc->length();
		file_size_ = NumScale((double)len_mem) + "bytes";
		file_size_.TrimLeft();
		FILE_ADDRESS len_disk = -1;

		if (pf != NULL && (len_disk = pf->GetLength()) != len_mem)
		{
			// File in memory is different length to file on disk so we need to display:
			// disk length, length in memory and difference
			FILE_ADDRESS diff = len_mem - len_disk;
			CString ss;
			if (diff < 0)
				ss.Format("%lld[disk] - %lld = %lld", len_disk, -diff, len_mem);
			else
				ss.Format("%lld [disk] + %lld = %lld", len_disk, diff, len_mem);
			file_size_ += " ( " + ss + " )";
		}
		else if (len_mem >= 995)
		{
			// Abbreviated number lost some info so provide full number (in brackets)
			CString ss;
			ss.Format("%lld", len_mem);
			AddCommas(ss);
			file_size_ += " (" + ss + ")";
		}
	}

	if (pf == NULL)
	{
		// Can't show anything else if no assoc. disk file
		UpdateData(FALSE);
		return;
	}

	file_name_ = pf->GetFileName();
	file_path_ = pf->GetFilePath();

	if (is_device)
	{
		CSpecialList *psl = theApp.GetSpecialList();
		int idx = psl->find((LPCTSTR)pf->GetFilePath());
		CString ss;
		switch (psl->type(idx))
		{
		case 0:
			GetDlgItem(IDC_FILE_CREATED_DESC) ->SetWindowText("Volume:");
			GetDlgItem(IDC_FILE_MODIFIED_DESC)->SetWindowText("Sector:");
			GetDlgItem(IDC_FILE_ACCESSED_DESC)->SetWindowText("Clusters:");
			file_type_ = psl->DriveType(idx) + " Drive";
			file_created_ = psl->VolumeName(idx);
			if (file_created_.IsEmpty())
			{
				GetDlgItem(IDC_FILE_CREATED_DESC) ->SetWindowText("FileSys:");
				file_created_ = psl->FileSystem(idx);
			}
			else
				file_created_ += " (" + psl->FileSystem(idx) + ")";
			// It also would be nice to show current cluster as used by filesystem but we
			// would need to know how many reserved sectors there were before 1st cluster.
			ss.Format("%ld", (long)psl->TotalClusters(idx));
			AddCommas(ss);
			file_accessed_.Format("%s (%ld sectors/cluster)", ss, (long)psl->SectorsPerCluster(idx));
			break;

		case 1:
			GetDlgItem(IDC_FILE_CREATED_DESC) ->SetWindowText("Model:");
			GetDlgItem(IDC_FILE_MODIFIED_DESC)->SetWindowText("Sector:");
			GetDlgItem(IDC_FILE_ACCESSED_DESC)->SetWindowText("Geometry:");
			file_type_ = psl->nicename(idx);
			file_created_ =	psl->Product(idx) + " Revision:" + psl->Revision(idx);
			file_accessed_.Format("%ld sec/trk, %ld trk/cyl, %ld cyl",
								  (long)psl->SectorsPerTrack(idx),
								  (long)psl->TracksPerCylinder(idx),
								  (long)psl->Cylinders(idx));
			break;

		default:
			ASSERT(0);
		}
		char buf[24];
		int sec_size = psl->sector_size(idx);
		if (sec_size > 0)                    // sector size becoming zero is unusual but could happen if media ejected
		{
			//ss.Format("%ld", long(address/sec_size));
			sprintf(buf, "%I64d", __int64(address/sec_size));
			ss = buf;
			AddCommas(ss);
			file_modified_ += ss;
			//ss.Format("%ld", long(pDoc->length()/sec_size));
			sprintf(buf, "%I64d", __int64(pDoc->length()/sec_size));
			ss = buf;
			AddCommas(ss);
			file_modified_ += " of " + ss;
			ss.Format(" @%ld bytes each", (long)sec_size);
			file_modified_ += ss;
		}

		// Check read only status of device
		file_readonly_ = psl->read_only(idx);

		CWnd *pwnd = GetDlgItem(IDC_FILE_READONLY);
		ASSERT(pwnd != NULL);
		pwnd->EnableWindow(TRUE);

		UpdateData(FALSE);

		pwnd = GetDlgItem(IDC_FILE_READONLY);
		ASSERT(pwnd != NULL);
		pwnd->EnableWindow(FALSE);
	}
	else
	{
		CFileStatus status;
		size_t path_len;            // Length of path (excluding file name)

		if ( (path_len = file_path_.ReverseFind('\\')) != -1 ||
			(path_len = file_path_.ReverseFind(':')) != -1 )
			file_path_ = file_path_.Left(path_len+1);
		else
			file_path_ = "";

		file_type_ = pDoc->GetType();
		if (file_type_ == "None") file_type_.Empty();

		pf->GetStatus(status);
		file_created_ = status.m_ctime.Format("%#c");
		file_modified_ = status.m_mtime.Format("%#c");
		file_accessed_ = status.m_atime.Format("%#c");
		file_readonly_ = (status.m_attribute & CFile::readOnly) != 0;
		file_hidden_ = (status.m_attribute & CFile::hidden) != 0;
		file_system_ = (status.m_attribute & CFile::system) != 0;
		file_archived_ = (status.m_attribute & CFile::archive) != 0;

		// Enable the check boxes so they can be set appropriately
		CWnd *pwnd = GetDlgItem(IDC_FILE_READONLY);
		ASSERT(pwnd != NULL);
		pwnd->EnableWindow(TRUE);
		pwnd = GetDlgItem(IDC_FILE_HIDDEN);
		ASSERT(pwnd != NULL);
		pwnd->EnableWindow(TRUE);
		pwnd = GetDlgItem(IDC_FILE_SYSTEM);
		ASSERT(pwnd != NULL);
		pwnd->EnableWindow(TRUE);

		UpdateData(FALSE);

		// Disable the check boxes again so the user doesn't think he can change them
		pwnd = GetDlgItem(IDC_FILE_READONLY);
		ASSERT(pwnd != NULL);
		pwnd->EnableWindow(FALSE);
		pwnd = GetDlgItem(IDC_FILE_HIDDEN);
		ASSERT(pwnd != NULL);
		pwnd->EnableWindow(FALSE);
		pwnd = GetDlgItem(IDC_FILE_SYSTEM);
		ASSERT(pwnd != NULL);
		pwnd->EnableWindow(FALSE);
	}

	// The call to UpdateWindow() is necessary during macro replay since (even
	// if property refresh is on) no changes are seen until the macro stops.
	UpdateWindow();
}

BEGIN_MESSAGE_MAP(CPropFilePage, CPropUpdatePage)
	ON_WM_SIZE()
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropFilePage message handlers

BOOL CPropFilePage::OnInitDialog()
{
	CPropUpdatePage::OnInitDialog();

	// Make sure when dialog is resized that the controls move to sensible places
	resizer_.Create(this);
	resizer_.SetMinimumTrackingSize();
	resizer_.SetGripEnabled(FALSE);
	// Controls to adjust -              left  top  width height
	resizer_.Add(IDC_FILE_NAME,             0,   0,  70,   0);
	resizer_.Add(IDC_FILE_PATH,             0,   0,  70,   0);
	resizer_.Add(IDC_FILE_CREATED,          0,   0,  70,   0);
	resizer_.Add(IDC_FILE_MODIFIED,         0,   0,  70,   0);
	resizer_.Add(IDC_FILE_ACCESSED,         0,   0,  70,   0);
	resizer_.Add(IDC_FILE_SIZE,             0,   0,  70,   0);
	resizer_.Add(IDC_FILE_TYPE,            70,   0,  28,   0);
	resizer_.Add(IDC_FILE_READONLY,        70,   0,   0,   0);
	resizer_.Add(IDC_FILE_READONLY_DESC,   70,   0,   0,   0);
	resizer_.Add(IDC_FILE_HIDDEN,          70,   0,   0,   0);
	resizer_.Add(IDC_FILE_HIDDEN_DESC,     70,   0,   0,   0);
	resizer_.Add(IDC_FILE_SYSTEM,          70,   0,   0,   0);
	resizer_.Add(IDC_FILE_SYSTEM_DESC,     70,   0,   0,   0);
	resizer_.Add(IDC_FILE_ARCHIVED,        70,   0,   0,   0);
	resizer_.Add(IDC_FILE_ARCHIVED_DESC,   70,   0,   0,   0);

	return TRUE;
}

void CPropFilePage::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
	CWnd * pwnd = GetDlgItem(IDC_FILE_TYPE);
	if (pwnd != NULL && pwnd->m_hWnd != (HWND)0)
		pwnd->Invalidate(0);  // When static text is resized it is not redrawn properly
}

BOOL CPropFilePage::OnSetActive()
{
	Update(GetView());
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_prop_file);
	return CPropUpdatePage::OnSetActive();
}

BOOL CPropFilePage::OnHelpInfo(HELPINFO* pHelpInfo)
{
	//theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs7);
	((CMainFrame *)AfxGetMainWnd())->m_wndProp.help_hwnd_ = (HWND)pHelpInfo->hItemHandle;
	return TRUE;
}

void CPropFilePage::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.HtmlHelpContextMenu(pWnd, id_pairs);
}


//===========================================================================
/////////////////////////////////////////////////////////////////////////////
// CPropInfoPage property page

IMPLEMENT_DYNCREATE(CPropInfoPage, CPropUpdatePage)

CPropInfoPage::CPropInfoPage() : CPropUpdatePage(CPropInfoPage::IDD)
{
}

CPropInfoPage::~CPropInfoPage()
{
}

void CPropInfoPage::DoDataExchange(CDataExchange* pDX)
{
	CPropUpdatePage::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_INFO_CATEGORY,  category_);
	DDX_Text(pDX, IDC_INFO_KEYWORDS,  keywords_);
	DDX_Text(pDX, IDC_INFO_COMMENTS,  comments_);
	//DDX_Text(pDX, IDC_INFO_DISK_SIZE, disk_size_);
	DDX_Text(pDX, IDC_INFO_VIEW_TIME, view_time_);
	DDX_Text(pDX, IDC_INFO_EDIT_TIME, edit_time_);
	DDX_Control(pDX, IDC_INFO_CATEGORY_SELECT, cat_sel_ctl_);
}

void CPropInfoPage::Update(CHexEditView *pv, FILE_ADDRESS /*not used*/)
{
	// Clear fields
	category_ = _T("");
	keywords_ = _T("");
	comments_ = _T("");
	//disk_size_ = _T("");
	view_time_ = _T("");
	edit_time_ = _T("");
	category_view_ = keywords_view_ = comments_view_ = NULL;

	// Get index into previously-opened file list
	CHexFileList *pfl = theApp.GetFileList();
	int ii = -1;
	if (pv != NULL && pv->GetDocument()->pfile1_ != NULL) // make sure there is a disk file to interrogate (pfl required disk file name)
		ii = pfl->GetIndex(pv->GetDocument()->pfile1_->GetFilePath());

	ASSERT(GetDlgItem(IDC_INFO_CATEGORY) != NULL);
	ASSERT(GetDlgItem(IDC_INFO_KEYWORDS) != NULL);
	ASSERT(GetDlgItem(IDC_INFO_COMMENTS) != NULL);

	// Make controls RO (disallow entry) if there is no recent file entry (since we can't store comments back anyway)
	GetDlgItem(IDC_INFO_CATEGORY)->SendMessage(EM_SETREADONLY, ii == -1);
	GetDlgItem(IDC_INFO_KEYWORDS)->SendMessage(EM_SETREADONLY, ii == -1);
	GetDlgItem(IDC_INFO_COMMENTS)->SendMessage(EM_SETREADONLY, ii == -1);
	cat_sel_ctl_.EnableWindow(ii != -1);
	if (pv == NULL)
	{
		// There is no file open so just return with fields blank
		UpdateData(FALSE);
		return;
	}

	if (ii != -1)
	{
		category_ = pfl->GetData(ii, CHexFileList::CATEGORY);
		keywords_ = pfl->GetData(ii, CHexFileList::KEYWORDS);
		comments_ = pfl->GetData(ii, CHexFileList::COMMENTS);
	}

	// Need document for edit/view times and current file size
	CHexEditDoc *pDoc = dynamic_cast<CHexEditDoc *>(pv->GetDocument());

	// Nicely format the length of view/edit time into hh:mm:ss or hhh:mm
	long secs = long(pDoc->view_time_.elapsed() + 0.5);  // ignore fractions of secs
	long mins = secs/60;
	long hours = mins/60;
	if (hours > 99)
		view_time_.Format("%ld:%02ld", long(hours), long(mins%60));
	else
		view_time_.Format("%2ld:%02ld:%02ld", long(hours), long(mins%60), long(secs%60));

	secs = long(pDoc->edit_time_.elapsed() + 0.5);  // ignore fractions of secs
	mins = secs/60;
	hours = mins/60;
	if (hours > 99)
		edit_time_.Format("%ld:%02ld", long(hours), long(mins%60));
	else
		edit_time_.Format("%2ld:%02ld:%02ld", long(hours), long(mins%60), long(secs%60));

	// Need current file to get file size on disk
	CFile64 *pf = pDoc->pfile1_;
	if (pf == NULL)
	{
		UpdateData(FALSE);
		return;
	}

	UpdateData(FALSE);

	// The call to UpdateWindow() is necessary during macro replay since (even
	// if property refresh is on) no changes are seen until the macro stops.
	UpdateWindow();
}

void CPropInfoPage::fix_menu()
{
	// Destroy previous menu if any
	if (cat_sel_ctl_.m_hMenu != (HMENU)0)
	{
		::DestroyMenu(cat_sel_ctl_.m_hMenu);
		cat_sel_ctl_.m_hMenu = (HMENU)0;
	}

	// Get a list of categories to display in the menu
	std::set<CString> categories;

	// We always provide a favourites category even if there are no files in it
	if (theApp.is_us_)
		categories.insert(_T("Favorites"));
	else
		categories.insert(_T("Favourites"));

	// Get all categories used in RFL
	CHexFileList *pfl = theApp.GetFileList();
	for (int ii = 0; ii < pfl->GetCount(); ++ii)
	{
		CString ss = pfl->GetData(ii, CHexFileList::CATEGORY);
		if (!ss.IsEmpty())
			categories.insert(ss);
	}

	// Add menu items
	CMenu mm;
	mm.CreatePopupMenu();
	int count = 0;
	for (std::set<CString>::iterator pp = categories.begin(); pp != categories.end(); ++pp)
		mm.AppendMenu(MF_STRING, ++count, *pp);
	ASSERT(count > 0);

	cat_sel_ctl_.m_hMenu = mm.GetSafeHmenu();
	mm.Detach();
}

BEGIN_MESSAGE_MAP(CPropInfoPage, CPropUpdatePage)
	ON_EN_CHANGE(IDC_INFO_CATEGORY, OnChangeCategory)
	ON_EN_KILLFOCUS(IDC_INFO_CATEGORY, OnKillFocusCategory)
	ON_EN_CHANGE(IDC_INFO_KEYWORDS, OnChangeKeywords)
	ON_EN_KILLFOCUS(IDC_INFO_KEYWORDS, OnKillFocusKeywords)
	ON_EN_CHANGE(IDC_INFO_COMMENTS, OnChangeComments)
	ON_EN_KILLFOCUS(IDC_INFO_COMMENTS, OnKillFocusComments)
	ON_BN_CLICKED(IDC_INFO_CATEGORY_SELECT, OnSelCategory)
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropInfoPage message handlers

BOOL CPropInfoPage::OnInitDialog()
{
	CPropUpdatePage::OnInitDialog();

	// Make sure when dialog is resized that the controls move to sensible places
	resizer_.Create(this);
	resizer_.SetMinimumTrackingSize();
	resizer_.SetGripEnabled(FALSE);
	// Controls to adjust -              left  top  width height
	resizer_.Add(IDC_INFO_CATEGORY,         0,   0, 100,   0);
	resizer_.Add(IDC_INFO_CATEGORY_SELECT,100,   0,   0,   0);
	resizer_.Add(IDC_INFO_VIEW_TIME_DESC, 100,   0,   0,   0);
	resizer_.Add(IDC_INFO_VIEW_TIME,      100,   0,   0,   0);
	resizer_.Add(IDC_INFO_EDIT_TIME_DESC, 100,   0,   0,   0);
	resizer_.Add(IDC_INFO_EDIT_TIME,      100,   0,   0,   0);
	resizer_.Add(IDC_INFO_KEYWORDS,         0,   0, 100,   0);
	resizer_.Add(IDC_INFO_COMMENTS,         0,   0, 100, 100);

	// Subclass non-read-only edit controls so we can intercept |, CR and Escape
	VERIFY(category_ctl_.SubclassDlgItem(IDC_INFO_CATEGORY, this));
	VERIFY(keywords_ctl_.SubclassDlgItem(IDC_INFO_KEYWORDS, this));
	VERIFY(comments_ctl_.SubclassDlgItem(IDC_INFO_COMMENTS, this));

	return TRUE;
}

BOOL CPropInfoPage::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_CHAR && pMsg->wParam == '\r')
	{
		VERIFY(UpdateData());
		CHexEditView *pview = GetView();
		ASSERT(pview != NULL);

		CHexFileList *pfl = theApp.GetFileList();
		int ii = -1;
		CString filename;
		if (pview != NULL && pview->GetDocument()->pfile1_ != NULL)
		{
			filename = pview->GetDocument()->pfile1_->GetFilePath();
			ii = pfl->GetIndex(filename);
		}

		if (pMsg->hwnd == GetDlgItem(IDC_INFO_CATEGORY)->m_hWnd && ii != -1)
		{
			ASSERT(category_view_ = pview);
			pfl->SetData(ii, CHexFileList::CATEGORY, category_);
			((CEdit*)GetDlgItem(IDC_INFO_CATEGORY))->SetSel(0, -1);
			category_view_ = NULL;
			((CMainFrame *)AfxGetMainWnd())->UpdateExplorer(filename);
			fix_menu();
		}
		else if (pMsg->hwnd == GetDlgItem(IDC_INFO_KEYWORDS)->m_hWnd && ii != -1)
		{
			ASSERT(keywords_view_ = pview);
			pfl->SetData(ii, CHexFileList::KEYWORDS, keywords_);
			((CEdit*)GetDlgItem(IDC_INFO_KEYWORDS))->SetSel(0, -1);
			keywords_view_ = NULL;
			((CMainFrame *)AfxGetMainWnd())->UpdateExplorer(filename);
		}
		else if (pMsg->hwnd == GetDlgItem(IDC_INFO_COMMENTS)->m_hWnd && ii != -1)
		{
			ASSERT(comments_view_ = pview);
			pfl->SetData(ii, CHexFileList::COMMENTS, comments_);
			((CEdit*)GetDlgItem(IDC_INFO_COMMENTS))->SetSel(0, -1);
			comments_view_ = NULL;
			((CMainFrame *)AfxGetMainWnd())->UpdateExplorer(filename);
		}

		return 1;
	}
	else if (pMsg->message == WM_CHAR && pMsg->wParam == '\t')
	{
		if (::GetKeyState(VK_SHIFT) < 0)
			SendMessage(WM_NEXTDLGCTL, 1);
		else
			SendMessage(WM_NEXTDLGCTL);

		return 1;
	}
	else if (pMsg->message == WM_CHAR && pMsg->wParam == '\033')
	{
		// Escape pressed - restore previous value from val_
		UpdateData(FALSE);
		ASSERT(GetDlgItem(IDC_INFO_CATEGORY) != NULL);
		ASSERT(GetDlgItem(IDC_INFO_KEYWORDS) != NULL);
		ASSERT(GetDlgItem(IDC_INFO_COMMENTS) != NULL);
		if (pMsg->hwnd == GetDlgItem(IDC_INFO_CATEGORY)->m_hWnd)
		{
			((CEdit*)GetDlgItem(IDC_INFO_CATEGORY))->SetSel(0, -1);
			category_view_ = NULL;
		}
		else if (pMsg->hwnd == GetDlgItem(IDC_INFO_KEYWORDS)->m_hWnd)
		{
			((CEdit*)GetDlgItem(IDC_INFO_KEYWORDS))->SetSel(0, -1);
			keywords_view_ = NULL;
		}
		else if (pMsg->hwnd == GetDlgItem(IDC_INFO_COMMENTS)->m_hWnd)
		{
			((CEdit*)GetDlgItem(IDC_INFO_COMMENTS))->SetSel(0, -1);
			comments_view_ = NULL;
		}

		return 1;
	}

	return CPropUpdatePage::PreTranslateMessage(pMsg);
}

BOOL CPropInfoPage::OnSetActive()
{
	CHexEditView *pv = GetView();
	Update(pv);

	// Now setup menu of existing categories
	fix_menu();

	// Only enable if there is a disk file name (else CATEGORY can't be written to recent files list)
	CHexFileList *pfl = theApp.GetFileList();
	int curr = -1;
	if (pv != NULL && pv->GetDocument()->pfile1_ != NULL) // make sure there is a disk file to interrogate (pfl required disk file name)
		curr = pfl->GetIndex(pv->GetDocument()->pfile1_->GetFilePath());
	cat_sel_ctl_.EnableWindow(curr != -1);

	return CPropUpdatePage::OnSetActive();
}

BOOL CPropInfoPage::OnKillActive()
{
	BOOL retval = CPropertyPage::OnKillActive();

#if 0  // This works well when changing pages but does not work if the user
	   // closes the dialog or moves focus outside the dialog.
	int changes = (category_view_ != NULL) + (keywords_view_ != NULL) + (comments_view_ != NULL);

	if (retval && changes > 0)
	{
		CHexEditView *pview = GetView();
		ASSERT(pview != NULL && pview->GetDocument()->pfile1_ != NULL);

		CHexFileList *pfl = theApp.GetFileList();
		int ii = pfl->GetIndex(pview->GetDocument()->pfile1_->GetFilePath());
		ASSERT(ii != -1);

		CString ss = "Changes have been made to ";
		int count = 0;
		if (category_view_ != NULL)
		{
			ss += "Category";
			++count;
		}
		if (keywords_view_ != NULL)
		{
			if (count > 0 && changes == 2)
				ss += " and ";
			else if (count > 0)
				ss += ", ";
			ss += "Keywords";
			++count;
		}
		if (comments_view_ != NULL)
		{
			if (count > 0)
				ss += " and ";
			ss += "Comments";
			++count;
		}
		ASSERT(count == changes);
		ss += ".\n\nDo you want to save your changes?";

		switch (AfxMessageBox(ss, MB_YESNOCANCEL))
		{
		case IDYES:
			if (category_view_ != NULL)
			{
				ASSERT(category_view_ = pview);
				pfl->SetData(ii, CHexFileList::CATEGORY, category_);
				((CEdit*)GetDlgItem(IDC_INFO_CATEGORY))->SetSel(0, -1);
				category_view_ = NULL;
			}
			if (keywords_view_)
			{
				ASSERT(keywords_view_ = pview);
				pfl->SetData(ii, CHexFileList::KEYWORDS, keywords_);
				((CEdit*)GetDlgItem(IDC_INFO_KEYWORDS))->SetSel(0, -1);
				keywords_view_ = NULL;
			}
			if (comments_view_ != NULL)
			{
				ASSERT(comments_view_ = pview);
				pfl->SetData(ii, CHexFileList::COMMENTS, comments_);
				((CEdit*)GetDlgItem(IDC_INFO_COMMENTS))->SetSel(0, -1);
				comments_view_ = NULL;
			}
			break;
		case IDNO:
			// just return TRUE
			break;
		case IDCANCEL:
			retval = FALSE;
			break;
		default:
			ASSERT(0);
		}
	}
#endif
	return retval;
}

void CPropInfoPage::OnChangeCategory()
{
	category_view_ = GetView();
}

void CPropInfoPage::OnChangeKeywords()
{
	keywords_view_ = GetView();
}

void CPropInfoPage::OnChangeComments()
{
	comments_view_ = GetView();
}

void CPropInfoPage::OnKillFocusCategory()
{
	CHexEditView * pview = (CHexEditView *)category_view_;
	if (pview != NULL && pview->GetDocument()->pfile1_ != NULL)
	{
		CHexFileList *pfl = theApp.GetFileList();
		CString filename = pview->GetDocument()->pfile1_->GetFilePath();
		ASSERT(!filename.IsEmpty());
		int ii = pfl->GetIndex(filename);
		ASSERT(ii != -1);

		UpdateData();
		pfl->SetData(ii, CHexFileList::CATEGORY, category_);
		//((CEdit*)GetDlgItem(IDC_INFO_CATEGORY))->SetSel(0, -1);
		category_view_ = NULL;
		((CMainFrame *)AfxGetMainWnd())->UpdateExplorer(filename);
		fix_menu();
	}
}

void CPropInfoPage::OnKillFocusKeywords()
{
	CHexEditView * pview = (CHexEditView *)keywords_view_;
	if (pview != NULL && pview->GetDocument()->pfile1_ != NULL)
	{
		CHexFileList *pfl = theApp.GetFileList();
		CString filename = pview->GetDocument()->pfile1_->GetFilePath();
		ASSERT(!filename.IsEmpty());
		int ii = pfl->GetIndex(filename);
		ASSERT(ii != -1);

		UpdateData();
		pfl->SetData(ii, CHexFileList::KEYWORDS, keywords_);
		keywords_view_ = NULL;
		((CMainFrame *)AfxGetMainWnd())->UpdateExplorer(filename);
	}
}

void CPropInfoPage::OnKillFocusComments()
{
	CHexEditView * pview = (CHexEditView *)comments_view_;
	if (pview != NULL && pview->GetDocument()->pfile1_ != NULL)
	{
		CHexFileList *pfl = theApp.GetFileList();
		CString filename = pview->GetDocument()->pfile1_->GetFilePath();
		ASSERT(!filename.IsEmpty());
		int ii = pfl->GetIndex(filename);
		ASSERT(ii != -1);

		UpdateData();
		pfl->SetData(ii, CHexFileList::COMMENTS, comments_);
		comments_view_ = NULL;
		((CMainFrame *)AfxGetMainWnd())->UpdateExplorer(filename);
	}
}

void CPropInfoPage::OnSelCategory()
{
	if (cat_sel_ctl_.m_nMenuResult != 0)
	{
		CMenu menu;
		menu.Attach(cat_sel_ctl_.m_hMenu);
		CString ss = get_menu_text(&menu, cat_sel_ctl_.m_nMenuResult);
		menu.Detach();

		// Add the text to the category text box
		category_ctl_.SetWindowText("");

		for (int ii = 0; ii < ss.GetLength (); ii++)
			category_ctl_.SendMessage(WM_CHAR, (TCHAR)ss[ii]);
		category_ctl_.SetFocus();
	}
}

BOOL CPropInfoPage::OnHelpInfo(HELPINFO* pHelpInfo)
{
	//theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs1);
	((CMainFrame *)AfxGetMainWnd())->m_wndProp.help_hwnd_ = (HWND)pHelpInfo->hItemHandle;
	return TRUE;
}

void CPropInfoPage::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.HtmlHelpContextMenu(pWnd, id_pairs);
}

//===========================================================================
/////////////////////////////////////////////////////////////////////////////
// CPropCharPage property page

IMPLEMENT_DYNCREATE(CPropCharPage, CPropUpdatePage)

CPropCharPage::CPropCharPage() : CPropUpdatePage(CPropCharPage::IDD)
{
		//{{AFX_DATA_INIT(CPropCharPage)
		char_ascii_ = _T("");
		char_binary_ = _T("");
		char_ebcdic_ = _T("");
		char_hex_ = _T("");
		char_octal_ = _T("");
		char_dec_ = _T("");
		//}}AFX_DATA_INIT
		active_code_page_ = ::GetACP();
		CPINFO cpi;
		VERIFY(GetCPInfo(active_code_page_, &cpi));
		is_dbcs_ = cpi.MaxCharSize == 2;

#ifdef SHOW_CODE_PAGE
		code_page_ = 0;
#endif
}

CPropCharPage::~CPropCharPage()
{
}

void CPropCharPage::DoDataExchange(CDataExchange* pDX)
{
		CPropUpdatePage::DoDataExchange(pDX);
		//{{AFX_DATA_MAP(CPropCharPage)
		DDX_Text(pDX, IDC_CHAR_ASCII, char_ascii_);
		DDX_Text(pDX, IDC_CHAR_BINARY, char_binary_);
		DDV_MaxChars(pDX, char_binary_, 8);
		DDX_Text(pDX, IDC_CHAR_HEX, char_hex_);
		DDV_MaxChars(pDX, char_hex_, 2);
		DDX_Text(pDX, IDC_CHAR_OCTAL, char_octal_);
		DDV_MaxChars(pDX, char_octal_, 3);
		DDX_Text(pDX, IDC_CHAR_DEC, char_dec_);
		DDV_MaxChars(pDX, char_dec_, 3);
		//}}AFX_DATA_MAP
#ifdef SHOW_CODE_PAGE
		DDX_Control(pDX, IDC_CODE_PAGE, ctl_code_page_);
		DDX_CBIndex(pDX, IDC_CODE_PAGE, code_page_);
#else
		DDX_Text(pDX, IDC_CHAR_EBCDIC, char_ebcdic_);
#endif
		if (!pDX->m_bSaveAndValidate)
		{
			CString ss;
			ss.Format("(page: %d)", active_code_page_);
			SetDlgItemText(IDC_CHAR_ACP, ss);

			// Before the Unicode window has been created or under Win 95
			// the IDC_CHAR_UNICODE edit control does not exist & GetDlgItem 
			// returns 0 - BoundsChecker erroneously signals this as a problem.
			HWND hw = ::GetDlgItem(m_hWnd, IDC_CHAR_UNICODE);
			if (hw != HWND(0))
				::SetWindowTextW(hw, char_unicode);
#ifdef SHOW_CODE_PAGE
			hw = ::GetDlgItem(m_hWnd, IDC_CHAR_MULTIBYTE);
			if (hw != HWND(0))
				::SetWindowTextW(hw, char_multibyte);
#endif
		}
}

#ifdef SHOW_CODE_PAGE
static const int MAX_BYTES = 16;                 // Max bytes to get from files for multibyte char processing (10 is probably enough)
static vector<int> page_number;                  // The code page number for all installed code pages
static vector<CString> page_name;                // Corresponding name of the code page
static vector<int> page_max_chars;               // Length of longest character of the code page (in bytes).

// We need to get a ptr to GetCPInfoEx since it is not present on Windows 95 and NT4
typedef BOOL (__stdcall *PFGetCPInfoEx)(UINT, DWORD, LPCPINFOEXA);
PFGetCPInfoEx pGetCPInfoEx;

static BOOL CALLBACK CodePageCallback(LPTSTR ss)
{
	// Get info about this page
	UINT cp = atoi(ss);
	CPINFOEX cpie;
	if ((*pGetCPInfoEx)(cp, 0, &cpie))
	{
		// Save what we need
		page_number.push_back(cpie.CodePage);
		page_name.push_back(CString(cpie.CodePageName));
		page_max_chars.push_back(cpie.MaxCharSize);
		ASSERT(cpie.MaxCharSize <= MAX_BYTES);
	}

	return TRUE;
}
#endif

void CPropCharPage::Update(CHexEditView *pv, FILE_ADDRESS address /*=-1*/)
{
	static char *ascii_control_char[] =
	{
		"NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL",
		"BS",  "HT",  "LF",  "VT",  "FF",  "CR",  "SO",  "SI",
		"DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB",
		"CAN", "EM",  "SUB", "ESC", "FS",  "GS",  "RS",  "US",
		"space", "**ERROR"
	};

	static char *ebcdic_control_char[] =
	{
		"NUL", "SOH", "STX", "ETX", "PF",  "HT",  "LC",  "DEL",
		"GE",  "RLF", "SMM", "VT",  "FF",  "CR",  "SO",  "SI",
		"DLE", "DC1", "DC2", "TM",  "RES", "NL",  "BS",  "IL",
		"CAN", "EM",  "CC",  "CU1", "IFS", "IGS", "IRS", "IUS",
		"DS",  "SOS", "FS",  "none","BYP", "LF",  "ETB", "ESC",
		"none","none","SM",  "CU2", "none","ENQ", "ACK", "BEL",
		"none","none","SYN", "none","PN",  "RS",  "UC",  "EOT",
		"none","none","none", "CU3", "DC4", "NAK", "none", "SUB",
		"space","**ERROR"
	};

	// Set fields to empty in case of no view, caret at EOF etc
	char_ascii_ = _T("");
	char_binary_ = _T("");
	char_ebcdic_ = _T("");
	char_hex_ = _T("");
	char_octal_ = _T("");
	char_dec_ = _T("");
	char_unicode[0] = L'\0';
	char_multibyte[0] = L'\0';

	ASSERT(GetDlgItem(IDC_CHAR_DEC)    != NULL);
	ASSERT(GetDlgItem(IDC_CHAR_OCTAL)  != NULL);
	ASSERT(GetDlgItem(IDC_CHAR_BINARY) != NULL);
	// If no view return else get view's current address (if not given)
	if (pv == NULL)
	{
		GetDlgItem(IDC_CHAR_DEC)->SendMessage(EM_SETREADONLY, TRUE);
		GetDlgItem(IDC_CHAR_OCTAL)->SendMessage(EM_SETREADONLY, TRUE);
		GetDlgItem(IDC_CHAR_BINARY)->SendMessage(EM_SETREADONLY, TRUE);
		UpdateData(FALSE);
		return;
	}
	if (address == -1)
		address = pv->GetPos();

#ifdef SHOW_CODE_PAGE
	unsigned char cc[MAX_BYTES+1];   // Allow +1 for string terminator when converting MBCS to Unicode
	size_t got = dynamic_cast<CHexEditDoc *>(pv->GetDocument())->
		GetData(cc, max(2, page_max_chars[code_page_]), address);       // get enough chars for Unicode (2) and code page (up to MAX_BYTES)
	ASSERT(got <= MAX_BYTES);
#else
	unsigned char cc[2];
	size_t got = dynamic_cast<CHexEditDoc *>(pv->GetDocument())->GetData(cc, 2, address);
#endif
	GetDlgItem(IDC_CHAR_DEC)->SendMessage(EM_SETREADONLY, got == 0 || pv->ReadOnly());
	GetDlgItem(IDC_CHAR_OCTAL)->SendMessage(EM_SETREADONLY, got == 0 || pv->ReadOnly());
	GetDlgItem(IDC_CHAR_BINARY)->SendMessage(EM_SETREADONLY, got == 0 || pv->ReadOnly());

	if (got > 0)
	{
		CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

		// Work out binary, octal, decimal, hex values
		for (int ii = 0; ii < 8; ++ii)
			char_binary_ += (cc[0] & (0x80>>ii)) ? '1' : '0';
		char_octal_.Format("%03o", cc[0]);
		char_dec_.Format("%d", cc[0]);
		if (aa->hex_ucase_)
			char_hex_.Format("%02X", cc[0]);
		else
			char_hex_.Format("%02x", cc[0]);

		// Note: When active code page is for DBCS we can't store a lead-in byte in char_ascii.
		// (It is stored OK but when later used we get get extra garbage characters due to
		// the nul-terminator being consumed by the lead-in byte deeep inside MFC.)

		// Work out display values (ASCII, EBCDIC, MBCS)
		if (cc[0] <= 0x20)      // Control char or space?
			char_ascii_ = ascii_control_char[cc[0]];
		else if (is_dbcs_ && ::IsDBCSLeadByteEx(active_code_page_, cc[0]))
			char_ascii_ = "lead-in";
		else
			char_ascii_ = char(cc[0]);
		if (cc[0] <= 0x40)
			char_ebcdic_ = ebcdic_control_char[cc[0]];
		else if (e2a_tab[cc[0]] != '\0')
			char_ebcdic_ = char(e2a_tab[cc[0]]);
		else
			char_ebcdic_ = "none";
	}

	HWND hw;
#ifdef SHOW_CODE_PAGE
	hw = ::GetDlgItem(m_hWnd, IDC_CHAR_MULTIBYTE);
	ASSERT(theApp.is_nt_ == (hw != HWND(0)));           // window should not be there if this is NT/2K/XP
	if (hw != HWND(0))
	{
		if (got > 0)
		{
			ASSERT(got < sizeof(cc));
			cc[got] = '\0';

			ASSERT(code_page_ == (int)ctl_code_page_.GetItemData(code_page_));
			//size_t len = CharNextExA(page_number[code_page_], (LPCSTR)cc, 0) - (LPSTR)cc;
			//TRACE("+_+_+_+_ MBCS LEN %d +_+_+_+_\r\n", len);
			// Note that we can't use len instead of -1 in the call below since for UTF-8 len is always 1 (or 0),
			// though we could do our own calculation of len
			MultiByteToWideChar(page_number[code_page_], 0, (LPCSTR)cc, -1, char_multibyte, sizeof(char_multibyte)/sizeof(char_multibyte[0]));
			char_multibyte[1] = L'\0';    // Only display the first char
		}

		if (page_max_chars[code_page_] == 1)
			SetDlgItemText(IDC_MULTIBYTE_DESC, "Single Byte\r\nChar set:");
		else if (page_max_chars[code_page_] == 2)
		{
			// Check if the first byte is a lead in byte
			if (got > 0 && ::IsDBCSLeadByteEx(page_number[code_page_], cc[0]))
				SetDlgItemText(IDC_MULTIBYTE_DESC, "DBCS:\r\n(2 bytes)");
			else
				SetDlgItemText(IDC_MULTIBYTE_DESC, "DBCS:");
		}
		else
		{
			CString ss;
			ss.Format("MBCS:\r\n(up to %d)", page_max_chars[code_page_]);
			SetDlgItemText(IDC_MULTIBYTE_DESC, ss);
		}
	}
#endif

	hw = ::GetDlgItem(m_hWnd, IDC_CHAR_UNICODE);
	ASSERT(theApp.is_nt_ == (hw != HWND(0)));           // window should not be there if this is NT/2K/XP
	if (got > 1 && hw != HWND(0))
	{
		// Create unicode string if we got 2 bytes and we're running NT
		char_unicode[0] = cc[0] + 256*cc[1];
		char_unicode[1] = L'\0';

		::SendMessageW(hw, WM_SETFONT, WPARAM(nfont_), 0L);
		if (char_unicode[0] <= 0x20)
		{
			VERIFY(MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS,
				ascii_control_char[cc[0]], -1,
				char_unicode, sizeof(char_unicode)/sizeof(char_unicode[0])));
		}
		else if (char_unicode[0] == 0x2028)
		{
			wcscpy(char_unicode, L"newline");
		}
		else if (char_unicode[0] == 0x2029)
		{
			wcscpy(char_unicode, L"para");
		}
		else if (char_unicode[0] == 0xFFFE)
		{
			wcscpy(char_unicode, L"order");
		}
		else if (char_unicode[0] == 0xFFFF)
		{
			wcscpy(char_unicode, L"spec.");
		}
		else
		{
			::SendMessageW(hw, WM_SETFONT, WPARAM(ufont_), 0L);
		}
	}

	//if (got > 1)
	//{
	//    // Create unicode string if we got 2 bytes and we're running NT
	//    char_unicode[0] = cc[0] + 256*cc[1];
	//    char_unicode[1] = L'\0';
	//    ::SendMessageW(hw, WM_SETFONT, WPARAM(nfont_), 0L);
	//    if (char_unicode[0] <= 0x20)
	//    {
	//        VERIFY(MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS,
	//            ascii_control_char[cc[0]], -1,
	//            char_unicode, sizeof(char_unicode)/sizeof(char_unicode[0])));
	//    }
	//    else if (char_unicode[0] == 0x2028)
	//    {
	//        wcscpy(char_unicode, L"newline");
	//    }
	//    else if (char_unicode[0] == 0x2029)
	//    {
	//        wcscpy(char_unicode, L"para");
	//    }
	//    else if (char_unicode[0] == 0xFFFE)
	//    {
	//        wcscpy(char_unicode, L"order");
	//    }
	//    else if (char_unicode[0] == 0xFFFF)
	//    {
	//        wcscpy(char_unicode, L"spec.");
	//    }
	//    else
	//    {
	//        ::SendMessageW(ctl_unicode_.m_hWnd, WM_SETFONT, WPARAM(ufont_), 0L);
	//    }
	//}
	//ctl_unicode_.Invalidate();
	UpdateData(FALSE);

	// The call to UpdateWindow() is necessary during macro replay since (even
	// if property refresh is on) no changes are seen until the macro stops.
	UpdateWindow();
}

BEGIN_MESSAGE_MAP(CPropCharPage, CPropUpdatePage)
		//{{AFX_MSG_MAP(CPropCharPage)
		ON_WM_HELPINFO()
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
	ON_WM_CONTEXTMENU()
	ON_BN_CLICKED(IDC_SELECT_FONT, OnBnClickedSelectFont)
#ifdef SHOW_CODE_PAGE
	ON_CBN_SELCHANGE(IDC_CODE_PAGE, OnSelchangeCodePage)
#endif
END_MESSAGE_MAP()

#if 0  // this works OK but is not needed
static WNDPROC prevWndProc = 0;
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ASSERT(prevWndProc != 0);
	if (msg == WM_PAINT)
	{
		//if (::GetUpdateRect(hWnd, NULL, TRUE) != 0)
		{
			PAINTSTRUCT ps;
			wchar_t ss[8];
			HDC hdc = ::BeginPaint(hWnd, &ps);
			RECT rct;
			::GetWindowTextW(hWnd, ss, 8);
			::GetClientRect(hWnd, &rct);

			::SetTextAlign(hdc, TA_BASELINE | TA_CENTER);
			::TextOutW(hdc, rct.right/2, rct.bottom*3/4, ss, wcslen(ss));
			::EndPaint(hWnd, &ps);
		}
		return 0;
	}
	else if (msg == WM_ERASEBKGND)
	{
		//if (::GetUpdateRect(hWnd, NULL, TRUE) != 0)
		{
			PAINTSTRUCT ps;
			HDC hdc = ::BeginPaint(hWnd, &ps);
			RECT rct;
			::GetClientRect(hWnd, &rct);
			::FillRect(hdc, &rct, (HBRUSH)*pBrush);
		}

		return 1;
	}
	else
		return ::CallWindowProc(prevWndProc, hWnd, msg, wParam, lParam);
}
#endif

HWND CPropCharPage::CreateUnicodeControl(int holder_id, int new_id)
{
	// Get position & style of placeholder control then hide it
	CRect parent_rct, rct;
	CWnd *pedit = GetDlgItem(holder_id);
	ASSERT(pedit != NULL);
	LONG style = ::GetWindowLong(pedit->m_hWnd, GWL_STYLE);
	LONG exstyle = ::GetWindowLong(pedit->m_hWnd, GWL_EXSTYLE);
	pedit->GetWindowRect(&rct);
	pedit->ShowWindow(SW_HIDE);
	GetWindowRect(&parent_rct);     // rectangle of dialog

	// Create Unicode window
	HWND hw = ::CreateWindowExW(exstyle, L"Edit", NULL, style,
								rct.left-parent_rct.left, rct.top-parent_rct.top,
								rct.Width(), rct.Height(),
								m_hWnd, (HMENU)new_id,
								AfxGetInstanceHandle(), NULL);
	ASSERT(hw != HWND(0));
	//prevWndProc = (WNDPROC)::SetWindowLong(hw, GWL_WNDPROC, LONG(&WndProc));
	//::SetWindowPos(hw, GetDlgItem(IDC_PLACE_HOLDER)->m_hWnd, 0,0,0,0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
	return hw;
}

/////////////////////////////////////////////////////////////////////////////
// CPropCharPage message handlers

BOOL CPropCharPage::OnInitDialog()
{
#ifdef SHOW_CODE_PAGE
	VERIFY(active_code_page_ == ::GetACP());   // Just make sure we got the system active code page OK
	if (page_name.empty())
	{
		HINSTANCE hh;
		if ((hh = ::LoadLibrary("KERNEL32.DLL")) != HINSTANCE(0) &&
			(pGetCPInfoEx = (PFGetCPInfoEx)::GetProcAddress(hh, "GetCPInfoExA")) != 0)
		{
			::EnumSystemCodePages(&CodePageCallback, CP_INSTALLED);
		}
		else
		{
			// Just use ANSI code page (this should only happen for Win 95 & NT4)
			CPINFO cpi;

			if (GetCPInfo(CP_ACP, &cpi))
			{
				page_number.push_back(CP_ACP);
				page_name.push_back(CString("ANSI Code Page"));
				page_max_chars.push_back(cpi.MaxCharSize);
				ASSERT(cpi.MaxCharSize <= MAX_BYTES);
			}
		}
		ASSERT(!page_name.empty());
		if (hh != HINSTANCE(0)) VERIFY(::FreeLibrary(hh));
	}
	int default_page_number = theApp.GetProfileInt("Property-Settings", "CodePage", active_code_page_);
	for (int ii = 0; ii < page_number.size(); ++ii)
		if (page_number[ii] == default_page_number)
		{
			code_page_ = ii;
			break;
		}
#endif

	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

	CPropUpdatePage::OnInitDialog();

	// Subclass non-read-only edit controls so we can intercept carriage return/Escape
	VERIFY(edit_dec_.SubclassDlgItem(IDC_CHAR_DEC, this));
	VERIFY(edit_octal_.SubclassDlgItem(IDC_CHAR_OCTAL, this));
	VERIFY(edit_binary_.SubclassDlgItem(IDC_CHAR_BINARY, this));
	//VERIFY(ctl_unicode_.SubclassDlgItem(IDC_PLACE_HOLDER, this));

	if (!aa->is_nt_)
	{
		// We can't show Unicode unless on NT so disable the controls
		ASSERT(GetDlgItem(IDC_UNICODE_DESC) != NULL);
		ASSERT(GetDlgItem(IDC_PLACE_HOLDER) != NULL);
		ASSERT(GetDlgItem(IDC_SELECT_FONT)  != NULL);
		GetDlgItem(IDC_UNICODE_DESC)->EnableWindow(FALSE);
		GetDlgItem(IDC_PLACE_HOLDER)->EnableWindow(FALSE);
		GetDlgItem(IDC_SELECT_FONT) ->EnableWindow(FALSE);
	}
	else
	{
		// Create a normal font for info strings in Unicode window
		memset((void *)&lf_, '\0', sizeof(lf_));
		strcpy(lf_.lfFaceName, "Arial");
		lf_.lfHeight = 14;
		lf_.lfWeight = 0;
		lf_.lfCharSet = DEFAULT_CHARSET;
		nfont_ = ::CreateFontIndirect(&lf_);

		// Create a nice large Unicode font (to display the char)
		memset((void *)&lf_, '\0', sizeof(lf_));
		strcpy(lf_.lfFaceName, theApp.GetProfileString("Property-Settings", "UnicodeFont", "Lucida Sans Unicode"));   // best std font for Unicode
		lf_.lfHeight = theApp.GetProfileInt("Property-Settings", "UnicodeFontSize", 16);  // = 12 points
		lf_.lfWeight = 0;
		lf_.lfCharSet = DEFAULT_CHARSET;
		ufont_ = ::CreateFontIndirect(&lf_);

		// Note: lf_ may be later used to change ufont_ (see use of CFontDialog below)
		HWND hw = CreateUnicodeControl(IDC_PLACE_HOLDER, IDC_CHAR_UNICODE);
		//::SendMessageW(ctl_unicode_.m_hWnd, WM_SETFONT, WPARAM(ufont_), 0L);
		::SendMessageW(hw, WM_SETFONT, WPARAM(ufont_), 0L);

#ifdef SHOW_CODE_PAGE
		hw = CreateUnicodeControl(IDC_MULTIBYTE_HOLDER, IDC_CHAR_MULTIBYTE);
		::SendMessageW(hw, WM_SETFONT, WPARAM(ufont_), 0L);

		// Fill in the code page control
		ASSERT(ctl_code_page_.GetCount() == 0);
		for (int ii = 0; ii < page_name.size(); ++ii)
		{
			int idx = ctl_code_page_.InsertString(-1, page_name[ii]);
			ASSERT(idx > -1);
			ctl_code_page_.SetItemData(idx, (DWORD_PTR)ii);
		}
		ctl_code_page_.SetDroppedWidth(256);
#endif

	}

	return TRUE;
}

BOOL CPropCharPage::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_CHAR && pMsg->wParam == '\r')
	{
		VERIFY(UpdateData());
		CHexEditView *pview = GetView();
		ASSERT(pview != NULL);

		FILE_ADDRESS start_addr, end_addr;          // Start and end of selection
		pview->GetSelAddr(start_addr, end_addr);

		ASSERT(GetDlgItem(IDC_CHAR_DEC) != NULL);
		ASSERT(GetDlgItem(IDC_CHAR_OCTAL) != NULL);
		ASSERT(GetDlgItem(IDC_CHAR_BINARY) != NULL);
		if (pMsg->hwnd == GetDlgItem(IDC_CHAR_DEC)->m_hWnd && pview != NULL)
		{
			unsigned char dec_val = (unsigned char)atoi(char_dec_);
			if (pview->ReadOnly())
			{
				AvoidableTaskDialog(IDS_PROP_READONLY, "You can't modify a read only file.");
#ifdef SYS_SOUNDS
				if (!CSystemSound::Play("Read Only"))
#endif
					::Beep(5000,200);
			}
			else if (start_addr >= pview->GetDocument()->length())
			{
				ASSERT(0);
				AfxMessageBox("Can't edit past end of file");
			}
			else
			{
				pview->GetDocument()->Change(mod_replace, start_addr, 1, 
											 &dec_val, 0, pview);
				if (end_addr - start_addr != 1)
					pview->MoveToAddress(start_addr, start_addr + 1, -1, -1, TRUE);
				((CEdit*)GetDlgItem(IDC_CHAR_DEC))->SetSel(0, -1);
			}
		}
		else if (pMsg->hwnd == GetDlgItem(IDC_CHAR_OCTAL)->m_hWnd && pview != NULL)
		{
			char_octal_.TrimLeft();
			char_octal_.TrimRight();

			char *endptr;
			unsigned char octal_val = (unsigned char)strtoul(char_octal_, &endptr, 8);
			if (pview->ReadOnly())
			{
				AvoidableTaskDialog(IDS_PROP_READONLY, "You can't modify a read only file.");
#ifdef SYS_SOUNDS
				if (!CSystemSound::Play("Read Only"))
#endif
					::Beep(5000,200);
			}
			else if (start_addr == pview->GetDocument()->length())
			{
				ASSERT(0);
				AfxMessageBox("Can't edit past end of file");
			}
			else if (endptr - (const char *)char_octal_ < char_octal_.GetLength())
			{
				CString ss;
				ss.Format("%s is not a valid octal number.", char_octal_);
				TaskMessageBox("Invalid Number", ss);
				Update(pview);
				((CEdit*)GetDlgItem(IDC_CHAR_OCTAL))->SetSel(0, -1);
				GetDlgItem(IDC_CHAR_OCTAL)->SetFocus();
			}
			else
			{
				pview->GetDocument()->Change(mod_replace, start_addr, 1, 
											 &octal_val, 0, pview);
				if (end_addr - start_addr != 1)
					pview->MoveToAddress(start_addr, start_addr + 1, -1, -1, TRUE);
				((CEdit*)GetDlgItem(IDC_CHAR_OCTAL))->SetSel(0, -1);
			}
		}
		else if (pMsg->hwnd == GetDlgItem(IDC_CHAR_BINARY)->m_hWnd && pview != NULL)
		{
			char_binary_.TrimLeft();
			char_binary_.TrimRight();

			char *endptr;
			unsigned char binary_val = (unsigned char)strtoul(char_binary_, &endptr, 2);
			if (pview->ReadOnly())
			{
				AvoidableTaskDialog(IDS_PROP_READONLY, "You can't modify a read only file.");
#ifdef SYS_SOUNDS
				if (!CSystemSound::Play("Read Only"))
#endif
					::Beep(5000,200);
			}
			else if (start_addr == pview->GetDocument()->length())
			{
				ASSERT(0);
				AfxMessageBox("Can't edit past end of file");
			}
			else if (endptr - (const char *)char_binary_ < char_binary_.GetLength())
			{
				CString ss;
				ss.Format("%s is an invalid binary number.", char_binary_);
				TaskMessageBox("Invalid Number", ss);
				Update(pview);
				((CEdit*)GetDlgItem(IDC_CHAR_BINARY))->SetSel(0, -1);
				GetDlgItem(IDC_CHAR_BINARY)->SetFocus();
			}
			else
			{
				pview->GetDocument()->Change(mod_replace, start_addr, 1, 
											 &binary_val, 0, pview);
				if (end_addr - start_addr != 1)
					pview->MoveToAddress(start_addr, start_addr + 1, -1, -1, TRUE);
				((CEdit*)GetDlgItem(IDC_CHAR_BINARY))->SetSel(0, -1);
			}
		}

		return 1;
	}
	else if (pMsg->message == WM_CHAR && pMsg->wParam == '\t')
	{
		if (::GetKeyState(VK_SHIFT) < 0)
			SendMessage(WM_NEXTDLGCTL, 1);
		else
			SendMessage(WM_NEXTDLGCTL);

		return 1;
	}
	else if (pMsg->message == WM_CHAR && pMsg->wParam == '\033')
	{
		// Escape pressed - restore previous value from val_
		UpdateData(FALSE);
		ASSERT(GetDlgItem(IDC_CHAR_DEC) != NULL);
		ASSERT(GetDlgItem(IDC_CHAR_OCTAL) != NULL);
		ASSERT(GetDlgItem(IDC_CHAR_BINARY) != NULL);
		if (pMsg->hwnd == GetDlgItem(IDC_CHAR_DEC)->m_hWnd)
			((CEdit*)GetDlgItem(IDC_CHAR_DEC))->SetSel(0, -1);
		else if (pMsg->hwnd == GetDlgItem(IDC_CHAR_OCTAL)->m_hWnd)
			((CEdit*)GetDlgItem(IDC_CHAR_OCTAL))->SetSel(0, -1);
		else if (pMsg->hwnd == GetDlgItem(IDC_CHAR_BINARY)->m_hWnd)
			((CEdit*)GetDlgItem(IDC_CHAR_BINARY))->SetSel(0, -1);

		return 1;
	}

	return CPropUpdatePage::PreTranslateMessage(pMsg);
}

void CPropCharPage::OnClose()
{
	// Deselect any font and destroy the fonts created above
	HWND hw = ::GetDlgItem(m_hWnd, IDC_CHAR_UNICODE);
	ASSERT(theApp.is_nt_ == (hw != HWND(0)));           // window should not be there if this is NT/2K/XP
	if (hw != HWND(0))
		::SendMessageW(hw, WM_SETFONT, WPARAM(::GetStockObject(ANSI_VAR_FONT)), 0L);
#ifdef SHOW_CODE_PAGE
	hw = ::GetDlgItem(m_hWnd, IDC_CHAR_MULTIBYTE);
	if (hw != HWND(0))
		::SendMessageW(hw, WM_SETFONT, WPARAM(::GetStockObject(ANSI_VAR_FONT)), 0L);
#endif

	if (theApp.is_nt_)
	{
		VERIFY(DeleteObject(ufont_));
		VERIFY(DeleteObject(nfont_));
	}
	//::SendMessageW(ctl_unicode_.m_hWnd, WM_SETFONT, WPARAM(::GetStockObject(ANSI_VAR_FONT)), 0L);

	CPropUpdatePage::OnClose();
}

BOOL CPropCharPage::OnSetActive()
{
	Update(GetView());
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_prop_char);
	return CPropUpdatePage::OnSetActive();
}

BOOL CPropCharPage::OnHelpInfo(HELPINFO *pHelpInfo)
{
	//theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs2);
	((CMainFrame *)AfxGetMainWnd())->m_wndProp.help_hwnd_ = (HWND)pHelpInfo->hItemHandle;
	return TRUE;
}

void CPropCharPage::OnContextMenu(CWnd* pWnd, CPoint point)
{
	// NOTE: For the static text control (ID = -1) to give the same help
	// the text box it is associated with must be next in the tab order + the
	// text box MUST HAVE THE TABSTOP OPTION TURNED ON (ie no NOT WS_TABSTOP).

	theApp.HtmlHelpContextMenu(pWnd, id_pairs);
}

#ifdef SHOW_CODE_PAGE
void CPropCharPage::OnSelchangeCodePage()
{
	if (UpdateData())
	{
		ASSERT(code_page_ == (int)ctl_code_page_.GetItemData(code_page_));
		theApp.WriteProfileInt("Property-Settings", "CodePage", page_number[code_page_]);
		Update(GetView());      // Redisplay char based on new code page
	}
}
#endif

void CPropCharPage::OnBnClickedSelectFont()
{
	CFontDialog dlg;
	dlg.m_cf.lpLogFont = &lf_;
	dlg.m_cf.Flags |= CF_INITTOLOGFONTSTRUCT | CF_SHOWHELP;
	dlg.m_cf.Flags &= ~(CF_EFFECTS);              // Disable selection of strikethrough, colours etc
	if (dlg.DoModal() == IDOK)
	{
		// Convert font size back to logical units
		dlg.GetCurrentFont(&lf_);
		VERIFY(DeleteObject(ufont_));
		ufont_ = ::CreateFontIndirect(&lf_);

		theApp.WriteProfileString("Property-Settings", "UnicodeFont", lf_.lfFaceName);
		theApp.WriteProfileInt("Property-Settings", "UnicodeFontSize", lf_.lfHeight);

		// Change display of char to use the new font
		//::SendMessageW(ctl_unicode_.m_hWnd, WM_SETFONT, WPARAM(ufont_), 0L);
		//ctl_unicode_.Invalidate(TRUE);
		HWND hw = ::GetDlgItem(m_hWnd, IDC_CHAR_UNICODE);
		ASSERT(hw != HWND(0));              // This button should not be present if we are not under NT
		::SendMessageW(hw, WM_SETFONT, WPARAM(ufont_), 0L);
		::InvalidateRect(hw, NULL, TRUE);
#ifdef SHOW_CODE_PAGE
		hw = ::GetDlgItem(m_hWnd, IDC_CHAR_MULTIBYTE);
		ASSERT(hw != HWND(0));              // This button should not be present if we are not under NT
		::SendMessageW(hw, WM_SETFONT, WPARAM(ufont_), 0L);
		::InvalidateRect(hw, NULL, TRUE);
#endif
	}
}

//===========================================================================

/////////////////////////////////////////////////////////////////////////////
// CPropDecPage property page

IMPLEMENT_DYNCREATE(CPropDecPage, CPropUpdatePage)

CPropDecPage::CPropDecPage() : CPropUpdatePage(CPropDecPage::IDD)
{
		//{{AFX_DATA_INIT(CPropDecPage)
		dec_64bit_ = _T("");
		dec_16bit_ = _T("");
		dec_32bit_ = _T("");
		dec_8bit_ = _T("");
		signed_ = -1;
		big_endian_ = FALSE;
		//}}AFX_DATA_INIT
		CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
		big_endian_ = aa->prop_dec_endian_;
		signed_ = aa->prop_dec_signed_;
		if (signed_ > 3) signed_ = 0;   // Only 4 formats supported currently
}

CPropDecPage::~CPropDecPage()
{
	// Save current state for next invocation of property dialog or save settings
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	aa->prop_dec_signed_ = signed_;
	aa->prop_dec_endian_ = big_endian_ != 0;
}

void CPropDecPage::DoDataExchange(CDataExchange* pDX)
{
		CPropUpdatePage::DoDataExchange(pDX);
		//{{AFX_DATA_MAP(CPropDecPage)
		DDX_Text(pDX, IDC_DEC_64BIT, dec_64bit_);
		DDV_MaxChars(pDX, dec_64bit_, 28);
		DDX_Text(pDX, IDC_DEC_16BIT, dec_16bit_);
		DDV_MaxChars(pDX, dec_16bit_, 7);
		DDX_Text(pDX, IDC_DEC_32BIT, dec_32bit_);
		DDV_MaxChars(pDX, dec_32bit_, 14);
		DDX_Text(pDX, IDC_DEC_8BIT, dec_8bit_);
		DDV_MaxChars(pDX, dec_8bit_, 4);
		DDX_Radio(pDX, IDC_DEC_UNSIGNED, signed_);
		DDX_Check(pDX, IDC_BIG_ENDIAN, big_endian_);
		//}}AFX_DATA_MAP
}

void CPropDecPage::Update(CHexEditView *pv, FILE_ADDRESS address /*=-1*/)
{
	// Set fields to empty in case no file, caret at EOF etc
	dec_8bit_ = _T("");
	dec_16bit_ = _T("");
	dec_32bit_ = _T("");
	dec_64bit_ = _T("");

	ASSERT(GetDlgItem(IDC_DEC_8BIT) != NULL);
	ASSERT(GetDlgItem(IDC_DEC_16BIT) != NULL);
	ASSERT(GetDlgItem(IDC_DEC_32BIT) != NULL);
	ASSERT(GetDlgItem(IDC_DEC_64BIT) != NULL);
	// If no view return else get view's current address (if not given)
	if (pv == NULL)
	{
		GetDlgItem(IDC_DEC_8BIT)->SendMessage(EM_SETREADONLY, TRUE);
		GetDlgItem(IDC_DEC_16BIT)->SendMessage(EM_SETREADONLY, TRUE);
		GetDlgItem(IDC_DEC_32BIT)->SendMessage(EM_SETREADONLY, TRUE);
		GetDlgItem(IDC_DEC_64BIT)->SendMessage(EM_SETREADONLY, TRUE);
		UpdateData(FALSE);
		return;
	}
	if (address == -1)
		address = pv->GetPos();

	unsigned char buf[8];
	size_t got = dynamic_cast<CHexEditDoc *>(pv->GetDocument())->GetData(buf, sizeof(buf), address);

	GetDlgItem(IDC_DEC_8BIT) ->SendMessage(EM_SETREADONLY, got < 1 || pv->ReadOnly());
	GetDlgItem(IDC_DEC_16BIT)->SendMessage(EM_SETREADONLY, got < 2 || pv->ReadOnly());
	GetDlgItem(IDC_DEC_32BIT)->SendMessage(EM_SETREADONLY, got < 4 || pv->ReadOnly());
	GetDlgItem(IDC_DEC_64BIT)->SendMessage(EM_SETREADONLY, got < 8 || pv->ReadOnly());

	if (got >= 1)
	{
		switch (signed_)
		{
		case 0:
			dec_8bit_.Format("%u", buf[0]);
			break;
		case 1:
			dec_8bit_.Format("%d", signed char(buf[0]));
			break;
		case 2:
			if ((buf[0]&0x80) != 0)
				dec_8bit_.Format("-%u", ~buf[0]&0x7F);
			else
				dec_8bit_.Format("%u", buf[0]);
			break;
		case 3:
			if ((buf[0]&0x80) != 0)
				dec_8bit_.Format("-%u", buf[0]&0x7F);
			else
				dec_8bit_.Format("%u", buf[0]);
			break;
		default:
			ASSERT(0);
		}
	}
	if (got >= 2)
	{
		unsigned char *pp, tt[2];
		if (!big_endian_)
		{
			tt[0] = buf[1];
			tt[1] = buf[0];
			pp = tt;
		}
		else
			pp = buf;

		switch (signed_)
		{
		case 0:
			dec_16bit_.Format("%u", (pp[0]<<8) + pp[1]);
			break;
		case 1:
			dec_16bit_.Format("%d", short((pp[0]<<8) + pp[1]));
			break;
		case 2:
			if ((pp[0]&0x80) != 0)
				dec_16bit_.Format("-%u", ~((pp[0]<<8) + pp[1])&0x7FFF);
			else
				dec_16bit_.Format("%u", (pp[0]<<8) + pp[1]);
			break;
		case 3:
			if ((pp[0]&0x80) != 0)
				dec_16bit_.Format("-%u", ((pp[0]<<8) + pp[1])&0x7FFF);
			else
				dec_16bit_.Format("%u", (pp[0]<<8) + pp[1]);
			break;
		default:
			ASSERT(0);
		}
		AddCommas(dec_16bit_);
	}
	if (got >= 4)
	{
		unsigned char *pp, tt[4];
		if (!big_endian_)
		{
			tt[0] = buf[3];
			tt[1] = buf[2];
			tt[2] = buf[1];
			tt[3] = buf[0];
			pp = tt;
		}
		else
			pp = buf;

		switch (signed_)
		{
		case 0:
			dec_32bit_.Format("%lu",   pp[3] + (long(pp[2])<<8) + 
								(long(pp[1])<<16) + (long(pp[0])<<24));
			break;
		case 1:
			dec_32bit_.Format("%ld",   pp[3] + (long(pp[2])<<8) + 
								(long(pp[1])<<16) + (long(pp[0])<<24));
			break;
		case 2:
			if ((pp[0]&0x80) != 0)
				dec_32bit_.Format("-%lu",   ~(pp[3] + (long(pp[2])<<8) + 
								(long(pp[1])<<16) + (long(pp[0])<<24))&0x7fffFFFFUL);
			else
				dec_32bit_.Format("%lu",   pp[3] + (long(pp[2])<<8) + 
								(long(pp[1])<<16) + (long(pp[0])<<24));
			break;
		case 3:
			if ((pp[0]&0x80) != 0)
				dec_32bit_.Format("-%lu",   pp[3] + (long(pp[2])<<8) + 
								(long(pp[1])<<16) + (long(pp[0]&0x7F)<<24));
			else
				dec_32bit_.Format("%lu",   pp[3] + (long(pp[2])<<8) + 
								(long(pp[1])<<16) + (long(pp[0])<<24));
			break;
		default:
			ASSERT(0);
		}
		AddCommas(dec_32bit_);
	}
	if (got >= 8)
	{
		unsigned char *pp, tt[8];
		if (!big_endian_)
		{
			tt[0] = buf[7];
			tt[1] = buf[6];
			tt[2] = buf[5];
			tt[3] = buf[4];
			tt[4] = buf[3];
			tt[5] = buf[2];
			tt[6] = buf[1];
			tt[7] = buf[0];
			pp = tt;
		}
		else
			pp = buf;

		// Note: %I64d does not seem to work with CString::Format() so use sprintf
		char aa[22] = {'\0'};                    // temp buf where we sprintf

		switch (signed_)
		{
		case 0:
			sprintf(aa, "%I64u",  pp[7] + (__int64(pp[6])<<8) +
							 (__int64(pp[5])<<16) + (__int64(pp[4])<<24) + 
							 (__int64(pp[3])<<32) + (__int64(pp[2])<<40) + 
							 (__int64(pp[1])<<48) + (__int64(pp[0])<<56));
			break;
		case 1:
			sprintf(aa, "%I64d",  pp[7] + (__int64(pp[6])<<8) +
							 (__int64(pp[5])<<16) + (__int64(pp[4])<<24) + 
							 (__int64(pp[3])<<32) + (__int64(pp[2])<<40) + 
							 (__int64(pp[1])<<48) + (__int64(pp[0])<<56));
			break;
		case 2:
			if ((pp[0]&0x80) != 0)
				sprintf(aa, "-%I64u",  ~(pp[7] + (__int64(pp[6])<<8) +
							 (__int64(pp[5])<<16) + (__int64(pp[4])<<24) + 
							 (__int64(pp[3])<<32) + (__int64(pp[2])<<40) + 
							 (__int64(pp[1])<<48) + (__int64(pp[0])<<56))&0x7fffFFFFffffFFFFi64);
			else
				sprintf(aa, "%I64u",  pp[7] + (__int64(pp[6])<<8) +
							 (__int64(pp[5])<<16) + (__int64(pp[4])<<24) + 
							 (__int64(pp[3])<<32) + (__int64(pp[2])<<40) + 
							 (__int64(pp[1])<<48) + (__int64(pp[0])<<56));
			break;
		case 3:
			if ((pp[0]&0x80) != 0)
				sprintf(aa, "-%I64u",  pp[7] + (__int64(pp[6])<<8) +
							 (__int64(pp[5])<<16) + (__int64(pp[4])<<24) + 
							 (__int64(pp[3])<<32) + (__int64(pp[2])<<40) + 
							 (__int64(pp[1])<<48) + (__int64(pp[0]&0x7F)<<56));
			else
				sprintf(aa, "%I64u",  pp[7] + (__int64(pp[6])<<8) +
							 (__int64(pp[5])<<16) + (__int64(pp[4])<<24) + 
							 (__int64(pp[3])<<32) + (__int64(pp[2])<<40) + 
							 (__int64(pp[1])<<48) + (__int64(pp[0])<<56));
			break;
		default:
			ASSERT(0);
		}

		dec_64bit_ = aa;
		AddCommas(dec_64bit_);
	}
	UpdateData(FALSE);

	// The call to UpdateWindow() is necessary during macro replay since (even
	// if property refresh is on) no changes are seen until the macro stops.
	UpdateWindow();
}

BEGIN_MESSAGE_MAP(CPropDecPage, CPropUpdatePage)
		//{{AFX_MSG_MAP(CPropDecPage)
		ON_WM_HELPINFO()
		ON_BN_CLICKED(IDC_DEC_UNSIGNED, OnChangeFormat)
		ON_BN_CLICKED(IDC_BIG_ENDIAN, OnChangeFormat)
		ON_BN_CLICKED(IDC_DEC_2COMP, OnChangeFormat)
		ON_BN_CLICKED(IDC_DEC_1COMP, OnChangeFormat)
		ON_BN_CLICKED(IDC_DEC_SIGN_MAG, OnChangeFormat)
	//}}AFX_MSG_MAP
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropDecPage message handlers

BOOL CPropDecPage::OnInitDialog()
{
	CPropUpdatePage::OnInitDialog();

	// Make sure when dialog is resized that the controls move to sensible places
	resizer_.Create(this);
	resizer_.SetMinimumTrackingSize();
	resizer_.SetGripEnabled(FALSE);
	// Controls to adjust -              left  top  width height
	resizer_.Add(IDC_DEC_GROUP,             0,   0,   100,   0);
	resizer_.Add(IDC_DEC_8BIT,              0,   0,   100,   0);
	resizer_.Add(IDC_DEC_16BIT,             0,   0,   100,   0);
	resizer_.Add(IDC_DEC_32BIT,             0,   0,   100,   0);
	resizer_.Add(IDC_DEC_64BIT,             0,   0,   100,   0);
	resizer_.Add(IDC_BIG_ENDIAN,          100,   0,   0,   0);
	resizer_.Add(IDC_DEC_FORMAT_BOX,          100,   0,   0,   0);
	resizer_.Add(IDC_DEC_UNSIGNED,        100,   0,   0,   0);
	resizer_.Add(IDC_DEC_2COMP,           100,   0,   0,   0);
	resizer_.Add(IDC_DEC_1COMP,           100,   0,   0,   0);
	resizer_.Add(IDC_DEC_SIGN_MAG,        100,   0,   0,   0);

	VERIFY(edit_8bit_.SubclassDlgItem(IDC_DEC_8BIT, this));
	VERIFY(edit_16bit_.SubclassDlgItem(IDC_DEC_16BIT, this));
	VERIFY(edit_32bit_.SubclassDlgItem(IDC_DEC_32BIT, this));
	VERIFY(edit_64bit_.SubclassDlgItem(IDC_DEC_64BIT, this));

	return TRUE;
}

BOOL CPropDecPage::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_CHAR && pMsg->wParam == '\r')
	{
		VERIFY(UpdateData());
		CHexEditView *pview = GetView();
		ASSERT(pview != NULL);

		FILE_ADDRESS start_addr, end_addr;          // Start and end of selection
		pview->GetSelAddr(start_addr, end_addr);

		ASSERT(GetDlgItem(IDC_DEC_8BIT) != NULL);
		ASSERT(GetDlgItem(IDC_DEC_16BIT) != NULL);
		ASSERT(GetDlgItem(IDC_DEC_32BIT) != NULL);
		ASSERT(GetDlgItem(IDC_DEC_64BIT) != NULL);
		if (pMsg->hwnd == GetDlgItem(IDC_DEC_8BIT)->m_hWnd && pview != NULL)
		{
			dec_8bit_.Remove(' ');
			dec_8bit_.Remove('\t');
			char dec_val = atoi(dec_8bit_);
			switch (signed_)
			{
			case 2:
				if (dec_val < 0)
					dec_val = ~(-dec_val);
				break;
			case 3:
				if (dec_val < 0)
					dec_val = (-dec_val) | 0x80;
				break;
			}
			if (pview->ReadOnly())
			{
				AvoidableTaskDialog(IDS_PROP_READONLY, "You can't modify a read only file.");
#ifdef SYS_SOUNDS
				if (!CSystemSound::Play("Read Only"))
#endif
					::Beep(5000,200);
			}
			else if (start_addr + sizeof(dec_val) > pview->GetDocument()->length())
			{
				ASSERT(0);
				AfxMessageBox("Can't modify values past end of file");
			}
			else
			{
				pview->GetDocument()->Change(mod_replace, start_addr, sizeof(dec_val), 
											 (unsigned char *)&dec_val, 0, pview);
				if (end_addr - start_addr != sizeof(dec_val))
					pview->MoveToAddress(start_addr, start_addr + sizeof(dec_val), -1, -1, TRUE);
				((CEdit*)GetDlgItem(IDC_DEC_8BIT))->SetSel(0, -1);
			}
		}
		else if (pMsg->hwnd == GetDlgItem(IDC_DEC_16BIT)->m_hWnd && pview != NULL)
		{
			dec_16bit_.Remove(edit_16bit_.sep_char_);
			dec_16bit_.Remove(' ');
			dec_16bit_.Remove('\t');
			short dec_val = atoi(dec_16bit_);
			switch (signed_)
			{
			case 2:
				if (dec_val < 0)
					dec_val = ~(-dec_val);
				break;
			case 3:
				if (dec_val < 0)
					dec_val = (-dec_val) | 0x8000;
				break;
			}
			if (big_endian_)
			{
				// Reverse byte order
				unsigned char *pp = (unsigned char *)&dec_val;
				unsigned char cc = pp[0]; pp[0] = pp[1]; pp[1] = cc;
			}
			if (pview->ReadOnly())
			{
				AvoidableTaskDialog(IDS_PROP_READONLY, "You can't modify a read only file.");
#ifdef SYS_SOUNDS
				if (!CSystemSound::Play("Read Only"))
#endif
					::Beep(5000,200);
}
			else if (start_addr + sizeof(dec_val) > pview->GetDocument()->length())
			{
				ASSERT(0);
				AfxMessageBox("Can't modify values past end of file");
			}
			else
			{
				pview->GetDocument()->Change(mod_replace, start_addr, sizeof(dec_val), 
											 (unsigned char *)&dec_val, 0, pview);
				if (end_addr - start_addr != sizeof(dec_val))
					pview->MoveToAddress(start_addr, start_addr + sizeof(dec_val), -1, -1, TRUE);
				((CEdit*)GetDlgItem(IDC_DEC_16BIT))->SetSel(0, -1);
			}
		}
		else if (pMsg->hwnd == GetDlgItem(IDC_DEC_32BIT)->m_hWnd && pview != NULL)
		{
			dec_32bit_.Remove(edit_32bit_.sep_char_);
			dec_32bit_.Remove(' ');
			dec_32bit_.Remove('\t');
			long dec_val = atol(dec_32bit_);
			switch (signed_)
			{
			case 2:
				if (dec_val < 0)
					dec_val = ~(-dec_val);
				break;
			case 3:
				if (dec_val < 0)
					dec_val = (-dec_val) | 0x80000000L;
				break;
			}
			if (big_endian_)
			{
				// Reverse byte order
				unsigned char *pp = (unsigned char *)&dec_val;
				unsigned char cc = pp[0]; pp[0] = pp[3]; pp[3] = cc;
				cc = pp[1]; pp[1] = pp[2]; pp[2] = cc;
			}
			if (pview->ReadOnly())
			{
				AvoidableTaskDialog(IDS_PROP_READONLY, "You can't modify a read only file.");
#ifdef SYS_SOUNDS
				if (!CSystemSound::Play("Read Only"))
#endif
					::Beep(5000,200);
			}
			else if (start_addr + sizeof(dec_val) > pview->GetDocument()->length())
			{
				ASSERT(0);
				AfxMessageBox("Can't modify values past end of file");
			}
			else
			{
				pview->GetDocument()->Change(mod_replace, start_addr, sizeof(dec_val), 
											 (unsigned char *)&dec_val, 0, pview);
				if (end_addr - start_addr != sizeof(dec_val))
					pview->MoveToAddress(start_addr, start_addr + sizeof(dec_val), -1, -1, TRUE);
				((CEdit*)GetDlgItem(IDC_DEC_32BIT))->SetSel(0, -1);
			}
		}
		else if (pMsg->hwnd == GetDlgItem(IDC_DEC_64BIT)->m_hWnd && pview != NULL)
		{
			dec_64bit_.Remove(edit_64bit_.sep_char_);
			dec_64bit_.Remove(' ');
			dec_64bit_.Remove('\t');
			__int64 dec_val = _atoi64(dec_64bit_);
			switch (signed_)
			{
			case 2:
				if (dec_val < 0)
					dec_val = ~(-dec_val);
				break;
			case 3:
				if (dec_val < 0)
					dec_val = (-dec_val) | 0x8000000000000000i64;
				break;
			}
			if (big_endian_)
			{
				// Reverse byte order
				unsigned char *pp = (unsigned char *)&dec_val;
				unsigned char cc = pp[0]; pp[0] = pp[7]; pp[7] = cc;
				cc = pp[1]; pp[1] = pp[6]; pp[6] = cc;
				cc = pp[2]; pp[2] = pp[5]; pp[5] = cc;
				cc = pp[3]; pp[3] = pp[4]; pp[4] = cc;
			}
			if (pview->ReadOnly())
			{
				AvoidableTaskDialog(IDS_PROP_READONLY, "You can't modify a read only file.");
#ifdef SYS_SOUNDS
				if (!CSystemSound::Play("Read Only"))
#endif
					::Beep(5000,200);
			}
			else if (start_addr + sizeof(dec_val) > pview->GetDocument()->length())
			{
				ASSERT(0);
				AfxMessageBox("Can't modify values past end of file");
			}
			else
			{
				pview->GetDocument()->Change(mod_replace, start_addr, sizeof(dec_val), 
											 (unsigned char *)&dec_val, 0, pview);
				if (end_addr - start_addr != sizeof(dec_val))
					pview->MoveToAddress(start_addr, start_addr + sizeof(dec_val), -1, -1, TRUE);
				((CEdit*)GetDlgItem(IDC_DEC_64BIT))->SetSel(0, -1);
			}
		}

		return 1;
	}
	else if (pMsg->message == WM_CHAR && pMsg->wParam == '\t')
	{
		if (::GetKeyState(VK_SHIFT) < 0)
			SendMessage(WM_NEXTDLGCTL, 1);
		else
			SendMessage(WM_NEXTDLGCTL);

		return 1;
	}
	else if (pMsg->message == WM_CHAR && pMsg->wParam == '\033')
	{
		// Escape pressed - restore previous value from val_
		UpdateData(FALSE);
		ASSERT(GetDlgItem(IDC_DEC_8BIT) != NULL);
		ASSERT(GetDlgItem(IDC_DEC_16BIT) != NULL);
		ASSERT(GetDlgItem(IDC_DEC_32BIT) != NULL);
		ASSERT(GetDlgItem(IDC_DEC_64BIT) != NULL);
		if (pMsg->hwnd == GetDlgItem(IDC_DEC_8BIT)->m_hWnd)
			((CEdit*)GetDlgItem(IDC_DEC_8BIT))->SetSel(0, -1);
		else if (pMsg->hwnd == GetDlgItem(IDC_DEC_16BIT)->m_hWnd)
			((CEdit*)GetDlgItem(IDC_DEC_16BIT))->SetSel(0, -1);
		else if (pMsg->hwnd == GetDlgItem(IDC_DEC_32BIT)->m_hWnd)
			((CEdit*)GetDlgItem(IDC_DEC_32BIT))->SetSel(0, -1);
		else if (pMsg->hwnd == GetDlgItem(IDC_DEC_64BIT)->m_hWnd)
			((CEdit*)GetDlgItem(IDC_DEC_64BIT))->SetSel(0, -1);

		return 1;
	}

	return CPropUpdatePage::PreTranslateMessage(pMsg);
}

void CPropDecPage::OnChangeFormat()
{
	if (UpdateData(TRUE))       // Update big_endian_, signed_ from buttons
		Update(GetView());      // Recalc values based on new format
}

BOOL CPropDecPage::OnSetActive()
{
	Update(GetView());
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_prop_dec);
	return CPropUpdatePage::OnSetActive();
}

BOOL CPropDecPage::OnHelpInfo(HELPINFO *pHelpInfo)
{
	//theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs3);
	((CMainFrame *)AfxGetMainWnd())->m_wndProp.help_hwnd_ = (HWND)pHelpInfo->hItemHandle;
	return TRUE;
}

void CPropDecPage::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.HtmlHelpContextMenu(pWnd, id_pairs);
}

//===========================================================================

/////////////////////////////////////////////////////////////////////////////
// CPropRealPage property page

IMPLEMENT_DYNCREATE(CPropRealPage, CPropUpdatePage)

CPropRealPage::CPropRealPage() : CPropUpdatePage(CPropRealPage::IDD)
{
		val_ = _T("");
		mant_ = _T("");
		exp_ = _T("");
		desc_ = _T("");
		exp_desc_ = _T("");

		big_endian_ = theApp.prop_fp_endian_;
		format_ = theApp.prop_fp_format_;
		if (format_ > FMT_LAST) format_ = 0;
}

CPropRealPage::~CPropRealPage()
{
	// Save current state for next invocation of property dialog or save settings
	theApp.prop_fp_format_ = format_;
	theApp.prop_fp_endian_ = big_endian_ != 0;
}

void CPropRealPage::DoDataExchange(CDataExchange* pDX)
{
	CPropUpdatePage::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_FP_VAL, val_);
	DDV_MaxChars(pDX, val_, 31);        // C# Decimal can have 29 digits + sign + decimal point
	DDX_Text(pDX, IDC_FP_MANT, mant_);
	//DDV_MaxChars(pDX, mant_, 20);
	DDX_Text(pDX, IDC_FP_EXP, exp_);
	//DDV_MaxChars(pDX, exp_, 5);
	DDX_Check(pDX, IDC_BIG_ENDIAN, big_endian_);
	DDX_CBIndex(pDX, IDC_FP_FORMAT, format_);
	DDX_Text(pDX, IDC_FP_EXP_DESC, exp_desc_);
	DDX_Text(pDX, IDC_FP_DESC, desc_);
	DDX_Control(pDX, IDC_FP_GROUP, ctl_group_);
}

void CPropRealPage::Update(CHexEditView *pv, FILE_ADDRESS address /*=-1*/)
{
	// Set fields to empty in case no view, not enough bytes to EOF etc
	val_ = _T("");
	mant_ = _T("");
	exp_ = _T("");

	// If no view return else get view's current address (if not given)
	if (pv == NULL)
	{
		GetDlgItem(IDC_FP_VAL)->SendMessage(EM_SETREADONLY, TRUE);
		UpdateData(FALSE);
		return;
	}

	size_t needed = 8;
	switch (format_)
	{
	case FMT_IEEE32:
	case FMT_IBM32:
		needed = 4;
		break;
	case FMT_REAL48:
		needed = 6;
		break;
	case FMT_DECIMAL:
		needed = 16;
		break;
	}

	if (address == -1)
		address = pv->GetPos();

	unsigned char buf[16];
	size_t got = dynamic_cast<CHexEditDoc *>(pv->GetDocument())->GetData(buf, sizeof(buf), address);

	// If not enough byte to display FP value just display empty fields
	ASSERT(GetDlgItem(IDC_FP_VAL) != NULL);
	if (got < needed)
	{
		GetDlgItem(IDC_FP_VAL)->SendMessage(EM_SETREADONLY, TRUE);
		UpdateData(FALSE);
		return;
	}
	else
		GetDlgItem(IDC_FP_VAL)->SendMessage(EM_SETREADONLY, pv->ReadOnly());

	if (big_endian_)
		flip_bytes(buf, needed);

	if (format_ == FMT_DECIMAL)
	{
		// Work out info. about the C# Decimal type (which a poor relative of other floating point types)
		val_ = Decimal2String(buf, mant_, exp_);
	}
	else
	{
		// Work out value etc of a "real" floating point number
		double value;
		long double mantissa;
		int exponent;

		switch (format_)
		{
		case FMT_IEEE32:
			value = *(float *)buf;
			mantissa = frexp(value, &exponent);
			mant_.Format("%.7f", (double)mantissa);
			break;
		case FMT_IEEE64:
			value = *(double *)buf;
			mantissa = frexp(value, &exponent);
			mant_.Format("%.16f", (double)mantissa);
			break;
		case FMT_IBM32:
			value = ::ibm_fp32(buf, &exponent, &mantissa, true);
			mant_.Format("%.7f", (double)mantissa);
			break;
		case FMT_IBM64:
			value = ::ibm_fp64(buf, &exponent, &mantissa, true);
			mant_.Format("%.16f", (double)mantissa);
			break;
		case FMT_REAL48:
			value = ::real48(buf, &exponent, &mantissa);
			mant_.Format("%.12f", (double)mantissa);
			break;
		default:
			ASSERT(0);
			value = 0.0;
			exponent = 0;
			break;
		}

		// Work out what to display
		exp_.Format("%d", (int)exponent);
		switch (_fpclass(value))
		{
		case _FPCLASS_SNAN:
		case _FPCLASS_QNAN:
			val_ = "NaN";
			break;
		case _FPCLASS_NINF:
			val_ = "-Inf";
			break;
		case _FPCLASS_PINF:
			val_ = "+Inf";
			break;
		default:
			if (format_ == FMT_IEEE32 || format_ == FMT_IBM32)
				val_.Format("%.7g", (double)value);
			else if (format_ == FMT_REAL48)
				val_.Format("%.12g", (double)value);
			else
				val_.Format("%.16g", (double)value);
			break;
		}
	}

	UpdateData(FALSE);

	// The call to UpdateWindow() is necessary during macro replay since (even
	// if property refresh is on) no changes are seen until the macro stops.
	UpdateWindow();
}

void CPropRealPage::FixDesc()
{
	switch (format_)
	{
	case FMT_IEEE32:
	case FMT_IEEE64:
	case FMT_REAL48:
		exp_desc_ = "Exponent (Power 2):";
		break;
	case FMT_IBM32:
	case FMT_IBM64:
		exp_desc_ = "Exponent (Power 16):";
		break;
	case FMT_DECIMAL:
		exp_desc_ = "Exponent (Power 10):";
		break;
	default:
		ASSERT(0);
	}
}

BEGIN_MESSAGE_MAP(CPropRealPage, CPropUpdatePage)
	ON_CBN_SELCHANGE(IDC_FP_FORMAT, OnChangeFormat)
	ON_BN_CLICKED(IDC_BIG_ENDIAN, OnChangeFormat)
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropRealPage message handlers

BOOL CPropRealPage::OnInitDialog()
{
	CPropUpdatePage::OnInitDialog();

	// Make sure when dialog is resized that the controls move to sensible places
	resizer_.Create(this);
	resizer_.SetMinimumTrackingSize();
	resizer_.SetGripEnabled(FALSE);
	// Controls to adjust -              left  top  width height
	resizer_.Add(IDC_FP_GROUP,              0,   0,   100,   0);
	resizer_.Add(IDC_FP_VAL,                0,   0,   100,   0);
	resizer_.Add(IDC_FP_MANT,               0,   0,   100,   0);
	resizer_.Add(IDC_BIG_ENDIAN,          100,   0,     0,   0);
	resizer_.Add(IDC_FP_FORMAT_DESC,      100,   0,     0,   0);
	resizer_.Add(IDC_FP_FORMAT,           100,   0,     0,   0);
	resizer_.Add(IDC_FP_DESC,             100,   0,     0,   0);

	VERIFY(ctl_val_.SubclassDlgItem(IDC_FP_VAL, this));
	FixDesc();

	return TRUE;
}

BOOL CPropRealPage::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_CHAR && pMsg->wParam == '\r')
	{
		VERIFY(UpdateData());
		CHexEditView *pview = GetView();
		ASSERT(pview != NULL);
		if (pMsg->hwnd == ctl_val_.m_hWnd && pview != NULL)
		{
			val_.TrimLeft();
			val_.TrimRight();

			char *endptr;
			enum {NORMAL, INF, NINF, NaN} special = NORMAL;
			double dbl_val = strtod(val_, &endptr);

			if (format_ == FMT_IEEE32 || format_ == FMT_IEEE64)
			{
				// Special values (only supported for IEEE floats)
				val_.MakeLower();
				if (strncmp(val_, "inf", 3) == 0 || strncmp(val_, "+inf", 4) == 0 || dbl_val == HUGE_VAL)
					special = INF;
				else if (strncmp(val_, "-inf", 4) == 0 || dbl_val == -HUGE_VAL)
					special = NINF;
				else if (strncmp(val_, "nan", 3) == 0)
					special = NaN;
			}

			size_t len = 8;
			switch (format_)
			{
			case FMT_IEEE32:
			case FMT_IBM32:
				len = 4;
				break;
			case FMT_REAL48:
				len = 6;
				break;
			case FMT_DECIMAL:
				len = 16;
				break;
			}

			FILE_ADDRESS start_addr, end_addr;          // Start and end of selection
			pview->GetSelAddr(start_addr, end_addr);
			if (special == NORMAL && endptr - (const char *)val_ < val_.GetLength())
			{
				// Scan of numeric value failed
				CString ss;
				ss.Format("%s is an invalid floating point number.", val_);
				TaskMessageBox("Invalid Number", ss);
				Update(pview);
				ctl_val_.SetFocus();
			}
			else if (pview->ReadOnly())
			{
				AvoidableTaskDialog(IDS_PROP_READONLY, "You can't modify a read only file.");
#ifdef SYS_SOUNDS
				if (!CSystemSound::Play("Read Only"))
#endif
					::Beep(5000,200);
			}
			else if (start_addr + len > pview->GetDocument()->length())
			{
				ASSERT(0);  // this should not happen since the field should be read-only
				AfxMessageBox("Not enough space before end of file");
			}
			else
			{
				unsigned char *pp = NULL;
				float flt_val;
				unsigned char buf[16];

				switch (format_)
				{
				case FMT_IEEE32:
					// Write the 4 byte IEEE number to file
					flt_val = (float)dbl_val;

					// Write special IEEE bit patterns for +/-Inf & NaN
					if (special == INF)
						memcpy((char *)&flt_val, "\x00\x00\x80\x7F", 4);
					else if (special == NINF)
						memcpy((char *)&flt_val, "\x00\x00\x80\xFF", 4);
					else if (special == NaN)
						memcpy((char *)&flt_val, "\xFF\xFF\xFF\xFF", 4);

					pp = (unsigned char *)&flt_val;
					break;
				case FMT_IEEE64:
					// Write special IEEE bit patterns for +/-Inf & NaN
					if (special == INF)
						memcpy((char *)&dbl_val, "\x00\x00\x00\x00\x00\x00\xf0\x7F", 8);
					else if (special == NINF)
						memcpy((char *)&dbl_val, "\x00\x00\x00\x00\x00\x00\xf0\xFF", 8);
					else if (special == NaN)
						memcpy((char *)&dbl_val, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 8);

						pp = (unsigned char *)&dbl_val;
					break;
				case FMT_IBM32:
					if (!::make_ibm_fp32(buf, dbl_val, true))
					{
						TaskMessageBox("Invalid Number", "The value is too big.");
						Update(pview);
						ctl_val_.SetFocus();
					}
					else
						pp = buf;
					break;
				case FMT_IBM64:
					if (!::make_ibm_fp64(buf, dbl_val, true))
					{
						TaskMessageBox("Invalid Number", "The value is too big.");
						Update(pview);
						ctl_val_.SetFocus();
					}
					else
						pp = buf;
					break;
				case FMT_REAL48:
					if (!::make_real48(buf, dbl_val, true))
					{
						TaskMessageBox("Invalid Number", "The value is too big.");
						Update(pview);
						ctl_val_.SetFocus();
					}
					else
						pp = buf;
					break;
				case FMT_DECIMAL:
					if (!String2Decimal(val_, buf))
					{
						TaskMessageBox("Invalid Number", "This number cannot be represented as a Decimal value.");
						Update(pview);
						ctl_val_.SetFocus();
					}
					else
						pp = buf;
					break;
				default:
					ASSERT(0);
				}

				// If we have a value replace it in the file and select it
				if (pp != NULL)
				{
					if (big_endian_)
						flip_bytes(pp, len);

					pview->GetDocument()->Change(mod_replace, start_addr, len,
												pp, 0, pview);
					if (end_addr - start_addr != len)
						pview->MoveToAddress(start_addr, start_addr + len, -1, -1, TRUE);
				}
			}
			ctl_val_.SetSel(0, -1);

			return 1;
		}
	}
	else if (pMsg->message == WM_CHAR && pMsg->wParam == '\t')
	{
		if (::GetKeyState(VK_SHIFT) < 0)
			SendMessage(WM_NEXTDLGCTL, 1);
		else
			SendMessage(WM_NEXTDLGCTL);

		return 1;
	}
	else if (pMsg->message == WM_CHAR && pMsg->wParam == '\033')
	{
		// Escape pressed - restore previous value from val_
		UpdateData(FALSE);
		if (pMsg->hwnd == ctl_val_.m_hWnd)
			ctl_val_.SetSel(0, -1);

		return 1;
	}

	return CPropUpdatePage::PreTranslateMessage(pMsg);
}

void CPropRealPage::OnChangeFormat()
{
	if (UpdateData(TRUE))       // Update big_endian_, (32/64 bit) format_ from buttons
	{
		FixDesc();
		Update(GetView());      // Recalc values based on new format
	}
}

BOOL CPropRealPage::OnSetActive()
{
	Update(GetView());
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_prop_float);
	return CPropUpdatePage::OnSetActive();
}

BOOL CPropRealPage::OnHelpInfo(HELPINFO* pHelpInfo)
{
	//theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs4);
	((CMainFrame *)AfxGetMainWnd())->m_wndProp.help_hwnd_ = (HWND)pHelpInfo->hItemHandle;
	return TRUE;
}

void CPropRealPage::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.HtmlHelpContextMenu(pWnd, id_pairs);
}

/////////////////////////////////////////////////////////////////////////////
// CPropDatePage property page

IMPLEMENT_DYNCREATE(CPropDatePage, CPropUpdatePage)

// Macros to extract just the time or date part of a DATE
#define TIME_PART(x)  ((x)<0.0 ? fabs((x)-ceil(x)) : (x)-floor(x))
#define DATE_PART(x)  ((x)<0.0 ? ceil(x) : floor(x))

CPropDatePage::CPropDatePage() : CPropUpdatePage(CPropDatePage::IDD)
{
	//{{AFX_DATA_INIT(CPropDatePage)
	format_ = -1;
	format_desc_ = _T("");
	date_display_ = _T("");
	big_endian_ = FALSE;
	local_time_ = FALSE;
	//}}AFX_DATA_INIT

	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	big_endian_ = aa->prop_date_endian_;
	local_time_ = TRUE;  // default to local time for now

	format_ = aa->prop_date_format_;
	if (format_ >= FORMAT_LAST) format_ = FORMAT_TIME_T;
	stop_update_ = false;

#ifdef _DEBUG
	// Make sure time_t is as we expect with this compiler
	{
		time_t tt = time_t(86400);
		struct tm *pp = gmtime(&tt);

		if (pp->tm_year != 70 || pp->tm_mon != 0 || pp->tm_mday != 2 || 
			pp->tm_hour != 0  || pp->tm_min != 0 || pp->tm_sec  != 0   )
			ASSERT(0);
	}
#endif

	for (int ii = 0; ii < FORMAT_LAST; ++ii)
	{
		switch (ii)
		{
		case FORMAT_TIME_T:
			date_size_[ii] = 4;
			date_first_[ii].m_dt = 365.0*70.0 + 17 + 2 + TZDiff();  // days from 30/12/1899 to 1/1/1970
			date_first_[ii].SetStatus(COleDateTime::valid);
			date_last_[ii].m_dt = date_first_[ii].m_dt + (0x7fffFFFFUL+0.2)/(24.0*60.0*60.0);
			date_last_[ii].SetStatus(COleDateTime::valid);
			break;
#ifdef TIME64_T
		case FORMAT_TIME64_T:
			date_size_[ii] = 8;
			date_first_[ii].m_dt = 365.0*70.0 + 17 + 2 + TZDiff();  // days from 30/12/1899 to 1/1/1970
			date_first_[ii].SetStatus(COleDateTime::valid);
			date_last_[ii].m_dt = COleDateTime(2999, 12, 31, 23, 59, 59);  // last valid __time64_t (acc to VS help) is the last second of this millenium
			break;
#endif
		case FORMAT_TIME_T_80:
			date_size_[ii] = 4;
			date_first_[ii].m_dt = 365.0*80.0 + 19 + 2 + TZDiff();  // days from 30/12/1899 to 1/1/1980
			date_first_[ii].SetStatus(COleDateTime::valid);
			date_last_[ii].m_dt = date_first_[ii].m_dt + (0x7fffFFFFUL+0.2)/(24.0*60.0*60.0);
			date_last_[ii].SetStatus(COleDateTime::valid);
			break;
		case FORMAT_TIME_T_MINS:
			date_size_[ii] = 4;
			date_first_[ii].m_dt = 365.0*70.0 + 17 + 2 + TZDiff();   // days from 30/12/1899 to 1/1/1970
			date_first_[ii].SetStatus(COleDateTime::valid);
			date_last_[ii].m_dt = date_first_[ii].m_dt + (0x7fffFFFFUL+0.2)/(24.0*60.0);
			date_last_[ii].SetStatus(COleDateTime::valid);
			break;
		case FORMAT_TIME_T_1899:
			date_size_[ii] = 4;
			date_first_[ii].m_dt = TZDiff();
			date_first_[ii].SetStatus(COleDateTime::valid);
			date_last_[ii].m_dt = TZDiff() + (0xffffFFFEUL+0.2)/(24.0*60.0*60.0);
			date_last_[ii].SetStatus(COleDateTime::valid);
			break;
		case FORMAT_OLE:
			date_size_[ii] = sizeof(DATE);
//            date_first_[ii].SetDateTime(100, 1, 1, 0, 0, 0);
			date_first_[ii].SetDateTime(1601, 1, 1, 0, 0, 0);
			// DateTimeCtrl does not seem to like going past midnight of 31/12/9999
//            date_last_[ii].SetDateTime(9999, 12, 31, 23, 59, 59);
			date_last_[ii].SetDateTime(9999, 12, 31, 0, 0, 0);
			break;
		case FORMAT_SYSTEMTIME:
			date_size_[ii] = sizeof(SYSTEMTIME);
//            date_first_[ii].SetDateTime(0, 1, 1, 0, 0, 0);
			date_first_[ii].SetDateTime(1601, 1, 1, 0, 0, 0);
			// DateTimeCtrl does not seem to like going past midnight of 31/12/9999
//            date_last_[ii].SetDateTime(9999, 12, 31, 23, 59, 59);
			date_last_[ii].SetDateTime(9999, 12, 31, 0, 0, 0);
			break;
		case FORMAT_FILETIME:
			date_size_[ii] = sizeof(FILETIME);
			date_first_[ii].SetDateTime(1601, 1, 1, 0, 0, 0);
			// DateTimeCtrl does not seem to like going past midnight of 31/12/9999
//            date_last_[ii].SetDateTime(9999, 12, 31, 23, 59, 59);
			date_last_[ii].SetDateTime(9999, 12, 31, 0, 0, 0);
			break;
		case FORMAT_MSDOS:
			date_size_[ii] = 4;
			date_first_[ii].m_dt = 365.0*80.0 + 19 + 2 + TZDiff();  // days from 30/12/1899 to 1/1/1980
			date_first_[ii].SetStatus(COleDateTime::valid);
			date_last_[ii].m_dt = 365.0*207.0 + 50 + 2 + TZDiff();   // days from 30/12/1899 to 1/1/2107
			date_last_[ii].SetStatus(COleDateTime::valid);
			break;
		default:
			ASSERT(0);
			break;
		}
	}
}

CPropDatePage::~CPropDatePage()
{
	// Save current state for next invocation of property dialog or save settings
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	aa->prop_date_format_ = format_;
	aa->prop_date_endian_ = big_endian_ != 0;
}

void CPropDatePage::DoDataExchange(CDataExchange* pDX)
{
	CPropUpdatePage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropDatePage)
	DDX_Control(pDX, IDC_DATE_TIME, time_ctrl_);
	DDX_Control(pDX, IDC_DATE_DATE, date_ctrl_);
	DDX_CBIndex(pDX, IDC_DATE_FORMAT, format_);
	DDX_Text(pDX, IDC_DATE_FORMAT_DESC, format_desc_);
	DDX_Text(pDX, IDC_DATE_DISPLAY, date_display_);
	DDX_Check(pDX, IDC_DATE_BIG_ENDIAN, big_endian_);
	DDX_Check(pDX, IDC_DATE_LOCAL_TIME, local_time_);
	//}}AFX_DATA_MAP
}

void CPropDatePage::Update(CHexEditView *pv, FILE_ADDRESS address /*=-1*/)
{
	if (stop_update_)
		return;

	COleDateTime odt;               // date time value from file
	odt.m_dt = 0.0;
	odt.SetStatus(COleDateTime::invalid); // default to an invalid date
	bool not_avail = true;                // default to no date available

	// If we have a current view
	if (pv != NULL)
	{
		// Get current address if none supplied
		if (address == -1)
			address = pv->GetPos();

		// Get eneough meory for current date format
		unsigned char buf[128];
		size_t got = dynamic_cast<CHexEditDoc *>(pv->GetDocument())->GetData(buf, date_size_[format_], address);

		// If we got enough bytes (before EOF)
		if (got == date_size_[format_])
		{
			not_avail = false;
			switch (format_)
			{
			case FORMAT_TIME_T:
				if (big_endian_) flip_bytes(buf, got);
				if (*((time_t *)buf) > (time_t)-1)
					odt = COleDateTime(*((time_t *)buf));
				break;
#ifdef TIME64_T
			case FORMAT_TIME64_T:
				if (big_endian_) flip_bytes(buf, got);
				if (*((__time64_t *)buf) > (__time64_t)-1)
					odt = COleDateTime(*((__time64_t *)buf));
				break;
#endif
			case FORMAT_TIME_T_80:
				if (big_endian_) flip_bytes(buf, got);
				if (*((long *)buf) > (time_t)-1)
					odt = COleDateTime(FromTime_t_80(*((long *)buf)));
				break;
			case FORMAT_TIME_T_MINS:
				if (big_endian_) flip_bytes(buf, got);
				if (*((long *)buf) > (time_t)-1)
					odt = COleDateTime(FromTime_t_mins(*((long *)buf)));
				break;
			case FORMAT_TIME_T_1899:
				if (big_endian_) flip_bytes(buf, got);
				if (*(unsigned long *)buf != 0xffffFFFFUL)
					odt = COleDateTime(FromTime_t_1899(*((long *)buf)));
				break;
			case FORMAT_OLE:
				if (big_endian_) flip_bytes(buf, got);
				switch (_fpclass(*((double *)buf)))
				{
				case _FPCLASS_SNAN:
				case _FPCLASS_QNAN:
				case _FPCLASS_NINF:
				case _FPCLASS_PINF:
					// Nothing - odt remains invalid
					break;
				default:
					odt = *((DATE *)buf);
					break;
				}
				break;
			case FORMAT_SYSTEMTIME:
				if (big_endian_)
					flip_each_word(buf, 16);
				odt = *((SYSTEMTIME *)buf);
				break;
			case FORMAT_FILETIME:
				if (big_endian_)
				{
					flip_bytes(buf, 4);
					flip_bytes(buf+4, 4);
				}
				odt = *((FILETIME *)buf);
				break;
			case FORMAT_MSDOS:
#if 0 // No big endian option for MSDOS date/time
				if (big_endian_)
				{
					flip_bytes(buf, 2);
					flip_bytes(buf+2, 2);
				}
#endif
				{
					FILETIME ft;
					if (DosDateTimeToFileTime(*((WORD *)(buf+2)), *((WORD *)buf), &ft))
						odt = ft;
				}
				break;
			default:
				ASSERT(0);
				break;
			}

		}
	}

	// Make sure the date is not apparently valid but out of range
	if (odt.GetStatus() == COleDateTime::valid &&
		(odt < date_first_[format_] || odt > date_last_[format_]))
	{
		odt.SetStatus(COleDateTime::invalid);
	}

	// If date/time not available or is invalid
	if (not_avail || odt.GetStatus() != COleDateTime::valid)
	{
		// Clear the date and time (make invalid)
		date_ctrl_.SetTime((LPSYSTEMTIME)NULL);
		time_ctrl_.SetTime((LPSYSTEMTIME)NULL);
		date_display_ = not_avail ? "No date" : "Invalid date";
	}
	else
	{
		COleDateTime tmp;

		tmp = odt;
		tmp.m_dt = TIME_PART(tmp.m_dt);
		time_ctrl_.SetTime(tmp);

		tmp = odt;
//        tmp.m_dt = DATE_PART(tmp.m_dt);
		date_ctrl_.SetTime(tmp);

		date_display_ = odt.Format("%#c");
	}

	format_desc_.Format("%ld bytes", long(date_size_[format_]));

	ASSERT(GetDlgItem(IDC_DATE_DATE) != NULL);
	ASSERT(GetDlgItem(IDC_DATE_TIME) != NULL);
	ASSERT(GetDlgItem(IDC_DATE_FIRST) != NULL);
	ASSERT(GetDlgItem(IDC_DATE_LAST) != NULL);
	ASSERT(GetDlgItem(IDC_DATE_NOW) != NULL);
	ASSERT(GetDlgItem(IDC_DATE_NULL) != NULL);
	ASSERT(GetDlgItem(IDC_DATE_BIG_ENDIAN) != NULL);
	ASSERT(GetDlgItem(IDC_DATE_LOCAL_TIME) != NULL);

	// Don't allow changes if date/time not available or file is read-only
	GetDlgItem(IDC_DATE_DATE)->EnableWindow(!(not_avail || pv->ReadOnly()));
	GetDlgItem(IDC_DATE_TIME)->EnableWindow(!(not_avail || pv->ReadOnly()));
	GetDlgItem(IDC_DATE_FIRST)->EnableWindow(!(not_avail || pv->ReadOnly()));
	GetDlgItem(IDC_DATE_LAST)->EnableWindow(!(not_avail || pv->ReadOnly()));
	GetDlgItem(IDC_DATE_NOW)->EnableWindow(!(not_avail || pv->ReadOnly()));
	GetDlgItem(IDC_DATE_NULL)->EnableWindow(!(not_avail || pv->ReadOnly()));

	GetDlgItem(IDC_DATE_BIG_ENDIAN)->EnableWindow(format_ != FORMAT_MSDOS);

	UpdateData(FALSE);

	// The call to UpdateWindow() is necessary during macro replay since (even
	// if property refresh is on) no changes are seen until the macro stops.
	UpdateWindow();
}

// Write value from date and time controls to file (using current format_)
void CPropDatePage::save_date()
{
	unsigned char buf[128];

//    VERIFY(UpdateData());
	CHexEditView *pv = GetView();
	if (pv != NULL && !pv->ReadOnly())
	{
		FILE_ADDRESS start_addr, end_addr;          // Start and end of selection
		pv->GetSelAddr(start_addr, end_addr);
		if (start_addr + date_size_[format_] > pv->GetDocument()->length())
		{
			ASSERT(0);
			AfxMessageBox("Not enough space before end of file");
			date_display_ = "No date";
		}
		else
		{
			COleDateTime odt_date, odt_time;
			date_ctrl_.GetTime(odt_date);
			time_ctrl_.GetTime(odt_time);

			// If both date and time are null then use invalid date/time value
			bool is_invalid = odt_date.GetStatus() != COleDateTime::valid &&
							  odt_time.GetStatus() != COleDateTime::valid;

			// For invalid date or time use 0 value (date=30/12/1899 OR time=midnight)
			if (odt_date.GetStatus() != COleDateTime::valid)
				odt_date.m_dt = 0.0;
			if (odt_time.GetStatus() != COleDateTime::valid)
				odt_time.m_dt = 0.0;

			// Combine the date and time (into odt_date)
			odt_date.m_dt = TIME_PART(odt_time.m_dt) + DATE_PART(odt_date.m_dt);
			odt_date.SetStatus(COleDateTime::valid);

			switch (format_)
			{
			case FORMAT_TIME_T:
				if (is_invalid)
					*((time_t *)buf) = (time_t)-1;
				else
					*((time_t *)buf) = time_t((odt_date.m_dt - (365.0*70.0 + 17 + 2) - TZDiff())*(24.0*60.0*60.0) + 0.5);
				if (big_endian_) flip_bytes(buf, date_size_[format_]);
				break;
#ifdef TIME64_T
			case FORMAT_TIME64_T:
				if (is_invalid)
					*((__time64_t *)buf) = (__time64_t)-1;
				else
					*((__time64_t *)buf) = __time64_t((odt_date.m_dt - (365.0*70.0 + 17 + 2) - TZDiff())*(24.0*60.0*60.0) + 0.5);
				if (big_endian_) flip_bytes(buf, date_size_[format_]);
				break;
#endif
			case FORMAT_TIME_T_80:
				if (is_invalid)
					*((long *)buf) = -1L;
				else
					*((long *)buf) = long((odt_date.m_dt - (365.0*80.0 + 19 + 2) - TZDiff())*(24.0*60.0*60.0) + 0.5);
				if (big_endian_) flip_bytes(buf, date_size_[format_]);
				break;
			case FORMAT_TIME_T_MINS:
				if (is_invalid)
					*((long *)buf) = -1L;
				else
					*((long *)buf) = long((odt_date.m_dt - (365.0*70.0 + 17 + 2) - TZDiff())*(24.0*60.0) + 0.5);
				if (big_endian_) flip_bytes(buf, date_size_[format_]);
				break;
			case FORMAT_TIME_T_1899:
				if (is_invalid)
					*((unsigned long *)buf) = 0xffffFFFFUL;
				else
					*((unsigned long *)buf) = (unsigned long)((odt_date.m_dt-TZDiff())*(24.0*60.0*60.0) + 0.5);
				if (big_endian_) flip_bytes(buf, date_size_[format_]);
				break;
			case FORMAT_OLE:
				if (is_invalid)
					memset(buf, '\xFF', date_size_[format_]);
				else
					*((DATE *)buf) = odt_date;
				if (big_endian_) flip_bytes(buf, date_size_[format_]);
				break;
			case FORMAT_SYSTEMTIME:
				memset(buf, '\0', sizeof(SYSTEMTIME));
				if (!is_invalid)
				{
					((SYSTEMTIME *)buf)->wYear = odt_date.GetYear();
					((SYSTEMTIME *)buf)->wMonth = odt_date.GetMonth();
					((SYSTEMTIME *)buf)->wDay = odt_date.GetDay();
					((SYSTEMTIME *)buf)->wDayOfWeek = odt_date.GetDayOfWeek();
					((SYSTEMTIME *)buf)->wHour = odt_date.GetHour();
					((SYSTEMTIME *)buf)->wMinute = odt_date.GetMinute();
					((SYSTEMTIME *)buf)->wSecond = odt_date.GetSecond();
					((SYSTEMTIME *)buf)->wMilliseconds = 500;
				}
				if (big_endian_)
					flip_each_word(buf, 16);
				break;
			case FORMAT_FILETIME:
				memset(buf, '\0', date_size_[format_]-1);
				buf[date_size_[format_]-1] = '\x80';
				if (!is_invalid)
				{
					SYSTEMTIME st;
					FILETIME ft;
					st.wYear = odt_date.GetYear();
					st.wMonth = odt_date.GetMonth();
					st.wDay = odt_date.GetDay();
					st.wDayOfWeek = odt_date.GetDayOfWeek();
					st.wHour = odt_date.GetHour();
					st.wMinute = odt_date.GetMinute();
					st.wSecond = odt_date.GetSecond();
					st.wMilliseconds = 500;
					SystemTimeToFileTime(&st, &ft);
					LocalFileTimeToFileTime(&ft, (FILETIME *)buf);
				}
				if (big_endian_)
				{
					flip_bytes(buf, 4);
					flip_bytes(buf+4, 4);
				}
				break;
			case FORMAT_MSDOS:
				memset(buf, '\0', date_size_[FORMAT_MSDOS]);
				if (!is_invalid)
				{
					SYSTEMTIME st;
					FILETIME ft, ft2;
					st.wYear = odt_date.GetYear();
					st.wMonth = odt_date.GetMonth();
					st.wDay = odt_date.GetDay();
					st.wDayOfWeek = odt_date.GetDayOfWeek();
					st.wHour = odt_date.GetHour();
					st.wMinute = odt_date.GetMinute();
					st.wSecond = odt_date.GetSecond();
					st.wMilliseconds = 500;
					SystemTimeToFileTime(&st, &ft);
					LocalFileTimeToFileTime(&ft, &ft2);
					FileTimeToDosDateTime(&ft2, LPWORD(buf+2), LPWORD(buf));
				}
#if 0 // No big endian option for MSDOS date/time
				if (big_endian_)
				{
					flip_bytes(buf, 2);
					flip_bytes(buf+2, 2);
				}
#endif
				break;
			default:
				ASSERT(0);
				break;
			}

			// Write the new date to file
			pv->GetDocument()->Change(mod_replace, start_addr, date_size_[format_],
										 buf, 0, pv);
			if (end_addr - start_addr != date_size_[format_])
				pv->MoveToAddress(start_addr, start_addr + date_size_[format_], -1, -1, TRUE);

			// Update the long date display string too
			if (is_invalid)
				date_display_ = "Invalid date";
			else
				date_display_ = odt_date.Format("%#c");
		}
	}
	else
		ASSERT(0);

	UpdateData(FALSE);  // update long date display
}

BEGIN_MESSAGE_MAP(CPropDatePage, CPropUpdatePage)
	//{{AFX_MSG_MAP(CPropDatePage)
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_DATE_FIRST, OnDateFirst)
	ON_BN_CLICKED(IDC_DATE_LAST, OnDateLast)
	ON_BN_CLICKED(IDC_DATE_NOW, OnDateNow)
	ON_BN_CLICKED(IDC_DATE_NULL, OnDateNull)
	ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATE_DATE, OnDateChange)
	ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATE_TIME, OnTimeChange)
	ON_BN_CLICKED(IDC_DATE_BIG_ENDIAN, OnChangeFormat)
	ON_BN_CLICKED(IDC_DATE_LOCAL_TIME, OnChangeFormat)
	ON_CBN_SELCHANGE(IDC_DATE_FORMAT, OnChangeFormat)
	//}}AFX_MSG_MAP
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropDatePage message handlers

BOOL CPropDatePage::OnInitDialog()
{
	CPropUpdatePage::OnInitDialog();
	date_ctrl_.SetRange(&date_first_[format_], &date_last_[format_]);

	return TRUE;
}

BOOL CPropDatePage::OnSetActive()
{
	Update(GetView());
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_prop_date);
	return CPropUpdatePage::OnSetActive();
}

BOOL CPropDatePage::OnHelpInfo(HELPINFO* pHelpInfo)
{
	//theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs6);
	((CMainFrame *)AfxGetMainWnd())->m_wndProp.help_hwnd_ = (HWND)pHelpInfo->hItemHandle;
	return TRUE;
}

void CPropDatePage::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.HtmlHelpContextMenu(pWnd, id_pairs);
}

void CPropDatePage::OnDateFirst()
{
	COleDateTime tmp;
	tmp = date_first_[format_];
	tmp.m_dt = TIME_PART(tmp.m_dt);
	time_ctrl_.SetTime(tmp);
	tmp = date_first_[format_];
//    tmp.m_dt = DATE_PART(tmp.m_dt);
	date_ctrl_.SetTime(tmp);
	save_date();
}

void CPropDatePage::OnDateLast()
{
	COleDateTime tmp;
	tmp = date_last_[format_];
	tmp.m_dt = TIME_PART(tmp.m_dt);
	time_ctrl_.SetTime(tmp);
	tmp = date_last_[format_];
//    tmp.m_dt = DATE_PART(tmp.m_dt);
	date_ctrl_.SetTime(tmp);
	save_date();
}

void CPropDatePage::OnDateNow()
{
	COleDateTime odt = COleDateTime::GetCurrentTime();
	COleDateTime tmp;
	tmp = odt;
	tmp.m_dt = TIME_PART(tmp.m_dt);
	time_ctrl_.SetTime(tmp);
	tmp = odt;
//    tmp.m_dt = DATE_PART(tmp.m_dt);
	date_ctrl_.SetTime(tmp);
	save_date();
}

void CPropDatePage::OnDateNull()
{
	COleDateTime tmp;
	tmp = date_first_[format_];
	tmp.m_dt = TIME_PART(tmp.m_dt);
	time_ctrl_.SetTime(tmp);
	time_ctrl_.SetTime((LPSYSTEMTIME)NULL);
	tmp = date_first_[format_];
//    tmp.m_dt = DATE_PART(tmp.m_dt);
	date_ctrl_.SetTime(tmp);
	date_ctrl_.SetTime((LPSYSTEMTIME)NULL);
	save_date();
}

void CPropDatePage::OnDateChange(NMHDR* pNMHDR, LRESULT* pResult)
{
	SYSTEMTIME st;
	if (date_ctrl_.GetTime(&st) != GDT_VALID)
	{
		// Date invalid - also make time invalid
		time_ctrl_.SetTime((LPSYSTEMTIME)NULL);
	}
	else if (time_ctrl_.GetTime(&st) != GDT_VALID)
	{
		// Date valid but time not - make time valid
		time_ctrl_.SetTime(&st);
	}

	stop_update_ = true;  // Prevent write to file causing call to Update() which make control lose focus
	save_date();
	stop_update_ = false;

	*pResult = 0;
}

void CPropDatePage::OnTimeChange(NMHDR* pNMHDR, LRESULT* pResult)
{
	SYSTEMTIME st;
	if (time_ctrl_.GetTime(&st) != GDT_VALID)
	{
		// Time invalid - also make date invalid
		date_ctrl_.SetTime((LPSYSTEMTIME)NULL);
	}
	else if (date_ctrl_.GetTime(&st) != GDT_VALID)
	{
		// Time valid but date not - make date valid
		date_ctrl_.SetTime(&st);
	}

	stop_update_ = true;  // Prevent write to file causing call to Update() which make control lose focus
	save_date();
	stop_update_ = false;

	*pResult = 0;
}

void CPropDatePage::OnChangeFormat()
{
	if (UpdateData(TRUE))       // Update format_, big_endian_, local_time_ from buttons
	{
		// TBD: TODO if UTC subtract TZDiff() ?
		date_ctrl_.SetRange(&date_first_[format_], &date_last_[format_]);
		Update(GetView());      // Recalc values based on new format
	}
}

//===========================================================================
/////////////////////////////////////////////////////////////////////////////
// CPropGraphPage property page

IMPLEMENT_DYNCREATE(CPropGraphPage, CPropUpdatePage)

CPropGraphPage::CPropGraphPage() : CPropUpdatePage(CPropGraphPage::IDD)
{
	m_graph = new CSimpleGraph();
}

CPropGraphPage::~CPropGraphPage()
{
	delete m_graph;
}

void CPropGraphPage::DoDataExchange(CDataExchange* pDX)
{
	CPropUpdatePage::DoDataExchange(pDX);
//	DDX_Control(pDX, IDC_STATS_GRAPH, *m_graph);
}

void CPropGraphPage::Update(CHexEditView *pv, FILE_ADDRESS address)
{
	CString mess;
	std::vector<FILE_ADDRESS> cnt;
	std::vector<COLORREF> col;

	if (!theApp.bg_stats_)
	{
		mess = "Counts and statistics are not available.  Please turn on the Statistics option "
			   "in the Background Processing page of the Options dialog.";
		GetDlgItem(IDC_STATS_OPTIONS)->ShowWindow(SW_SHOW);
		goto error_return;
	}

	if (pv == NULL)
	{
		mess = "There is no file open.";
		goto error_return;
	}

	FILE_ADDRESS biggest = pv->GetDocument()->GetByteCounts(cnt);

	if (biggest == -2)
	{
		mess.Format("Statistics calculations in progress (%d%% complete).", pv->GetDocument()->StatsProgress());
		goto error_return;
	}

	if (biggest < 0)
	{
		mess = "Background statistics are not permitted for this type of file.  "
			   "Please see the Background Processing page of the Options dialog.";
		GetDlgItem(IDC_STATS_OPTIONS)->ShowWindow(SW_SHOW);
		goto error_return;
	}

	pv->get_colours(col);

	m_graph->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_STATS_MESSAGE)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATS_OPTIONS)->ShowWindow(SW_HIDE);
	m_graph->SetData(biggest, cnt, col);
	return;

error_return:
	m_graph->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATS_MESSAGE)->ShowWindow(SW_SHOW);

	// Only update the string if it is different to avoid flicker
	CString strTmp;
	GetDlgItemText(IDC_STATS_MESSAGE, strTmp);
	if (mess != strTmp)
		SetDlgItemText(IDC_STATS_MESSAGE, mess);

	return;
}

BEGIN_MESSAGE_MAP(CPropGraphPage, CPropUpdatePage)
	ON_WM_HELPINFO()
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_BN_CLICKED(IDC_STATS_OPTIONS, OnStatsOptions)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropGraphPage message handlers

BOOL CPropGraphPage::OnInitDialog()
{
	CPropUpdatePage::OnInitDialog();

	static CString graphClass;
	if (graphClass.IsEmpty())
	{
		// Register new window class
		graphClass = AfxRegisterWndClass(0);
		ASSERT(!graphClass.IsEmpty());
	}

	CRect rct;  // New window location
	ASSERT(GetDlgItem(IDC_PLACEHOLDER) != NULL);
	GetDlgItem(IDC_PLACEHOLDER)->GetWindowRect(&rct);
	GetDlgItem(IDC_PLACEHOLDER)->ShowWindow(SW_HIDE);
	ScreenToClient(&rct);

	DWORD graphStyle = WS_CHILD | WS_VISIBLE;
	VERIFY(m_graph->Create(graphClass, NULL, graphStyle, rct, this, IDC_STATS_GRAPH));

	// Make window topmost - xxx is this nec. or a good idea?
	m_graph->SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOSIZE|SWP_NOOWNERZORDER);

	return TRUE;
}

void CPropGraphPage::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
	if (m_graph != NULL && m_graph->m_hWnd != (HWND)0)
		m_graph->MoveWindow(0, 0, cx, cy);
}

BOOL CPropGraphPage::OnSetActive()
{
	Update(GetView());
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_prop_graph);
	return CPropUpdatePage::OnSetActive();
}

void CPropGraphPage::OnStatsOptions()
{
	theApp.display_options(BACKGROUND_OPTIONS_PAGE, TRUE);
}

BOOL CPropGraphPage::OnHelpInfo(HELPINFO* pHelpInfo)
{
	((CMainFrame *)AfxGetMainWnd())->m_wndProp.help_hwnd_ = (HWND)pHelpInfo->hItemHandle;
	return TRUE;
}

void CPropGraphPage::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.HtmlHelpContextMenu(pWnd, id_pairs);
}

//===========================================================================
/////////////////////////////////////////////////////////////////////////////
// CPropStatsPage property page

IMPLEMENT_DYNCREATE(CPropStatsPage, CPropUpdatePage)

CPropStatsPage::CPropStatsPage() : CPropUpdatePage(CPropStatsPage::IDD)
{
}

CPropStatsPage::~CPropStatsPage()
{
}

void CPropStatsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropUpdatePage::DoDataExchange(pDX);
}

void CPropStatsPage::Update(CHexEditView *pv, FILE_ADDRESS address)
{
	CString mess, strCrc32, strMd5, strSha1, strSha256, strSha512;

	if (!theApp.bg_stats_)
	{
		mess = "Not available.";
		goto error_return;
	}

	if (pv == NULL)
	{
		mess = "No file open.";
		goto error_return;
	}

	unsigned char digest[128];
	unsigned long crc;

	if (pv->GetDocument()->GetCRC32(crc) >= 0)
	{
		if (theApp.hex_ucase_)
			strCrc32.Format("%08X", crc);
		else
			strCrc32.Format("%08X", crc);
	}

	if (pv->GetDocument()->GetMd5(digest) >= 0)
	{
		const char * fmt;
		if (theApp.hex_ucase_)
			fmt = "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X";
		else
			fmt = "%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x";
		strMd5.Format(fmt,
				digest[0], digest[1], digest[2], digest[3],
				digest[4], digest[5], digest[6], digest[7],
				digest[8], digest[9], digest[10], digest[11],
				digest[12], digest[13], digest[14], digest[15]);
		AddSpaces(strMd5);
	}

	if (pv->GetDocument()->GetSha1(digest) >= 0)
	{
		const char * fmt;
		if (theApp.hex_ucase_)
			fmt = "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X";
		else
			fmt = "%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x";
		strSha1.Format(fmt,
				digest[0],  digest[1],  digest[2],  digest[3],
				digest[4],  digest[5],  digest[6],  digest[7],
				digest[8],  digest[9],  digest[10], digest[11],
				digest[12], digest[13], digest[14], digest[15],
				digest[16], digest[17], digest[18], digest[19]);
		AddSpaces(strSha1);
	}

	if (pv->GetDocument()->GetSha256(digest) >= 0)
	{
		const char * fmt;
		if (theApp.hex_ucase_)
			fmt = "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X"
			"%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X"
			"%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X"
			"%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X";
		else
			fmt = "%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%"
			"%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x"
			"%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x"
			"%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x";
		strSha256.Format(fmt,
			digest[0], digest[1], digest[2], digest[3],
			digest[4], digest[5], digest[6], digest[7],
			digest[8], digest[9], digest[10], digest[11],
			digest[12], digest[13], digest[14], digest[15],
			digest[16], digest[17], digest[18], digest[19],
			digest[20], digest[21], digest[22], digest[23],
			digest[24], digest[25], digest[26], digest[27],
			digest[28], digest[29], digest[30], digest[31]
			);
		AddSpaces(strSha256);
	}

	if (pv->GetDocument()->GetSha512(digest) >= 0)
	{
		const char * fmt;
		if (theApp.hex_ucase_)
			fmt = "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X"
			"%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X"
			"%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X"
			"%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X"
			"%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X"
			"%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X"
			"%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X"
			"%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X";
		else
			fmt = "%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%"
			"%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x"
			"%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x"
			"%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x"
			"%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x"
			"%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x"
			"%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x"
			"%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x";
		strSha512.Format(fmt,
			digest[0], digest[1], digest[2], digest[3],
			digest[4], digest[5], digest[6], digest[7],
			digest[8], digest[9], digest[10], digest[11],
			digest[12], digest[13], digest[14], digest[15],
			digest[16], digest[17], digest[18], digest[19],
			digest[20], digest[21], digest[22], digest[23],
			digest[24], digest[25], digest[26], digest[27],
			digest[28], digest[29], digest[30], digest[31],
			digest[32], digest[33], digest[34], digest[35],
			digest[36], digest[37], digest[38], digest[39],
			digest[40], digest[41], digest[42], digest[43],
			digest[44], digest[45], digest[46], digest[47],
			digest[48], digest[49], digest[50], digest[51],
			digest[52], digest[53], digest[54], digest[55],
			digest[56], digest[57], digest[58], digest[59],
			digest[60], digest[61], digest[62], digest[63]
			);
		AddSpaces(strSha512);
	}

error_return:
	CString ss;
	GetDlgItemText(IDC_STATS_CRC32, ss);
	if (ss != strCrc32)
		SetDlgItemText(IDC_STATS_CRC32, strCrc32);
	GetDlgItemText(IDC_STATS_MD5, ss);
	if (ss != strMd5)
		SetDlgItemText(IDC_STATS_MD5, strMd5);
	GetDlgItemText(IDC_STATS_SHA1, ss);
	if (ss != strSha1)
		SetDlgItemText(IDC_STATS_SHA1, strSha1);
	GetDlgItemText(IDC_STATS_SHA256, ss);
	if (ss != strSha256)
		SetDlgItemText(IDC_STATS_SHA256, strSha256);
	GetDlgItemText(IDC_STATS_SHA512, ss);
	if (ss != strSha512)
		SetDlgItemText(IDC_STATS_SHA512, strSha512);

	return;
}

BEGIN_MESSAGE_MAP(CPropStatsPage, CPropUpdatePage)
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropStatsPage message handlers

BOOL CPropStatsPage::OnInitDialog()
{
	CPropUpdatePage::OnInitDialog();

	// Make sure when dialog is resized that the controls move to sensible places
	resizer_.Create(this);
	resizer_.SetMinimumTrackingSize();
	resizer_.SetGripEnabled(FALSE);
	// Controls to adjust -              left  top  width height
	resizer_.Add(IDC_STATS_CRC32,          0,   0, 100,   0);
	resizer_.Add(IDC_STATS_MD5,            0,   0, 100,   0);
	resizer_.Add(IDC_STATS_SHA1, 0, 0, 100, 0);
	resizer_.Add(IDC_STATS_SHA256, 0, 0, 100, 0);
	resizer_.Add(IDC_STATS_SHA512, 0, 0, 100, 0);

	return TRUE;
}

BOOL CPropStatsPage::OnSetActive()
{
	Update(GetView());
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_prop_stats);
	return CPropUpdatePage::OnSetActive();
}

BOOL CPropStatsPage::OnHelpInfo(HELPINFO* pHelpInfo)
{
	((CMainFrame *)AfxGetMainWnd())->m_wndProp.help_hwnd_ = (HWND)pHelpInfo->hItemHandle;
	return TRUE;
}

void CPropStatsPage::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.HtmlHelpContextMenu(pWnd, id_pairs);
}

//===========================================================================
/////////////////////////////////////////////////////////////////////////////
// CPropBitmapPage property page

IMPLEMENT_DYNCREATE(CPropBitmapPage, CPropUpdatePage)

CPropBitmapPage::CPropBitmapPage() : CPropUpdatePage(CPropBitmapPage::IDD)
{
}

CPropBitmapPage::~CPropBitmapPage()
{
}

void CPropBitmapPage::DoDataExchange(CDataExchange* pDX)
{
	CPropUpdatePage::DoDataExchange(pDX);
}

void CPropBitmapPage::Update(CHexEditView *pv, FILE_ADDRESS address)
{
	CHexEditDoc *pDoc;
	CString ss;                           // used to get old value for comparison to avoid flicker
	CString format, bpp, width, height;   // values for text controls

	if (pv == NULL || (pDoc = pv->GetDocument()) == NULL)
	{
		hide_disk_info();
		goto error_return;
	}

	if (pDoc->IsModified() && pDoc->GetDiskBmpInfo(format, bpp, width, height) == 0)
	{
		SetDlgItemText(IDC_BMP_DESC_MEMORY, "MEMORY");    // Distinguish MEMORY column from DISK column
		hide_disk_info(false);                            // Show disk info text boxes

		GetDlgItemText(IDC_BMP_FORMAT_DISK, ss);
		if (ss != format)
			SetDlgItemText(IDC_BMP_FORMAT_DISK, format);
		GetDlgItemText(IDC_BMP_BPP_DISK, ss);
		if (ss != bpp)
			SetDlgItemText(IDC_BMP_BPP_DISK, bpp);
		GetDlgItemText(IDC_BMP_WIDTH_DISK, ss);
		if (ss != width)
			SetDlgItemText(IDC_BMP_WIDTH_DISK, width);
		GetDlgItemText(IDC_BMP_HEIGHT_DISK, ss);
		if (ss != height)
			SetDlgItemText(IDC_BMP_HEIGHT_DISK, height);
	}
	else
	{
		SetDlgItemText(IDC_BMP_DESC_MEMORY, "");
		hide_disk_info(true);
	}

	if (pDoc->GetBmpInfo(format, bpp, width, height) == -4)
	{
		SetDlgItemText(IDC_BMP_DESC_MEMORY, "Bitmap properties only available in preview");
		hide_disk_info(true);
	}

error_return:
	GetDlgItemText(IDC_BMP_FORMAT, ss);
	if (ss != format)
		SetDlgItemText(IDC_BMP_FORMAT, format);
	GetDlgItemText(IDC_BMP_BPP, ss);
	if (ss != bpp)
		SetDlgItemText(IDC_BMP_BPP, bpp);
	GetDlgItemText(IDC_BMP_WIDTH, ss);
	if (ss != width)
		SetDlgItemText(IDC_BMP_WIDTH, width);
	GetDlgItemText(IDC_BMP_HEIGHT, ss);
	if (ss != height)
		SetDlgItemText(IDC_BMP_HEIGHT, height);
	return;
}

// If memory info is the same as disk info we hide all the controls showing disk info
void CPropBitmapPage::hide_disk_info(bool hide)
{
	GetDlgItem(IDC_BMP_DESC_DISK)->ShowWindow(hide ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_BMP_FORMAT_DISK)->ShowWindow(hide ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_BMP_BPP_DISK)->ShowWindow(hide ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_BMP_WIDTH_DISK)->ShowWindow(hide ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_BMP_HEIGHT_DISK)->ShowWindow(hide ? SW_HIDE : SW_SHOW);
}

BEGIN_MESSAGE_MAP(CPropBitmapPage, CPropUpdatePage)
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropBitmapPage message handlers

BOOL CPropBitmapPage::OnInitDialog()
{
	CPropUpdatePage::OnInitDialog();

	return TRUE;
}

BOOL CPropBitmapPage::OnSetActive()
{
	Update(GetView());
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_prop_bitmap);
	return CPropUpdatePage::OnSetActive();
}

BOOL CPropBitmapPage::OnHelpInfo(HELPINFO* pHelpInfo)
{
	((CMainFrame *)AfxGetMainWnd())->m_wndProp.help_hwnd_ = (HWND)pHelpInfo->hItemHandle;
	return TRUE;
}

void CPropBitmapPage::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.HtmlHelpContextMenu(pWnd, id_pairs);
}
