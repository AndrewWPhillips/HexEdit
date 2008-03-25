// Register.cpp : Handles the About and Register dialogs.  Also responsible for
// generating registration codes and updating registration info in the registry.
//

// TODO: Add help about registration and costs of each type

#include "stdafx.h"
#include "HexEdit.h"
#include "Register.h"
#include "SystemSound.h"
#ifdef USE_FREE_IMAGE
#include <FreeImage.h>
#else
#include "EnBitmap.h"
#endif
#include "misc.h"
#include "resource.hm"
#include "HelpID.hm"            // User defined help IDs

#ifndef _DEBUG
#include "SVNRevision.h"
#endif

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
    //line2_ = "Copyright © 2006 Expert Commercial Software Pty Ltd";
    //line3_ = "Written by Andrew W. Phillips";
    //line4_ = _T("");
}

void CAbout::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAbout)
	DDX_Control(pDX, IDC_ACTIVATE, activate_ctrl_);
    DDX_Control(pDX, IDC_BUTTON_URL, url_ctl_);
	//}}AFX_DATA_MAP
    DDX_Control(pDX, IDC_LINE1, ctl_line1_);
    DDX_Control(pDX, IDC_LINE2, ctl_line2_);
    DDX_Control(pDX, IDC_LINE3, ctl_line3_);
    DDX_Control(pDX, IDC_LINE4, ctl_line4_);
    DDX_Control(pDX, IDC_LINE5, ctl_line5_);
    DDX_Control(pDX, IDC_BUTTON_REG_URL, reg_url_ctl_);
    DDX_Control(pDX, IDC_LICENCE, ctl_licence_);
    DDX_Control(pDX, IDC_ACK, ctl_ack_);
    DDX_Text(pDX, IDC_LINE1, text1_);
    //DDX_Text(pDX, IDC_LICENCE, text2_);
    //DDX_Text(pDX, IDC_LINE2, line2_);
    //DDX_Text(pDX, IDC_LINE3, line3_);
    //DDX_Text(pDX, IDC_LINE4, line4_);
}

BEGIN_MESSAGE_MAP(CAbout, CDialog)
    //{{AFX_MSG_MAP(CAbout)
	ON_BN_CLICKED(IDC_ACTIVATE, OnActivate)
	//}}AFX_MSG_MAP
    ON_WM_ERASEBKGND()
	ON_BN_CLICKED(IDC_EMAIL, OnEmail)
	ON_BN_CLICKED(IDC_ACK_MORE, OnAckMore)
	ON_LBN_DBLCLK(IDC_ACK, OnDblclkAck)
END_MESSAGE_MAP()


// Acknowledged software used in HexEdit and associated web addresses
static const char *ack_name[] =
{
	"BCG Control Bar - BCGSoft Ltd",
	"MFC Grid Control - Chris Maunder",
	"Tree control for MFC Grid - Ken Bertelson",
	"Dialog Resize class (MFC) - Herbert Menke",
	"ZLIB compression - J. Gailly and M. Adler",
	"MD5 Message-Digest Alg - RSA Data Security",
	"CFile64 (MFC) class by Samuel R. Blackburn",
	"Window Splitter class (MFC) - Robert A. T. Káldy",
#ifdef USE_FREE_IMAGE
    "FreeImage DLL - Hervé Drolon (+ F. van den Berg)",
#else
	"CEnBitmap class (MFC) - Daniel Godson",
#endif
	"Transparent controls (MFC) - Ali Rafiee",
	"BigInteger used for C# Decimal - Cap'n Code",
	"range_set class - Andrew W. Phillips",
	"Folder Selection Dialog - Andrew Phillips",
	"SVN source control - Jim Blandy et al",
	"TortoiseSVN SVN client - Stefan Küng et al",
	"WIX installer toolkit - Rob Mensching et al",
	NULL
};

