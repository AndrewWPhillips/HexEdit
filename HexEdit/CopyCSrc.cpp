// CopyCSrc.cpp : implementation file
//

#include "stdafx.h"
#include "hexedit.h"
#include "CopyCSrc.h"
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

CCopyCSrc::CCopyCSrc(CWnd* pParent /*=NULL*/)
	: CDialog(CCopyCSrc::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCopyCSrc)
	big_endian_ = FALSE;
	type_ = -1;
	float_size_ = -1;
	int_size_ = -1;
	int_type_ = -1;
	show_address_ = FALSE;
	indent_ = 0;
	//}}AFX_DATA_INIT

	type_        = theApp.GetProfileInt("CopySource", "Type", STRING);
	int_type_    = theApp.GetProfileInt("CopySource", "IntType", INT_HEX);
	int_size_    = theApp.GetProfileInt("CopySource", "IntSize", INT_32);
	float_size_  = theApp.GetProfileInt("CopySource", "FloatSize", FLOAT_64);
	big_endian_  = theApp.GetProfileInt("CopySource", "BigEndian", FALSE);
	show_address_= theApp.GetProfileInt("CopySource", "ShowAddress", FALSE);
	indent_      = theApp.GetProfileInt("CopySource", "Indent", 8);
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
	DDX_Check(pDX, IDC_BIG_ENDIAN, big_endian_);
	DDX_Radio(pDX, IDC_CSRC_STRING, type_);
	DDX_CBIndex(pDX, IDC_CSRC_FLOAT_SIZE, float_size_);
	DDX_CBIndex(pDX, IDC_CSRC_INT_SIZE, int_size_);
	DDX_CBIndex(pDX, IDC_CSRC_INT_TYPE, int_type_);
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
	//}}AFX_MSG_MAP
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()


void CCopyCSrc::fix_controls()
{
	ctl_int_type_.EnableWindow(type_ == INT);
	ctl_int_size_.EnableWindow(type_ == INT);
//	ctl_.EnableWindow(type_ == INT);
	ctl_float_size_.EnableWindow(type_ == FLOAT);
	ctl_big_endian_.EnableWindow(type_ == INT || type_ == FLOAT);
}

/////////////////////////////////////////////////////////////////////////////
// CCopyCSrc message handlers

BOOL CCopyCSrc::OnInitDialog() 
{
	CDialog::OnInitDialog();

	ctl_indent_spin_.SetRange(0, 127);
	fix_controls();

	return TRUE;
}

void CCopyCSrc::OnChangeType() 
{
	UpdateData();

	fix_controls();
}

void CCopyCSrc::OnOK() 
{
	UpdateData();

	theApp.WriteProfileInt("CopySource", "Type",      type_);
	theApp.WriteProfileInt("CopySource", "IntType",   int_type_);
	theApp.WriteProfileInt("CopySource", "IntSize",   int_size_);
	theApp.WriteProfileInt("CopySource", "FloatSize", float_size_);
	theApp.WriteProfileInt("CopySource", "BigEndian", big_endian_);
	theApp.WriteProfileInt("CopySource", "ShowAddress", show_address_);
	theApp.WriteProfileInt("CopySource", "Indent",    indent_);

	CDialog::OnOK();
}

void CCopyCSrc::OnCsrcHelp() 
{
	// Display help for this page
	if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_CSRC_HELP))
		AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

static DWORD id_pairs[] = { 
	IDC_CSRC_INDENT, HIDC_CSRC_INDENT,
	IDC_CSRC_INDENT_SPIN, HIDC_CSRC_INDENT,
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

