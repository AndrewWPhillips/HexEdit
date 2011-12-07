// Dialog.cpp : implementation of misc. dialogs
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
//#include <sys/stat.h>
//#include <shlwapi.h>    // for PathRemoveFileSpec

#include "HexEdit.h"
#include "Dialog.h"
#include <Dlgs.h>               // For file dialog control IDs
#include "HexFileList.h"
#include "resource.hm"
#include "HelpID.hm"            // User defined help IDs

extern CHexEditApp theApp;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// The following are for file dialogs

/////////////////////////////////////////////////////////////////////////////
// CHexFileDialog - derived from CFileDialog, base class of other file dialogs

IMPLEMENT_DYNAMIC(CHexFileDialog, CFileDialog)

BOOL CHexFileDialog::OnInitDialog()
{
   CFileDialog::OnInitDialog();

   // This is just so we can do stuff at startup (see OnPostInit).  If some of these
   // things were done in OnInitDone() or earlier then things would stuff up.
   PostMessage(WM_USER+1);
   return TRUE;
}

BOOL CHexFileDialog::OnFileNameOK()
{
	// Remember current window pos for when window is reopened
	CRect rr;
	GetParent()->GetWindowRect(&rr);
	theApp.WriteProfileInt("Window-Settings", strName+"X1", rr.left);
	theApp.WriteProfileInt("Window-Settings", strName+"Y1", rr.top);
	theApp.WriteProfileInt("Window-Settings", strName+"X2", rr.right);
	theApp.WriteProfileInt("Window-Settings", strName+"Y2", rr.bottom);

	// Remember current list view mode
	ASSERT(GetParent() != NULL);
	CWnd *psdv  = FindWindowEx(GetParent()->m_hWnd, NULL, "SHELLDLL_DefView", NULL);
	ASSERT(psdv != NULL);
	CWnd *plv   = FindWindowEx(psdv->m_hWnd, NULL, "SysListView32", NULL);
	ASSERT(plv != NULL);

	int mode = 0;
	switch (plv->SendMessage(LVM_FIRST + 143 /*LVM_GETVIEW*/))
	{
	case LVS_ICON:
	case LVS_SMALLICON:
		mode = ICON;
		break;
	case LVS_REPORT:
		mode = REPORT;
		break;
	case LVS_LIST:
		mode = LIST;
		break;
	default:
		mode = TILE;
		break;
	}
	theApp.WriteProfileInt("Window-Settings", strName+"Mode", mode);

	return CFileDialog::OnFileNameOK();
}

BEGIN_MESSAGE_MAP(CHexFileDialog, CFileDialog)
   ON_MESSAGE(WM_USER+1, OnPostInit)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
// CHexFileDialog message handlers