static const char *ack_url[] =
{
	"http://bcgsoft.com/",
	"http://codeproject.com/miscctrl/gridctrl.asp",
	"http://codeproject.com/miscctrl/gridtreectrl.asp",
	"http://www.codeproject.com/dialog/resizectrl.asp",
	"http://www.zlib.net/",
	"http://theory.lcs.mit.edu/~rivest/md5.c",
	"http://www.samblackburn.com/wfc/",
	"http://codeproject.com/splitter/kaldysimplesplitter.asp",
#ifdef USE_FREE_IMAGE
    "http://freeimage.sourceforge.net/"
#else
	"http://www.codeproject.com/bitmap/extendedbitmap2.asp",
#endif
	"http://www.codeproject.com/staticctrl/TransparentStaticCtrl.asp",
	"http://www.codeproject.com/cpp/CppIntegerClass.asp",
	"http://www.ddj.com/dept/cpp/184403660",
	"http://www.codeguru.com/cpp/w-d/dislog/dialogforselectingfolders/article.php/c1885/",
	"http://subversion.tigris.org/",
	"http://tortoisesvn.sourceforge.net/",
	"http://wix.sourceforge.net/",
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

#ifdef USE_FREE_IMAGE
BOOL CAbout::OnEraseBkgnd(CDC* pDC)
{
    FIBITMAP *dib = FreeImage_Load(FIF_BMP, ::GetExePath() + FILENAME_ABOUTBG);
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
#else
BOOL CAbout::OnEraseBkgnd(CDC* pDC)
{
	CEnBitmap bg;
	bg.LoadImage(::GetExePath() + FILENAME_ABOUTBG);
	if (HBITMAP(bg) == 0)
	    return CDialog::OnEraseBkgnd(pDC);

    CRect rct;
    this->GetClientRect(rct);

	CSize siz;
	BITMAP bi;
	bg.GetBitmap(&bi);
	siz.cx = bi.bmWidth;
	siz.cy = bi.bmHeight;

	CDC dcTmp;
	dcTmp.CreateCompatibleDC(pDC);
	dcTmp.SelectObject(&bg);
	pDC->StretchBlt(0, 0, rct.Width(), rct.Height(), 
			&dcTmp, 0, 0, siz.cx, siz.cy, SRCCOPY);
	dcTmp.DeleteDC();
	return TRUE;
}
#endif

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

#ifndef _DEBUG   // revision string is only updated for release builds so don't show for debugs to avoid confusion
	// Display the revision (if beta or if shift held down)
    if (theApp.beta_ > 0 || ::GetKeyState(VK_SHIFT) < 0)
		text1_ += CString(" Revision ") + SVNRevision;
#endif

    // Licence info
    ctl_licence_.ResetContent();
    switch (theApp.security_type_)
	{
	case 1:
        ctl_licence_.AddString("Your trial period has expired.");
		ctl_licence_.AddString("Click the link below to register.");
		break;
	case 2:
        ss.Format("Your trial expires in %ld days.", long(theApp.days_left_));
		ctl_licence_.AddString(ss);
		ctl_licence_.AddString("Click the link below to register.");
		break;
	case 3:
		ctl_licence_.AddString("Temporary license. Click the link below to register.");
		break;
	case 4:
        ctl_licence_.AddString("Licensed is for an earlier version.");
		ctl_licence_.AddString("Click the link below to register.");
		break;
	case 5:
        ctl_licence_.AddString("Licensed is for an earlier version.");
        ctl_licence_.AddString("Click the link below to upgrade.");
		break;
	case 6:
        ss.Format("Registered for use by: %s.", theApp.security_name_);
		ctl_licence_.AddString(ss);
		break;
	default:
		ctl_licence_.AddString("Unregistered copy.   Click the link below to register.");
		break;
	}

	// Let the user know what licence they have if the licence is for previous version
    if (theApp.security_type_ > 3 && theApp.security_licensed_version_ > 2)
	{
		ss = "Your license is for version ";

		// See INTERNAL_VERSION for current value
		switch (theApp.security_licensed_version_)
		{
		case 3:
			ss += "2.2";
			break;
		case 4:
			ss += "2.5";
			break;
		case 5:
			ss += "2.6";
			break;
		case 6:
			ss += "3.0";
			break;
		case 7:
			ss += "3.1";
			break;
		case 8:
			ss += "3.2";
			break;
		case 9:
			ss += "3.3";
			break;
		default:
			ss += "3.4 or later";
			break;
		}
		ctl_licence_.AddString(ss);
	}

	// Web site link
    VERIFY(ss.LoadString(IDS_WEB_ADDRESS));
    url_ctl_.SetTooltip(_T("HexEdit web site"));
    url_ctl_.SetWindowText(ss);
    url_ctl_.SetURL(ss);
    url_ctl_.SizeToContent(TRUE, TRUE);

	// Set up registration link and activea button
    if (theApp.security_type_ == 6)
    {
		// fully licensed
        VERIFY(ss.LoadString(IDS_WEB_REG_USER));
        reg_url_ctl_.SetTooltip(NULL);

        activate_ctrl_.EnableWindow(FALSE);        // Already registered - no need to register or upgrade
    }
    else if (theApp.security_type_ == 5)
    {
		// licensed for an earlier version - upgradeable
        VERIFY(ss.LoadString(IDS_WEB_UPGRADE));
        reg_url_ctl_.SetTooltip(_T("Upgrade now"));

        activate_ctrl_.SetWindowText("&Upgrade...");
        activate_ctrl_.EnableWindow(TRUE);
    }
    else
    {
        VERIFY(ss.LoadString(IDS_WEB_REG_USER));
        reg_url_ctl_.SetTooltip(_T("Register now"));

        activate_ctrl_.SetWindowText("&Activate...");
        activate_ctrl_.EnableWindow(TRUE);
    }
    reg_url_ctl_.SetWindowText(ss);
    reg_url_ctl_.SetURL(ss);
    reg_url_ctl_.SizeToContent(TRUE, TRUE);

    UpdateData(FALSE);
}

BOOL CAbout::PreTranslateMessage(MSG* pMsg) 
{
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == 'R')
    {
        // Only display registration dialog if unregistered
        if (theApp.security_type_ == 1 || theApp.security_type_ == 2 || theApp.security_type_ == 3)
        {
            OnActivate();
            return TRUE;
        }
    }
    return CDialog::PreTranslateMessage(pMsg);
}

