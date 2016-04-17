// Register.cpp : Only handles the About dialog now.
//
// Copyright (c) 2015 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#include "stdafx.h"
#include "HexEdit.h"
#include "Register.h"
#include "SystemSound.h"
#include <FreeImage.h>
#include "misc.h"
#include "resource.hm"
#include "HelpID.hm"            // User defined help IDs

#include <HtmlHelp.h>

extern CHexEditApp theApp;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAbout dialog


CAbout::CAbout(CWnd* pParent /*=NULL*/)
	: CDialog(CAbout::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAbout)
	//}}AFX_DATA_INIT
	text1_ = _T("");
}

void CAbout::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAbout)
	DDX_Control(pDX, IDC_BUTTON_URL, url_ctl_);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_LINE1, ctl_line1_);
	DDX_Control(pDX, IDC_LINE2, ctl_line2_);
	DDX_Control(pDX, IDC_LINE3, ctl_line3_);
	DDX_Control(pDX, IDC_LINE4, ctl_line4_);
	DDX_Control(pDX, IDC_LINE5, ctl_line5_);
	DDX_Control(pDX, IDC_ACK, ctl_ack_);
	DDX_Text(pDX, IDC_LINE1, text1_);
}

BEGIN_MESSAGE_MAP(CAbout, CDialog)
	ON_WM_ERASEBKGND()
	ON_BN_CLICKED(IDC_EMAIL, OnEmail)
	ON_BN_CLICKED(IDC_ACK_MORE, OnAckMore)
	ON_LBN_DBLCLK(IDC_ACK, OnDblclkAck)
END_MESSAGE_MAP()


// Acknowledged software used in HexEdit and associated web addresses
// xxx add Boost and MPIR
static const char *ack_name[] =
{
	"MFC Grid Control - Chris Maunder",
	"Tree control for MFC Grid - Ken Bertelson",
	"Dialog Resize class (MFC) - Herbert Menke",
	"ZLIB compression - J. Gailly and M. Adler",
	"MD5 Message-Digest Alg - RSA Data Security",
	"Window Splitter class (MFC) - Robert A. T. Káldy",
	"Splash screen (MFC) - P. J. Naughter",
	"FreeImage DLL - Hervé Drolon (+ F. van den Berg)",
	"Transparent controls (MFC) - Ali Rafiee",
	"range_set class - Andrew W. Phillips",
	"Folder Selection Dialog - Andrew Phillips",
	"SVN source control - Jim Blandy et al",
	"TortoiseSVN SVN client - Stefan Küng et al",
	NULL
};

static const char *ack_url[] =
{
	"http://www.codeproject.com/KB/miscctrl/gridctrl.aspx",
	"http://www.codeproject.com/KB/miscctrl/gridtreectrl.aspx",
	"http://www.codeproject.com/KB/dialog/resizectrl.aspx",
	"http://www.zlib.net/",
	"http://theory.lcs.mit.edu/~rivest/md5.c",
	"http://www.codeproject.com/KB/splitter/kaldysimplesplitter.aspx",
	"http://www.codeproject.com/KB/dialog/splasher.aspx",
	"http://freeimage.sourceforge.net/",
	"http://www.codeproject.com/KB/static/TransparentStaticCtrl.aspx",
	"http://www.ddj.com/dept/cpp/184403660",
	"http://www.codeguru.com/cpp/w-d/dislog/dialogforselectingfolders/article.php/c1885/",
	"http://subversion.tigris.org/",
	"http://tortoisesvn.sourceforge.net/",
	NULL
};

/////////////////////////////////////////////////////////////////////////////
// CAbout message handlers

BOOL CAbout::OnInitDialog()
{
	CDialog::OnInitDialog();

	fix_controls();

	// Set up acknowledgements
	ctl_ack_.ResetContent();
	for (const char** ppp = ack_name; *ppp != NULL; ++ppp)
		ctl_ack_.AddString(*ppp);

	return TRUE;
}

BOOL CAbout::OnEraseBkgnd(CDC* pDC)
{
	FIBITMAP *dib = FreeImage_Load(FIF_JPEG, ::GetExePath() + FILENAME_ABOUTBG);
	if (dib == NULL)
		dib = FreeImage_Load(FIF_BMP, ::GetExePath() + FILENAME_ABOUTBG);
	if (dib == NULL)
		return CDialog::OnEraseBkgnd(pDC);

	CRect rct;
	this->GetClientRect(rct);
	::StretchDIBits(pDC->GetSafeHdc(),
					0, 0, rct.Width(), rct.Height(),
					0, 0, FreeImage_GetWidth(dib), FreeImage_GetHeight(dib),
					FreeImage_GetBits(dib), FreeImage_GetInfo(dib), DIB_RGB_COLORS, SRCCOPY);
	return TRUE;
}

void CAbout::fix_controls()
{
	char rel_char = ' ';
	CString ss;

	// Set up first line (version info)
	if (theApp.version_%10 > 0)
		rel_char = 'A' - 1 + theApp.version_%10;
	text1_.Format("HexEdit version %d.%d%c", theApp.version_/100, (theApp.version_%100)/10, rel_char);
	if (theApp.beta_ > 0)
	{
		ss.Format(" Beta %d", theApp.beta_);
		text1_ += ss;
	}

	// Display the revision (if beta or if shift held down)
	if (theApp.beta_ > 0 || ::GetKeyState(VK_SHIFT) < 0)
	{
		ss.Format(" Revision %d", 1429);  // TODO: use Git revision number now instead of SVNRevision from SubVersion?
		text1_ += ss;
	}

	// Web site link
	VERIFY(ss.LoadString(IDS_WEB_ADDRESS));
	url_ctl_.SetTooltip(_T("HexEdit web site"));
	url_ctl_.SetWindowText(ss);
	url_ctl_.SetURL(ss);
	url_ctl_.SizeToContent(TRUE, TRUE);

	UpdateData(FALSE);
}

void CAbout::OnEmail()
{
	::SendEmail();
}

void CAbout::OnAckMore()
{
	if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HID_ABOUT_ACK))
		AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

void CAbout::OnDblclkAck()
{
	int idx = ctl_ack_.GetCurSel();
	::ShellExecute(AfxGetMainWnd()->m_hWnd, _T("open"), ack_url[idx], NULL, NULL, SW_SHOWNORMAL);
}