LRESULT CHexFileDialog::OnPostInit(WPARAM wp, LPARAM lp)
{
	// Set text of "OK" button
	if (!strOKName.IsEmpty())
		SetControlText(IDOK, strOKName);

	// Restore the window position and size
	CRect rr(theApp.GetProfileInt("Window-Settings", strName+"X1", -30000),
			 theApp.GetProfileInt("Window-Settings", strName+"Y1", -30000),
			 theApp.GetProfileInt("Window-Settings", strName+"X2", -30000),
			 theApp.GetProfileInt("Window-Settings", strName+"Y2", -30000));
	if (rr.top != -30000)
		GetParent()->MoveWindow(&rr);

	// Restore the list view display mode (details, report, icons, etc)
	ASSERT(GetParent() != NULL);
	CWnd *psdv  = FindWindowEx(GetParent()->m_hWnd, NULL, "SHELLDLL_DefView", NULL);
	if (psdv != NULL)
	{
		int mode = theApp.GetProfileInt("Window-Settings", strName+"Mode", REPORT);
		psdv->SendMessage(WM_COMMAND, mode, 0);
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CImportDialog - derived from CFileDialog (via CHexFileDialog) for handling extra controls during import

void CImportDialog::OnInitDone()
{
	CRect rct;                          // Used to move/resize controls
	CWnd *pp;                           // Parent = the dialog window itself
	VERIFY(pp = GetParent());

	ASSERT(pp->GetDlgItem(cmb1) != NULL && pp->GetDlgItem(IDOK) != NULL);

	// Create a new button below the "Type" drop down list
	pp->GetDlgItem(cmb1)->GetWindowRect(rct); // Get button rectangle
	pp->ScreenToClient(rct);
	rct.InflateRect(0, 4, -rct.Width()/3, 4);   // Make 2/3 width, + make higher for 2 lines of text
	rct.OffsetRect(0, 30);

	m_discon.Create(_T("Use import addresses\r\n(for non-adjoining records)"),
					WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX | BS_MULTILINE, rct, pp, IDC_DISCON);
	m_discon.SetFont(pp->GetDlgItem(IDOK)->GetFont());
	m_discon.SetCheck(theApp.import_discon_);

	rct.OffsetRect(rct.Width()+4, 0);
	rct.InflateRect(0, 0, -(rct.Width()/2), 0);  // Make half width of IDC_DISCON checkbox (= 1/3 of cmb1 width)
	m_highlight.Create(_T("Highlight"),
					WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX | BS_MULTILINE, rct, pp, IDC_IMPORT_HIGHLIGHT);
	m_highlight.SetFont(pp->GetDlgItem(IDOK)->GetFont());
	m_highlight.SetCheck(theApp.import_highlight_);
	CHexFileDialog::OnInitDone();
}

BOOL CImportDialog::OnFileNameOK()
{
	theApp.import_discon_    = m_discon.GetCheck() == 1    ? TRUE : FALSE;
	theApp.import_highlight_ = m_highlight.GetCheck() == 1 ? TRUE : FALSE;

	//CRect rr;
	//GetParent()->GetDlgItem(lst1)->GetWindowRect(rr);

	return CHexFileDialog::OnFileNameOK();
}

/////////////////////////////////////////////////////////////////////////////
// CExportDialog - derived from CFileDialog (via CHexFileDialog) for handling extra control during export

void CExportDialog::OnInitDone()
{
	CRect rct;                          // Used to move/resize controls
	CWnd *pp;                           // Parent = the dialog window itself
	VERIFY(pp = GetParent());

	ASSERT(pp->GetDlgItem(cmb1) != NULL && pp->GetDlgItem(IDOK) != NULL);

	// Create a new button below the "Type" drop down list
	pp->GetDlgItem(cmb1)->GetWindowRect(rct); // Get button rectangle
	pp->ScreenToClient(rct);
	rct.InflateRect(0, 4, -(rct.Width()/2), 4);   // make higher for 2 lines of text and half the width
	rct.OffsetRect(0, 30);

	m_discon_hl.Create(_T("Export (non-adjoining)\r\nhighlighted areas only"),
					WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX | BS_MULTILINE, rct, pp, IDC_DISCON);
	m_discon_hl.SetFont(pp->GetDlgItem(IDOK)->GetFont());
	m_discon_hl.SetCheck(theApp.import_discon_);

	FILE_ADDRESS start, end;
	m_pview->GetSelAddr(start, end);
	if (start >= end)
		m_discon_hl.SetCheck(1);
	if (!m_pview->HasHighlights())
	{
		// If no highlights disable the checkbox
		m_discon_hl.SetCheck(0);
		m_discon_hl.EnableWindow(FALSE);
	}

	CHexFileDialog::OnInitDone();
}

BOOL CExportDialog::OnFileNameOK()
{
	theApp.import_discon_ = m_discon_hl.GetCheck() == 1    ? TRUE : FALSE;

	return CHexFileDialog::OnFileNameOK();
}

/////////////////////////////////////////////////////////////////////////////
// CFileOpenDialog - derived from CFileDialog (via CHexFileDialog) for handling extra control
void CFileOpenDialog::OnInitDone()
{
	CRect rct;                          // Used to move/resize controls
	CWnd *pp;                           // Parent = the dialog window itself
	VERIFY(pp = GetParent());

	ASSERT(pp->GetDlgItem(cmb1) != NULL && pp->GetDlgItem(chx1) != NULL && pp->GetDlgItem(IDOK) != NULL);

	// Create a new check box next to Read only check box
	pp->GetDlgItem(cmb1)->GetWindowRect(rct); // Get "Type" drop down list width
	int width = rct.Width();
	pp->GetDlgItem(chx1)->GetWindowRect(rct); // Get "Read only" rectangle
	pp->ScreenToClient(rct);
	// Make read only check box half the width of "Type" box (it is sometimes much wider than it needs to be)
	rct.right = rct.left + width/2 - 2;
	pp->GetDlgItem(chx1)->MoveWindow(rct);
	// Work out rect of new check box
	rct.OffsetRect(width/2, 0);              // move over (so not on top of "read only" checkbox)
	//rct.InflateRect(0, 0, 10, 0);

	m_open_shared.Create(_T("Open shareable"),
					WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX | BS_MULTILINE, rct, pp, IDC_OPEN_SHARED);
	m_open_shared.SetFont(pp->GetDlgItem(IDOK)->GetFont());
	m_open_shared.SetCheck(theApp.open_file_shared_);

#ifdef FILE_PREVIEW
	if (theApp.thumbnail_)
	{
		int preview_size = (int)(theApp.thumb_size_ * theApp.thumb_zoom_);

		// Work out the place for the preview - to the right of the existing window
		pp->GetClientRect(&rct);
		rct.left = rct.right;
		rct.right = rct.left + preview_size;
		rct.top = 5;
		rct.bottom = rct.top + preview_size;

		// Create the preview window
		m_preview.Create(_T("Preview"), WS_CHILD | WS_VISIBLE, rct, pp, IDC_OPEN_PREVIEW);
		m_preview.SetBackground(::GetSysColor(COLOR_3DFACE));

		// Expand the main file open dlg window to show the preview window
		pp->GetWindowRect(&rct);
		rct.right += preview_size + 5;
		pp->MoveWindow(&rct);
	}
#endif

	CHexFileDialog::OnInitDone();
}

// Called when one or more files have been selected (eg OK button clicked)
BOOL CFileOpenDialog::OnFileNameOK()
{
	theApp.open_file_shared_ = m_open_shared.GetCheck() == 1 ? TRUE : FALSE;

	return CHexFileDialog::OnFileNameOK();
}

#ifdef FILE_PREVIEW
// Called when the file open dialog switches to a different directory
void CFileOpenDialog::OnFolderChange()
{
	// As the lst2 child window (parent of list control) is recreated when the current folder is 
	// changed we need to unhook now and re-hook later (see below).
	if (m_wndHook.GetSafeHwnd() != HWND(NULL))
		m_wndHook.UnsubclassWindow();
  
	CHexFileDialog::OnFolderChange();

	m_wndHook.SubclassWindow(GetParent()->GetDlgItem(lst2)->GetSafeHwnd());

	// If we have changed folders no files are now selected
	m_strPreview.Empty();
    UpdatePreview();
}

// Given a file name in m_strPreview it tries to show its thumbnail.
// To clear the preview pane make m_strPreview invalid (eg empty).
void CFileOpenDialog::UpdatePreview()
{
	CString fname;

	// We need to get the name of the file containing the thumbnail from the recent file list
	CHexFileList *pfl = theApp.GetFileList();
	int idx;

	if (!m_strPreview.IsEmpty() &&                    // only if we have a selected file AND
		(idx = pfl->GetIndex(m_strPreview)) >= 0 &&   // it's in recent file list AND
		::GetDataPath(fname) )                        // we can get the path to preview files
	{
		// Create the full path to the thumbnail file
		fname += DIRNAME_PREVIEW;
		fname += _T("HEPV");
		fname += pfl->GetData(idx, CHexFileList::PREVIEWFILENAME);
	}

	m_preview.SetFile(fname.IsEmpty() ? NULL : fname);  // tell preview window what to display
}

// Called when things happen to the list of files.  We are only interested in the selection changing.
BOOL CFileOpenDialog::CHookWnd::OnNotify(WPARAM, LPARAM lParam, LRESULT* pResult)
{
    LPNMLISTVIEW pLVHdr = reinterpret_cast<LPNMLISTVIEW>(lParam);

    if (pLVHdr->hdr.code == LVN_ITEMCHANGED && (pLVHdr->uChanged & LVIF_STATE) && pLVHdr->iItem != -1)
    {
		char buf[_MAX_PATH], fname[_MAX_PATH], fext[_MAX_PATH];

		// Get the name of the selected file
        CListCtrl ctl;
        ctl.Attach(pLVHdr->hdr.hwndFrom);
        LPCITEMIDLIST pidl = (LPCITEMIDLIST)ctl.GetItemData(pLVHdr->iItem);
        SHGetPathFromIDList(pidl, buf);
		_tsplitpath(buf, NULL, NULL, fname, fext);  // ignore directory as pidl is relative path
        ctl.Detach();

		// Work out the full path
		CString strPath = m_pOwner->GetFolderPath();
		CString strFull;
		strFull.Format(_T("%s%s%s%s"), strPath, strPath[strPath.GetLength()-1] == '\\' ? "" : "\\", fname, fext);

		// If a different file was selected try to preview it
		if (m_pOwner->m_strPreview.CompareNoCase(strFull) != 0)
        {
			m_pOwner->m_strPreview = strFull;
            m_pOwner->UpdatePreview();
        }
    }

    *pResult = 0;
    return FALSE;  // return 0 to allow further processing
}
#endif  // FILE_PREVIEW

/////////////////////////////////////////////////////////////////////////////
// The following are dialogs for use with/in macros

/////////////////////////////////////////////////////////////////////////////
// CMultiplay dialog

CMultiplay::CMultiplay(CWnd* pParent /*=NULL*/)
		: CDialog(CMultiplay::IDD, pParent)
{
		//{{AFX_DATA_INIT(CMultiplay)
		plays_ = 0;
		//}}AFX_DATA_INIT
}

void CMultiplay::DoDataExchange(CDataExchange* pDX)
{
		CDialog::DoDataExchange(pDX);
		//{{AFX_DATA_MAP(CMultiplay)
	DDX_Control(pDX, IDC_PLAY_NAME, name_ctrl_);
		DDX_Text(pDX, IDC_PLAYS, plays_);
		DDV_MinMaxLong(pDX, plays_, 1, 999999999);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMultiplay, CDialog)
		//{{AFX_MSG_MAP(CMultiplay)
		ON_BN_CLICKED(IDC_PLAY_OPTIONS, OnPlayOptions)
		ON_WM_HELPINFO()
	ON_CBN_SELCHANGE(IDC_PLAY_NAME, OnSelchangePlayName)
	ON_BN_CLICKED(IDC_PLAY_HELP, OnHelp)
	//}}AFX_MSG_MAP
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

void CMultiplay::FixControls()
{
	name_ctrl_.GetLBText(name_ctrl_.GetCurSel(), macro_name_);

	if (macro_name_ == DEFAULT_MACRO_NAME)
	{
		plays_ = 1;
	}
	else
	{
		std::vector<key_macro> tmp;
		CString comment;
		int halt_lev;
		long plays;
		int version;  // Version of HexEdit in which the macro was recorded

		ASSERT(theApp.mac_dir_.Right(1) == "\\");
		if (theApp.macro_load(theApp.mac_dir_ + macro_name_ + ".hem", &tmp, comment, halt_lev, plays, version))
			plays_ = plays;
		else
		{
			ASSERT(0);
			plays_ = 1;
		}
	}

	UpdateData(FALSE);  // Put number of plays into control
}

/////////////////////////////////////////////////////////////////////////////
// CMultiplay message handlers

BOOL CMultiplay::OnInitDialog()
{
		CDialog::OnInitDialog();

		if (!theApp.recording_ && theApp.mac_.size() > 0)
			name_ctrl_.AddString(DEFAULT_MACRO_NAME);

		// Find all .hem files in macro dir
		CFileFind ff;
		ASSERT(theApp.mac_dir_.Right(1) == "\\");
		BOOL bContinue = ff.FindFile(theApp.mac_dir_ + "*.hem");

		while (bContinue)
		{
			// At least one match - check them all
			bContinue = ff.FindNextFile();

			// Hide macro files beginning with underscore unless recording.
			// This is so that "sub-macros" are not normally seen but can be
			// selected when recording a macro that invokes them.
			if (theApp.recording_ || ff.GetFileTitle().Left(1) != "_")
				name_ctrl_.AddString(ff.GetFileTitle());
		}
		ASSERT(name_ctrl_.GetCount() > 0);
		name_ctrl_.SetCurSel(0);

		ASSERT(GetDlgItem(IDC_SPIN_PLAYS) != NULL);
		((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_PLAYS))->SetRange(1, UD_MAXVAL);

		ASSERT(GetDlgItem(IDC_PLAYS) != NULL);
		ASSERT(GetDlgItem(IDC_DESC_PLAYS) != NULL);
		ASSERT(GetDlgItem(IDC_SPIN_PLAYS) != NULL);
		GetDlgItem(IDC_PLAYS)->EnableWindow(!theApp.recording_);
		GetDlgItem(IDC_DESC_PLAYS)->EnableWindow(!theApp.recording_);
		GetDlgItem(IDC_SPIN_PLAYS)->EnableWindow(!theApp.recording_);
		FixControls();

		return TRUE;
}

void CMultiplay::OnOK()
{
	name_ctrl_.GetLBText(name_ctrl_.GetCurSel(), macro_name_);

	CDialog::OnOK();
}

void CMultiplay::OnHelp()
{
	// Display help for this page
	if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_MULTIPLAY_HELP))
		AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

static DWORD id_pairs_play[] = { 
	IDC_PLAY_NAME, HIDC_PLAY_NAME,
	IDC_PLAY_OPTIONS, HIDC_PLAY_OPTIONS,
	IDC_DESC_PLAYS, HIDC_PLAYS,
	IDC_PLAYS, HIDC_PLAYS,
	IDC_SPIN_PLAYS, HIDC_PLAYS,
	IDC_PLAY_HELP, HIDC_HELP_BUTTON,
	IDOK, HID_MULTIPLAY_OK,
	0,0 
};

BOOL CMultiplay::OnHelpInfo(HELPINFO* pHelpInfo)
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs_play);
	return TRUE;
}