void CAbout::OnActivate() 
{
    CRegister dlg;
    if (dlg.DoModal() == IDOK)
    {
		fix_controls();
    }
}

void CAbout::OnEmail() 
{
	::SendEmail();
}

void CAbout::OnAckMore() 
{
    if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HID_ABOUT_ACK))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

void CAbout::OnDblclkAck()
{
	int idx = ctl_ack_.GetCurSel();
	::ShellExecute(AfxGetMainWnd()->m_hWnd, _T("open"), ack_url[idx], NULL, NULL, SW_SHOWNORMAL);
}

/////////////////////////////////////////////////////////////////////////////
// CCodeEdit

CCodeEdit::CCodeEdit()
{
}

CCodeEdit::~CCodeEdit()
{
}


BEGIN_MESSAGE_MAP(CCodeEdit, CEdit)
    //{{AFX_MSG_MAP(CCodeEdit)
    ON_WM_CHAR()
    ON_WM_KEYDOWN()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCodeEdit message handlers

void CCodeEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    // Get the current text from the edit control
    CString ss;
    GetWindowText(ss);

    // Get the currently selected bytes (if any)
    int start, end;
    GetSel(start, end);

    if (nChar == '\b' && start > 1 && start == end && ss[start-1] == ' ')
    {
        // Backspace - if at 1st of group then also delete preceding space
        SetSel(start-2, end);
        // no return
    }
    else if (nChar < 32)
    {
        ; // Do nothing (let base class handle backspace and other control chars)
    }
    else if (nChar == ' ')
    {
        // Just ignore spaces
        return;
    }
    else if (::letter_valid(nChar) == '\0')
    {
#ifdef VER31
         CSystemSound::Play("Invalid Character");
#else
        ::Beep(5000,200);
#endif
        return;
    }

    CEdit::OnChar(nChar, nRepCnt, nFlags);
    add_spaces();
}

void CCodeEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    // Get the current text from the edit control
    CString ss;
    GetWindowText(ss);

    // Get the currently selected bytes (if any)
    int start, end;
    GetSel(start, end);

    if (nChar == VK_DELETE && start == end && ss.GetLength() > start+1 &&
             ss[start+1] == ' ')
    {
        // Delete - if at last of group delete following space
        SetSel(start, start+2);
    }
    else if (nChar == VK_LEFT && start == end && start > 0 && ss[start-1] == ' ')
    {
        // Left at start of group - skip space too
        SetSel(start-1, start-1);
    }
        
    CEdit::OnKeyDown(nChar, nRepCnt, nFlags);

    // Tidy display if char(s) deleted or caret moved
    GetWindowText(ss);
    if ((nChar == VK_DELETE || nChar == VK_RIGHT || nChar == VK_LEFT) &&
        ::GetKeyState(VK_SHIFT) >= 0 && ss.GetLength() > 0)
    {
        add_spaces();
    }
}

