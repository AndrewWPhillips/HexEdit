// CopyCSrc.cpp : implementation file
//

#include "stdafx.h"
#include "hexedit.h"
#include "CopyCSrc.h"
#include "Bin2Src.h"
#include "resource.hm"

#include <HtmlHelp.h>
#include "HelpID.hm"            // For help IDs for the pages

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCopyCSrc dialog

CCopyCSrc::CCopyCSrc(CHexEditDoc *pdoc, __int64 start, __int64 end, int rowsize, int offset, int addr_flags, CWnd* pParent /*=NULL*/)
	: init_(false),
	  pdoc_(pdoc), start_(start), end_(end),
	  rowsize_(rowsize), offset_(offset), addr_flags_(addr_flags),
	  CDialog(CCopyCSrc::IDD, pParent)
{

	//{{AFX_DATA_INIT(CCopyCSrc)
	for_ = -1;
	big_endian_ = FALSE;
	type_ = -1;
	float_size_ = -1;
	int_size_ = -1;
	int_type_ = -1;
	align_cols_ = FALSE;
	show_address_ = FALSE;
	indent_ = 0;
	//}}AFX_DATA_INIT

	for_         = theApp.GetProfileInt("CopySource", "For", 1);
	type_        = theApp.GetProfileInt("CopySource", "Type", STRING);
	int_type_    = theApp.GetProfileInt("CopySource", "IntType", INT_HEX);
	int_size_    = theApp.GetProfileInt("CopySource", "IntSize", INT_32);
	float_size_  = theApp.GetProfileInt("CopySource", "FloatSize", FLOAT_64);
	big_endian_  = theApp.GetProfileInt("CopySource", "BigEndian", FALSE);
	align_cols_  = theApp.GetProfileInt("CopySource", "Align", TRUE);
	show_address_= theApp.GetProfileInt("CopySource", "ShowAddress", FALSE);
	indent_      = theApp.GetProfileInt("CopySource", "Indent", 8);
	preview_     = theApp.GetProfileInt("CopySource", "Preview", 0) != 0;
}

void CCopyCSrc::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCopyCSrc)
	DDX_Control(pDX, IDC_CSRC_INDENT_SPIN, ctl_indent_spin_);
	DDX_Control(pDX, IDC_CSRC_INT_TYPE, ctl_int_type_);
	DDX_Control(pDX, IDC_CSRC_INT_SIZE, ctl_int_size_);
	DDX_Control(pDX, IDC_CSRC_FLOAT_SIZE, ctl_float_size_);
	DDX_Control(pDX, IDC_BIG_ENDIAN, ctl_big_endian_);
	DDX_CBIndex(pDX, IDC_CSRC_FOR, for_);
	DDX_Check(pDX, IDC_BIG_ENDIAN, big_endian_);
	DDX_Radio(pDX, IDC_CSRC_STRING, type_);
	DDX_CBIndex(pDX, IDC_CSRC_FLOAT_SIZE, float_size_);
	DDX_CBIndex(pDX, IDC_CSRC_INT_SIZE, int_size_);
	DDX_CBIndex(pDX, IDC_CSRC_INT_TYPE, int_type_);
	DDX_Check(pDX, IDC_CSRC_ALIGN, align_cols_);
	DDX_Check(pDX, IDC_CSRC_ADDRESS, show_address_);
	DDX_Text(pDX, IDC_CSRC_INDENT, indent_);
	DDV_MinMaxUInt(pDX, indent_, 0, 127);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CCopyCSrc, CDialog)
	//{{AFX_MSG_MAP(CCopyCSrc)
	ON_BN_CLICKED(IDC_CSRC_CHAR, OnChangeType)
	ON_BN_CLICKED(IDC_CSRC_FLOAT, OnChangeType)
	ON_BN_CLICKED(IDC_CSRC_INT, OnChangeType)
	ON_BN_CLICKED(IDC_CSRC_STRING, OnChangeType)
	ON_BN_CLICKED(IDC_CSRC_HELP, OnCsrcHelp)
	ON_BN_CLICKED(IDC_PREVIEW, OnPreviewToggle)
	ON_EN_CHANGE(IDC_CSRC_INDENT, OnChange)
	ON_BN_CLICKED(IDC_CSRC_ALIGN, OnChange)
	ON_BN_CLICKED(IDC_CSRC_ADDRESS, OnChange)
	ON_CBN_SELCHANGE(IDC_CSRC_FOR, OnChange)
	ON_CBN_SELCHANGE(IDC_CSRC_INT_TYPE, OnChange)
	ON_CBN_SELCHANGE(IDC_CSRC_INT_SIZE, OnChange)
	ON_CBN_SELCHANGE(IDC_CSRC_FLOAT_SIZE, OnChange)
	ON_BN_CLICKED(IDC_BIG_ENDIAN, OnChange)
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CCopyCSrc::fix_controls()
{
	ctl_int_type_.EnableWindow(type_ == INT);
	ctl_int_size_.EnableWindow(type_ == INT);
//	ctl_.EnableWindow(type_ == INT);
	ctl_float_size_.EnableWindow(type_ == FLOAT);
	ctl_big_endian_.EnableWindow(type_ == INT || type_ == FLOAT);
}