void CMultiplay::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.HtmlHelpContextMenu(pWnd, id_pairs_play);
}

void CMultiplay::OnSelchangePlayName()
{
	FixControls();
}

void CMultiplay::OnPlayOptions()
{
	// Invoke the Options dlg with the macro page displayed
	theApp.display_options(MACRO_OPTIONS_PAGE, TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CSaveMacro dialog


CSaveMacro::CSaveMacro(CWnd* pParent /*=NULL*/)
		: CDialog(CSaveMacro::IDD, pParent)
{
		//{{AFX_DATA_INIT(CSaveMacro)
		plays_ = 0;
		name_ = _T("");
		comment_ = _T("");
		halt_level_ = -1;
		//}}AFX_DATA_INIT
		halt_level_ = 1;
}

void CSaveMacro::DoDataExchange(CDataExchange* pDX)
{
		CDialog::DoDataExchange(pDX);
		//{{AFX_DATA_MAP(CSaveMacro)
		DDX_Text(pDX, IDC_PLAYS, plays_);
	DDV_MinMaxLong(pDX, plays_, 1, 999999999);
		DDX_Text(pDX, IDC_MACRO_NAME, name_);
		DDX_Text(pDX, IDC_MACRO_COMMENT, comment_);
		DDX_Radio(pDX, IDC_HALT0, halt_level_);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSaveMacro, CDialog)
		//{{AFX_MSG_MAP(CSaveMacro)
		ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_MACRO_HELP, OnMacroHelp)
	//}}AFX_MSG_MAP
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSaveMacro message handlers

BOOL CSaveMacro::OnInitDialog()
{
	aa_ = dynamic_cast<CHexEditApp *>(AfxGetApp());
	name_ = aa_->mac_filename_;
	plays_ = aa_->plays_;
	comment_ = aa_->mac_comment_;
//    halt_level_ = aa_->halt_level_;

	CDialog::OnInitDialog();

	ASSERT(GetDlgItem(IDC_SPIN_PLAYS) != NULL);
	((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_PLAYS))->SetRange(1, UD_MAXVAL);

	return TRUE;
}

void CSaveMacro::OnOK()
{
	if (!UpdateData(TRUE))
		return;                         // DDV failed

	if (name_.IsEmpty())
	{
		CAvoidableDialog::Show(IDS_NO_MACRO_NAME, "Please enter a name for the macro.");
		ASSERT(GetDlgItem(IDC_MACRO_NAME) != NULL);
		GetDlgItem(IDC_MACRO_NAME)->SetFocus();
		return;
	}

	// If the macro was saved OK end the dialog
	ASSERT(aa_->mac_dir_.Right(1) == "\\");
	if (aa_->macro_save(aa_->mac_dir_ + name_ + ".hem", NULL, comment_, halt_level_, plays_, aa_->macro_version_))
		CDialog::OnOK();
}

static DWORD id_pairs_save[] = {
	IDC_MACRO_NAME, HIDC_MACRO_NAME,
	IDC_MACRO_COMMENT, HIDC_MACRO_COMMENT,
	IDC_PLAYS, HIDC_PLAYS,
	IDC_SPIN_PLAYS, HIDC_PLAYS,
	IDC_HALT0, HIDC_HALT0,
	IDC_HALT1, HIDC_HALT1,
	IDC_HALT2, HIDC_HALT2,
	IDOK, HID_MACRO_SAVE,
	IDC_MACRO_HELP, HIDC_HELP_BUTTON,
	0,0 
};

BOOL CSaveMacro::OnHelpInfo(HELPINFO* pHelpInfo)
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs_save);
	return TRUE;
}