void CCodeEdit::add_spaces()
{
    CString ss;                         // Current string
    GetWindowText(ss);
    int start, end;                     // Current selection (start==end if just caret)
    GetSel(start, end);

    const char *pp;                     // Ptr into orig. (hex digit) string
    int newstart, newend;               // New caret position/selection
    newstart = newend = 0;              // In case of empty string

    // Allocate enough space (allowing for space padding)
    char *out = new char[(ss.GetLength()*5)/4+2]; // Where chars are stored
    int ii, jj;                 // Number of hex nybbles written/read
    char *dd = out;                     // Ptr to current output char

    for (pp = ss.GetBuffer(0), ii = jj = 0; /*forever*/; ++pp, ++ii)
    {
        if (ii == start)
            newstart = dd - out;        // save new caret position
        if (ii == end)
            newend = dd - out;

        if (*pp == '\0')
            break;

        *dd = ::letter_valid(*pp);

        // Ignore spaces (and anything else)
        if (*dd == '\0')
            continue;                   // ignore all but valid characters

        ++dd;
        ++jj;
        if (jj % 3 == 0)
            *dd++ = ' ';                // pad with space after every 4th char
    }
    if (dd > out && *(dd-1) == ' ')
        dd--;                           // Forget last space if added
    *dd = '\0';                         // Terminate string

    SetWindowText(out);
    SetSel(newstart, newend);
    delete[] out;
}

/////////////////////////////////////////////////////////////////////////////
// CRegister dialog

CRegister::CRegister(CWnd* pParent /*=NULL*/)
    : CDialog(CRegister::IDD, pParent)
{
    //{{AFX_DATA_INIT(CRegister)
    user_name_ = _T("");
    type_ = -1;
    install_code_ = _T("");
    //}}AFX_DATA_INIT
    type_ = 1;
}

void CRegister::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CRegister)
    DDX_Text(pDX, IDC_USER_NAME, user_name_);
    DDV_MaxChars(pDX, user_name_, 50);
    DDX_Radio(pDX, IDC_MACHINE, type_);
    DDX_Text(pDX, IDC_INSTALL_CODE, install_code_);
    DDV_MaxChars(pDX, install_code_, 11);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRegister, CDialog)
    //{{AFX_MSG_MAP(CRegister)
    ON_BN_CLICKED(IDC_EMAIL, OnEmail)
    ON_BN_CLICKED(IDC_MACHINE, OnMachine)
    ON_BN_CLICKED(IDC_USER, OnUser)
    ON_EN_CHANGE(IDC_INSTALL_CODE, OnChangeInstallCode)
    ON_EN_CHANGE(IDC_USER_NAME, OnChangeUserName)
    ON_WM_HELPINFO()
    ON_BN_CLICKED(IDC_REG_HELP, OnRegHelp)
    ON_WM_CLOSE()
    //}}AFX_MSG_MAP
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRegister message handlers

BOOL CRegister::OnInitDialog() 
{
    // Get stored user name and type (if any)
    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
    user_name_ = aa->GetProfileString("Reg", "User", "Type your name here");
//    type_ = user_name_.IsEmpty() ? 0 : 1;

    CDialog::OnInitDialog();
    
    VERIFY(edit_.SubclassDlgItem(IDC_INSTALL_CODE, this));

    ASSERT(GetDlgItem(IDC_REG_CODE) != NULL);
    if (type_ == 0)
    {
        // Disallow entry of a user name for machine licensing
        ASSERT(GetDlgItem(IDC_USER_NAME) != NULL);
        GetDlgItem(IDC_USER_NAME)->EnableWindow(FALSE);
        ASSERT(GetDlgItem(IDC_DESC_NAME) != NULL);
        GetDlgItem(IDC_DESC_NAME)->EnableWindow(FALSE);

        GetDlgItem(IDC_REG_CODE)->SetWindowText(reg_code(1));
    }
    else
    {
        ASSERT(type_ = 1);
    }

    return TRUE;
}