void CCopyCSrc::show_preview()
{
	CRect wrct, rctp, rctb;
	GetClientRect(&wrct);
	GetDlgItem(IDC_PREVIEW)->GetWindowRect(&rctb);       // button (just left of preview box)
	ScreenToClient(&rctb);
	GetDlgItem(IDC_CSRC_PREVIEW)->GetWindowRect(&rctp);  // preview box
	ScreenToClient(&rctp);

	int diff = rctp.right - rctb.right;     // Amount to resize the window by (larger or smaller)

	if (preview_)
	{
		if (wrct.right > rctp.right)
			return;
		SetDlgItemText(IDC_PREVIEW, "Preview <<");
	}
	else
	{
		if (wrct.right < rctp.left)
			return;

		SetDlgItemText(IDC_PREVIEW, "Preview >>");
		diff = -diff;   // make smaller
	}

	GetWindowRect(&wrct);
	wrct.right += diff;
	SetWindowPos(NULL, wrct.left, wrct.top, wrct.Width(), wrct.Height(), SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);
}

void CCopyCSrc::update_preview()
{
	Bin2Src formatter(pdoc_);

	Bin2Src::lang lang = Bin2Src::NOLANG;
	if (for_ == 1)
		lang = Bin2Src::C;

	Bin2Src::type typ;
	switch (type_)
	{
	case CCopyCSrc::STRING:
		typ = Bin2Src::STRING;
		break;
	case CCopyCSrc::CHAR:
		typ = Bin2Src::CHAR;
		break;
	case CCopyCSrc::INT:
		typ = Bin2Src::INT;
		break;
	case CCopyCSrc::FLOAT:
		typ = Bin2Src::FLOAT;
		break;
	}
	int size = 1;
	if (type_ == CCopyCSrc::FLOAT)
	{
		switch(float_size_)
		{
		case CCopyCSrc::FLOAT_32:
			size = 4;
			break;
		case CCopyCSrc::FLOAT_64:
			size = 8;
			break;
		case CCopyCSrc::REAL_48:
			size = 6;
			break;
		}
	}
	else if (type_ == CCopyCSrc::INT)
	{
		switch (int_size_)
		{
		case CCopyCSrc::INT_8:
			size = 1;
			break;
		case CCopyCSrc::INT_16:
			size = 2;
			break;
		case CCopyCSrc::INT_32:
			size = 4;
			break;
		case CCopyCSrc::INT_64:
			size = 8;
			break;
		}
	}
	int base = 10;
	if (int_type_ == CCopyCSrc::INT_HEX)
		base = 16;
	else if (int_type_ == CCopyCSrc::INT_OCTAL)
		base = 8;
	int flags = Bin2Src::NONE;
	if (int_type_ == CCopyCSrc::INT_UNSIGNED || base != 10)
		flags |= Bin2Src::UNSIGNED;
	if (big_endian_)
		flags |= Bin2Src::BIG_ENDIAN;
	if (align_cols_)
		flags |= Bin2Src::ALIGN;
	if (show_address_)
		flags |= addr_flags_;

	formatter.Set(lang, typ, size, base, rowsize_, offset_, indent_, flags);

#ifdef _DEBUG
	char buf[1024];      // try to get a value that causes buffer overflow
#else
	char buf[4000];
#endif

	formatter.GetString(start_, end_, buf, sizeof(buf));

	SetDlgItemText(IDC_CSRC_PREVIEW, buf);
}