void CSaveMacro::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.HtmlHelpContextMenu(pWnd, id_pairs_save);
}

void CSaveMacro::OnMacroHelp()
{
	// Display help for the dialog
	if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_MACRO_SAVE_HELP))
		AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

/////////////////////////////////////////////////////////////////////////////
// CMacroMessage dialog

CMacroMessage::CMacroMessage(CWnd* pParent /*=NULL*/)
	: CDialog(CMacroMessage::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMacroMessage)
	message_ = _T("");
	//}}AFX_DATA_INIT
}

void CMacroMessage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMacroMessage)
	DDX_Text(pDX, IDC_MACRO_MESSAGE, message_);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMacroMessage, CDialog)
	//{{AFX_MSG_MAP(CMacroMessage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMacroMessage message handlers

void CMacroMessage::OnOK()
{
	UpdateData();

	if (message_.IsEmpty())
	{
		CAvoidableDialog::Show(IDS_NO_MACRO_MESSAGE, "Please enter the text of the message to be displayed when the macro is run.");
		ASSERT(GetDlgItem(IDC_MACRO_MESSAGE) != NULL);
		GetDlgItem(IDC_MACRO_MESSAGE)->SetFocus();
		return;
	}

	CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
//
// GetInt, GetBool, GetStr
// The following are used to get a simple value from the user.  They are
// called as a result of use of the corresponding functions in expressions.
//

/////////////////////////////////////////////////////////////////////////////
// GetInt dialog

IMPLEMENT_DYNAMIC(GetInt, CDialog)

GetInt::GetInt(CWnd* pParent /*=NULL*/)
	: CDialog(GetInt::IDD, pParent)
{
}

void GetInt::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_VALUE, value_);
	DDV_MinMaxLong(pDX, value_, min_, max_);
	DDX_Text(pDX, IDC_PROMPT, prompt_);
}