void CRegister::OnOK() 
{
    BOOL bTempReg = FALSE;

    UpdateData();

    if (type_ == 0)
    {
        // Machine codes (de-activated)
        if (strnicmp(install_code_, reg_code(0, 1), 11) == 0)
        {
            bTempReg = TRUE;            // Signal temp reg
        }
        else if (strnicmp(install_code_, reg_code(0), 11) != 0)
        {
            ::HMessageBox("The activation code is invalid for this machine.\n"
                          "Please make sure you typed it correctly.");
            return;
        }
    }
    else
    {
        ASSERT(type_ == 1);
        int code_type = -1;
		int flags;
        for (flags = 0; flags < 256; ++flags)
        {
            if (strnicmp(install_code_, user_reg_code(0, user_name_, code_type = 6, flags), 11) == 0 ||
                strnicmp(install_code_, user_reg_code(0, user_name_, code_type = 6, flags, -1), 11) == 0 ||
                strnicmp(install_code_, user_reg_code(0, user_name_, code_type = 14, flags, -1), 11) == 0)
            {
                break;
            }
        }

		// code_type is 6 (110b) for full registration or 14 (1110b) for upgrade
		// theApp.security_licensed_version_ is the current licensed version (may be < security_version_)
		// theApp.security_version_ is the current running version of the software
		// flags >> 1 is the new licensed version stored in the activation code
        ASSERT(code_type == 6 || code_type == 14);
        if (flags >= 256)
        {
            ASSERT(flags == 256);
            ::HMessageBox("The activation code is incorrect.\n"
                          "Please make sure you entered your name\n"
                          "and/or the activation code correctly.");
            return;
        }
//        else if (theApp.security_type_ < 4 || (flags >> 1) < theApp.security_version_ - 5)
//        {
//            ::HMessageBox("The activation code is for an older version.\n"
//                          "Please make sure you entered it correctly.");
//            return;
//        }
        else if (code_type == 14 && theApp.security_licensed_version_ + UPGRADE_DIFF < (flags >> 1))
        {
            ::HMessageBox("This is an upgrade activation code.\n"
                          "Please make sure you entered it correctly.");
            return;
        }
        else if (code_type == 14 && (flags >> 1) < theApp.security_version_ - 1)
        {
            ::HMessageBox("This is an upgrade activation code for an earlier version.\n"
                          "You need to further upgrade your license or install\n"
                          "the correct (earlier) licensed version of the software.");
            // Allow the licence to be updated but we are still note licensed.
        }
        else if ((flags >> 1) < theApp.security_version_ - 1)
        {
            ::HMessageBox("The activation code is for an older version.\n"
                          "You need to upgrade your license or install the\n"
                          "correct (earlier) licensed version of the software.");
        }
        else if ((flags & 1) != 0)
        {
            bTempReg = TRUE;            // Signal temp reg
        }
    }

    CDialog::OnOK();

    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

    if (bTempReg)
    {
        aa->AddSecurity("");
    }
//    else if (type_ == 0)
//    {
//        aa->AddSecurity(NULL);
//        aa->WriteProfileString("Reg", "User", NULL);
//    }
    else if (type_ == 1)
    {
        aa->AddSecurity(user_name_);
        aa->WriteProfileString("Reg", "User", user_name_);
    }
}

void CRegister::OnCancel() 
{
    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

    UpdateData();
    if (type_ == 0)
        aa->WriteProfileString("Reg", "User", NULL);
    else
        aa->WriteProfileString("Reg", "User", user_name_);
    
    CDialog::OnCancel();
}

void CRegister::OnClose() 
{
    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

    UpdateData();
    if (type_ == 0)
        aa->WriteProfileString("Reg", "User", NULL);
    else
        aa->WriteProfileString("Reg", "User", user_name_);

    CDialog::OnClose();
}

static DWORD id_pairs2[] = { 
    IDC_MACHINE, HIDC_MACHINE,
    IDC_USER, HIDC_USER,
    IDC_REG_CODE, HIDC_REG_CODE,
    IDC_USER_NAME, HIDC_USER_NAME,
    IDC_DESC_NAME, HIDC_USER_NAME,
    IDC_INSTALL_CODE, HIDC_INSTALL_CODE,
//        IDC_REG_EMAIL, HIDC_REG_EMAIL,
//        IDC_REG_WEB, HIDC_REG_WEB,
    IDOK, HID_REGISTER_INSTALL,
    IDC_REG_HELP, HIDC_HELP_BUTTON,
    0,0 
}; 

BOOL CRegister::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs2);
    return TRUE;
}

void CRegister::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	theApp.HtmlHelpContextMenu((HWND)pWnd->GetSafeHwnd(), id_pairs2);
}

void CRegister::OnRegHelp() 
{
    // Display help for the dialog
	if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_REGISTER_HELP))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

void CRegister::OnEmail() 
{
    AfxMessageBox("Not implemented");
}