void CCopyCSrc::save_settings()
{
	theApp.WriteProfileInt("CopySource", "For",       for_);
	theApp.WriteProfileInt("CopySource", "Type",      type_);
	theApp.WriteProfileInt("CopySource", "IntType",   int_type_);
	theApp.WriteProfileInt("CopySource", "IntSize",   int_size_);
	theApp.WriteProfileInt("CopySource", "FloatSize", float_size_);
	theApp.WriteProfileInt("CopySource", "BigEndian", big_endian_);
	theApp.WriteProfileInt("CopySource", "Align",     align_cols_);
	theApp.WriteProfileInt("CopySource", "ShowAddress", show_address_);
	theApp.WriteProfileInt("CopySource", "Indent",    indent_);
	theApp.WriteProfileInt("CopySource", "Preview",   preview_ ? 1 : 0);
}

/////////////////////////////////////////////////////////////////////////////
// CCopyCSrc message handlers

BOOL CCopyCSrc::OnInitDialog()
{
	CDialog::OnInitDialog();

	show_preview();

	ctl_indent_spin_.SetRange(0, 79);
	fix_controls();

    LOGFONT lf;
    GetObject(GetStockObject(ANSI_FIXED_FONT), sizeof(LOGFONT), &lf);
	mono_fnt_.CreateFontIndirect(&lf);
    GetDlgItem(IDC_CSRC_PREVIEW)->SetFont(&mono_fnt_);

	update_preview();

	init_ = true;

	return TRUE;
}

void CCopyCSrc::OnChangeType()
{
	if (!init_) return;

	UpdateData();

	fix_controls();
	update_preview();
}

void CCopyCSrc::OnChange()
{
	if (!init_) return;

	UpdateData();

	update_preview();
}

void CCopyCSrc::OnOK()
{
	UpdateData();

	save_settings();
	mono_fnt_.DeleteObject();

	CDialog::OnOK();
}

void CCopyCSrc::OnCancel()
{
	save_settings();
	mono_fnt_.DeleteObject();

	CDialog::OnCancel();
}

void CCopyCSrc::OnPreviewToggle()
{
	preview_ = !preview_;
	show_preview();
}

void CCopyCSrc::OnCsrcHelp()
{
	// Display help for this page
	if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_CSRC_HELP))
		AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

static DWORD id_pairs[] = { 
	IDC_CSRC_FOR, HIDC_CSRC_FOR,
	IDC_CSRC_INDENT, HIDC_CSRC_INDENT,
	IDC_CSRC_INDENT_SPIN, HIDC_CSRC_INDENT,
	IDC_CSRC_ALIGN, HIDC_CSRC_ALIGN,
	IDC_CSRC_ADDRESS, HIDC_CSRC_ADDRESS,
	IDC_CSRC_STRING, HIDC_CSRC_STRING,
	IDC_CSRC_CHAR, HIDC_CSRC_CHAR,
	IDC_CSRC_INT, HIDC_CSRC_INT,
	IDC_CSRC_INT_TYPE, HIDC_CSRC_INT_TYPE,
	IDC_CSRC_INT_SIZE, HIDC_CSRC_INT_SIZE,
	IDC_BIG_ENDIAN, HIDC_BIG_ENDIAN,
	IDC_CSRC_FLOAT, HIDC_CSRC_FLOAT,
	IDC_CSRC_FLOAT_SIZE, HIDC_CSRC_FLOAT_SIZE,
	0,0
};

BOOL CCopyCSrc::OnHelpInfo(HELPINFO* pHelpInfo)
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs);
	return TRUE;
}

void CCopyCSrc::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.HtmlHelpContextMenu(pWnd, id_pairs);
}