BEGIN_MESSAGE_MAP(GetInt, CDialog)
END_MESSAGE_MAP()

BOOL GetInt::OnInitDialog()
{
	if (min_ > max_) min_ = max_;
	CDialog::OnInitDialog();

	GetDlgItem(IDC_VALUE)->SetFocus();                  // put user straight into control
	((CEdit*)GetDlgItem(IDC_VALUE))->SetSel(0, -1);     // select all so they can overwrite the default
	if (min_ < SHRT_MIN || max_ > SHRT_MAX)
	{
		((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN))->SetRange((short)-32768, (short)32767);
		GetDlgItem(IDC_SPIN)->ShowWindow(FALSE);
	}
	else
		((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN))->SetRange((short)min_, (short)max_);
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// GetStr dialog

IMPLEMENT_DYNAMIC(GetStr, CDialog)

GetStr::GetStr(CWnd* pParent /*=NULL*/)
	: CDialog(GetStr::IDD, pParent)
{
}

void GetStr::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_PROMPT, prompt_);
	DDX_Text(pDX, IDC_VALUE, value_);
}

BEGIN_MESSAGE_MAP(GetStr, CDialog)
END_MESSAGE_MAP()

BOOL GetStr::OnInitDialog()
{
	CDialog::OnInitDialog();

	GetDlgItem(IDC_VALUE)->SetFocus();                  // put user straight into control
	((CEdit*)GetDlgItem(IDC_VALUE))->SetSel(0, -1);     // select all so they can overwrite the default
	return FALSE;                                       // FALSE indicates we set focus
}

/////////////////////////////////////////////////////////////////////////////
// GetBool dialog

IMPLEMENT_DYNAMIC(GetBool, CDialog)

GetBool::GetBool(CWnd* pParent /*=NULL*/)
	: CDialog(GetBool::IDD, pParent)
{
	yes_ = "&YES";
	no_ = "&NO";
}

void GetBool::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_PROMPT, prompt_);
	if (!pDX->m_bSaveAndValidate)
	{
		SetDlgItemText(IDOK, yes_);
		SetDlgItemText(IDCANCEL, no_);
	}
}

BEGIN_MESSAGE_MAP(GetBool, CDialog)
END_MESSAGE_MAP()