// The following will no longer get called now that the controls are not visible
void CRegister::OnMachine() 
{
    AfxMessageBox("Not implemented");
//    // Disable the name field
//    ASSERT(GetDlgItem(IDC_USER_NAME) != NULL);
//    GetDlgItem(IDC_USER_NAME)->EnableWindow(FALSE);
//    ASSERT(GetDlgItem(IDC_DESC_NAME) != NULL);
//    GetDlgItem(IDC_DESC_NAME)->EnableWindow(FALSE);
//
//    // Get the registration code for this machine
//    ASSERT(GetDlgItem(IDC_REG_CODE) != NULL);
//    GetDlgItem(IDC_REG_CODE)->SetWindowText(reg_code(1));
}

void CRegister::OnUser() 
{
    AfxMessageBox("Not implemented");
//    // Enable the name field
//    ASSERT(GetDlgItem(IDC_USER_NAME) != NULL);
//    GetDlgItem(IDC_USER_NAME)->EnableWindow(TRUE);
//    ASSERT(GetDlgItem(IDC_DESC_NAME) != NULL);
//    GetDlgItem(IDC_DESC_NAME)->EnableWindow(TRUE);
//    GetDlgItem(IDC_DESC_NAME)->SetFocus();
//
//    OnChangeUserName();
}

void CRegister::OnChangeInstallCode() 
{
    // When the correct number of characters have been entered enable the Install button
    UpdateData();

    ASSERT(GetDlgItem(IDOK) != NULL);
    GetDlgItem(IDOK)->EnableWindow(install_code_.GetLength() > 10);
}

void CRegister::OnChangeUserName() 
{
    // Update the registration code based on the new name
    UpdateData();

#if 0
    ASSERT(GetDlgItem(IDC_REG_CODE) != NULL);
    CString ss = user_name_;
    ss.Remove(' ');
    ss.Remove('\t');
    ss.Remove('.');
    if (ss.GetLength() > 5)
        GetDlgItem(IDC_REG_CODE)->SetWindowText(user_reg_code(1, user_name_));
    else
        GetDlgItem(IDC_REG_CODE)->SetWindowText("");
#endif
}


#if 0   // No longer used - most functionality moved to CStartup

/////////////////////////////////////////////////////////////////////////////
// CReg dialog


CReg::CReg(CWnd* pParent /*=NULL*/)
    : CDialog(CReg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CReg)
    user_name_ = _T("");
    type_ = -1;
    costs_ = _T("");
    //}}AFX_DATA_INIT
        type_ = 1;
}

void CReg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CReg)
    DDX_Text(pDX, IDC_USER_NAME, user_name_);
    DDX_Radio(pDX, IDC_MACHINE, type_);
    DDX_Text(pDX, IDC_COSTS, costs_);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CReg, CDialog)
    //{{AFX_MSG_MAP(CReg)
    ON_BN_CLICKED(IDC_USER, OnUser)
    ON_BN_CLICKED(IDC_MACHINE, OnMachine)
    ON_EN_CHANGE(IDC_USER_NAME, OnChangeUserName)
    ON_BN_CLICKED(IDC_REG_WEB, OnRegWeb)
    ON_BN_CLICKED(IDC_REG_EMAIL, OnRegEmail)
    ON_BN_CLICKED(IDC_REG_HELP, OnRegHelp)
    ON_WM_CLOSE()
    ON_WM_HELPINFO()
    ON_BN_CLICKED(IDC_REG_FORM, OnRegForm)
    //}}AFX_MSG_MAP
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReg message handlers

BOOL CReg::OnInitDialog() 
{
    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
    user_name_ = aa->GetProfileString("Reg", "User", "Type your name here");
    type_ = user_name_.IsEmpty() ? 0 : 1;

    costs_ = "All purchases from sales are put back into the product.  By purchasing\n"
             "HexEdit you will help make it better.  It also entitles you to free support,\n"
             "upgrades and priority in enhancement requests.\n"
             "\n"
             "A single user licence is $40 (US). A single machine licence is usually $20.\n"
             "\n"
             "You can pay on-line by clicking the \"Web Site\" button, or by regular mail\n"
             "by sending an order form (click the \"Order Form\" button).  You must always\n"
             "send your name or registration code && email address with your payment.\n"
             "\n"
             "Click the \"Help\" button for more information.\n";

    if (aa->is_us_)
        costs_.Replace("licence", "license");

    CDialog::OnInitDialog();

    ASSERT(GetDlgItem(IDC_REG_CODE) != NULL);
    if (type_ == 0)
    {
        // Disallow entry of a user name for machine licensing
        ASSERT(GetDlgItem(IDC_USER_NAME) != NULL);
        GetDlgItem(IDC_USER_NAME)->EnableWindow(FALSE);
        ASSERT(GetDlgItem(IDC_DESC_NAME) != NULL);
        GetDlgItem(IDC_DESC_NAME)->EnableWindow(FALSE);

        GetDlgItem(IDC_REG_CODE)->SetWindowText(reg_code(1));
//        ASSERT(GetDlgItem(IDC_REG_EMAIL) != NULL);
//        GetDlgItem(IDC_REG_EMAIL)->EnableWindow(TRUE);
    }
    else
    {
        ASSERT(type_ = 1);
#if 0
//        GetDlgItem(IDC_REG_CODE)->SetWindowText(user_reg_code(1, user_name_));
        ASSERT(GetDlgItem(IDC_REG_CODE) != NULL);
//        ASSERT(GetDlgItem(IDC_REG_EMAIL) != NULL);
        CString ss = user_name_;
        ss.Remove(' ');
        ss.Remove('\t');
        ss.Remove('.');
        if (ss.GetLength() > 5)
        {
            GetDlgItem(IDC_REG_CODE)->SetWindowText(user_reg_code(1, user_name_));
//            GetDlgItem(IDC_REG_EMAIL)->EnableWindow(TRUE);
        }
        else
        {
            GetDlgItem(IDC_REG_CODE)->SetWindowText("");
//            GetDlgItem(IDC_REG_EMAIL)->EnableWindow(FALSE);
        }
#endif
    }

    return TRUE;
}

void CReg::OnOK() 
{
    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
    CDialog::OnOK();

    if (type_ == 0)
        aa->WriteProfileString("Reg", "User", NULL);
    else
        aa->WriteProfileString("Reg", "User", user_name_);

    aa->CheckBGSearchFinished();
}

void CReg::OnClose() 
{
    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

    UpdateData();
    if (type_ == 0)
        aa->WriteProfileString("Reg", "User", NULL);
    else
        aa->WriteProfileString("Reg", "User", user_name_);

    CDialog::OnClose();

    aa->CheckBGSearchFinished();
}

static DWORD id_pairs1[] = { 
    IDC_MACHINE, HIDC_MACHINE,
    IDC_USER, HIDC_USER,
    IDC_REG_CODE, HIDC_REG_CODE,
    IDC_USER_NAME, HIDC_USER_NAME,
    IDC_DESC_NAME, HIDC_USER_NAME,
    IDC_REG_EMAIL, HIDC_REG_EMAIL,
    IDC_REG_FORM, HIDC_REG_FORM,
    IDC_REG_WEB, HIDC_REG_WEB,
    IDC_COSTS, HIDC_COSTS,
    IDC_REG_HELP, HIDC_HELP_BUTTON,
    0,0
}; 

BOOL CReg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs1);
    return TRUE;
}

void CReg::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	theApp.HtmlHelpContextMenu((HWND)pWnd->GetSafeHwnd(), id_pairs1);
}

void CReg::OnRegHelp() 
{
    // Display help for the dialog
    if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_REG_HELP))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

void CReg::OnUser() 
{
    // Enable the name field
    ASSERT(GetDlgItem(IDC_USER_NAME) != NULL);
    GetDlgItem(IDC_USER_NAME)->EnableWindow(TRUE);
    ASSERT(GetDlgItem(IDC_DESC_NAME) != NULL);
    GetDlgItem(IDC_DESC_NAME)->EnableWindow(TRUE);
    GetDlgItem(IDC_DESC_NAME)->SetFocus();

    OnChangeUserName();
}

void CReg::OnMachine() 
{
    // Disable the name field
    ASSERT(GetDlgItem(IDC_USER_NAME) != NULL);
    GetDlgItem(IDC_USER_NAME)->EnableWindow(FALSE);
    ASSERT(GetDlgItem(IDC_DESC_NAME) != NULL);
    GetDlgItem(IDC_DESC_NAME)->EnableWindow(FALSE);

    // Get the registration code for this machine
    ASSERT(GetDlgItem(IDC_REG_CODE) != NULL);
    GetDlgItem(IDC_REG_CODE)->SetWindowText(reg_code(1));
//    ASSERT(GetDlgItem(IDC_REG_EMAIL) != NULL);
//    GetDlgItem(IDC_REG_EMAIL)->EnableWindow(TRUE);
}

void CReg::OnChangeUserName() 
{
    // Update the registration code based on the new name
    UpdateData();

#if 0
    ASSERT(GetDlgItem(IDC_REG_CODE) != NULL);
//    ASSERT(GetDlgItem(IDC_REG_EMAIL) != NULL);
    CString ss = user_name_;
    ss.Remove(' ');
    ss.Remove('\t');
    ss.Remove('.');
    if (ss.GetLength() > 5)
    {
        GetDlgItem(IDC_REG_CODE)->SetWindowText(user_reg_code(1, user_name_));
//        GetDlgItem(IDC_REG_EMAIL)->EnableWindow(TRUE);
    }
    else
    {
        GetDlgItem(IDC_REG_CODE)->SetWindowText("");
//        GetDlgItem(IDC_REG_EMAIL)->EnableWindow(FALSE);
    }
#endif
}

void CReg::OnRegWeb() 
{
    UpdateData();
//    if (type_ == 0)
//        ::BrowseWeb(IDS_WEB_REG_MC);
//    else
        ::BrowseWeb(IDS_WEB_REG_USER);
}

void CReg::OnRegEmail() 
{
    CString ss;                         // default email text
    const char *name;                   // default name field

    UpdateData();
    if (type_ == 0)
    {
        ss.Format("REGISTRATION CODE: %s", reg_code(1));
        name = NULL;
    }
    else
    {
        ss.Format("REGISTRATION CODE: %s", user_reg_code(1, user_name_));
        name = user_name_;
    }
    if (SendEmail(2, ss, name))
    {
        ::HMessageBox("To pay for the software, click on the \"Order Form\"\n"
                      "button in the Registration dialog.  Print the form and\n"
                      "fill it out and send with your payment to:\n"
                      "Expert Commercial Software Pty Ltd\n"
                      "\n"
                      "PO Box 2117\n"
                      "Normanhurst NSW  2076\n"
                      "AUSTRALIA");
    }
}

void CReg::OnRegForm() 
{
    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

    if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_+CString(">form"), 
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

#endif  // end CReg removal


/////////////////////////////////////////////////////////////////////////////
// CStartup dialog

CStartup::CStartup(CWnd* pParent /*=NULL*/)
    : CDialog(CStartup::IDD, pParent)
{
    //{{AFX_DATA_INIT(CStartup)
    text_ = _T("");
    //}}AFX_DATA_INIT
}


void CStartup::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_BUTTON_URL, url_ctl_);
    //{{AFX_DATA_MAP(CStartup)
    DDX_Text(pDX, IDC_TEXT, text_);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStartup, CDialog)
    //{{AFX_MSG_MAP(CStartup)
    ON_BN_CLICKED(IDC_MORE, OnMore)
    ON_BN_CLICKED(IDC_REG_EMAIL, OnRegEmail)
    ON_BN_CLICKED(IDC_REG_FORM, OnRegForm)
    ON_BN_CLICKED(IDC_REG_HELP, OnRegHelp)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStartup message handlers

BOOL CStartup::OnInitDialog() 
{
    CDialog::OnInitDialog();

    // Get registration web address
    CString ss;
    VERIFY(ss.LoadString(IDS_WEB_REG_USER));
    url_ctl_.SetTooltip(_T("Register now"));
    url_ctl_.SetWindowText("  HexEdit registration web site ");
    url_ctl_.SetURL(ss);
    url_ctl_.SizeToContent(TRUE, TRUE);
    
    return TRUE;
}

void CStartup::OnMore() 
{
//    CReg dlg;
//    dlg.DoModal();
}

void CStartup::OnRegEmail() 
{
    if (SendEmail(2, "", "Type your name here"))
    {
        ::HMessageBox("To pay for the software, click on the \"Order Form\"\n"
                      "button in the Registration dialog.  Print the form and\n"
                      "fill it out and send with your payment to:\n"
                      "Expert Commercial Software Pty Ltd\n"
                      "\n"
                      "PO Box 2117\n"
                      "Normanhurst NSW  2076\n"
                      "AUSTRALIA");
    }
}

void CStartup::OnRegForm() 
{
    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

    if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_+CString(">form"), 
                    HH_HELP_CONTEXT, aa->is_us_ ? HID_REG_FORM_US : HID_REG_FORM))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

void CStartup::OnRegHelp() 
{
    // Display help for the dialog
    if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_REG_HELP))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}
