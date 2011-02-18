// MainFrm.cpp : implementation of the CMainFrame class
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
#include <io.h>                 // For _access()
#include <MultiMon.h>
#include <imagehlp.h>           // For ::MakeSureDirectoryPathExists()
#include <afxpriv.h>            // for WM_COMMANDHELP
#include "HexEdit.h"
#include "ChildFrm.h"
#include "HexEditDoc.h"
#include "HexEditView.h"
#include "MainFrm.h"
#include "Bookmark.h"
#include "BookmarkDlg.h"
#include "BookmarkFind.h"
#include "HexFileList.h"
#include "Boyer.h"
#include "SystemSound.h"
#include "Misc.h"
#include "BCGMisc.h"
#include <afxribbonres.h>
#include "HelpID.hm"            // User defined help IDs
#include "GRIDCTRL_SRC\InPlaceEdit.h"
#include <HtmlHelp.h>
#pragma warning(push)
#pragma warning(disable:4005)
#include <afxhh.h>
#pragma warning(pop)

extern CHexEditApp theApp;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CHexEditApp theApp;

IMPLEMENT_SERIAL(CHexEditFontCombo, CMFCToolBarFontComboBox, 1)  // see BCGMisc.h
IMPLEMENT_SERIAL(CHexEditFontSizeCombo, CMFCToolBarFontSizeComboBox, 1)  // see BCGMisc.h

// We need to derive our own class from BCG customize class so we can 
// handle What's This help properly using Html Help.
class CHexEditCustomize : public CMFCToolBarsCustomizeDialog
{
public:
	CHexEditCustomize(CFrameWnd* pParent) : CMFCToolBarsCustomizeDialog(pParent, TRUE,
						AFX_CUSTOMIZE_MENU_SHADOWS | AFX_CUSTOMIZE_TEXT_LABELS | 
						AFX_CUSTOMIZE_MENU_ANIMATIONS |  AFX_CUSTOMIZE_CONTEXT_HELP)
	{ }
//	DECLARE_DYNAMIC(CHexEditCustomize)

protected:
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	DECLARE_MESSAGE_MAP()
};

//IMPLEMENT_DYNAMIC(CHexEditCustomize, CMFCToolBarsCustomizeDialog)
BEGIN_MESSAGE_MAP(CHexEditCustomize, CMFCToolBarsCustomizeDialog)
	ON_WM_HELPINFO()
END_MESSAGE_MAP()

BOOL CHexEditCustomize::OnHelpInfo(HELPINFO* pHelpInfo)
{
#if 0 // xxx needs fix for MFC 9
	CWaitCursor wait;
	if (::HtmlHelp((HWND)pHelpInfo->hItemHandle, theApp.htmlhelp_file_+"::BcgIdMap.txt", HH_TP_HELP_WM_HELP, (DWORD)(LPVOID)dwBCGResHelpIDs) == HWND(0))
	{
		AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
		return FALSE;
	}
#endif
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWndEx)

// static UINT WM_FINDREPLACE = ::RegisterWindowMessage(FINDMSGSTRING);

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWndEx)
		ON_WM_INITMENU()
		//{{AFX_MSG_MAP(CMainFrame)
		ON_WM_CREATE()
		ON_WM_CLOSE()
		ON_WM_SIZE()
		ON_COMMAND(ID_EDIT_FIND, OnEditFind)
		ON_UPDATE_COMMAND_UI(ID_EDIT_FIND, OnUpdateEditFind)
		ON_COMMAND(ID_EDIT_FIND2, OnEditFind2)
		ON_COMMAND(ID_CALCULATOR, OnCalculator)
	ON_COMMAND(ID_CUSTOMIZE, OnCustomize)
	ON_WM_HELPINFO()
		ON_UPDATE_COMMAND_UI(ID_EDIT_FIND2, OnUpdateEditFind)
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
		ON_WM_SYSCOMMAND()
		ON_WM_MENUSELECT()
		//ON_WM_ENTERIDLE()

		ON_COMMAND(ID_HELP_FINDER, OnHelpFinder)
		ON_COMMAND(ID_HELP, OnHelp)
		ON_COMMAND(ID_HELP_INDEX, OnHelpIndex)
		ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
		ON_COMMAND(ID_CONTEXT_HELP, OnContextHelp)
		ON_COMMAND(ID_DEFAULT_HELP, OnHelpFinder)
		ON_COMMAND(ID_HELP_KEYBOARDMAP, OnHelpKeyboardMap)
		ON_COMMAND_RANGE(ID_HELP_TUTE1, ID_HELP_TUTE1+9, OnHelpTute)

		ON_COMMAND_EX(ID_WINDOW_ARRANGE, OnMDIWindowCmd)
		ON_COMMAND_EX(ID_WINDOW_CASCADE, OnMDIWindowCmd)
		ON_COMMAND_EX(ID_WINDOW_TILE_HORZ, OnMDIWindowCmd)
		ON_COMMAND_EX(ID_WINDOW_TILE_VERT, OnMDIWindowCmd)

		ON_COMMAND(ID_WINDOW_NEW, OnWindowNew)

		// Searching
		ON_COMMAND(ID_EDIT_REPLACE, OnEditReplace)
		ON_UPDATE_COMMAND_UI(ID_EDIT_REPLACE, OnUpdateEditFind)
		ON_COMMAND(ID_FIND_NEXT, OnFindNext)
		ON_UPDATE_COMMAND_UI(ID_FIND_NEXT, OnUpdateSearch)
		ON_COMMAND(ID_BOOKMARK_ALL, OnBookmarkAll)
		ON_UPDATE_COMMAND_UI(ID_BOOKMARK_ALL, OnUpdateSearch)
		ON_COMMAND(ID_REPLACE, OnReplace)
		ON_UPDATE_COMMAND_UI(ID_REPLACE, OnUpdateSearch)
		ON_COMMAND(ID_REPLACE_ALL, OnReplaceAll)
		ON_UPDATE_COMMAND_UI(ID_REPLACE_ALL, OnUpdateSearch)

		ON_COMMAND(ID_SEARCH_FORW, OnSearchForw)
		ON_UPDATE_COMMAND_UI(ID_SEARCH_FORW, OnUpdateSearch)
		ON_COMMAND(ID_SEARCH_BACK, OnSearchBack)
		ON_UPDATE_COMMAND_UI(ID_SEARCH_BACK, OnUpdateSearch)
		ON_COMMAND(ID_SEARCH_SEL, OnSearchSel)
		ON_UPDATE_COMMAND_UI(ID_SEARCH_SEL, OnUpdateSearchSel)

//        ON_REGISTERED_MESSAGE(WM_FINDREPLACE, OnFindDlgMess)

		// Toolbars, ruler, status bar
		ON_COMMAND(ID_VIEW_VIEWBAR, OnViewViewbar)
		ON_UPDATE_COMMAND_UI(ID_VIEW_VIEWBAR, OnUpdateViewViewbar)
		ON_COMMAND(ID_VIEW_EDITBAR, OnViewEditbar)
		ON_UPDATE_COMMAND_UI(ID_VIEW_EDITBAR, OnUpdateViewEditbar)
		ON_COMMAND(ID_VIEW_FORMATBAR, OnViewFormatbar)
		ON_UPDATE_COMMAND_UI(ID_VIEW_FORMATBAR, OnUpdateViewFormatbar)
		ON_COMMAND(ID_VIEW_NAVBAR, OnViewNavbar)
		ON_UPDATE_COMMAND_UI(ID_VIEW_NAVBAR, OnUpdateViewNavbar)
		ON_COMMAND(ID_VIEW_CALCULATOR, OnViewCalculator)
		ON_UPDATE_COMMAND_UI(ID_VIEW_CALCULATOR, OnUpdateViewCalculator)
		ON_COMMAND(ID_VIEW_BOOKMARKS, OnViewBookmarks)
		ON_UPDATE_COMMAND_UI(ID_VIEW_BOOKMARKS, OnUpdateViewBookmarks)
		ON_COMMAND(ID_VIEW_FIND, OnViewFind)
		ON_UPDATE_COMMAND_UI(ID_VIEW_FIND, OnUpdateViewFind)
		ON_COMMAND(ID_VIEW_EXPL, OnViewExpl)
		ON_UPDATE_COMMAND_UI(ID_VIEW_EXPL, OnUpdateViewExpl)
		ON_COMMAND(ID_VIEW_RULER, OnViewRuler)
		ON_UPDATE_COMMAND_UI(ID_VIEW_RULER, OnUpdateViewRuler)
		ON_COMMAND(ID_VIEW_HL_CURSOR, OnViewHighlightCaret)
		ON_UPDATE_COMMAND_UI(ID_VIEW_HL_CURSOR, OnUpdateViewHighlightCaret)
		ON_COMMAND(ID_VIEW_HL_MOUSE, OnViewHighlightMouse)
		ON_UPDATE_COMMAND_UI(ID_VIEW_HL_MOUSE, OnUpdateViewHighlightMouse)
		ON_COMMAND(ID_VIEW_PROPERTIES, OnViewProperties)
		ON_UPDATE_COMMAND_UI(ID_VIEW_PROPERTIES, OnUpdateViewProperties)

		ON_COMMAND(ID_DIALOGS_DOCKABLE, OnDockableToggle)
		ON_UPDATE_COMMAND_UI(ID_DIALOGS_DOCKABLE, OnUpdateDockableToggle)

		ON_COMMAND_EX(ID_VIEW_STATUS_BAR, OnPaneCheck)
		ON_UPDATE_COMMAND_UI(ID_INDICATOR_COMPARES, OnUpdateCompares)
		ON_UPDATE_COMMAND_UI(ID_INDICATOR_OCCURRENCES, OnUpdateOccurrences)
		ON_UPDATE_COMMAND_UI(ID_INDICATOR_VALUES, OnUpdateValues)
		ON_UPDATE_COMMAND_UI(ID_INDICATOR_HEX_ADDR, OnUpdateAddrHex)
		ON_UPDATE_COMMAND_UI(ID_INDICATOR_DEC_ADDR, OnUpdateAddrDec)
		ON_UPDATE_COMMAND_UI(ID_INDICATOR_FILE_LENGTH, OnUpdateFileLength)
		ON_UPDATE_COMMAND_UI(ID_INDICATOR_BIG_ENDIAN, OnUpdateBigEndian)
		ON_UPDATE_COMMAND_UI(ID_INDICATOR_READONLY, OnUpdateReadonly)
		ON_UPDATE_COMMAND_UI(ID_INDICATOR_OVR, OnUpdateOvr)
		ON_UPDATE_COMMAND_UI(ID_INDICATOR_REC, OnUpdateRec)
		ON_REGISTERED_MESSAGE(CHexEditApp::wm_hexedit, OnOpenMsg)

		// MFC9 (BCG) stuff
		ON_REGISTERED_MESSAGE(AFX_WM_RESETTOOLBAR, OnToolbarReset)
		ON_REGISTERED_MESSAGE(AFX_WM_RESETMENU, OnMenuReset)
		//ON_REGISTERED_MESSAGE(AFX_WM_TOOLBARMENU, OnToolbarContextMenu)
		//ON_COMMAND_EX_RANGE(ID_VIEW_USER_TOOLBAR1, ID_VIEW_USER_TOOLBAR10, OnToolsViewUserToolbar)
		//ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_USER_TOOLBAR1, ID_VIEW_USER_TOOLBAR10, OnUpdateToolsViewUserToolbar)
		ON_REGISTERED_MESSAGE(AFX_WM_CUSTOMIZEHELP, OnHelpCustomizeToolbars)
		ON_COMMAND(ID_WINDOW_MANAGER, ShowWindowsDialog)

		// When vertically docked the combobox reverts to a button and sends this command when clicked
		ON_COMMAND(ID_SEARCH_COMBO, OnSearchCombo)
		ON_UPDATE_COMMAND_UI(ID_SEARCH_COMBO, OnUpdateSearchCombo)
		ON_COMMAND(ID_JUMP_HEX_COMBO, OnHexCombo)
		ON_UPDATE_COMMAND_UI(ID_JUMP_HEX_COMBO, OnUpdateHexCombo)
		ON_COMMAND(ID_JUMP_DEC_COMBO, OnDecCombo)
		ON_UPDATE_COMMAND_UI(ID_JUMP_DEC_COMBO, OnUpdateDecCombo)
		ON_COMMAND(ID_BOOKMARKS_COMBO, OnBookmarks)
		ON_UPDATE_COMMAND_UI(ID_BOOKMARKS_COMBO, OnUpdateBookmarksCombo)

		ON_COMMAND_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_OFF_2007_AQUA, &CMainFrame::OnApplicationLook)
		ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_OFF_2007_AQUA, &CMainFrame::OnUpdateApplicationLook)

		// Misc commands
		ON_COMMAND(ID_NAV_BACK, OnNavigateBackwards)
		ON_UPDATE_COMMAND_UI(ID_NAV_BACK, OnUpdateNavigateBackwards)
		ON_COMMAND_RANGE(ID_NAV_BACK_FIRST, ID_NAV_BACK_FIRST+NAV_RESERVED-1, OnNavBack)
		ON_COMMAND(ID_NAV_FORW, OnNavigateForwards)
		ON_UPDATE_COMMAND_UI(ID_NAV_FORW, OnUpdateNavigateForwards)
		ON_COMMAND_RANGE(ID_NAV_FORW_FIRST, ID_NAV_FORW_FIRST+NAV_RESERVED-1, OnNavForw)

		ON_REGISTERED_MESSAGE(AFX_WM_ON_GET_TAB_TOOLTIP, OnGetTabToolTip)
//        ON_MESSAGE(WM_USER, OnReturn)

		ON_COMMAND(ID_TEST, OnTest)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	preview_page_ = -1;
	progress_on_ = false;
//    timer_id_ = 0;
	// Load background image
	CString filename;
	if (::GetDataPath(filename))
		filename += FILENAME_BACKGROUND;
	else
		filename = ::GetExePath() + FILENAME_BACKGROUND;   // No data path only on Win 95?

	CString bgfile = theApp.GetProfileString("MainFrame", "BackgroundFileName", filename);
#ifdef USE_FREE_IMAGE
	/*
	  // Test creating a big bitmap using FreeImage
	  m_dib = FreeImage_Allocate(1000, 200000, 24); // 200 million pixels seems to be OK!!
	  int sz = FreeImage_GetDIBSize(m_dib);
	  FreeImage_Unload(m_dib); m_dib = NULL;
	*/
	if ((m_dib = FreeImage_Load(FIF_BMP, bgfile)) == NULL)          // MUST be 24-bit BMP file
		m_dib = FreeImage_Load(FIF_BMP, ::GetExePath() + FILENAME_BACKGROUND);
#else
	if (!m_background.LoadImage(bgfile))
		m_background.LoadImage(GetExePath() + FILENAME_BACKGROUND);
#endif
	m_background_pos = theApp.GetProfileInt("MainFrame", "BackgroundPosition", 4); // 4 = bottom-right
	theApp.m_nAppLook = theApp.GetInt(_T("ApplicationLook"), ID_VIEW_APPLOOK_OFF_2007_BLUE);

	// Status bar stuff
	m_search_image.LoadBitmap(IDB_SEARCH);
	ComparesWidth = OccurrencesWidth = ValuesWidth = AddrHexWidth = AddrDecWidth = FileLengthWidth = -999;
	bg_progress_colour_ = bg_compare_colour_ = -1;

	// History lists
	hex_hist_changed_ = dec_hist_changed_ = clock();
}

CMainFrame::~CMainFrame()
{
	//if (pcalc_ != NULL)
	//    delete pcalc_;
}

static UINT indicators[] =
{
		ID_SEPARATOR,           // status line indicator
		ID_INDICATOR_COMPARES,
		ID_INDICATOR_OCCURRENCES,
		ID_INDICATOR_VALUES,
		ID_INDICATOR_HEX_ADDR,
		ID_INDICATOR_DEC_ADDR,
		ID_INDICATOR_FILE_LENGTH,
		ID_INDICATOR_BIG_ENDIAN,
		ID_INDICATOR_READONLY,
		ID_INDICATOR_OVR,
		ID_INDICATOR_REC,
		ID_INDICATOR_CAPS,
		ID_INDICATOR_NUM,
//      ID_INDICATOR_SCRL,
};

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
		if (CMDIFrameWndEx::OnCreate(lpCreateStruct) == -1)
				return -1;

		//menu_tip_.SetParent(this);
		// set the visual manager and style based on persisted value
		CMFCButton::EnableWindowsTheming();
		OnApplicationLook(theApp.m_nAppLook);

		if (theApp.mditabs_)
		{
			EnableMDITabs(TRUE, theApp.tabicons_,
						  theApp.tabsbottom_ ? CMFCTabCtrl::LOCATION_BOTTOM : CMFCTabCtrl::LOCATION_TOP,
						  0, CMFCTabCtrl::STYLE_3D_ONENOTE, 1);
			// TBD: TODO - replace with call to EnableMDITabbedGroups?
		}

		EnableDocking(CBRS_ALIGN_ANY);

		// Enable new docking window  and auto-hide behavior
		CDockingManager::SetDockingMode(DT_SMART);
		EnableAutoHidePanes(CBRS_ALIGN_ANY);

		// Create BCG menu bar
		if (!m_wndMenuBar.Create(this))
		{
				TRACE0("Failed to create a menu bar\n");
				return -1;      // fail to create
		}
		m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);
		m_wndMenuBar.EnableDocking(CBRS_ALIGN_ANY);
		DockPane(&m_wndMenuBar);

		// Create main Tool bar
		//if (!m_wndStandardBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_STDBAR) || 
		if (!m_wndStandardBar.CreateEx(this,
								TBSTYLE_FLAT,
								WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC,
								CRect(1, 1, 1, 1),
								IDR_STDBAR ) ||
#if SHADED_TOOLBARS
			!m_wndStandardBar.LoadToolBar(IDR_STDBAR, IDB_STDBAR_C, 0, FALSE, IDB_STDBAR_D, 0, IDB_STDBAR_H))
#else
			!m_wndStandardBar.LoadToolBar(IDR_STDBAR))
#endif
		{
			TRACE0("Failed to create Standard Toolbar\n");
			return -1;      // fail to create
		}
		m_wndStandardBar.SetWindowText("Standard Toolbar");
		//m_wndStandardBar.SetPaneStyle(m_wndStandardBar.GetPaneStyle() |CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
		m_wndStandardBar.EnableCustomizeButton(TRUE, ID_CUSTOMIZE, _T("Customize..."));
		m_wndStandardBar.EnableDocking(CBRS_ALIGN_ANY);
		//m_wndStandardBar.EnableTextLabels();
		DockPane(&m_wndStandardBar);

		// Create "edit" bar
		//if (!m_wndEditBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_EDITBAR) ||
		if (!m_wndEditBar.CreateEx(this,
								TBSTYLE_FLAT,
								WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC,
								CRect(1, 1, 1, 1),
								IDR_EDITBAR ) ||
#if SHADED_TOOLBARS
			!m_wndEditBar.LoadToolBar(IDR_EDITBAR, IDB_EDITBAR_C, 0, FALSE, IDB_EDITBAR_D, 0, IDB_EDITBAR_H))
#else
			!m_wndEditBar.LoadToolBar(IDR_EDITBAR))
#endif
		{
			TRACE0("Failed to create Edit Bar\n");
			return -1;      // fail to create
		}
		m_wndEditBar.SetWindowText("Edit Bar");
		//m_wndEditBar.SetPaneStyle(m_wndEditBar.GetPaneStyle() |CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
		m_wndEditBar.EnableCustomizeButton(TRUE, ID_CUSTOMIZE, _T("Customize..."));
		m_wndEditBar.EnableDocking(CBRS_ALIGN_ANY);
		DockPane(&m_wndEditBar);

		// Create Format bar
		//if (!m_wndFormatBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_FORMATBAR) ||
		if (!m_wndFormatBar.CreateEx(this,
								TBSTYLE_FLAT,
								WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC,
								CRect(1, 1, 1, 1),
								IDR_FORMATBAR ) ||
#if SHADED_TOOLBARS
			!m_wndFormatBar.LoadToolBar(IDR_FORMATBAR, IDB_FMTBAR_C, 0, FALSE, IDB_FMTBAR_D, 0, IDB_FMTBAR_H))
#else
			!m_wndFormatBar.LoadToolBar(IDR_FORMATBAR))
#endif
		{
			TRACE0("Failed to create Format Bar\n");
			return -1;      // fail to create
		}
		m_wndFormatBar.SetWindowText("Format Bar");
		//m_wndFormatBar.SetPaneStyle(m_wndFormatBar.GetPaneStyle() |CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
		m_wndFormatBar.EnableCustomizeButton(TRUE, ID_CUSTOMIZE, _T("Customize..."));
		m_wndFormatBar.EnableDocking(CBRS_ALIGN_ANY);
		DockPane(&m_wndFormatBar);
		m_wndFormatBar.ShowPane(FALSE, FALSE, FALSE);

		// Create Navigation bar
		//if (!m_wndNavBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_NAVBAR) ||
		if (!m_wndNavBar.CreateEx(this,
								TBSTYLE_FLAT,
								WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC,
								CRect(1, 1, 1, 1),
								IDR_NAVBAR ) ||
#if SHADED_TOOLBARS
			!m_wndNavBar.LoadToolBar(IDR_NAVBAR, IDB_NAVBAR_C, 0, FALSE, IDB_NAVBAR_D, 0, IDB_NAVBAR_H))
#else
			!m_wndNavBar.LoadToolBar(IDR_NAVBAR))
#endif
		{
			TRACE0("Failed to create Nav Bar\n");
			return -1;      // fail to create
		}
		m_wndNavBar.SetWindowText("Navigation Bar");
		//m_wndNavBar.SetPaneStyle(m_wndNavBar.GetPaneStyle() |CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
		m_wndNavBar.EnableCustomizeButton(TRUE, ID_CUSTOMIZE, _T("Customize..."));
		m_wndNavBar.EnableDocking(CBRS_ALIGN_ANY);
		DockPane(&m_wndNavBar);
		m_wndNavBar.ShowPane(FALSE, FALSE, FALSE);

		if (!m_wndStatusBar.Create(this) ||
			!m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(*indicators)))
		{
				TRACE0("Failed to create status bar\n");
				return -1;      // fail to create
		}
		m_wndStatusBar.SetPaneWidth(0, 340);
		m_wndStatusBar.SetToolTips();
		ASSERT(m_wndStatusBar.CommandToIndex(ID_INDICATOR_OCCURRENCES) > -1);
		if (HBITMAP(m_search_image) != 0)
			m_wndStatusBar.SetPaneIcon(m_wndStatusBar.CommandToIndex(ID_INDICATOR_OCCURRENCES), HBITMAP(m_search_image), RGB(255,255,255));
		for (int pane = 1; pane < sizeof(indicators)/sizeof(*indicators)-3; ++pane)
			m_wndStatusBar.SetPaneText(pane, "");   // clear out dummy text

		// Create the dockable windows
		if (!m_paneBookmarks.Create("Bookmarks", this, CSize(500, 250), TRUE, IDD_BOOKMARKS_PARENT, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT| CBRS_FLOAT_MULTI) ||
			!m_wndBookmarks.Create(&m_paneBookmarks) )
		{
			return FALSE; // failed to create
		}
		m_paneBookmarks.InitialUpdate(&m_wndBookmarks);
		m_paneBookmarks.EnableDocking(CBRS_ALIGN_ANY);

		if (!m_paneFind.Create("Find", this, CSize(500, 250), TRUE, IDD_FIND_PARENT, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT| CBRS_FLOAT_MULTI) ||
			!m_wndFind.Create(&m_paneFind, WS_CHILD | WS_VISIBLE) )
		{
			return FALSE; // failed to create
		}
		m_paneFind.InitialUpdate(&m_wndFind);
		m_paneFind.EnableDocking(CBRS_ALIGN_ANY);

		if (!m_paneCalc.Create("Calculator", this, CSize(500, 250), TRUE, IDD_CALC_PARENT, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT| CBRS_FLOAT_MULTI) ||
			!m_wndCalc.Create(&m_paneCalc) )
		{
			return FALSE; // failed to create
		}
		m_paneCalc.InitialUpdate(&m_wndCalc);
		CRect rct;
		m_wndCalc.GetWindowRect(&rct);
		m_paneCalc.SetMinSize(rct.Size());
		m_paneCalc.EnableDocking(CBRS_ALIGN_ANY);

		if (!m_paneProp.Create("Properties", this, CSize(500, 250), TRUE, IDD_PROP_PARENT, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT| CBRS_FLOAT_MULTI) ||
			!m_wndProp.Create(&m_paneProp, WS_CHILD | WS_VISIBLE) )
		{
			return FALSE; // failed to create
		}
		m_paneProp.InitialUpdate(&m_wndProp);
		m_paneProp.EnableDocking(CBRS_ALIGN_ANY);

		if (!m_paneExpl.Create("HexEdit Explorer", this, CSize(500, 250), TRUE, IDD_EXPLORER_PARENT, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT| CBRS_FLOAT_MULTI) ||
			!m_wndExpl.Create(&m_paneExpl) )
		{
			return FALSE; // failed to create
		}
		m_paneExpl.InitialUpdate(&m_wndExpl);
		m_paneExpl.EnableDocking(CBRS_ALIGN_ANY);

		// Set initial positions and docking/floating status but then hide all the windows
		InitDockWindows();

		m_paneBookmarks.DockToFrameWindow(CBRS_ALIGN_LEFT);
		m_paneCalc.DockToWindow(&m_paneBookmarks, CBRS_ALIGN_BOTTOM);
		m_paneExpl.DockToFrameWindow(CBRS_ALIGN_BOTTOM);
		m_paneBookmarks.Hide();
		m_paneFind.Hide();
		m_paneProp.Hide();
		m_paneCalc.Hide();
		m_paneExpl.Hide();

		// Get extra command images (without creating a toolbar)
#if SHADED_TOOLBARS
		CMFCToolBar::AddToolBarForImageCollection(IDR_MISC, IDB_MISCBAR_H, IDB_MISCBAR_C, 0, IDB_MISCBAR_D);
		CMFCToolBar::AddToolBarForImageCollection(IDR_OPER, IDB_OPERBAR_H, IDB_OPERBAR_C, 0, IDB_OPERBAR_D);
#else
		CMFCToolBar::AddToolBarForImageCollection(IDR_MISC);
		CMFCToolBar::AddToolBarForImageCollection(IDR_OPER);
#endif

		CString strDefault = ::GetExePath() + "DefaultToolbarImages.bmp";
		::GetDataPath(m_strImagesFileName);

		if (!m_strImagesFileName.IsEmpty())
		{
			// We have a data path so check if the toolbar images file is there
			m_strImagesFileName += "ToolbarImages.bmp";

			CFileFind ff;
			if (!ff.FindFile(m_strImagesFileName))
			{
				// The user's toolbar images file was not found so copy it from the default
				::MakeSureDirectoryPathExists(m_strImagesFileName);

				if (!::CopyFile(strDefault, m_strImagesFileName, TRUE))
				{
					// Could not copy the default images file to user's toolbar images file
					if (GetLastError() == ERROR_PATH_NOT_FOUND)
					{
						AfxMessageBox("The HexEdit Application Data folder is invalid");
						m_strImagesFileName = strDefault;
					}
					else
					{
						ASSERT(GetLastError() == ERROR_FILE_NOT_FOUND);
						AfxMessageBox("The default toolbar images file was not found in the HexEdit folder.");
						m_strImagesFileName.Empty();  // we can't do anything so signal that the file is not available
					}
				}
			}
		}
		else
		{
			AfxMessageBox("The HexEdit Application Data folder is not found");
			m_strImagesFileName = strDefault;
		}

		if (!m_strImagesFileName.IsEmpty())
		{
			// Open images files and allow user to use/edit toolbar images
			VERIFY(m_UserImages.Load(m_strImagesFileName));
			if (m_UserImages.IsValid())
				CMFCToolBar::SetUserImages (&m_UserImages);
		}

		CMFCToolBar::EnableQuickCustomization();
		InitUserToolbars(NULL,
						ID_VIEW_USER_TOOLBAR1,
						ID_VIEW_USER_TOOLBAR10);
		EnableWindowsDialog (ID_WINDOW_MANAGER, _T("Windows..."), TRUE);

		EnablePaneMenu(TRUE, ID_CUSTOMIZE, _T("Customize..."), ID_VIEW_TOOLBARS, FALSE, FALSE);

		// Load search strings into mainframe edit bar
		LoadSearchHistory(&theApp);
		LoadJumpHistory(&theApp);
		VERIFY(expr_.LoadVars());

//        VERIFY(timer_id_ = SetTimer(1, 1000, NULL));

		// Set main window title (mucked up by CBCGSizingControlBar::OnSetText)
		CString strMain;
		strMain.LoadString(IDR_MAINFRAME);
		SetWindowText(strMain);

		return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	WNDCLASS wndclass;
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	HINSTANCE hInst = AfxGetInstanceHandle();

	BOOL retval = CMDIFrameWndEx::PreCreateWindow(cs);

	::GetClassInfo(hInst, cs.lpszClass, &wndclass);
	wndclass.style &= ~(CS_HREDRAW|CS_VREDRAW);
	wndclass.lpszClassName = aa->szHexEditClassName;
	wndclass.hIcon = ::LoadIcon(hInst, MAKEINTRESOURCE(IDR_MAINFRAME));
	ASSERT(wndclass.hIcon != 0);
	if (!AfxRegisterClass(&wndclass))
		AfxThrowResourceException();

	cs.lpszClass = aa->szHexEditClassName;

	if (aa->open_restore_)
	{
		int ss;
		if ((ss = aa->GetProfileInt("MainFrame", "WindowState", -1)) != -1)
			aa->m_nCmdShow = ss;

		// Get the window position/size
		int top, left, bottom, right;
		top = aa->GetProfileInt("MainFrame", "WindowTop", -30000);
		left = aa->GetProfileInt("MainFrame", "WindowLeft", -30000);
		bottom = aa->GetProfileInt("MainFrame", "WindowBottom", -30000);
		right = aa->GetProfileInt("MainFrame", "WindowRight", -30000);

		// If the values look OK change the CREATESTRUCT value correspondingly
		if (top != -30000 && right != -30000 && top < bottom && left < right)
		{
			// Get the work area within the display
			CRect rct;
			if (aa->mult_monitor_)
			{
				CRect rr(left, top, right, bottom);
				HMONITOR hh = MonitorFromRect(&rr, MONITOR_DEFAULTTONEAREST);
				MONITORINFO mi;
				mi.cbSize = sizeof(mi);
				if (hh != 0 && GetMonitorInfo(hh, &mi))
					rct = mi.rcWork;  // work area of nearest monitor
				else
				{
					// Shouldn't happen but if it does use the whole virtual screen
					ASSERT(0);
					rct = CRect(::GetSystemMetrics(SM_XVIRTUALSCREEN),
						::GetSystemMetrics(SM_YVIRTUALSCREEN),
						::GetSystemMetrics(SM_XVIRTUALSCREEN) + ::GetSystemMetrics(SM_CXVIRTUALSCREEN),
						::GetSystemMetrics(SM_YVIRTUALSCREEN) + ::GetSystemMetrics(SM_CYVIRTUALSCREEN));
				}
			}
			else if (!::SystemParametersInfo(SPI_GETWORKAREA, 0, &rct, 0))
			{
				// I don't know if this will ever happen since the Windows documentation
				// is pathetic and does not say when or why SystemParametersInfo might fail.
				rct = CRect(0, 0, ::GetSystemMetrics(SM_CXFULLSCREEN),
								  ::GetSystemMetrics(SM_CYFULLSCREEN));
			}

			// Make sure that the window is not off the screen (or just on it).
			// (There might be a different screen resolution since options were saved.)
			if (left > rct.right - 20)
			{
				left = rct.right - (right - left);
				right = rct.right;
			}
			if (right < rct.left + 20)
			{
				right = rct.left + (right - left);
				left = rct.left;
			}
			if (top > rct.bottom - 20)
			{
				top = rct.bottom - (bottom - top);
				bottom = rct.bottom;
			}
			// Make sure top is not more than a little bit off the top of screen
			if (top < rct.top - 15)
			{
				bottom = rct.top + (bottom - top);
				top = rct.top;
			}

			// Set window width and height
			cs.cx = right - left;
			cs.cy = bottom - top;

			cs.x = left;
			cs.y = top;
		}
	}
	return retval;
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	CWnd *pwnd = CWnd::FromHandlePermanent(pMsg->hwnd);

	if (pwnd != NULL && 
		(pwnd->IsKindOf(RUNTIME_CLASS(CSearchEditControl)) ||
		 pwnd->IsKindOf(RUNTIME_CLASS(CHexEditControl)) ||
		 pwnd->IsKindOf(RUNTIME_CLASS(CDecEditControl)) ||
		 pwnd->IsKindOf(RUNTIME_CLASS(CCalcEdit)) ||
		 pwnd->IsKindOf(RUNTIME_CLASS(CInPlaceEdit))  ) )
	{
		return FALSE;
	}

	return CMDIFrameWndEx::PreTranslateMessage(pMsg);
}

void CMainFrame::OnClose()
{
	// Save state (posn, docked, hidden etc) of control bars (tool, edit
	// & status bars) if save options on exit is on
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

	SaveSearchHistory(aa);
	SaveJumpHistory(aa);
	expr_.SaveVars();

	SaveBarState("DockState");

	if (aa->save_exit_)
		SaveFrameOptions();

	// The following were moved here from HexEditApp::ExitInstance as they require
	// access to the main windows (AfxGetMainWnd() returns NULL in ExitInstance)

	// Clear histories if the option to clear on exit is on
	if (theApp.clear_on_exit_)
	{
		CHexFileList *pfl = theApp.GetFileList();
		if (pfl != NULL && theApp.clear_recent_file_list_)
		{
			pfl->ClearAll();
		}

		CBookmarkList *pbl = theApp.GetBookmarkList();
		if (pbl != NULL && theApp.clear_bookmarks_)
		{
			pbl->ClearAll();
		}
	}

	if (aa->delete_all_settings_)
		remove(m_strImagesFileName);

	CMDIFrameWndEx::OnClose();
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CMDIFrameWndEx::OnSize(nType, cx, cy);
	Invalidate(TRUE);
}


// Handles control menu commands and system buttons (Minimize etc)
void CMainFrame::OnSysCommand(UINT nID, LONG lParam)
{
	CMDIFrameWndEx::OnSysCommand(nID, lParam);

	nID &= 0xFFF0;
	if (nID == SC_MINIMIZE || nID == SC_RESTORE || nID == SC_MAXIMIZE)
		theApp.SaveToMacro(km_mainsys, nID);
}

// Given a popup menu and an ID of an item within the menu return the
// rectangle of the item in screen coordinates
CRect CMainFrame::item_rect(CMFCPopupMenu *pm, UINT id)
{
	CRect rct;
	pm->GetMenuBar()->GetWindowRect(&rct);

	rct.top += ::GetSystemMetrics(SM_CYEDGE) + 1;
	for (int ii = 0; ii < pm->GetMenuItemCount(); ++ii)
	{
		CRect item_rct;
		pm->GetMenuItem(ii)->GetImageRect(item_rct);
#if _MSC_VER < 1400
		if (::GetMenuItemID(pm->GetMenu(), ii) == id)
#else
		if (::GetMenuItemID(pm->GetMenu()->m_hMenu, ii) == id)
#endif
		{
			rct.bottom = rct.top + item_rct.Height();
			return rct;
		}
		rct.top += item_rct.Height();
	}
	return CRect(0, 0, 0, 0);
}

void CMainFrame::show_tip(UINT id /* = -1 */)
{
	menu_tip_.Hide();
	if (id == -1)
		id = last_id_;
	else
		last_id_ = id;
	if (popup_menu_.empty())
		return;

	CHexEditView *pview = GetView();

	menu_tip_.Clear();
	menu_tip_.SetBgCol(::GetSysColor(COLOR_INFOBK));
	menu_tip_.SetStockFont(ANSI_VAR_FONT);

	if (id >= ID_NAV_BACK_FIRST && id < ID_NAV_BACK_FIRST + NAV_RESERVED)
	{
		menu_tip_.AddString(theApp.navman_.GetInfo(true, id - ID_NAV_BACK_FIRST));
	}
	else if (id >= ID_NAV_FORW_FIRST && id < ID_NAV_FORW_FIRST + NAV_RESERVED)
	{
		menu_tip_.AddString(theApp.navman_.GetInfo(false, id - ID_NAV_FORW_FIRST + 1));
	}
	else if (pview != NULL)
	{
		CRect rct;
		COLORREF addr_col = pview->DecAddresses() ? pview->GetDecAddrCol() : pview->GetHexAddrCol();
		COLORREF text_col = pview->GetDefaultTextCol();
		menu_tip_.SetBgCol(pview->GetBackgroundCol());
		menu_tip_.SetStockFont(ANSI_FIXED_FONT);

		switch (id)
		{
		case ID_DISPLAY_HEX:
			menu_tip_.AddString("200: ", addr_col);
			menu_tip_.AddString("206: ", addr_col);
			rct = menu_tip_.GetRect(0);
			menu_tip_.AddString("23 68 71 62  53 2A", text_col, &CPoint(rct.Width(), rct.top));
			rct = menu_tip_.GetRect(1);
			menu_tip_.AddString("68 6E 62 78  79 00", text_col, &CPoint(rct.Width(), rct.top));
			break;
		case ID_DISPLAY_CHAR:
			menu_tip_.AddString("200: ", addr_col);
			menu_tip_.AddString("206: ", addr_col);
			rct = menu_tip_.GetRect(0);
			menu_tip_.AddString("#hqbS*", text_col, &CPoint(rct.Width(), rct.top));
			rct = menu_tip_.GetRect(1);
			menu_tip_.AddString("hnbxy.", text_col, &CPoint(rct.Width(), rct.top));
			break;
		case ID_DISPLAY_BOTH:
			menu_tip_.AddString("200: ", addr_col);
			menu_tip_.AddString("206: ", addr_col);
			rct = menu_tip_.GetRect(0);
			menu_tip_.AddString("23 68 71 62  53 2A  #hqbS*", text_col, &CPoint(rct.Width(), rct.top));
			rct = menu_tip_.GetRect(1);
			menu_tip_.AddString("68 6E 62 78  79 00  hnbxy.", text_col, &CPoint(rct.Width(), rct.top));
			break;
		case ID_DISPLAY_STACKED:
			menu_tip_.AddString("200: ", addr_col);
			rct = menu_tip_.GetRect(0);
			menu_tip_.AddString("#hqb S*hn bxy.", text_col, &CPoint(rct.Width(), rct.top));
			rct = menu_tip_.GetRect(1);
			menu_tip_.AddString("2676 5266 6770", text_col, &CPoint(rct.left, rct.bottom));
			rct = menu_tip_.GetRect(2);
			menu_tip_.AddString("3812 3A8E 2890", text_col, &CPoint(rct.left, rct.bottom));
			break;
		}
		//menu_tip_.SetFont(pview->GetFont());  // We need a non-prop. font but this does not do anything
	}

	if (menu_tip_.Count() > 0)
	{
		menu_tip_.Hide(0);
		CRect rct = item_rect(popup_menu_.back(), id);
		menu_tip_.Move(CPoint(rct.right, rct.top), false);
		CRect wnd_rct;
		menu_tip_.GetWindowRect(&wnd_rct);
		if (::OutsideMonitor(wnd_rct))
			menu_tip_.Move(CPoint(rct.left, rct.bottom), false);
		menu_tip_.Show();
	}
}

void CMainFrame::OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hMenu)
{
	//TRACE("WM_MENUSELECT %d, %x, %d\n", nItemID, nFlags, int(hMenu));
	if ((nFlags & 0xFFFF) == 0xFFFF || (nFlags & MF_POPUP) != 0 || (nFlags & MF_SEPARATOR) != 0)
		menu_tip_.Hide();
	else
		show_tip(nItemID);

	CMDIFrameWndEx::OnMenuSelect(nItemID, nFlags, hMenu);
}

// We could draw into the MDI client area here (eg HexEdit logo).
BOOL CMainFrame::OnEraseMDIClientBackground(CDC* pDC)
{
#ifdef USE_FREE_IMAGE
	if (m_dib == NULL || m_background_pos == 0)
		return FALSE;  // No bitmap so let Windows draw the background
#else
	if (HBITMAP(m_background) == 0 || m_background_pos == 0)
		return FALSE;  // No bitmap so let Windows draw the background
#endif

	CRect rct;
	m_wndClientArea.GetClientRect(rct);

	// TBD: TODO check the problem below still occurs with MFC10

	// Before MFC9 drawing the background was simple - even if drawing relative to bottom
	// or right side it was a simple matter to redraw the background whenever this event was
	// generated - even when things were docked to the sides of the window.
	// With MFC9, if the user docks (toolbar or tool window) to the sides of the window then
	// the background is not refreshed properly unless drawing relative to top-left.  Ie:
	//  - if drawing relative to top-left (m_background_pos == 1) drawing is always OK
	//  - if resizing the window drawing is always OK
	//  - when docking/undocking (and m_background_pos != 1) some parts of the background are not redrawn

	// Solutions tried that did not work:
	//  - ignore clipping rect and redraw the whole window - somehow Windows prevents this working
	//  - if clip rect smaller than whole window invalidate the whole window (see code below) - causes weird side effects
#if 0
	// Get rectangle to be drawn into
	CRect clip_rct;
	pDC->GetClipBox(&clip_rct);
	if (m_background_pos != 1 && rct != clip_rct)
	{
		// The kludgey solution was to invalidate the whole window if the clip rect is smaller than it.
		m_wndClientArea.InvalidateRect(&rct, TRUE);
		return TRUE;  // prevent any other erasure since we will erase everything shortly anyway.
	}
#endif
	// pDC->FillSolidRect(rct, ::GetSysColor(COLOR_APPWORKSPACE));

	CSize siz;
#ifdef USE_FREE_IMAGE
	siz.cx = FreeImage_GetWidth(m_dib);
	siz.cy = FreeImage_GetHeight(m_dib);
#else
	BITMAP bi;
	m_background.GetBitmap(&bi);
	siz.cx = bi.bmWidth;
	siz.cy = bi.bmHeight;
#endif

	CPoint point;
	CBrush backBrush;
	switch (m_background_pos)
	{
	case 1:  // top left
		point.x = point.y = 0;
		break;
	case 2:  // top right
		point.x = rct.Width() - siz.cx;
		point.y = 0;
		break;
	case 3:  // bottom left
		point.x = 0;
		point.y = rct.Height() - siz.cy;
		break;
	case 4:  // bottom right
		point.x = rct.Width() - siz.cx;
		point.y = rct.Height() - siz.cy;
		break;
	case 5:  // centre
		point.x = (rct.Width() - siz.cx)/2;
		point.y = (rct.Height() - siz.cy)/2;
		break;
	case 6: // stretch
		{
#ifdef USE_FREE_IMAGE
		  ::StretchDIBits(pDC->GetSafeHdc(),
						  0, 0, rct.Width(), rct.Height(),
						  0, 0, siz.cx, siz.cy,
						  FreeImage_GetBits(m_dib), FreeImage_GetInfo(m_dib), DIB_RGB_COLORS, SRCCOPY);
#else
			CDC dcTmp;
			dcTmp.CreateCompatibleDC(pDC);
			dcTmp.SelectObject(&m_background);
			pDC->StretchBlt(0, 0, rct.Width(), rct.Height(), 
					&dcTmp, 0, 0, siz.cx, siz.cy, SRCCOPY);
			dcTmp.DeleteDC();
#endif
		}
		goto no_fill;
	case 7:  // tile
		for (point.x = 0; point.x < rct.Width(); point.x += siz.cx)
			for (point.y = 0; point.y < rct.Height(); point.y += siz.cy)
#ifdef USE_FREE_IMAGE
				::StretchDIBits(pDC->GetSafeHdc(),
								point.x, point.y, siz.cx, siz.cy,
								0, 0, siz.cx, siz.cy,
								FreeImage_GetBits(m_dib), FreeImage_GetInfo(m_dib), DIB_RGB_COLORS, SRCCOPY);
#else
				pDC->DrawState(point, siz, &m_background, DST_BITMAP | DSS_NORMAL);
#endif
		goto no_fill;
	}

	// Create background brush using top left pixel of bitmap
	{
#ifdef USE_FREE_IMAGE
		RGBQUAD px = { 192, 192, 192, 0};  // default to grey in case GetPixelColor fails
		int height = FreeImage_GetHeight(m_dib);
		VERIFY(FreeImage_GetPixelColor(m_dib, 0, height - 1, &px));  // get colour from top-left pixel (may fail if not 24 bit colours)
		backBrush.CreateSolidBrush(RGB(px.rgbRed, px.rgbGreen, px.rgbBlue));
#else
		CDC dcTmp;
		dcTmp.CreateCompatibleDC(pDC);
		dcTmp.SelectObject(&m_background);

		// Use top left pixel as background colour
		backBrush.CreateSolidBrush(::GetPixel(dcTmp, 0, 0));  // get colour from (0,0) pixel
#endif
		backBrush.UnrealizeObject();
	} // dcTmp destroyed here

	pDC->FillRect(rct, &backBrush);
#ifdef USE_FREE_IMAGE
	::StretchDIBits(pDC->GetSafeHdc(),
					point.x, point.y, siz.cx, siz.cy,
					0, 0, siz.cx, siz.cy,
					FreeImage_GetBits(m_dib), FreeImage_GetInfo(m_dib), DIB_RGB_COLORS, SRCCOPY);
#else
	pDC->DrawState(point, siz, &m_background, DST_BITMAP | DSS_NORMAL);
#endif
no_fill:
	return TRUE;
}

// The following bizare stuff is used to set the current page in print preview
// mode.  This is necessary due to a bug in MFC.  CPreviewViewKludge is just
// used to get access to the protected members of CPreviewView.
class CPreviewViewKludge : public CPreviewView
{
public:
	BOOL Inited() { return CPreviewView::m_hMagnifyCursor != 0; }
	void SetCurrentPage2(UINT nPage, BOOL bClearRatios) { CPreviewView::SetCurrentPage(nPage, bClearRatios); }
};

void CMainFrame::RecalcLayout(BOOL bNotify)
{
	CPreviewViewKludge *ppv = (CPreviewViewKludge *)GetActiveView();
	if (ppv != NULL && ppv->IsKindOf(RUNTIME_CLASS(CPreviewView)))
	{
		if (!ppv->Inited() && preview_page_ != -1)
			ppv->SetCurrentPage2(preview_page_, TRUE);
	}

	CMDIFrameWndEx::RecalcLayout(bNotify);
}

LONG CMainFrame::OnOpenMsg(UINT, LONG lParam)
{
	char filename[256];
	filename[0] = '\0';
	if (::GlobalGetAtomName((ATOM)lParam, filename, sizeof(filename)) == 0)
		return FALSE;
	ASSERT(theApp.open_current_readonly_ == -1);
	return theApp.OpenDocumentFile(filename) != NULL;
}

void CMainFrame::SaveFrameOptions()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

	// Save info about the main frame window position
	aa->WriteProfileInt("MainFrame", "Restore", aa->open_restore_ ? 1 : 0);
	if (aa->open_restore_)
	{
		WINDOWPLACEMENT wp;
		GetWindowPlacement(&wp);
		aa->WriteProfileInt("MainFrame", "WindowState", wp.showCmd);
		aa->WriteProfileInt("MainFrame", "WindowTop",    wp.rcNormalPosition.top);
		aa->WriteProfileInt("MainFrame", "WindowLeft",   wp.rcNormalPosition.left);
		aa->WriteProfileInt("MainFrame", "WindowBottom", wp.rcNormalPosition.bottom);
		aa->WriteProfileInt("MainFrame", "WindowRight",  wp.rcNormalPosition.right);
	}
}

// Set the default positions of all docking windows
void CMainFrame::InitDockWindows()
{
	// Initially all are floating
	m_paneFind.Float(false);
	m_paneBookmarks.Float(false);
	m_paneProp.Float(false);
	m_paneCalc.Float(false);
	m_paneExpl.Float(false);

	// We get the main window rectangle so we can position the floating
	// windows around its edges.
	CSize sz;
	CRect mainRect, rct;
	GetWindowRect(&mainRect);   // get main window in order to set float positions

	// Position bookmarks in middle of left side
	sz = m_paneBookmarks.GetFrameSize();
	rct.left = mainRect.left;
	rct.right = rct.left + sz.cx;
	rct.top = mainRect.top + mainRect.Size().cy/2 - sz.cy/2;
	rct.bottom = rct.top + sz.cy;
	m_paneBookmarks.GetParent()->MoveWindow(rct);

	// Position Find dialog at bottom left
	sz = m_paneFind.GetFrameSize();
	rct.left = mainRect.left;
	rct.right = rct.left + sz.cx;
	rct.top = mainRect.bottom - sz.cy;
	rct.bottom = rct.top + sz.cy;
	m_paneFind.GetParent()->MoveWindow(rct);

	// Position Properties dialog at middle of bottom
	sz = m_paneProp.GetFrameSize();
	rct.left = mainRect.left + mainRect.Size().cx/2 - sz.cx/2;
	rct.right = rct.left + sz.cx;
	rct.top = mainRect.bottom - sz.cy;
	rct.bottom = rct.top + sz.cy;
	m_paneProp.GetParent()->MoveWindow(rct);

	// Position Calculator at bottom right
	sz = m_paneCalc.GetFrameSize();
	rct.left = mainRect.right - sz.cx;
	rct.right = rct.left + sz.cx;
	rct.top = mainRect.bottom - sz.cy;
	rct.bottom = rct.top + sz.cy;
	m_paneCalc.GetParent()->MoveWindow(rct);

	// Position Explorer at top right
	sz = m_paneExpl.GetFrameSize();
	rct.left = mainRect.right - sz.cx;
	rct.right = rct.left + sz.cx;
	rct.top = mainRect.top;
	rct.bottom = rct.top + sz.cy;
	m_paneExpl.GetParent()->MoveWindow(rct);
}

void CMainFrame::FixPanes()
{
	// Doing this at the strat avoids a problem where panes floating by themself are
	// not drawn properly when their position is restored on startup.  The disadvantage
	// to the user is that any window they left floating will not be restored when they
	// reopen HexEdit (though the position is remembered and restored when they open it).
	if (m_paneBookmarks.IsFloating())
		m_paneBookmarks.Hide();
	if (m_paneFind.IsFloating())
		m_paneFind.Hide();
	if (m_paneCalc.IsFloating())
		m_paneCalc.Hide();
	if (m_paneProp.IsFloating())
		m_paneProp.Hide();
	if (m_paneExpl.IsFloating())
		m_paneExpl.Hide();
}

void CMainFrame::move_dlgbar(CGenDockablePane &bar, const CRect &rct)
{
	// We don't need to move it if hidden or docked
	if (!bar.IsWindowVisible() || 
		bar.IsDocked() && !bar.IsInFloatingMultiPaneFrameWnd())
		return;

	// Get height of screen (needed later)
	int scr_height = GetSystemMetrics(SM_CYSCREEN);
	int scr_width  = GetSystemMetrics(SM_CXSCREEN);

	CWnd * pFrm = &bar;   // Window we (may) need to move
	CRect wnd_rct;        // Rect of the window we need to move

	while ((pFrm = pFrm->GetParent()) != NULL)
	{
		if (pFrm->IsKindOf(RUNTIME_CLASS(CMultiPaneFrameWnd)))
			break;
	}
	if (pFrm == NULL)
	{
		ASSERT(0);  // We din't find it which should not happen
		return;
	}
	pFrm->GetWindowRect(&wnd_rct);

	CRect intersect_rct;               // Intersection of dialog & selection
	if (!intersect_rct.IntersectRect(&wnd_rct, &rct))
		return;                        // No intersection so we do nothing

	CRect new_rct(wnd_rct);             // New position of dialog

	bool move_horiz = rct.top > wnd_rct.top &&
					  rct.bottom < wnd_rct.bottom &&
					  (rct.left < wnd_rct.left && rct.right < wnd_rct.right ||
					   rct.left > wnd_rct.left && rct.right > wnd_rct.right);

	// Most of the time we move vertically - this is when
	if (!move_horiz)
	{
		if (new_rct.CenterPoint().y > rct.CenterPoint().y)
		{
			// Move downwards
			new_rct.OffsetRect(0, rct.bottom - new_rct.top);
			if (new_rct.bottom > scr_height - 40)
				new_rct.OffsetRect(0, rct.top - new_rct.bottom);
			if (new_rct.top < 0)
				move_horiz = true;
		}
		else
		{
			new_rct.OffsetRect(0, rct.top - new_rct.bottom);
			if (new_rct.top < 40)
				new_rct.OffsetRect(0, rct.bottom - new_rct.top);
			if (new_rct.top > scr_height - 80)
				move_horiz = true;
		}
	}

	if (move_horiz)
	{
		new_rct = wnd_rct;              // Start again
		if (new_rct.CenterPoint().x > rct.CenterPoint().x)
		{
			// Move right
			new_rct.OffsetRect(rct.right - new_rct.left, 0);
			if (new_rct.right > scr_width - 40)
				new_rct.OffsetRect(rct.left - new_rct.right, 0);
			if (new_rct.right < 40)
				new_rct = wnd_rct;      // Give up
		}
		else
		{
			// Move left
			new_rct.OffsetRect(rct.left - new_rct.right, 0);
			if (new_rct.left < 40)
				new_rct.OffsetRect(rct.right - new_rct.left, 0);
			if (new_rct.left > scr_width - 40)
				new_rct = wnd_rct;      // Give up
		}
	}

	pFrm->MoveWindow(&new_rct);

#if 0 // causes weird things if dragging a selection
	// If the mouse ptr (cursor) was over the dialog move it too
	CPoint mouse_pt;
	::GetCursorPos(&mouse_pt);
	if (wnd_rct.PtInRect(mouse_pt))
	{
		// Mouse was over find dialog (probably over "Find Next" button)
		mouse_pt.x += new_rct.left - wnd_rct.left;
		mouse_pt.y += new_rct.top - wnd_rct.top;
		::SetCursorPos(mouse_pt.x, mouse_pt.y);
	}
#endif
}

void CMainFrame::show_calc()
{
	m_wndCalc.SetWindowText("Calculator");
	m_paneCalc.ShowAndUnroll();
}

void CMainFrame::OnContextMenu(CWnd* pWnd, CPoint point)
{
	if (pWnd->GetSafeHwnd () == m_wndClientArea.GetMDITabs().GetSafeHwnd())
	{
		const CMFCTabCtrl & wndTab = m_wndClientArea.GetMDITabs();
		CRect rectTabs;
		wndTab.GetTabsRect (rectTabs);
		CPoint ptTab = point;
		wndTab.ScreenToClient (&ptTab);
		int iClickedTab = wndTab.GetTabFromPoint (ptTab);
		if (iClickedTab >= 0)
			m_wndClientArea.SetActiveTab(wndTab.GetTabWnd(iClickedTab)->m_hWnd);

		theApp.ShowPopupMenu (IDR_CONTEXT_TABS, point, pWnd);
	}
}

BOOL CMainFrame::OnHelpInfo(HELPINFO* pHelpInfo)
{
	return CMDIFrameWndEx::OnHelpInfo(pHelpInfo);
}

void CMainFrame::OnContextHelp()
{
	CMDIFrameWndEx::OnContextHelp();
}

void CMainFrame::OnHelpFinder()
{
	CMDIFrameWndEx::OnHelpFinder();
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_topics);
}

void CMainFrame::OnHelpKeyboardMap()
{
	CMFCKeyMapDialog dlg(this, TRUE);
	dlg.DoModal();
}

void CMainFrame::OnHelpTute(UINT nID)
{
	// Display help for this page
	if (!::HtmlHelp(m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, 0x10000+nID))
		AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

void CMainFrame::OnHelp()
{
	CMDIFrameWndEx::OnHelp();
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_help);
}

void CMainFrame::HtmlHelp(DWORD_PTR dwData, UINT nCmd)
{
	if (dwData == AFX_HIDD_FILEOPEN || dwData == AFX_HIDD_FILESAVE)
		dwData = hid_last_file_dialog;
	CMDIFrameWndEx::HtmlHelp(dwData, nCmd);
}

// This is here just so we can intercept calls in the debugger
LRESULT CMainFrame::OnCommandHelp(WPARAM wParam, LPARAM lParam)
{
	return CMDIFrameWndEx::OnCommandHelp(wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
		CMDIFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
		CMDIFrameWndEx::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::OnNavigateBackwards()
{
	theApp.navman_.GoBack();
}

void CMainFrame::OnUpdateNavigateBackwards(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(theApp.navman_.BackAllowed());
}

void CMainFrame::OnNavigateForwards()
{
	theApp.navman_.GoForw();
}

void CMainFrame::OnUpdateNavigateForwards(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(theApp.navman_.ForwAllowed());
}

void CMainFrame::OnNavBack(UINT nID)
{
	ASSERT(nID - ID_NAV_BACK_FIRST < NAV_RESERVED);
	theApp.navman_.GoBack(nID - ID_NAV_BACK_FIRST);
}

void CMainFrame::OnNavForw(UINT nID)
{
	ASSERT(nID - ID_NAV_FORW_FIRST < NAV_RESERVED);
	theApp.navman_.GoForw(nID - ID_NAV_FORW_FIRST + 1);
}

LRESULT CMainFrame::OnGetTabToolTip(WPARAM /*wp*/, LPARAM lp)
{
	CMFCTabToolTipInfo * pInfo = (CMFCTabToolTipInfo *)lp;
	CFrameWnd * pFrame;
	CDocument * pDoc;

	if (pInfo == NULL ||
		!pInfo->m_pTabWnd->IsMDITab() ||
		(pFrame = DYNAMIC_DOWNCAST(CFrameWnd, pInfo->m_pTabWnd->GetTabWnd(pInfo->m_nTabIndex))) == NULL ||
		(pDoc = pFrame->GetActiveDocument()) == NULL
	   )
	{
		return 0;
	}

	pInfo->m_strText = pDoc->GetPathName();
	return 0;
}

void CMainFrame::OnDockableToggle()
{
	theApp.dlg_dock_ = !theApp.dlg_dock_;

	if (!theApp.dlg_dock_)
	{
		// Make sure the windows aren't docked and turn off dockability
		m_paneFind.Float();
		m_paneFind.EnableDocking(0);
		m_paneBookmarks.Float();
		m_paneBookmarks.EnableDocking(0);
		m_paneProp.Float();
		m_paneProp.EnableDocking(0);
		m_paneCalc.Float();
		m_paneCalc.EnableDocking(0);
		m_paneExpl.Float();
		m_paneExpl.EnableDocking(0);
	}
	else
	{
		// Turn on dockability for all modeless (pane) dialogs
		m_paneFind.EnableDocking(CBRS_ALIGN_ANY);
		m_paneBookmarks.EnableDocking(CBRS_ALIGN_ANY);
		m_paneProp.EnableDocking(CBRS_ALIGN_ANY);
		m_paneCalc.EnableDocking(CBRS_ALIGN_ANY);
		m_paneExpl.EnableDocking(CBRS_ALIGN_ANY);
	}
}

void CMainFrame::OnUpdateDockableToggle(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(theApp.dlg_dock_);
}

void CMainFrame::OnWindowNew()
{
	// Store options for the active view so that the new view will get the same ones
	if (GetView() != NULL)
		GetView()->StoreOptions();

	CMDIFrameWndEx::OnWindowNew();

	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	if (aa->recording_ && aa->mac_.size() > 0 && (aa->mac_.back()).ktype == km_focus)
	{
		// We don't want focus change recorded (see CHexEditView::OnSetFocus)
		aa->mac_.pop_back();
	}
	aa->SaveToMacro(km_win_new);
}

// Handles the window menu commands: cascade, tile, arrange
BOOL CMainFrame::OnMDIWindowCmd(UINT nID)
{
	BOOL retval = CMDIFrameWndEx::OnMDIWindowCmd(nID);

	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	if (retval)
		aa->SaveToMacro(km_win_cmd, nID);
	else
		aa->mac_error_ = 20;
	return retval;
}

void CMainFrame::OnUpdateViewViewbar(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck((m_wndStandardBar.GetStyle() & WS_VISIBLE) != 0);
}

void CMainFrame::OnViewViewbar()
{
	m_wndStandardBar.ShowPane((m_wndStandardBar.GetStyle() & WS_VISIBLE) == 0, FALSE, FALSE);
	//ShowControlBar(&m_wndStandardBar, (m_wndStandardBar.GetStyle() & WS_VISIBLE) == 0, FALSE);
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_toolbar, 1);
}

void CMainFrame::OnUpdateViewEditbar(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck((m_wndEditBar.GetStyle() & WS_VISIBLE) != 0);
}

void CMainFrame::OnViewEditbar()
{
	m_wndEditBar.ShowPane((m_wndEditBar.GetStyle() & WS_VISIBLE) == 0, FALSE, FALSE);
	//ShowControlBar(&m_wndEditBar, (m_wndEditBar.GetStyle() & WS_VISIBLE) == 0, FALSE);
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_toolbar, 2);
}

void CMainFrame::OnUpdateViewFormatbar(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck((m_wndFormatBar.GetStyle() & WS_VISIBLE) != 0);
}

void CMainFrame::OnViewFormatbar()
{
	m_wndFormatBar.ShowPane((m_wndFormatBar.GetStyle() & WS_VISIBLE) == 0, FALSE, FALSE);
	//ShowControlBar(&m_wndFormatBar, (m_wndFormatBar.GetStyle() & WS_VISIBLE) == 0, FALSE);
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_toolbar, 4);
}

void CMainFrame::OnUpdateViewNavbar(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck((m_wndNavBar.GetStyle() & WS_VISIBLE) != 0);
}

void CMainFrame::OnViewNavbar()
{
	m_wndNavBar.ShowPane((m_wndNavBar.GetStyle() & WS_VISIBLE) == 0, FALSE, FALSE);
	//ShowControlBar(&m_wndNavBar, (m_wndNavBar.GetStyle() & WS_VISIBLE) == 0, FALSE);
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_toolbar, 5);
}

void CMainFrame::OnUpdateViewCalculator(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_paneCalc.IsWindowVisible());
}

void CMainFrame::OnViewCalculator()
{
	m_paneCalc.Toggle();
	theApp.SaveToMacro(km_toolbar, 10);
}

void CMainFrame::OnUpdateViewBookmarks(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_paneBookmarks.IsWindowVisible());
}

void CMainFrame::OnViewBookmarks()
{
	m_paneBookmarks.Toggle();
	theApp.SaveToMacro(km_toolbar, 11);
}

void CMainFrame::OnUpdateViewFind(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_paneFind.IsWindowVisible());
}

void CMainFrame::OnViewFind()
{
	m_paneFind.Toggle();
	theApp.SaveToMacro(km_toolbar, 12);
}

void CMainFrame::OnUpdateViewProperties(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_paneProp.IsWindowVisible());
}

void CMainFrame::OnViewProperties()
{
	m_paneProp.Toggle();
	theApp.SaveToMacro(km_toolbar, 13);
}

void CMainFrame::OnUpdateViewExpl(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_paneExpl.IsWindowVisible());
}

void CMainFrame::OnViewExpl()
{
	m_paneExpl.Toggle();
	theApp.SaveToMacro(km_toolbar, 14);
}

void CMainFrame::OnUpdateViewRuler(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(theApp.ruler_);
}

void CMainFrame::OnViewRuler()
{
	theApp.ruler_ = !theApp.ruler_;
	theApp.UpdateAllViews();
	theApp.SaveToMacro(km_toolbar, 15);
}

void CMainFrame::OnUpdateViewHighlightCaret(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(theApp.hl_caret_);
}

void CMainFrame::OnViewHighlightCaret()
{
	CHexEditView *pview = GetView();
	if (pview != NULL)
	{
		theApp.hl_caret_ = !theApp.hl_caret_;
	}
	theApp.SaveToMacro(km_toolbar, 16);
}

void CMainFrame::OnUpdateViewHighlightMouse(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(theApp.hl_mouse_);
}

void CMainFrame::OnViewHighlightMouse()
{
	CHexEditView *pview = GetView();
	if (pview != NULL)
	{
		theApp.hl_mouse_ = !theApp.hl_mouse_;
	}
	theApp.SaveToMacro(km_toolbar, 17);
}

void CMainFrame::OnUpdateReadonly(CCmdUI *pCmdUI)
{
	// Get the active view
	CHexEditView *pview = GetView();
	if (pview != NULL)
	{
		pCmdUI->Enable(TRUE);
		if (pview->ReadOnly())
			pCmdUI->SetText("RO");
		else
			pCmdUI->SetText("RW");
	}
	else
		pCmdUI->Enable(FALSE);
}

void CMainFrame::OnUpdateOvr(CCmdUI *pCmdUI)
{
	// Get the active view
	CHexEditView *pview = GetView();
	if (pview != NULL && !pview->ReadOnly())
	{
		pCmdUI->Enable(TRUE);
		if (pview->OverType())
			pCmdUI->SetText("OVR");
		else
			pCmdUI->SetText("INS");
	}
	else
		pCmdUI->Enable(FALSE);
}

void CMainFrame::OnUpdateRec(CCmdUI *pCmdUI)
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	if (aa->recording_)
	{
		pCmdUI->Enable(TRUE);
//        pCmdUI->SetText("REC");
	}
	else
		pCmdUI->Enable(FALSE);
}

BOOL CMainFrame::UpdateBGSearchProgress()
{
	CHexEditView *pview = GetView();

	int ii;
	if (pview != NULL)
	{
		if ((ii = pview->GetDocument()->SearchOccurrences()) == -2)
		{
			int index = m_wndStatusBar.CommandToIndex(ID_INDICATOR_OCCURRENCES);
			//COLORREF text_col = RGB(0,0,0);
			//int hue, luminance, saturation;

			//get_hls(::GetSearchCol(), hue, luminance, saturation);
			//if (hue != -1)
			//{
			//    if (luminance > 50) luminance = 1; else luminance = 99;
			//    text_col  = get_rgb((hue+50)%100, luminance, 99);
			//}
			if (bg_progress_colour_ != pview->GetSearchCol())
			{
				bg_progress_colour_ = pview->GetSearchCol();
				m_wndStatusBar.EnablePaneProgressBar(index, 100, TRUE, bg_progress_colour_);
			}
			m_wndStatusBar.SetPaneProgress(index, pview->GetDocument()->SearchProgress(ii));
			return TRUE;
		}
	}
	return FALSE;
}

void CMainFrame::OnUpdateOccurrences(CCmdUI *pCmdUI)
{
	CMFCStatusBar *psb = (CMFCStatusBar *)pCmdUI->m_pOther;
	ASSERT_KINDOF(CMFCStatusBar, psb);
	ASSERT_VALID(psb);
	int index = psb->CommandToIndex(ID_INDICATOR_OCCURRENCES);

	CHexEditView *pview = GetView();
	int pane_width = 80;

	if (pview != NULL)
	{
		int ii;
		if ((ii = pview->GetDocument()->SearchOccurrences()) > -1)
		{
			psb->EnablePaneProgressBar(index, -1);  // turn off progress bar so we can show the text
			bg_progress_colour_ = -1;

			CString ss;
			ss.Format("%ld ", long(ii));
			AddCommas(ss);

			// Work out pane width
			CClientDC dc(psb);
			dc.SelectObject(psb->GetFont());
			pane_width = max(dc.GetTextExtent(ss, ss.GetLength()).cx + 20, 35);
			pCmdUI->SetText(ss);
			pCmdUI->Enable();
		}
		else if (ii == -2)
		{
			UpdateBGSearchProgress();
//            pCmdUI->SetText("...");
			pCmdUI->Enable();
		}
		else if (ii == -4)
		{
			psb->EnablePaneProgressBar(index, -1);  // turn off progress bar so we can show the text
			bg_progress_colour_ = -1;
			pCmdUI->SetText("OFF");
			pCmdUI->Enable();
		}
		else
			pCmdUI->Enable(FALSE);
	}
	else
	{
		psb->EnablePaneProgressBar(index, -1);      // turn off progress bar so we can show the text
		bg_progress_colour_ = -1;
		pCmdUI->SetText("");
		pCmdUI->Enable(FALSE);
	}

	if (pane_width != OccurrencesWidth)
	{
		psb->SetPaneWidth(index, pane_width);
		OccurrencesWidth = pane_width;
	}
}

BOOL CMainFrame::UpdateBGCompareProgress()
{
	CHexEditView *pview = GetView();

	if (pview != NULL)
	{
		int ii;
		if ((ii = pview->GetDocument()->CompareDifferences()) == -2)
		{
			int index = m_wndStatusBar.CommandToIndex(ID_INDICATOR_COMPARES);
			if (bg_compare_colour_ != pview->GetCompareCol())
			{
				bg_compare_colour_ = pview->GetCompareCol();
				m_wndStatusBar.EnablePaneProgressBar(index, 100, TRUE, bg_compare_colour_);
			}
			m_wndStatusBar.SetPaneProgress(index, pview->GetDocument()->CompareProgress());
			return TRUE;
		}
	}
	return FALSE;
}

void CMainFrame::OnUpdateCompares(CCmdUI *pCmdUI)
{
	CMFCStatusBar *psb = (CMFCStatusBar *)pCmdUI->m_pOther;
	ASSERT_KINDOF(CMFCStatusBar, psb);
	ASSERT_VALID(psb);
	int index = psb->CommandToIndex(ID_INDICATOR_COMPARES);
	ASSERT(index != -1);
	if (index == -1)
	{
		ASSERT(0); // should not happen
		return;
	}

	CHexEditView *pview = GetView();
	int pane_width = 10;

	if (pview != NULL)
	{
		int ii;
		if ((ii = pview->GetDocument()->CompareDifferences()) > -1)
		{
			psb->EnablePaneProgressBar(index, -1);  // turn off progress bar so we can show the text
			bg_compare_colour_ = -1;

			CString ss;
			ss.Format("%ld ", long(ii));
			AddCommas(ss);

			// Work out pane width
			CClientDC dc(psb);
			dc.SelectObject(psb->GetFont());
			pane_width = max(dc.GetTextExtent(ss, ss.GetLength()).cx + 2, 35);
			pCmdUI->SetText(ss);
			pCmdUI->Enable();
		}
		else if (ii == -2)   // compare in progress
		{
			UpdateBGCompareProgress();
			pane_width = 64;
			pCmdUI->Enable();
		}
		else if (ii == -4)  // no comparison
		{
			psb->EnablePaneProgressBar(index, -1);  // turn off progress bar so we can show the text
			bg_compare_colour_ = -1;
			pCmdUI->SetText("");
			pCmdUI->Enable();
		}
		else
			pCmdUI->Enable(FALSE);
	}
	else
	{
		psb->EnablePaneProgressBar(index, -1);      // turn off progress bar so we can show the text
		bg_compare_colour_ = -1;
		pCmdUI->SetText("");
		pCmdUI->Enable(FALSE);
	}

	if (pane_width != ComparesWidth)
	{
		psb->SetPaneWidth(index, pane_width);
		ComparesWidth = pane_width;
	}
}

void CMainFrame::OnUpdateValues(CCmdUI *pCmdUI)
{
	unsigned char cc;

	CHexEditView *pview = GetView();
	if (pview != NULL && pview->GetDocument()->GetData(&cc, 1, pview->GetPos()) == 1)
	{
		CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

		// Update pane with values of current byte
		char binbuf[9];             // To display binary value
		for (int ii = 0; ii < 8; ++ii)
			binbuf[ii] = (cc & (0x80>>ii)) ? '1' : '0';
		binbuf[8] = '\0';
		CString ss;

		if (aa->hex_ucase_)
		{
			if (!pview->EbcdicMode() && cc < 0x20)      // Control char?
				ss.Format("%02X, %d, %03o, %8s, '^%c' ",cc,cc,cc,binbuf,cc+0x40);
			else if (!pview->EbcdicMode())
				ss.Format("%02X, %d, %03o, %8s, '%c' ",cc,cc,cc,binbuf,cc);
			else if (e2a_tab[cc] != '\0')
				ss.Format("%02X, %d, %03o, %8s, '%c' ",cc,cc,cc,binbuf, e2a_tab[cc]);
			else
				ss.Format("%02X, %d, %03o, %8s, none ",cc,cc,cc,binbuf);
		}
		else
		{
			if (!pview->EbcdicMode() && cc < 0x20)      // Control char?
				ss.Format("%02x, %d, %03o, %8s, '^%c' ",cc,cc,cc,binbuf,cc+0x40);
			else if (!pview->EbcdicMode())
				ss.Format("%02x, %d, %03o, %8s, '%c' ",cc,cc,cc,binbuf,cc);
			else if (e2a_tab[cc] != '\0')
				ss.Format("%02x, %d, %03o, %8s, '%c' ",cc,cc,cc,binbuf, e2a_tab[cc]);
			else
				ss.Format("%02x, %d, %03o, %8s, none ",cc,cc,cc,binbuf);
		}

		// Get status bar control
		CMFCStatusBar *psb = (CMFCStatusBar *)pCmdUI->m_pOther;
		ASSERT_KINDOF(CMFCStatusBar, psb);
		ASSERT_VALID(psb);
		int idx = psb->CommandToIndex(ID_INDICATOR_VALUES);   // index of the byte values pane

		// Set pane width
		CClientDC dc(psb);
		dc.SelectObject(psb->GetFont());
		int text_width = dc.GetTextExtent(ss, ss.GetLength()).cx + 4;
		if (abs(text_width - ValuesWidth) > 4)
		{
			psb->SetPaneWidth(idx, text_width);
			ValuesWidth = text_width;
		}
		pCmdUI->SetText(ss);
		pCmdUI->Enable();
	}
	else
	{
		pCmdUI->SetText("");
		pCmdUI->Enable(FALSE);
	}

}

void CMainFrame::OnUpdateAddrHex(CCmdUI *pCmdUI)
{
	// Get the active view
	CHexEditView *pview = GetView();
	if (pview != NULL)
	{
		// Update pane with Current offset from marked position (hex)
		char buf[32];
		CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
		__int64 mark = __int64(pview->GetMarkOffset());
		size_t len;

		if (aa->hex_ucase_)
			len = sprintf(buf, "%I64X", mark);
		else
			len = sprintf(buf, "%I64x", mark);
		if (mark > 9)
			strcpy(buf+len, "h");

		CString ss(buf);
		AddSpaces(ss);
		ss += " ";

#if 0 // Colours now handled by CMFCStatusBar
		// We don't want text drawn since it will write over our own custom text
		pCmdUI->SetText("");
		pCmdUI->Enable();

		CStatBar *psb = (CStatBar *)pCmdUI->m_pOther;
		ASSERT_KINDOF(CStatBar, psb);
		ASSERT_VALID(psb);

		CRect rct;
		psb->GetItemRect(pCmdUI->m_nIndex, &rct);
		rct.DeflateRect(2,2);

		CClientDC dc(psb);

		dc.FillSolidRect(rct, GetSysColor(COLOR_3DFACE));
		dc.SetBkMode(TRANSPARENT);
		dc.SetTextColor(::GetHexAddrCol());
		dc.DrawText(ss, rct, DT_SINGLELINE | DT_CENTER | DT_VCENTER |
							 DT_NOPREFIX | DT_END_ELLIPSIS);
#else
		// Get status bar control
		CMFCStatusBar *psb = (CMFCStatusBar *)pCmdUI->m_pOther;
		ASSERT_KINDOF(CMFCStatusBar, psb);
		ASSERT_VALID(psb);
		int idx = psb->CommandToIndex(ID_INDICATOR_HEX_ADDR);   // index of the hex addr pane

		// Set pane colour
		psb->SetPaneTextColor(idx, ::GetHexAddrCol());

		// Set pane width
		CClientDC dc(psb);
		dc.SelectObject(psb->GetFont());
		int text_width = max(dc.GetTextExtent(ss, ss.GetLength()).cx + 2, 35);
		if (abs(text_width - AddrHexWidth) > 4)
		{
			psb->SetPaneWidth(idx, text_width);
			AddrHexWidth = text_width;
		}

		pCmdUI->SetText(ss);
		pCmdUI->Enable();
#endif
	}
	else
		pCmdUI->Enable(FALSE);
}

void CMainFrame::OnUpdateAddrDec(CCmdUI *pCmdUI)
{
	// Get the active view
	CHexEditView *pview = GetView();
	if (pview != NULL)
	{
		// Update pane with Current offset from marked position (decimal)
		char buf[32];
		sprintf(buf, "%I64d", __int64(pview->GetMarkOffset()));
		CString ss(buf);
		AddCommas(ss);
		if (buf[0] == '-')
			ss = " " + ss;    // Add a space before minus sign for visibility
		ss += " ";

#if 0 // Colours now handled by CMFCStatusBar
		// We don't want text drawn since it will write over our own custom text
		pCmdUI->SetText("");
		pCmdUI->Enable();

		CStatusBar *psb = (CStatusBar *)pCmdUI->m_pOther;
		ASSERT_KINDOF(CStatusBar, psb);
		ASSERT_VALID(psb);

		CRect rct;
		psb->GetItemRect(pCmdUI->m_nIndex, &rct);
		rct.DeflateRect(2,2);

		CClientDC dc(psb);

		dc.FillSolidRect(rct, GetSysColor(COLOR_3DFACE));
		dc.SetBkMode(TRANSPARENT);
		dc.SetTextColor(::GetDecAddrCol());
		dc.DrawText(ss, rct, DT_SINGLELINE | DT_CENTER | DT_VCENTER |
							 DT_NOPREFIX | DT_END_ELLIPSIS);
#else
		CMFCStatusBar *psb = (CMFCStatusBar *)pCmdUI->m_pOther;
		ASSERT_KINDOF(CMFCStatusBar, psb);
		ASSERT_VALID(psb);
		int idx = psb->CommandToIndex(ID_INDICATOR_DEC_ADDR);

		// Set pane colour
		psb->SetPaneTextColor(idx, ::GetDecAddrCol());

		// Set pane width
		CClientDC dc(psb);
		dc.SelectObject(psb->GetFont());
		int text_width = max(dc.GetTextExtent(ss, ss.GetLength()).cx + 2, 30);
		if (abs(text_width - AddrDecWidth) > 4)
		{
			psb->SetPaneWidth(idx, text_width);
			AddrDecWidth = text_width;
		}

		pCmdUI->SetText(ss);
		pCmdUI->Enable();
#endif
	}
	else
		pCmdUI->Enable(FALSE);
}

void CMainFrame::OnUpdateFileLength(CCmdUI *pCmdUI)
{
	// Get the active view
	CHexEditView *pview = GetView();
	if (pview != NULL)
	{
		// Get ptr to status bar and index of file length pane
		CMFCStatusBar *psb = (CMFCStatusBar *)pCmdUI->m_pOther;
		ASSERT_KINDOF(CMFCStatusBar, psb);
		ASSERT_VALID(psb);
		int idx = psb->CommandToIndex(ID_INDICATOR_FILE_LENGTH);

		CString ss;                     // text to put in the pane
		char buf[32];                   // used with sprintf (CString::Format can't handle __int64)

		int seclen = pview->GetDocument()->GetSectorSize();
		ASSERT(seclen > 0 || !pview->GetDocument()->IsDevice());  // no sectors (sec len is zero) if its an unsaved file (but not device)
		if (pview->GetDocument()->IsDevice() && seclen > 0)
		{
			// Show sector + total sectors
			__int64 end = pview->GetDocument()->length();
			__int64 now = pview->GetPos();
			CString tmp;

			if (pview->DecAddresses())
			{
				sprintf(buf, "%I64d", __int64(now/seclen));
				ss = buf;
				AddCommas(ss);
				sprintf(buf, "%I64d", __int64(end/seclen));
				tmp = buf;
				AddCommas(tmp);

				psb->SetPaneTextColor(idx, ::GetDecAddrCol());
				psb->SetTipText(psb->CommandToIndex(ID_INDICATOR_FILE_LENGTH), "Current/total sectors (decimal)");
			}
			else
			{
				if (theApp.hex_ucase_)
					sprintf(buf, "%I64X", __int64(now/seclen));
				else
					sprintf(buf, "%I64x", __int64(now/seclen));
				ss = buf;
				AddSpaces(ss);

				if (theApp.hex_ucase_)
					sprintf(buf, "%I64X", __int64(end/seclen));
				else
					sprintf(buf, "%I64x", __int64(end/seclen));
				tmp = buf;
				AddSpaces(tmp);

				psb->SetPaneTextColor(idx, ::GetHexAddrCol());
				psb->SetTipText(psb->CommandToIndex(ID_INDICATOR_FILE_LENGTH), "Current/total sectors (hex)");
			}
			ss = "Sector: " + ss + "/" + tmp + " ";
		}
		else
		{
			// Show file length + difference
			FILE_ADDRESS orig_len = 0, curr_len, diff;
			curr_len = pview->GetDocument()->length();
			if (pview->GetDocument()->pfile1_ != NULL)
				orig_len = pview->GetDocument()->pfile1_->GetLength();
			else
				orig_len = curr_len;       // Don't display diff if there is no disk file
			diff = curr_len - orig_len;

			// Update pane with original file length and difference in length
			if (pview->DecAddresses())
			{
				sprintf(buf, "%I64d ", __int64(curr_len));
				ss = buf;
				AddCommas(ss);

				// Set pane colour for decimal addresses
				psb->SetPaneTextColor(idx, ::GetDecAddrCol());
			}
			else
			{
				// hex addresses
				size_t len;

				if (theApp.hex_ucase_)
					len = sprintf(buf, "%I64X", __int64(curr_len));
				else
					len = sprintf(buf, "%I64x", __int64(curr_len));

				ss = buf;
				AddSpaces(ss);
				if (curr_len > 9)
					ss += "h";

				// Set pane colour for hex addresses
				psb->SetPaneTextColor(idx, ::GetHexAddrCol());
			}
			if (diff != 0)
			{
				if (diff < -9)
					ss += " [-*]";
				else if (diff > 9)
					ss += " [+*]";
				else
				{
					sprintf(buf, " [%+I64d]", __int64(diff));
					ss += buf;
				}
				psb->SetTipText(psb->CommandToIndex(ID_INDICATOR_FILE_LENGTH), "Length [+ bytes inserted]");
			}
			else
				psb->SetTipText(psb->CommandToIndex(ID_INDICATOR_FILE_LENGTH), "File length");
			ss = "Length: " + ss + " ";
		}

		// Set pane width
		CClientDC dc(psb);
		dc.SelectObject(psb->GetFont());
		int text_width = max(dc.GetTextExtent(ss, ss.GetLength()).cx, 40);
		if (text_width != FileLengthWidth)
		{
			psb->SetPaneWidth(idx, text_width);
			FileLengthWidth = text_width;
		}

		pCmdUI->SetText(ss);
		pCmdUI->Enable();
	}
	else
		pCmdUI->Enable(FALSE);
}

void CMainFrame::OnUpdateBigEndian(CCmdUI *pCmdUI)
{
	// Get the active view
	CHexEditView *pview = GetView();
	if (pview == NULL)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	// Get ptr to status bar and index of file length pane
	CMFCStatusBar *psb = (CMFCStatusBar *)pCmdUI->m_pOther;
	ASSERT_KINDOF(CMFCStatusBar, psb);
	ASSERT_VALID(psb);
	int idx = psb->CommandToIndex(ID_INDICATOR_BIG_ENDIAN);

	if (pview->BigEndian())
	{
		// Set pane colour
		psb->SetPaneTextColor(idx, RGB(224,0,0));
		pCmdUI->SetText("BE ");
	}
	else
	{
		// Set pane colour
		psb->SetPaneTextColor(idx, ::GetSysColor(COLOR_BTNTEXT));
		pCmdUI->SetText("LE ");
	}
	pCmdUI->Enable();
}

void CMainFrame::StatusBarText(const char *mess /*=NULL*/)
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

	if (mess == NULL)
		m_wndStatusBar.SetPaneText(0, last_mess_);      // Update now
	else if (aa->refresh_off_)
		last_mess_ = mess;                              // Save message for later update
	else
		m_wndStatusBar.SetPaneText(0, mess);            // Display immediately
}

// Saves a decimal address or expression for later retrieval
void CMainFrame::SetDecAddress(const char *ss)
{
	current_dec_address_ = ss;
	current_hex_address_.Empty();
	CString dummy1, dummy2;
	(void)GetDecAddress(dummy1, dummy2);
}

// Saves an expression (involving hex ints) for later retrieval
void CMainFrame::SetHexAddress(const char *ss)
{
	current_hex_address_ = ss;
	current_dec_address_.Empty();
	CString dummy1, dummy2;
	(void)GetHexAddress(dummy1, dummy2);
}

FILE_ADDRESS CMainFrame::GetDecAddress(CString &ss, CString &err_str)
{
	ss = current_dec_address_;

	// Work out address from dec address string
	int ac;
	CJumpExpr::value_t vv;
	if (current_dec_address_.IsEmpty())
	{
		err_str = "Expression is empty";
		return -1;
	}

	vv = expr_.evaluate(current_dec_address_, 0 /*unused*/, ac /*unused*/, 10 /*dec ints*/);

	if (vv.typ == CJumpExpr::TYPE_INT)
		return current_address_ = vv.int64;
	else if (vv.typ == CJumpExpr::TYPE_NONE)
	{
		err_str = expr_.get_error_message();
		return -1;
	}
	else
	{
		err_str = "Expression does not return an integer";
		return -1;
	}
}

FILE_ADDRESS CMainFrame::GetHexAddress(CString &ss, CString &err_str)
{
	ss = current_hex_address_;

	// Work out address from hex address string
	int ac;
	CJumpExpr::value_t vv;
	if (current_hex_address_.IsEmpty())
	{
		err_str = "Expression is empty";
		return -1;
	}

	vv = expr_.evaluate(current_hex_address_, 0 /*unused*/, ac /*unused*/, 16 /*hex int*/);

	if (vv.typ == CJumpExpr::TYPE_INT)
		return current_address_ = vv.int64;
	else if (vv.typ == CJumpExpr::TYPE_NONE)
	{
		err_str = expr_.get_error_message();
		return -1;
	}
	else
	{
		err_str = "Expression does not return an integer";
		return -1;
	}
}

// Add an address to the list of hex address combo
void CMainFrame::AddHexHistory(const CString &ss)
{
	if (ss.IsEmpty()) return;

	SetHexAddress(ss);

	// If recording macro indicate jump & save address jumped to
//    ((CHexEditApp *)AfxGetApp())->SaveToMacro(km_address_tool, current_address_);

	if (hex_hist_.size() > 0 && hex_hist_.back() == ss)
		return;

	// Remove any duplicate entry from the list
	for (std::vector<CString>::iterator ps = hex_hist_.begin();
		 ps != hex_hist_.end(); ++ps)
	{
		if (*ps == ss)
		{
			hex_hist_.erase(ps);
			break;
		}
	}

	hex_hist_.push_back(ss);
	hex_hist_changed_ = clock();
}

// Add an address to the list of dec address combo
void CMainFrame::AddDecHistory(const CString &ss)
{
	if (ss.IsEmpty()) return;

	SetDecAddress(ss);

	// If recording macro indicate jump & save address jumped to
//    ((CHexEditApp *)AfxGetApp())->SaveToMacro(km_address_tool, current_address_);

	if (dec_hist_.size() > 0 && dec_hist_.back() == ss)
		return;

	// Remove any duplicate entry from the list
	for (std::vector<CString>::iterator ps = dec_hist_.begin();
		 ps != dec_hist_.end(); ++ps)
	{
		if (*ps == ss)
		{
			dec_hist_.erase(ps);
			break;
		}
	}

	dec_hist_.push_back(ss);
	dec_hist_changed_ = clock();
}

// Retrieve hex/dec jump histories from registry
void CMainFrame::LoadJumpHistory(CHexEditApp *aa)
{
	::LoadHist(hex_hist_, "HexJump", theApp.max_hex_jump_hist_);
	::LoadHist(dec_hist_, "DecJump", theApp.max_dec_jump_hist_);

	hex_hist_changed_ = dec_hist_changed_ = clock();
}

// Save hex/dec histories to registry
void CMainFrame::SaveJumpHistory(CHexEditApp *aa)
{
	::SaveHist(hex_hist_, "HexJump", theApp.max_hex_jump_hist_);
	::SaveHist(dec_hist_, "DecJump", theApp.max_dec_jump_hist_);
}

// Add a search string to the list box of the search combo
void CMainFrame::AddSearchHistory(const CString &ss)
{
//    current_search_string_ = ss;

	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_find_text, ss);  // Keep track of bytes entered in macro

	// Don't add to the list if already at the top
	if (search_hist_.size() > 0 && ss == search_hist_.back())
		return;

	// Remove any duplicate entry from the list
	for (std::vector<CString>::iterator ps = search_hist_.begin();
		 ps != search_hist_.end(); ++ps)
	{
		if (ss == *ps)
		{
			search_hist_.erase(ps);
			break;
		}
		else if (ss.GetLength() > 0 && 
				 ss[0] != CSearchEditControl::sflag_char && 
				 ss.CompareNoCase(*ps) == 0)
		{
			// Doing hex search or case-insensitive search and strings
			// are the same (except for case) - so remove old one
			search_hist_.erase(ps);
			break;
		}
	}
	search_hist_.push_back(ss);
}

// Add a replace string to the history list
void CMainFrame::AddReplaceHistory(const CString &ss)
{
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_replace_text, ss);  // Keep track of bytes entered in macro

	// Don't add to the list if already at the top
	if (replace_hist_.size() > 0 && ss == replace_hist_.back())
		return;

	// Remove any duplicate entry from the list
	for (std::vector<CString>::iterator ps = replace_hist_.begin();
		 ps != replace_hist_.end(); ++ps)
	{
		if (ss == *ps)
		{
			replace_hist_.erase(ps);
			break;
		}
	}
	replace_hist_.push_back(ss);
}

// xxx changes required here (3.4?):
// 1. Make max_search_hist_ etc options the user can change
// 1a. Add individual "Clear Now" buttons for all lists
// 2. On load - load all entries (ignore max_search_hist_ etc)
// 3. On Save - set size of list to max_search_hist_ etc, then save all entries in the list
// 4. Get rid of clear_on_exit_ etc options as this can now be done by seeting a size of zero.
// 5. Clear reg entries past last used (since user can change hist sizes and we don't weant to leave unused one behind)

// Retrieve search/replace histories from registry
void CMainFrame::LoadSearchHistory(CHexEditApp *aa)
{
	::LoadHist(search_hist_,  "Search",  theApp.max_search_hist_);
	::LoadHist(replace_hist_, "Replace", theApp.max_replace_hist_);
}

// Save search/replace histories to registry
void CMainFrame::SaveSearchHistory(CHexEditApp *aa)
{
	::SaveHist(search_hist_,  "Search",  theApp.max_search_hist_);
	::SaveHist(replace_hist_, "Replace", theApp.max_replace_hist_);
}

void CMainFrame::OnFindNext()
{
	AddSearchHistory(current_search_string_);

	if (DoFind() && theApp.recording_)
		theApp.SaveToMacro(km_find_next, m_wndFind.GetOptions());
}

void CMainFrame::OnSearchForw()
{
	m_wndFind.SetDirn(CFindSheet::DIRN_DOWN);
	CFindSheet::scope_t scope = m_wndFind.GetScope();
	if (scope != CFindSheet::SCOPE_EOF && scope != CFindSheet::SCOPE_ALL)
		m_wndFind.SetScope(CFindSheet::SCOPE_EOF);

	AddSearchHistory(current_search_string_);

	if (DoFind() && theApp.recording_)
		theApp.SaveToMacro(km_find_forw, m_wndFind.GetOptions());
}

void CMainFrame::OnSearchBack()
{
	m_wndFind.SetDirn(CFindSheet::DIRN_UP);
	CFindSheet::scope_t scope = m_wndFind.GetScope();
	if (scope != CFindSheet::SCOPE_EOF && scope != CFindSheet::SCOPE_ALL)
		m_wndFind.SetScope(CFindSheet::SCOPE_EOF);

	AddSearchHistory(current_search_string_);

	if (DoFind() && theApp.recording_)
		theApp.SaveToMacro(km_find_back, m_wndFind.GetOptions());
}

void CMainFrame::OnUpdateSearch(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(GetView() != NULL &&
				   (current_search_string_.GetLength() > 1 &&
					(current_search_string_[0] == CSearchEditControl::sflag_char ||
					 current_search_string_[0] == CSearchEditControl::iflag_char   ) ||
				   current_search_string_.FindOneOf("0123456789ABCDEFabcdef") > -1) );
}

void CMainFrame::OnSearchSel()
{
	CHexEditView *pview = GetView();
	if (pview == NULL)
	{
		AfxMessageBox("There is no file open to search.");
		theApp.mac_error_ = 10;
		return;
	}

	// Get selection
	FILE_ADDRESS start, end;
	pview->GetSelAddr(start, end);
	ASSERT(start >= 0 && start <= end && end <= pview->GetDocument()->length());
	if (start == end)
	{
		// Nothing selected, presumably in macro playback
		ASSERT(theApp.playing_);
		AfxMessageBox("Nothing selected to search for!");
		theApp.mac_error_ = 10;
		return;
	}

	// Load the current selection into buf.  Allow for trailing '\0', but there may 
	// be less bytes if non-EBCDIC chars are found (EBCDIC mode) or chars < 13 (ASCII mode)
	unsigned char *buf = new unsigned char[size_t(end - start) + 1];
	VERIFY(pview->GetDocument()->GetData(buf, size_t(end - start), start) == end - start);

	boolean do_hex = true;
	if (pview->CharMode() && pview->EbcdicMode())
	{
		unsigned char *pp;              // Pointer into buf
		buf[end-start] = '\0';
		// Check if all chars are valid EBCDIC and set search text if so
		for (pp = buf; pp < buf+end-start; ++pp)
			if (e2a_tab[*pp] == '\0')
				break;
			else
				*pp = e2a_tab[*pp];     // Convert EBCDIC to ASCII

		if (pp == buf+end-start)
			do_hex = false;             // No invalid EBCDIC chars found
		ASSERT(buf[end-start] == '\0'); // Check for buffer overrun

		m_wndFind.SetCharSet(CFindSheet::RB_CHARSET_EBCDIC);
		m_wndFind.NewText((char *)buf);
	}
	else if (pview->CharMode())
	{
		buf[end-start] = '\0';
		// Check if all chars are normal ASCII text and set search text
		unsigned char *pp;
		for (pp = buf; pp < buf+end-start; ++pp)
			if (*pp <= '\r')
				break;
		if (pp == buf+end-start)
			do_hex = false;             // No invalid chars found so do text search
		ASSERT(buf[end-start] == '\0'); // Check for buffer overrun

		m_wndFind.SetCharSet(CFindSheet::RB_CHARSET_ASCII);
		m_wndFind.NewText((char *)buf);
	}
	delete[] buf;

	if (do_hex)
	{
		// Change buf so that there's room for 2 hex digits + space per byte
		char *tbuf = new char[size_t(end - start)*3];
		unsigned char cc;               // One character from document
		FILE_ADDRESS address;           // Current byte address in document
		char *pp;                       // Current position in search string being built

		const char *hex;
		CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
		if (theApp.hex_ucase_)
			hex = "0123456789ABCDEF?";
		else
			hex = "0123456789abcdef?";

		ASSERT(start < end);            // Make sure we don't put '\0' at tbuf[-1]
		for (address = start, pp = tbuf; address < end; ++address)
		{
			pview->GetDocument()->GetData(&cc, 1, address);
			*pp++ = hex[(cc>>4)&0xF];
			*pp++ = hex[cc&0xF];
			*pp++ = ' ';
		}
		*(pp-1) = '\0';                 // Terminate string (overwrite last space)
		ASSERT(pp == tbuf + (end-start)*3);
		ASSERT(address == end);

		m_wndFind.NewHex((char *)tbuf);

		delete[] tbuf;
	}

	// Must be a forward search and fix scope
	m_wndFind.SetDirn(CFindSheet::DIRN_DOWN);
	CFindSheet::scope_t scope = m_wndFind.GetScope();
	if (scope != CFindSheet::SCOPE_EOF && scope != CFindSheet::SCOPE_ALL)
		m_wndFind.SetScope(CFindSheet::SCOPE_EOF);

	AddSearchHistory(current_search_string_);

	if (DoFind() && theApp.recording_)
		theApp.SaveToMacro(km_find_sel, m_wndFind.GetOptions());

	pview->SetFocus();                  // This is nec as focus gets set to Find dialog
}

void CMainFrame::OnUpdateSearchSel(CCmdUI* pCmdUI)
{
	CHexEditView *pview = GetView();

	if (pview == NULL)
	{
		pCmdUI->Enable(FALSE);
	}
	else
	{
		FILE_ADDRESS start, end;            // Current selection
		pview->GetSelAddr(start, end);
		pCmdUI->Enable(start < end);
	}
}

// Gets the previous document in the apps list of docs.  If the current doc is at the start
// it returns the last doc in the list.  If there is only one entry it returns it.
// Note that ther must be at least one entry (the current doc).
CHexEditDoc *CMainFrame::GetPrevDoc(CHexEditDoc *pdoc)
{
	CHexEditDoc *retval, *pdoc_temp;
	POSITION posn = theApp.m_pDocTemplate->GetFirstDocPosition();
	ASSERT(posn != NULL);

	// First find pdoc in the list
	retval = NULL;
	while ((pdoc_temp = dynamic_cast<CHexEditDoc *>(theApp.m_pDocTemplate->GetNextDoc(posn))) != pdoc)
		retval = pdoc_temp;

	if (retval == NULL)
	{
		retval = pdoc_temp;
		// Current doc is at top of the list so get the last in the list
		while (posn != NULL)
			retval = dynamic_cast<CHexEditDoc *>(theApp.m_pDocTemplate->GetNextDoc(posn));
	}

	return retval;
}

BOOL CMainFrame::DoFind()
{
	// Get current view thence document to search
	CHexEditView *pview = GetView();

	if (pview == NULL)
	{
		AfxMessageBox("There is no file open to search.");
		theApp.mac_error_ = 10;
		return FALSE;
	}

	CHexEditDoc *pdoc = pview->GetDocument();

	// Get current find options
	const unsigned char *ss;
	const unsigned char *mask = NULL;
	size_t length;

	m_wndFind.GetSearch(&ss, &mask, &length);

	CFindSheet::dirn_t dirn = m_wndFind.GetDirn();
	CFindSheet::scope_t scope = m_wndFind.GetScope();
	BOOL wholeword = m_wndFind.GetWholeWord();
	BOOL icase = !m_wndFind.GetMatchCase();
	int tt = int(m_wndFind.GetCharSet()) + 1;       // 1 = ASCII, 2 = Unicode, 3 = EBCDIC
	ASSERT(tt == 1 || tt == 2 || tt == 3);
	int alignment = m_wndFind.GetAlignment();
	int offset    = m_wndFind.GetOffset();
	bool align_rel = m_wndFind.AlignRel();
	FILE_ADDRESS base_addr;
	if (align_rel)
		base_addr = pview->GetSearchBase();
	else
		base_addr = 0;

	FILE_ADDRESS start, end;            // Range of bytes in the current file to search
	FILE_ADDRESS found_addr;            // The address where the search text was found (or -1, -2)

	if (dirn == CFindSheet::DIRN_UP)
	{
		switch (scope)
		{
#if 0
		case CFindSheet::SCOPE_SEL:
			pview->GetSelAddr(start, end);
			break;
#endif
		case CFindSheet::SCOPE_TOMARK:
			pview->GetSelAddr(start, end);
			if (start < end)
				end--;
			start = pview->GetMark();
			break;
		case CFindSheet::SCOPE_FILE:
			start = 0;
			end = pdoc->length();
			break;
		case CFindSheet::SCOPE_EOF:
		case CFindSheet::SCOPE_ALL:     // We will search other files later too if nec.
			pview->GetSelAddr(start, end);
			if (start < end)
				end--;
			start = 0;;
			break;
		default:
			ASSERT(0);
		}
		// Do the search
		if ((found_addr = search_back(pdoc, start, end, ss, mask, length, icase, tt, wholeword, alignment, offset, align_rel, base_addr)) == -2)
		{
			// User abort or some error (message already displayed) - just restore original display pos
			pview->show_pos();
			theApp.mac_error_ = 10;
			return FALSE;
		}
		else if (found_addr == -1)
		{
			if (scope == CFindSheet::SCOPE_ALL)
			{
				// Search through all other open files starting at the one before the current
				CHexEditDoc *pdoc2 = pdoc;

				// Now loop backwards through all the other docs starting at the one before the current
				while ((pdoc2 = GetPrevDoc(pdoc2)) != pdoc)
				{
					CHexEditView *pv2 = pdoc2->GetBestView();
					ASSERT(pv2 != NULL);
					if (align_rel)
						base_addr = pv2->GetSearchBase();
					else
						base_addr = 0;

					// Search this file
					if ((found_addr = search_back(pdoc2, 0, pdoc2->length(),
												  ss, mask, length, icase, 
												  tt, wholeword, alignment, offset, align_rel, base_addr)) == -2)
					{
						// Restore original display pos
						pview->show_pos();
						theApp.mac_error_ = 10;
						return FALSE;
					}
					else if (found_addr != -1)
					{
						// Found in this file
						CChildFrame *pf2 = pv2->GetFrame();
						if (pf2->IsIconic())
							pf2->MDIRestore();
						MDIActivate(pf2);
						pv2->MoveWithDesc("Search Text Found ", found_addr, found_addr + length);  // space at end means significant nav pt
#ifdef SYS_SOUNDS
						CSystemSound::Play("Search Text Found");
#endif
						return TRUE;
					}
					// else (not found) try the next open file
				}

				// Now go back and search the start of the active document
				if ((found_addr = search_back(pdoc,
											  end - (length-1) < 0 ? 0 : end - (length-1),
											  pdoc->length(),
											  ss, mask, length, icase,
											  tt, wholeword, alignment, offset, align_rel, base_addr)) == -2)
				{
					// Restore original display pos
					pview->show_pos();
					theApp.mac_error_ = 10;
					return FALSE;
				}
				else if (found_addr == -1)
				{
					// Display not found message
					pview->show_pos();                         // Restore display of orig address
					AfxMessageBox(not_found_mess(FALSE, icase, tt, wholeword, alignment));
				}
				else
				{
					pview->MoveWithDesc("Search Text Found ", found_addr, found_addr + length);  // space at end means significant nav pt
#ifdef SYS_SOUNDS
					CSystemSound::Play("Search Text Found");
#endif
					return TRUE;
				}
			}
			else if (scope == CFindSheet::SCOPE_EOF && end < pdoc->length())
			{
				CString mess = CString("The start of file was reached.\r\n") +
							   not_found_mess(FALSE, icase, tt, wholeword, alignment) +
							   CString("\r\nDo you want to continue from the end of file?");

				SetAddress(start);            // start == 0
				theApp.OnIdle(0);             // Force display of updated address

				// Give the user the option to search the top bit of the file that has not been searched
				if (AfxMessageBox(mess, MB_YESNO) == IDYES)
				{
					if ((found_addr = search_back(pdoc,
												  end - (length-1) < 0 ? 0 : end - (length-1),
												  pdoc->length(),
												  ss, mask, length, icase,
												  tt, wholeword, alignment, offset, align_rel, base_addr)) == -2)
					{
						// Restore original display pos
						pview->show_pos();
						theApp.mac_error_ = 10;
						return FALSE;
					}
					else if (found_addr == -1)
					{
						// Display not found message
						pview->show_pos();                         // Restore display of orig address
						AfxMessageBox(not_found_mess(FALSE, icase, tt, wholeword, alignment));
					}
					else
					{
						pview->MoveToAddress(found_addr, found_addr + length); // xxx should this add to nav pts??
#ifdef SYS_SOUNDS
						CSystemSound::Play("Search Text Found");
#endif
						return TRUE;
					}
				}
				else
				{
					pview->show_pos();                         // Restore display of orig address
				}
			}
			else
			{
				// Display not found message
				SetAddress(start);
				theApp.OnIdle(0);             // Force display of updated address
				AfxMessageBox(not_found_mess(FALSE, icase, tt, wholeword, alignment));
				pview->show_pos();                         // Restore display of orig address
			}
		}
		else
		{
			// Found
			pview->MoveWithDesc("Search Text Found ", found_addr, found_addr + length);   // space at end means significant nav pt
			if (scope == CFindSheet::SCOPE_FILE)
			{
				// Change to SCOPE_EOF so that next search does not find the same thing
				m_wndFind.SetScope(CFindSheet::SCOPE_EOF);
			}
#ifdef SYS_SOUNDS
			CSystemSound::Play("Search Text Found");
#endif
			return TRUE;
		}
	}
	else
	{
		// Downwards search
		switch (scope)
		{
#if 0
		case CFindSheet::SCOPE_SEL:
			pview->GetSelAddr(start, end);
			break;
#endif
		case CFindSheet::SCOPE_TOMARK:
			pview->GetSelAddr(start, end);
			if (start < end)
				++start;                // Change to start search at byte after start of selection
			end = pview->GetMark();
			break;
		case CFindSheet::SCOPE_FILE:
			start = 0;
			end = pdoc->length();
			break;
		case CFindSheet::SCOPE_EOF:
		case CFindSheet::SCOPE_ALL:
			pview->GetSelAddr(start, end);
			if (start < end)
				++start;                // Change to start search at byte after start of selection
			end = pdoc->length();
			break;
		default:
			ASSERT(0);
		}

		// Do the search
		if ((found_addr = search_forw(pdoc, start, end, ss, mask, length, icase, tt, wholeword, alignment, offset, align_rel, base_addr)) == -2)
		{
			// User abort or some error (message already displayed) - just restore original display pos
			pview->show_pos();
			theApp.mac_error_ = 10;
			return FALSE;
		}
		else if (found_addr == -1)
		{
			if (scope == CFindSheet::SCOPE_ALL)
			{
				// Search through all other open files starting at the one after current
				// Note: GetFirst/NextDoc returns docs in the order they are displayed in the Window menu
				CHexEditDoc *pdoc2;
				POSITION posn = theApp.m_pDocTemplate->GetFirstDocPosition();
				ASSERT(posn != NULL);

				// First find the current doc in the list
				while ((pdoc2 = dynamic_cast<CHexEditDoc *>(theApp.m_pDocTemplate->GetNextDoc(posn))) != pdoc)
					ASSERT(posn != NULL);   // We must be able to find the current doc?

				// Now loop through all the other docs starting at the one after the current
				for (;;)
				{
					// Get the next doc (looping back to the top of the list if nec.)
					if (posn == NULL)
						posn = theApp.m_pDocTemplate->GetFirstDocPosition();
					pdoc2 = dynamic_cast<CHexEditDoc *>(theApp.m_pDocTemplate->GetNextDoc(posn));
					if (pdoc2 == pdoc)
						break;              // We have gone through the whole list of docs

					CHexEditView *pv2 = pdoc2->GetBestView();
					ASSERT(pv2 != NULL);
					if (align_rel)
						base_addr = pv2->GetSearchBase();
					else
						base_addr = 0;

					// Search this file
					if ((found_addr = search_forw(pdoc2, 0, pdoc2->length(),
												  ss, mask, length, icase, 
												  tt, wholeword, alignment, offset, align_rel, base_addr)) == -2)
					{
						// Restore original display pos
						pview->show_pos();
						theApp.mac_error_ = 10;
						return FALSE;
					}
					else if (found_addr != -1)
					{
						// Found in this file
						CChildFrame *pf2 = pv2->GetFrame();
						if (pf2->IsIconic())
							pf2->MDIRestore();
						MDIActivate(pf2);
						pv2->MoveWithDesc("Search Text Found ", found_addr, found_addr + length);   // space at end means significant nav pt
#ifdef SYS_SOUNDS
						CSystemSound::Play("Search Text Found");
#endif
						return TRUE;
					}
					// else (not found) try the next open file
				}

				// Now go back and search the start of the active document
				if ((found_addr = search_forw(pdoc, 0,
											  start+length-1 > pdoc->length() ? pdoc->length() : start+length-1,
											  ss, mask, length, icase,
											  tt, wholeword, alignment, offset, align_rel, base_addr)) == -2)
				{
					// Restore original display pos
					pview->show_pos();
					theApp.mac_error_ = 10;
					return FALSE;
				}
				else if (found_addr == -1)
				{
					// Display not found message
					pview->show_pos();                         // Restore display of orig address
					AfxMessageBox(not_found_mess(TRUE, icase, tt, wholeword, alignment));
				}
				else
				{
					pview->MoveWithDesc("Search Text Found ", found_addr, found_addr + length);  // space at end means significant nav pt
#ifdef SYS_SOUNDS
					CSystemSound::Play("Search Text Found");
#endif
					return TRUE;
				}
			}
			else if (scope == CFindSheet::SCOPE_EOF && start > 0)
			{
				CString mess = CString("The end of file was reached.\r\n") +
							   not_found_mess(TRUE, icase, tt, wholeword, alignment) +
							   CString("\r\nDo you want to continue from the start of file?");

				SetAddress(end);
				theApp.OnIdle(0);             // Force display of updated address

				// Give the user the option to search the top bit of the file that has not been searched
				if (AfxMessageBox(mess, MB_YESNO) == IDYES)
				{
					if ((found_addr = search_forw(pdoc, 0,
												  start+length-1 > pdoc->length() ? pdoc->length() : start+length-1,
												  ss, mask, length, icase,
												  tt, wholeword, alignment, offset, align_rel, base_addr)) == -2)
					{
						// Restore original display pos
						pview->show_pos();
						theApp.mac_error_ = 10;
						return FALSE;
					}
					else if (found_addr == -1)
					{
						// Display not found message
						pview->show_pos();                         // Restore display of orig address
						AfxMessageBox(not_found_mess(TRUE, icase, tt, wholeword, alignment));
					}
					else
					{
						pview->MoveWithDesc("Search Text Found ", found_addr, found_addr + length);  // space at end means significant nav pt
#ifdef SYS_SOUNDS
						CSystemSound::Play("Search Text Found");
#endif
						return TRUE;
					}
				}
				else
				{
					pview->show_pos();                         // Restore display of orig address
				}
			}
			else
			{
				// Display not found message
				SetAddress(end);
				theApp.OnIdle(0);             // Force display of updated address
				AfxMessageBox(not_found_mess(TRUE, icase, tt, wholeword, alignment));
				pview->show_pos();                         // Restore display of orig address
			}
		}
		else
		{
			// Found
			pview->MoveWithDesc("Search Text Found ", found_addr, found_addr + length);  // space at end means significant nav pt
			if (scope == CFindSheet::SCOPE_FILE)
			{
				// Change to SCOPE_EOF so that next search does not find the same thing
				m_wndFind.SetScope(CFindSheet::SCOPE_EOF);
			}
#ifdef SYS_SOUNDS
			CSystemSound::Play("Search Text Found");
#endif
			return TRUE;
		}
	}
	theApp.mac_error_ = 10;
	return FALSE;
}

void CMainFrame::OnReplace()
{
	// Get current view thence document to search
	CHexEditView *pview = GetView();

	if (pview == NULL)
	{
		AfxMessageBox("There is no file open to replace in.");
		theApp.mac_error_ = 2;
		return;
	}

	CHexEditDoc *pdoc = pview->GetDocument();

	// Get current find options
	const unsigned char *ss;
	const unsigned char *mask = NULL;
	size_t length;

	m_wndFind.GetSearch(&ss, &mask, &length);

	CFindSheet::dirn_t dirn = m_wndFind.GetDirn();
	CFindSheet::scope_t scope = m_wndFind.GetScope();
	BOOL wholeword = m_wndFind.GetWholeWord();
	BOOL icase = !m_wndFind.GetMatchCase();
	int tt = int(m_wndFind.GetCharSet()) + 1;       // 1 = ASCII, 2 = Unicode, 3 = EBCDIC
	ASSERT(tt == 1 || tt == 2 || tt == 3);
	int alignment = m_wndFind.GetAlignment();
	int offset    = m_wndFind.GetOffset();
	bool align_rel = m_wndFind.AlignRel();
	FILE_ADDRESS base_addr;
	if (align_rel)
		base_addr = pview->GetSearchBase();
	else
		base_addr = 0;

	FILE_ADDRESS start, end;            // Range of bytes in the current file to search
	FILE_ADDRESS found_addr;            // The address where the search text was found (or -1, -2)

	// Check that the selection is the same as the search length (+ at start of file for SCOPE_FILE)
	pview->GetSelAddr(start, end);

	// If the appropriate search bytes are already selected we replace them and search for the next
	// ones, otherwise we just search for them

	// For whole file searches we need to check if the current selection is actually the first
	// (or last for backward searches) occurrence of the search bytes in the file
	if (scope == CFindSheet::SCOPE_FILE && end-start == length)
	{
		found_addr = -2;
		if (dirn == CFindSheet::DIRN_DOWN &&
			(found_addr = search_forw(pdoc, 0, end, ss, mask, length, icase, tt, wholeword, alignment, offset, align_rel, base_addr)) == -2)
		{
			// User abort or some error (message already displayed) - just restore original display pos
			pview->show_pos();
			return;
		}
		else if (dirn == CFindSheet::DIRN_UP &&
			(found_addr = search_back(pdoc, start, pdoc->length(), ss, mask, length, icase, tt, wholeword, alignment, offset, align_rel, base_addr)) == -2)
		{
			// User abort or some error (message already displayed) - just restore original display pos
			pview->show_pos();
			return;
		}
		else
			ASSERT(found_addr != -2);

		// Check that the found occurrence is actually the selection
		if (found_addr != start)
		{
			// Not found (-1) or earlier occurrence - don't replace yet
			start = 0;
			end = -1;
		}
	}

	if (end-start == length)
	{
		// First test OK (selection length == search length) - now check if there is a match
		// Note: this is direction insensitive (we could have used search_back instead)
		if ((found_addr = search_forw(pdoc, start, end, ss, mask, length, icase, tt, wholeword, alignment, offset, align_rel, base_addr)) == -2)
		{
			// User abort or some error (message already displayed) - just restore original display pos
			pview->show_pos();
			return;
		}
		else if (found_addr > -1)
		{
			// The current selection == search text - replace the text
			unsigned char *pp;
			size_t replen;
			m_wndFind.GetReplace(&pp, &replen);
			pview->do_replace(start, start+length, pp, replen);
		}
	}

	DoFind();

	AddSearchHistory(current_search_string_);
	AddReplaceHistory(current_replace_string_);

	if (theApp.recording_)
		theApp.SaveToMacro(km_replace, m_wndFind.GetOptions());
}

void CMainFrame::OnReplaceAll()
{
	// Get current view thence (first) document to search
	CHexEditView *pview = GetView();

	if (pview == NULL)
	{
		AfxMessageBox("There is no file open to search.");
		theApp.mac_error_ = 2;
		return;
	}

#ifdef REFRESH_OFF
	bool bb = theApp.refresh_off_;
	if (!bb)
	{
		theApp.refresh_off_ = true;
		theApp.disable_carets();
	}
#endif

	CWaitCursor wc;
	CHexEditDoc *pdoc = pview->GetDocument();

	// Init counts
	int replace_count = 0;
	int file_count = 1;

	// Get current find options
	const unsigned char *ss;
	const unsigned char * mask = NULL;
	size_t length;

	m_wndFind.GetSearch(&ss, &mask, &length);

	CFindSheet::dirn_t dirn = m_wndFind.GetDirn();
	CFindSheet::scope_t scope = m_wndFind.GetScope();
	BOOL wholeword = m_wndFind.GetWholeWord();
	BOOL icase = !m_wndFind.GetMatchCase();
	int tt = int(m_wndFind.GetCharSet()) + 1;       // 1 = ASCII, 2 = Unicode, 3 = EBCDIC
	ASSERT(tt == 1 || tt == 2 || tt == 3);
	int alignment = m_wndFind.GetAlignment();
	int offset    = m_wndFind.GetOffset();
	bool align_rel = m_wndFind.AlignRel();

	FILE_ADDRESS start, end;            // Range of bytes in the current file to search
	FILE_ADDRESS found_addr;            // The address where the search text was found (or -1, -2)

	switch (scope)
	{
	case CFindSheet::SCOPE_TOMARK:
		pview->GetSelAddr(start, end);
		if (dirn == CFindSheet::DIRN_UP)
		{
			if (start < end)
				end--;                  // Change to start search at byte before end of selection
			start = pview->GetMark();
		}
		else
		{
			if (start < end)
				++start;                // Change to start search at byte after start of selection
			end = pview->GetMark();
		}
		break;
	case CFindSheet::SCOPE_FILE:
		start = 0;
		end = pdoc->length();
		break;
	case CFindSheet::SCOPE_EOF:
		pview->GetSelAddr(start, end);
		if (dirn == CFindSheet::DIRN_UP)
		{
			if (start < end)
				end--;                  // Change to start search at byte before end of selection
			start = 0;
		}
		else
		{
			if (start < end)
				++start;                // Change to start search at byte after start of selection
			end = pdoc->length();
		}
		break;
	case CFindSheet::SCOPE_ALL:
		start = 0;
		end = pdoc->length();
		break;
	default:
		ASSERT(0);
	}
	FILE_ADDRESS byte_count = end-start;// Count of number of bytes searched

	// Get the replacement text
	unsigned char *pp;
	size_t replen;
	m_wndFind.GetReplace(&pp, &replen);

	// Init the loop
	FILE_ADDRESS curr = start;          // Current search position in current file
	CHexEditDoc *pdoc2 = pdoc;          // Current file we are searching
	CHexEditView *pv2 = pview;          // View of current file
	FILE_ADDRESS base_addr;
	if (align_rel)
		base_addr = pv2->GetSearchBase();
	else
		base_addr = 0;

	for (;;)
	{
		// Do the search
		if ((found_addr = search_forw(pdoc2, curr, end, ss, mask, length, icase, tt, wholeword, alignment, offset, align_rel, base_addr)) == -2)
		{
			// User abort or some error
#ifdef REFRESH_OFF
			if (!bb)
			{
				theApp.refresh_display(true);
				theApp.refresh_off_ = false;
				theApp.enable_carets();
			}
#endif
			// Message already displayed so just restore original display pos
			pview->show_pos();
			return;
		}
		else if (found_addr == -1)
		{
			if (scope == CFindSheet::SCOPE_ALL && (pdoc2=GetPrevDoc(pdoc2)) != pdoc)
			{
				++file_count;
				curr = 0;
				end = pdoc2->length();
				pv2 = pdoc2->GetBestView();
				if (align_rel)
					base_addr = pv2->GetSearchBase();
				else
					base_addr = 0;

				byte_count += pdoc2->length();
			}
			else if (scope == CFindSheet::SCOPE_EOF && start > 0)
			{
				CString mess = CString("The end of file was reached.\r\n") +
							   not_found_mess(TRUE, icase, tt, wholeword, alignment) +
							   CString("\r\nDo you want to continue from the start of file?");

				// Give the user the option to search the top bit of the file that has not been searched
				SetAddress(end);
				theApp.OnIdle(0);             // Force display of updated address
				if (AfxMessageBox(mess, MB_YESNO) == IDYES)
				{
					wc.Restore();
					curr = 0;
					end = start + length - 1;
					byte_count = end;         // Whole file will now be searched
					start = 0;                // Stop asking of above question again
				}
				else
				{
					break;                    // EOF and user chose not to continue from other end
				}
			}
			else
			{
				break;                        // search finished
			}
		}
		else
		{
			// Found
			pv2->do_replace(found_addr, found_addr+length, pp, replen);
			++replace_count;
			curr = found_addr + replen;   // Don't do replacements within subst. text as we could get into infinite loop
			if (dirn == CFindSheet::DIRN_DOWN)
				end += int(replen) - int(length);
		}
	}

	pview->show_pos();                         // Restore display of orig address
	AddSearchHistory(current_search_string_);
	AddReplaceHistory(current_replace_string_);

#ifdef REFRESH_OFF
	if (!bb)
	{
		theApp.refresh_display(true);
		theApp.refresh_off_ = false;
		theApp.enable_carets();
	}
#endif

	if (theApp.recording_)
		theApp.SaveToMacro(km_replace_all, m_wndFind.GetOptions());

	// Get a comma-separated version of bytes searched
	char buf[40];
	sprintf(buf, "%I64u", __int64(byte_count));
	CString temp(buf);
	::AddCommas(temp);

	CString mess;
	if (file_count > 1)
		mess.Format("Searched %s bytes in %d files.\n", temp, file_count);

	if (replace_count == 0)
		mess += "No occurrences were found.";
	else if (replace_count == 1)
		mess += "One occurrence replaced.";
	else
	{
		CString ss;
		ss.Format("Replaced %d occurrences.", replace_count);
		mess += ss;
	}

	AfxMessageBox(mess);
}

void CMainFrame::OnBookmarkAll()
{
	if (m_wndFind.bookmark_prefix_.IsEmpty())
	{
		AfxMessageBox("Bookmark All requires a bookmark prefix.");
		theApp.mac_error_ = 2;
		return;
	}
	// Get current view thence (first) document to search
	CHexEditView *pview = GetView();

	if (pview == NULL)
	{
		AfxMessageBox("There is no file open to search.");
		theApp.mac_error_ = 2;
		return;
	}

	CWaitCursor wc;
	CHexEditDoc *pdoc = pview->GetDocument();

	// Check if any bookmarks already exist in the set we are going to create
	CBookmarkList *pbl = theApp.GetBookmarkList();
	ASSERT(pbl != NULL);
	int count;                          // Number of bookmarks already in the set
	long next_number = 0;               // Number to use for next bookmark in the set

	if ((next_number = pbl->GetSetLast(m_wndFind.bookmark_prefix_, count)) > 0)
	{
		CBookmarkFind dlg;

		dlg.mess_.Format("%d bookmarks with the prefix \"%s\"\r"
						 "already exist.  Do you want to overwrite or\r"
						 "append to this set of bookmarks?",
						 count, (const char *)m_wndFind.bookmark_prefix_);

		switch (dlg.DoModal())
		{
		case IDC_BM_FIND_OVERWRITE:
			pbl->RemoveSet(m_wndFind.bookmark_prefix_);  // xxx does not remove doc bookmarks or dlg entries
			next_number = 0;
			break;
		case IDC_BM_FIND_APPEND:
			// nothing required here (next_number points to the next bookmark set number to use)
			break;
		case IDCANCEL:
			return;
		default:
			ASSERT(0);
			return;
		}
	}

	long start_number = next_number;
	int file_count = 1;

	// Get current find options
	const unsigned char *ss;
	const unsigned char *mask = NULL;
	size_t length;

	m_wndFind.GetSearch(&ss, &mask, &length);

	CFindSheet::dirn_t dirn = m_wndFind.GetDirn();
	CFindSheet::scope_t scope = m_wndFind.GetScope();
	BOOL wholeword = m_wndFind.GetWholeWord();
	BOOL icase = !m_wndFind.GetMatchCase();
	int tt = int(m_wndFind.GetCharSet()) + 1;       // 1 = ASCII, 2 = Unicode, 3 = EBCDIC
	ASSERT(tt == 1 || tt == 2 || tt == 3);
	int alignment = m_wndFind.GetAlignment();
	int offset    = m_wndFind.GetOffset();
	bool align_rel = m_wndFind.AlignRel();

	FILE_ADDRESS start, end;            // Range of bytes in the current file to search
	FILE_ADDRESS found_addr;            // The address where the search text was found (or -1, -2)

	switch (scope)
	{
	case CFindSheet::SCOPE_TOMARK:
		pview->GetSelAddr(start, end);
		if (dirn == CFindSheet::DIRN_UP)
		{
			if (start < end)
				end--;                  // Change to start search at byte before end of selection
			start = pview->GetMark();
		}
		else
		{
			if (start < end)
				++start;                // Change to start search at byte after start of selection
			end = pview->GetMark();
		}
		break;
	case CFindSheet::SCOPE_FILE:
		start = 0;
		end = pdoc->length();
		break;
	case CFindSheet::SCOPE_EOF:
		pview->GetSelAddr(start, end);
		if (dirn == CFindSheet::DIRN_UP)
		{
			if (start < end)
				end--;                  // Change to start search at byte before end of selection
			start = 0;
		}
		else
		{
			if (start < end)
				++start;                // Change to start search at byte after start of selection
			end = pdoc->length();
		}
		break;
	case CFindSheet::SCOPE_ALL:
		start = 0;
		end = pdoc->length();
		break;
	default:
		ASSERT(0);
	}

	FILE_ADDRESS byte_count = end-start;// Count of number of bytes searched
	bool bmerror_reported = false;

	// Init the loop
	FILE_ADDRESS curr = start;          // Current search position in current file
	CHexEditDoc *pdoc2 = pdoc;          // Current file we are searching
	CHexEditView *pv2 = pview;          // View of current file
	FILE_ADDRESS base_addr;
	if (align_rel)
		base_addr = pv2->GetSearchBase();
	else
		base_addr = 0;

	for (;;)
	{
		// Do the search
		if ((found_addr = search_forw(pdoc2, curr, end, ss, mask, length, icase, tt, wholeword, alignment, offset, align_rel, base_addr)) == -2)
		{
			// User abort or some error (message already displayed) - just restore original display pos
			pview->show_pos();
			return;
		}
		else if (found_addr == -1)
		{
			if (scope == CFindSheet::SCOPE_ALL && (pdoc2=GetPrevDoc(pdoc2)) != pdoc)
			{
				++file_count;
				curr = 0;
				end = pdoc2->length();
				pv2 = pdoc2->GetBestView();
				if (align_rel)
					base_addr = pv2->GetSearchBase();
				else
					base_addr = 0;

				byte_count += pdoc2->length();
			}
			else if (scope == CFindSheet::SCOPE_EOF && start > 0)
			{
				CString mess = CString("The end of file was reached.\r\n") +
							   not_found_mess(TRUE, icase, tt, wholeword, alignment) +
							   CString("\r\nDo you want to continue from the start of file?");

				// Give the user the option to search the top bit of the file that has not been searched
				SetAddress(end);
				theApp.OnIdle(0);             // Force display of updated address

				if (AfxMessageBox(mess, MB_YESNO) == IDYES)
				{
					wc.Restore();
					curr = 0;
					end = start + length - 1;
					byte_count = end;           // Use whole file as byte count now
					start = 0;                  // Prevent "EOF reached" message appearing again
				}
				else
				{
					break;                      // EOF and user chose not to continue from other end
				}
			}
			else
			{
				break;                          // search finished
			}
		}
		else
		{
			// Found so add the bookmark
			CString bm_name;
			bm_name.Format("%s%05ld", (const char *)m_wndFind.bookmark_prefix_, long(next_number));
			++next_number;

			if (pdoc2->pfile1_ == NULL)
			{
				if (!bmerror_reported)
				{
					bmerror_reported = true;
					AfxMessageBox("Bookmarks cannot be added to new files\r\n"
								  "that have not yet been written to disk.");
				}
			}
			else
			{
				int ii = pbl->AddBookmark(bm_name, pdoc2->pfile1_->GetFilePath(), found_addr, NULL, pdoc2);
			}

			curr = found_addr + 1;              // Continue the search from the next byte
		}
	}

	pview->show_pos();                          // Restore display of orig address
	AddSearchHistory(current_search_string_);

	if (theApp.recording_)
		theApp.SaveToMacro(km_bookmark_all, m_wndFind.GetOptions());

	// Get a comma-separated version of bytes searched
	char buf[40];
	sprintf(buf, "%I64u", __int64(byte_count));
	CString temp(buf);
	::AddCommas(temp);

	CString mess;
	if (file_count > 1)
		mess.Format("Searched %s bytes in %d files.\n", temp, file_count);
	else
		mess.Format("Searched %s bytes.\n", temp);
	if (next_number == start_number)
		mess += "No occurrences were found";
	else if (next_number == start_number+1)
	{
		CString ss;

		ss.Format("Set one bookmark: %s%05ld", (const char *)m_wndFind.bookmark_prefix_, long(start_number));
		mess += ss;
	}
	else
	{
		CString ss;

		ss.Format("Set %d bookmarks.\n\nFirst bookmark: %s%05ld\nLast bookmark: %s%05ld",
				  next_number - start_number,
				  (const char *)m_wndFind.bookmark_prefix_, (long)start_number,
				  (const char *)m_wndFind.bookmark_prefix_, (long)next_number-1);
		mess += ss;
	}
	AfxMessageBox(mess);
}

// search_forw returns the address if the search text was found or
// -1 if it was not found, or -2 on some error (message already shown)
FILE_ADDRESS CMainFrame::search_forw(CHexEditDoc *pdoc, FILE_ADDRESS start_addr, FILE_ADDRESS end_addr,
									 const unsigned char *ss, const unsigned char *mask, size_t length,
									 BOOL icase, int tt, BOOL ww, int aa, int offset, bool align_rel, FILE_ADDRESS base_addr)
{
	ASSERT(start_addr <= end_addr && end_addr <= pdoc->length());

	FILE_ADDRESS bg_next = pdoc->GetNextFound(ss, mask, length, icase, tt, ww,
											  aa, offset, align_rel, base_addr, start_addr);

	if (bg_next == -1 || bg_next > -1 && bg_next + length > end_addr)
	{
		return -1;
	}
	else if (bg_next > -1)
	{
		// Found
		if (AbortKeyPress() && AfxMessageBox("Abort search?", MB_YESNO) == IDYES)
		{
			StatusBarText("Search aborted");
			theApp.mac_error_ = 10;
//                pview->show_pos();
			return -2;
		}

		return bg_next;
	}

	size_t buf_len = size_t(min(end_addr-start_addr, FILE_ADDRESS(search_buf_len + length - 1)));

//    ASSERT(length > 0 && length <= buf_len);

	// Warn of simple problems
	if (length == 0)
	{
		AfxMessageBox("Empty search sequence");
		return -2;
	}

	if (bg_next == -3)
		theApp.StopSearches();  // we must abort all bg searches here otherwise we'll later try to start on threads where bg search has not been stopped

	// We have to do our own search since either:
	// background searches are off (bg_next == -4)
	// or the background search doesn't match (bg_next == -3) - and was stopped above
	// or the background search is not finished (bg_next == -2) so we forget it and do our own

	unsigned char *buf = new unsigned char[buf_len];
	boyer bb(ss, length, mask);       // Boyer-Moore searcher
	FILE_ADDRESS addr_buf = start_addr; // Current location in doc of start of buf
	FILE_ADDRESS show_inc = 0x100000;    // How far between showing addresses
	FILE_ADDRESS next_show;             // Next address to show to user
	FILE_ADDRESS slow_show;             // Slow show update rate when we get here
	size_t got;                 // How many bytes just read

	// Are there enough bytes for a search?
	if (addr_buf + length <= end_addr)
	{
		got = pdoc->GetData(buf, length - 1, addr_buf);
		ASSERT(got == length - 1);      // xxx file length may change?
		next_show = (addr_buf/show_inc + 1)*show_inc;
		slow_show = ((addr_buf+0x1000000)/0x1000000 + 1)*0x1000000;
		while (1)
		{
			unsigned char *pp;  // Where search string's found (or NULL if not)

			if (addr_buf > next_show)
			{
				if (AbortKeyPress() && AfxMessageBox("Abort search?", MB_YESNO) == IDYES)
				{
					delete[] buf;
					// Start bg search anyway
					theApp.NewSearch(ss, mask, length, icase, tt, ww, aa, offset, align_rel);
					if (bg_next == -3)
					{
						pdoc->base_addr_ = base_addr;
						pdoc->StartSearch(start_addr, addr_buf);
						theApp.StartSearches(pdoc);
					}
					Progress(-1);
					StatusBarText("Search aborted");
					theApp.mac_error_ = 10;
//                    pview->show_pos();
					return -2;
				}

				// Show search progress
				SetAddress(next_show);
				if (next_show >= slow_show)
				{
					Progress(int(((next_show-start_addr)*100)/(end_addr-start_addr)));
					show_inc = 0x1000000;
				}
				theApp.OnIdle(0);         // Force display of updated address
				next_show += show_inc;
			}

			bool alpha_before = false;
			bool alpha_after = false;

			// Get the next buffer full and search it
			got = pdoc->GetData(buf + length - 1, buf_len - (length - 1),
								addr_buf + length - 1);
			if (ww)
			{
				// Work out if byte before current buffer is alphabetic
				if (tt == 1 && addr_buf > 0)
				{
					// Check if alphabetic ASCII
					unsigned char cc;
					VERIFY(pdoc->GetData(&cc, 1, addr_buf-1) == 1);

					alpha_before = isalnum(cc) != 0;
				}
				else if (tt == 2 && addr_buf > 1)
				{
					// Check if alphabetic Unicode
					unsigned char cc[2];
					VERIFY(pdoc->GetData(cc, 2, addr_buf-2) == 2);

					alpha_before = isalnum(cc[0]) != 0;  // Check if low byte has ASCII alpha
				}
				else if (tt == 3 && addr_buf > 0)
				{
					// Check if alphabetic EBCDIC
					unsigned char cc;
					VERIFY(pdoc->GetData(&cc, 1, addr_buf-1) == 1);

					alpha_before = isalnum(e2a_tab[cc]) != 0;
				}

				// Work out if byte after current buffer is alphabetic
				if (addr_buf + buf_len < pdoc->length())
				{
					unsigned char cc;
					VERIFY(pdoc->GetData(&cc, 1, addr_buf + buf_len) == 1);

					if (tt == 3)
						alpha_after = isalnum(e2a_tab[cc]) != 0;
					else
						alpha_after = isalnum(cc) != 0;      // ASCII and Unicode
				}
			}

			if ((pp = bb.findforw(buf, length - 1 + got, icase, tt, ww, alpha_before, alpha_after, aa, offset, base_addr, addr_buf)) != NULL)
			{
				// If we find lots of occurrences then the Esc key check in 
				// above is never done so do it here too.
				if (AbortKeyPress() && AfxMessageBox("Abort search?", MB_YESNO) == IDYES)
				{
					delete[] buf;
					// Start bg search anyway
					theApp.NewSearch(ss, mask, length, icase, tt, ww, aa, offset, align_rel);
					if (bg_next == -3)
					{
						pdoc->base_addr_ = base_addr;
						pdoc->StartSearch(start_addr, addr_buf);
						theApp.StartSearches(pdoc);
					}
					Progress(-1);
					StatusBarText("Search aborted");
					theApp.mac_error_ = 10;
//                    pview->show_pos();
					return -2;
				}

				// Start bg search to search the rest of the file
				theApp.NewSearch(ss, mask, length, icase, tt, ww, aa, offset, align_rel);
				if (bg_next == -3)
				{
					// We know that there are no occurrences from the start up to where we found
					// this one but we must not exclude the found address (addr_buf + (pp - buf))
					// so that background search finds it and it is displayed
					pdoc->base_addr_ = base_addr;
					pdoc->StartSearch(start_addr, addr_buf + (pp - buf));
					theApp.StartSearches(pdoc);
				}
//                MoveToAddress(addr_buf + (pp - buf), addr_buf + (pp - buf) + length);
				FILE_ADDRESS retval = addr_buf + (pp - buf);

				Progress(-1);
				delete[] buf;
				if (retval + length > end_addr)
					return -1;              // found but after end of search area
				else
					return retval;          // found
			}
			addr_buf += got;

			// Check if we're at the end yet
			if (addr_buf + length - 1 >= end_addr)
			{
				break;
			}

			// Move a little bit from the end to the start of the buffer
			// so that we don't miss sequences that overlap the pieces read
			memmove(buf, buf + buf_len - (length - 1), length - 1);
		}
	}
	Progress(-1);
	delete[] buf;

	// Start bg search to search the rest of the file
	theApp.NewSearch(ss, mask, length, icase, tt, ww, aa, offset, align_rel);
	if (bg_next == -3)
	{
		pdoc->base_addr_ = base_addr;
		if (end_addr - (length-1) > start_addr)
			pdoc->StartSearch(start_addr, end_addr - (length-1));
		else
			pdoc->StartSearch();
		theApp.StartSearches(pdoc);
	}

	return -1;                          // not found
}

// search_back returns the address if the search text was found or
// -1 if it was not found, or -2 on some error (message already shown)
FILE_ADDRESS CMainFrame::search_back(CHexEditDoc *pdoc, FILE_ADDRESS start_addr, FILE_ADDRESS end_addr,
									 const unsigned char *ss, const unsigned char *mask, size_t length,
									 BOOL icase, int tt, BOOL ww, int aa, int offset, bool align_rel, FILE_ADDRESS base_addr)
{
	ASSERT(start_addr <= end_addr && end_addr <= pdoc->length());

	FILE_ADDRESS bg_next = pdoc->GetPrevFound(ss, mask, length, icase, tt, ww, aa, offset, align_rel, base_addr, end_addr - length);

	if (bg_next == -1 || bg_next > -1 && bg_next < start_addr)
	{
		return -1;
	}
	else if (bg_next > -1)
	{
		return bg_next;
	}

	size_t buf_len = size_t(min(end_addr-start_addr, FILE_ADDRESS(search_buf_len + length - 1)));

//    ASSERT(length > 0 && length <= buf_len);

	// Warn of simple problems
	if (length == 0)
	{
		AfxMessageBox("Empty search sequence");
		return -2;
	}

	if (bg_next == -3)
		theApp.StopSearches();  // we must abort all bg searches here otherwise we'll later try to start on threads where bg search has not been stopped

	unsigned char *buf = new unsigned char[buf_len];
	boyer bb(ss, length, mask);         // Boyer-Moore searcher
	FILE_ADDRESS addr_buf = end_addr;   // Current location in doc of end of buf
//    FILE_ADDRESS show_inc = 0x20000;    // How far between showing addresses
	FILE_ADDRESS show_inc = 0x200000;   // How far between showing addresses
	FILE_ADDRESS next_show;             // Next address to show to user
//    FILE_ADDRESS slow_show;             // Slow show update rate when we get here
	size_t got;                 // How many bytes just read

	// Are there enough bytes for a search?
	if (addr_buf >= start_addr + length)
	{
		next_show = (addr_buf/show_inc)*show_inc;
//        slow_show = ((addr_buf+0x800000)/0x800000)*0x800000;
		while (1)
		{
			unsigned char *pp;  // Where search string's found (or NULL if not)

			if (addr_buf < next_show)
			{
				if (AbortKeyPress() && AfxMessageBox("Abort search?", MB_YESNO) == IDYES)
				{
					delete[] buf;
					// Start bg search anyway
					theApp.NewSearch(ss, mask, length, icase, tt, ww, aa, offset, align_rel);
					if (bg_next == -3)
					{
						pdoc->base_addr_ = base_addr;
						pdoc->StartSearch(addr_buf, end_addr - (length-1));
						theApp.StartSearches(pdoc);
					}
					StatusBarText("Search aborted");
					theApp.mac_error_ = 10;
//                    pview->show_pos();
					return -2;
				}

				// Show search progress
				SetAddress(next_show);
				theApp.OnIdle(0);         // Force display of updated address
//                if (next_show >= slow_show)
//                    show_inc = 0x200000;
				next_show += show_inc;
			}

			// Note that search_back uses a slightly different algorithm to search_forw,
			// in that for search_forw the buffer overlap area (so that search strings
			// are not missed that overlap reads) is kept using memmove() whereas
			// search_back reads the same bit of the file twice.  This is simpler
			// esp. for backward searches but may sometimes be slower.
			// Get the next buffer full and search it
			if (addr_buf - start_addr < buf_len)
			{
				got = pdoc->GetData(buf, size_t(addr_buf - start_addr), start_addr);
				ASSERT(got == addr_buf - start_addr);

				bool alpha_before = false;
				bool alpha_after = false;
				if (ww)
				{
					// Work out if byte before current buffer is alphabetic
					if (tt == 1 && start_addr > 0)
					{
						// Check if alphabetic ASCII
						unsigned char cc;
						VERIFY(pdoc->GetData(&cc, 1, start_addr-1) == 1);

						alpha_before = isalnum(cc) != 0;
					}
					else if (tt == 2 && start_addr > 1)
					{
						// Check if alphabetic Unicode
						unsigned char cc[2];
						VERIFY(pdoc->GetData(cc, 2, start_addr-2) == 2);

						alpha_before = isalnum(cc[0]) != 0;  // Check if low byte has ASCII alpha
					}
					else if (tt == 3 && start_addr > 0)
					{
						// Check if alphabetic EBCDIC
						unsigned char cc;
						VERIFY(pdoc->GetData(&cc, 1, start_addr-1) == 1);

						alpha_before = isalnum(e2a_tab[cc]) != 0;
					}

					// Work out if byte after current buffer is alphabetic
					if (addr_buf < pdoc->length())
					{
						unsigned char cc;
						VERIFY(pdoc->GetData(&cc, 1, addr_buf) == 1);

						if (tt == 3)
							alpha_after = isalnum(e2a_tab[cc]) != 0;
						else
							alpha_after = isalnum(cc) != 0;      // ASCII and Unicode
					}
				}
				pp = bb.findback(buf, got, icase, tt, ww, alpha_before, alpha_after, aa, offset, base_addr, start_addr);
			}
			else
			{
				got = pdoc->GetData(buf, buf_len, addr_buf - buf_len);
				ASSERT(got == buf_len);

				bool alpha_before = false;
				bool alpha_after = false;
				if (ww)
				{
					// Work out if byte before current buffer is alphabetic
					if (tt == 1 && addr_buf - buf_len > 0)
					{
						// Check if alphabetic ASCII
						unsigned char cc;
						VERIFY(pdoc->GetData(&cc, 1, addr_buf - buf_len - 1) == 1);

						alpha_before = isalnum(cc) != 0;
					}
					else if (tt == 2 && addr_buf - buf_len > 1)
					{
						// Check if alphabetic Unicode
						unsigned char cc[2];
						VERIFY(pdoc->GetData(cc, 2, addr_buf - buf_len - 2) == 2);

						alpha_before = isalnum(cc[0]) != 0;  // Check if low byte has ASCII alpha
					}
					else if (tt == 3 && addr_buf - buf_len > 0)
					{
						// Check if alphabetic EBCDIC
						unsigned char cc;
						VERIFY(pdoc->GetData(&cc, 1, addr_buf - buf_len - 1) == 1);

						alpha_before = isalnum(e2a_tab[cc]) != 0;
					}

					// Work out if byte after current buffer is alphabetic
					if (addr_buf < pdoc->length())
					{
						unsigned char cc;
						VERIFY(pdoc->GetData(&cc, 1, addr_buf) == 1);

						if (tt == 3)
							alpha_after = isalnum(e2a_tab[cc]) != 0;
						else
							alpha_after = isalnum(cc) != 0;      // ASCII and Unicode
					}
				}
				pp = bb.findback(buf, got, icase, tt, ww, alpha_before, alpha_after, aa, offset, base_addr, addr_buf - buf_len);
			}
			if (pp != NULL)
			{
//                MoveToAddress(addr_buf - got + (pp - buf), addr_buf - got + (pp - buf) + length);
				FILE_ADDRESS retval = addr_buf - got + (pp - buf);

				// Start bg search to search the rest of the file
				theApp.NewSearch(ss, mask, length, icase, tt, ww, aa, offset, align_rel);
				if (bg_next == -3)
				{
					// Search all the addresses that have not been searched, but make sure to 
					// include retval so this occurrence is also found in background search and displayed
					pdoc->base_addr_ = base_addr;
					pdoc->StartSearch(retval + 1, end_addr - (length-1));
					theApp.StartSearches(pdoc);
				}

				delete[] buf;
				if (retval < start_addr)    // Never happens?
					return -1;
				else
					return retval;          // found
			}
			addr_buf -= got - (length - 1);

			// Check if we're at start of search buf yet
			if (addr_buf - start_addr < length)
			{
				break;
			}
		}
	}
	delete[] buf;

	// Start bg search to search the rest of the file
	theApp.NewSearch(ss, mask, length, icase, tt, ww, aa, offset, align_rel);
	if (bg_next == -3)
	{
		pdoc->base_addr_ = base_addr;
		if (end_addr - (length-1) > start_addr)
			pdoc->StartSearch(start_addr, end_addr - (length-1));  // Start bg search (start_addr->EOF already done)
		else
			pdoc->StartSearch();
		theApp.StartSearches(pdoc);
	}

	return -1;                          // not found
}

CString CMainFrame::not_found_mess(BOOL forward, BOOL icase, int tt, BOOL ww, int aa)
{
#ifdef SYS_SOUNDS
	CSystemSound::Play("Search Text Not Found");
#endif

	CString mess = "The search item was not found:\r\n";

	// There are none
	if (icase && tt == 3)
		mess += "- case-insensitive EBCDIC ";
	else if (tt == 3)
		mess += "- EBCDIC";
	else if (icase && tt == 2)
		mess += "- case-insensitive Unicode ";
	else if (tt == 2)
		mess += "- Unicode ";
	else if (icase)
		mess += "- case-insensitive ";
	else if (forward)
		mess += "- forward ";
	else
		mess += "- backward ";

	if (ww)
		mess += "whole-word ";
	mess += "search\r\n";

	if (aa == 2)
		mess += "- with word alignment";
	else if (aa == 3)
		mess += "- with alignment on every 3rd byte";
	else if (aa == 4)
		mess += "- with double-word alignment";
	else if (aa == 8)
		mess += "- with quad-word alignment";
	else if (aa > 1)
	{
		CString ss;
		ss.Format("- with alignment on every %dth byte", aa);
		mess += ss;
	}

	return mess;
}

// Open first (simple search) page of Find dialog
void CMainFrame::OnEditFind()
{
	if (GetView() != NULL)
	{
		m_paneFind.ShowAndUnroll();
		m_wndFind.ShowPage(0);
	}
}

// Open 2nd page of find dialog
void CMainFrame::OnEditFind2()
{
	if (GetView() != NULL)
	{
		m_paneFind.ShowAndUnroll();
		m_wndFind.ShowPage(1);
	}
}

// Open 4th (replace) page of Find dialog
void CMainFrame::OnEditReplace()
{
	if (GetView() != NULL)
	{
		m_paneFind.ShowAndUnroll();
		m_wndFind.ShowPage(4);
	}
}

// We can't do a search unless there is a document open
void CMainFrame::OnUpdateEditFind(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(GetView() != NULL);
}

void CMainFrame::OnCalculator()
{
	show_calc();

	m_wndCalc.SetFocus();
}

/*
void CMainFrame::OnOptionsScheme()
{
	theApp.display_options(COLOUR_OPTIONS_PAGE, TRUE);
}
*/

void CMainFrame::OnBookmarks()
{
	m_paneBookmarks.ShowAndUnroll();
}

void CMainFrame::OnEditGotoDec()
{
	OnEditGoto(1);
}

void CMainFrame::OnEditGotoHex()
{
	OnEditGoto(2);
}

void CMainFrame::OnEditGoto(int base_mode /*= 0*/)
{
	CHexEditView *pview = GetView();
	if (pview != NULL)
	{
		ASSERT(m_wndCalc.m_hWnd != 0);

		m_wndCalc.change_bits(64);

		switch (base_mode)
		{
		case 1:
			// Use base 10 addresses
			m_wndCalc.change_base(10);
			m_wndCalc.change_signed(true);  // xxx make false (addresses are never -ve) when we have signed check-box
			break;
		case 2:
			// Use base 16 addresses
			m_wndCalc.change_base(16);
			m_wndCalc.change_signed(false);
			break;
		default:
			ASSERT(0);
			/* fall through */
		case 0:
			// Change base depending on how view is displaying addresses
			if (pview->DecAddresses())
			{
				m_wndCalc.change_base(10);
				m_wndCalc.change_signed(true);  // xxx make false (addresses are never -ve) when we have signed check-box
			}
			else
			{
				m_wndCalc.change_base(16);
				m_wndCalc.change_signed(false);
			}
			break;
		}
		m_wndCalc.SetWindowText("Go To");
		//ShowControlBar(&m_wndCalc, TRUE, FALSE); // xxx fix for MFC9
		m_paneCalc.ShowAndUnroll();

		// Make sure controls are up to date and put current address into it
		//m_wndCalc.UpdateData(FALSE);

		FILE_ADDRESS start, end;
		pview->GetSelAddr(start, end);
		m_wndCalc.Set(start);

		m_wndCalc.SetFocus();
		m_wndCalc.StartEdit();

		((CHexEditApp *)AfxGetApp())->SaveToMacro(km_goto);
	}
}

void CMainFrame::OnCalcSel()
{
	CHexEditView *pview = GetView();
	if (pview != NULL)
	{
		__int64 sel_val = 0;                    // Value from file to add to the calc

		ASSERT(m_wndCalc.m_hWnd != 0);

		FILE_ADDRESS start_addr, end_addr;              // Current selection
		pview->GetSelAddr(start_addr, end_addr);
		if (start_addr == end_addr)
			end_addr = start_addr + m_wndCalc.ByteSize();

		unsigned char buf[8];
		size_t got = pview->GetDocument()->GetData(buf, min(8, size_t(end_addr - start_addr)), start_addr);
		if (pview->BigEndian())
		{
			for (int ii = 0; ii < (int)got; ++ii)
				sel_val = (sel_val<<8) + buf[ii];
		}
		else
		{
			while (got > 0)
				sel_val = (sel_val<<8) + buf[--got];
		}

		// Change calc bits depending on size of selection
		if (end_addr - start_addr > 8 || got < size_t())
		{
			ASSERT(0);
			AfxMessageBox("Selection too big for calculator!");
			theApp.mac_error_ = 10;
		}
		else if (end_addr - start_addr > 4)
		{
			m_wndCalc.change_bits(64);
		}
		else if (end_addr - start_addr > 2)
		{
			m_wndCalc.change_bits(32);
		}
		else if (end_addr - start_addr > 1)
		{
			m_wndCalc.change_bits(16);
		}
		else
		{
			m_wndCalc.change_bits(8);
		}

		m_wndCalc.SetWindowText("Calculator");
		//ShowControlBar(&m_wndCalc, TRUE, FALSE); // xxx fix for MFC9
		m_paneCalc.ShowAndUnroll();

		// Make sure controls are up to date and put current address into it
		//m_wndCalc.UpdateData(FALSE);

		m_wndCalc.Set(sel_val);

		m_wndCalc.SetFocus();

		((CHexEditApp *)AfxGetApp())->SaveToMacro(km_calc_dlg, 1);
	}
}

#if 0
void CMainFrame::OnUpdateEditGoto(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(GetView() != NULL);
}
#endif

// Display control bars context menu
// Note: With BCG this is now only used for status bar - should probably move to CStatBar
void CMainFrame::bar_context(CPoint point)
{
#if 0
	// Get the top level menu that contains the submenus used as popup menus
	CMenu top;
	BOOL ok = top.LoadMenu(IDR_CONTEXT);
	ASSERT(ok);
	if (!ok) return;

	CMenu *ppop = top.GetSubMenu(0);
	ASSERT(ppop != NULL);
	if (ppop != NULL)
		VERIFY(ppop->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
							 point.x, point.y, this));

	top.DestroyMenu();
#else
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	CContextMenuManager *pCMM = aa->GetContextMenuManager();

	pCMM->ShowPopupMenu(IDR_CONTEXT_STATBAR, point.x, point.y, this);
#endif
}

void CMainFrame::OnInitMenu(CMenu* pMenu)
{
	// Remove this?
	CMDIFrameWndEx::OnInitMenu(pMenu);
}

// Toggle display of status bar
BOOL CMainFrame::OnPaneCheck(UINT nID)
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	if (CMDIFrameWndEx::OnPaneCheck(nID))
	{
		aa->SaveToMacro(km_bar, nID);
		return TRUE;
	}
	else
	{
		aa->mac_error_ = 2;
		return FALSE;
	}
}

// The following were added for BCG
void CMainFrame::OnCustomize()
{
	CHexEditCustomize *pdlg = new CHexEditCustomize(this);
	pdlg->EnableUserDefinedToolbars();

	// Allow user to select commands that don't appear on any menus
//    pdlg->AddMenu(IDR_MISC);
	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_MISC));
	pdlg->AddMenuCommands(menu.GetSubMenu(0), FALSE, "Format commands");
	pdlg->AddMenuCommands(menu.GetSubMenu(1), FALSE, "Navigation commands");
	pdlg->AddMenuCommands(menu.GetSubMenu(2), FALSE, "Find commands");
	pdlg->AddMenuCommands(menu.GetSubMenu(3), FALSE, "Aerial view commands");
	menu.DestroyMenu();

	pdlg->ReplaceButton(ID_JUMP_HEX, CHexComboButton ());
	pdlg->ReplaceButton(ID_JUMP_DEC, CDecComboButton ());
	pdlg->ReplaceButton(ID_SEARCH, CFindComboButton ());
	pdlg->ReplaceButton(ID_SCHEME,
					  CMFCToolBarComboBoxButton(::IsUs() ? ID_SCHEME_US : ID_SCHEME, 
												-1,
												CBS_DROPDOWNLIST | CBS_AUTOHSCROLL | WS_VSCROLL | WS_TABSTOP,
												120));
	pdlg->ReplaceButton(ID_BOOKMARKS, CBookmarksComboButton ());

	pdlg->ReplaceButton(IDC_FONTNAME,
		CHexEditFontCombo(IDC_FONTNAME, 
			-1 /*CImageHash::GetImageOfCommand(IDC_FONTNAME, FALSE)*/,
			RASTER_FONTTYPE | TRUETYPE_FONTTYPE | DEVICE_FONTTYPE,
			ANSI_CHARSET,
			WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST |
			CBS_AUTOHSCROLL | CBS_HASSTRINGS | CBS_OWNERDRAWFIXED, 175) );
	pdlg->ReplaceButton(IDC_FONTSIZE,
		CHexEditFontSizeCombo(IDC_FONTSIZE, 
			-1 /*CImageHash::GetImageOfCommand(IDC_FONTSIZE, FALSE)*/,
			WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWN, 50) );

//    pdlg->AddToolBar("Main", IDR_MAINFRAME);
//    pdlg->AddButton("User", CMFCToolbarButton(ID_MARK, 1, "User Tool 1", TRUE));
//    pdlg->AddButton("User", CMFCToolbarButton(ID_GOTO_MARK, 2, "User Tool 2", TRUE));
//    pdlg->SetUserCategory("User");

	menu.LoadMenu(IDR_MENUBUTTON);

	pdlg->ReplaceButton(ID_DISPLAY_DROPDOWN,
		CMFCToolBarMenuButton (-1, *menu.GetSubMenu(0), -1, _T("Show Area")));
	pdlg->ReplaceButton(ID_CHARSET_DROPDOWN,
		CMFCToolBarMenuButton (-1, *menu.GetSubMenu(1), -1, _T("Char Set")));
	pdlg->ReplaceButton(ID_CONTROL_DROPDOWN,
		CMFCToolBarMenuButton (-1, *menu.GetSubMenu(2), -1, _T("Ctrl Chars")));

	pdlg->ReplaceButton(ID_HIGHLIGHT_MENU,
		CMFCToolBarMenuButton(ID_HIGHLIGHT, *menu.GetSubMenu(3),
											  -1 /*CImageHash::GetImageOfCommand(ID_HIGHLIGHT)*/ ));
	pdlg->ReplaceButton(ID_MARK_MENU,
		CMFCToolBarMenuButton(ID_GOTO_MARK, *menu.GetSubMenu(4),
											  -1 /*CImageHash::GetImageOfCommand(ID_GOTO_MARK)*/ ));
	pdlg->ReplaceButton(ID_BOOKMARKS_MENU,
		CMFCToolBarMenuButton(ID_BOOKMARKS_EDIT, *menu.GetSubMenu(5),
											  -1 /*CImageHash::GetImageOfCommand(ID_BOOKMARKS_EDIT)*/ ));

	pdlg->ReplaceButton(ID_NAV_BACK,
		CMFCToolBarMenuButton (ID_NAV_BACK, *menu.GetSubMenu(6), -1, _T("Navigate Backward")));
	pdlg->ReplaceButton(ID_NAV_FORW,
		CMFCToolBarMenuButton (ID_NAV_FORW, *menu.GetSubMenu(7), -1, _T("Navigate Forward")));

	pdlg->Create();
}

BOOL CMainFrame::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext)
{
	if (!CMDIFrameWndEx::LoadFrame(nIDResource, dwDefaultStyle, pParentWnd, pContext))
	{
		return FALSE;
	}

	// Add some tools for example....
	CUserToolsManager* pUserToolsManager = theApp.GetUserToolsManager ();
	if (pUserToolsManager != NULL &&
		pUserToolsManager->GetUserTools().IsEmpty ())
	{
		CUserTool* pTool = pUserToolsManager->CreateNewTool();
		pTool->m_strLabel = _T("Notepad");
		pTool->m_strArguments = _T("$(FilePath)");
		pTool->SetCommand(_T("notepad.exe"));
		pTool = pUserToolsManager->CreateNewTool();
		pTool->m_strLabel = _T("Windows Calculator");
		pTool->SetCommand(_T("calc.exe"));
	}

	return TRUE;
}

#if 0  // This is now done in MFC9 by CMDIFrameWndEx::EnablePaneMenu
LRESULT CMainFrame::OnToolbarContextMenu(WPARAM,LPARAM lp)
{
	CPoint point(LOWORD(lp), HIWORD(lp));

	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_CONTEXT_BARS));

	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);

	SetupToolbarMenu(*pPopup, ID_VIEW_USER_TOOLBAR1, ID_VIEW_USER_TOOLBAR10);

	CMFCPopupMenu* pPopupMenu = new CMFCPopupMenu;
	pPopupMenu->SetAutoDestroy(FALSE);
	pPopupMenu->Create(this, point.x, point.y, pPopup->GetSafeHmenu());

	return 0;
}
#endif

#if 0 // This is now handled automatically in MFC9
BOOL CMainFrame::OnToolsViewUserToolbar(UINT uiId)
{
	CMFCToolBar* pUserToolBar = GetUserBarByIndex(uiId - ID_VIEW_USER_TOOLBAR1);
	if (pUserToolBar == NULL)
	{
		ASSERT(0);
		return FALSE;
	}

	ShowControlBar(pUserToolBar, !(pUserToolBar->GetStyle() & WS_VISIBLE), FALSE);
	RecalcLayout();
	return TRUE;
}

void CMainFrame::OnUpdateToolsViewUserToolbar(CCmdUI* pCmdUI)
{
	CMFCToolBar* pUserToolBar = GetUserBarByIndex(pCmdUI->m_nID - ID_VIEW_USER_TOOLBAR1);
	if (pUserToolBar == NULL)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	pCmdUI->Enable();
	pCmdUI->SetCheck(pUserToolBar->GetStyle() & WS_VISIBLE);
}
#endif

afx_msg LRESULT CMainFrame::OnMenuReset(WPARAM wp, LPARAM)
{
	return 0;
}

afx_msg LRESULT CMainFrame::OnToolbarReset(WPARAM wp, LPARAM)
{
	switch ((UINT)wp)
	{
	case IDR_STDBAR:
		{
			CMenu menu;
			menu.LoadMenu(IDR_MENUBUTTON);

			m_wndStandardBar.ReplaceButton(ID_DISPLAY_DROPDOWN,
				CMFCToolBarMenuButton (-1, *menu.GetSubMenu(0),
													   afxCommandManager->GetCmdImage(ID_DISPLAY_DROPDOWN) /*CImageHash::GetImageOfCommand(ID_DISPLAY_DROPDOWN)*/,
													   _T("Show Area")));
			m_wndStandardBar.ReplaceButton(ID_CHARSET_DROPDOWN,
				CMFCToolBarMenuButton (-1, *menu.GetSubMenu(1),
													   afxCommandManager->GetCmdImage(ID_CHARSET_DROPDOWN) /*CImageHash::GetImageOfCommand(ID_CHARSET_DROPDOWN)*/,
													   _T("Char Set")));
			m_wndStandardBar.ReplaceButton(ID_CONTROL_DROPDOWN,
				CMFCToolBarMenuButton (-1, *menu.GetSubMenu(2),
													  afxCommandManager->GetCmdImage(ID_CONTROL_DROPDOWN) /*CImageHash::GetImageOfCommand(ID_CONTROL_DROPDOWN)*/,
													  _T("Ctrl Chars")));
		}
		m_wndStandardBar.ReplaceButton(ID_SCHEME,
					  CMFCToolBarComboBoxButton(::IsUs() ? ID_SCHEME_US : ID_SCHEME, 
												-1,
												CBS_DROPDOWNLIST | CBS_AUTOHSCROLL | WS_VSCROLL | WS_TABSTOP,
												120));
		break;
	case IDR_EDITBAR:
		// Only req. because they're on default edit bar (customisation handled by BCG)
		m_wndEditBar.ReplaceButton(ID_SEARCH, CFindComboButton());
		m_wndEditBar.ReplaceButton(ID_JUMP_HEX, CHexComboButton());
		m_wndEditBar.ReplaceButton(ID_JUMP_DEC, CDecComboButton());
		break;
	case IDR_FORMATBAR:
		{

			m_wndFormatBar.ReplaceButton(IDC_FONTNAME,
				CHexEditFontCombo(IDC_FONTNAME, 
					-1 /*CImageHash::GetImageOfCommand(IDC_FONTNAME, FALSE)*/,
					RASTER_FONTTYPE | TRUETYPE_FONTTYPE | DEVICE_FONTTYPE,
					DEFAULT_CHARSET,
					WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST |
					CBS_AUTOHSCROLL | CBS_HASSTRINGS | CBS_OWNERDRAWFIXED, 175) );

#if 0  // We need to call InsertButton else the size does not get updated properly for some reason
			m_wndFormatBar.ReplaceButton(IDC_FONTSIZE,
				CHexEditFontSizeCombo(IDC_FONTSIZE, 
					CImageHash::GetImageOfCommand(IDC_FONTSIZE, FALSE),
					WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWN, 50) );
#else
			int index = m_wndFormatBar.CommandToIndex(IDC_FONTSIZE);
			if (index == -1 || index > m_wndFormatBar.GetCount()) index = m_wndFormatBar.GetCount();
			m_wndFormatBar.RemoveButton(index);
			m_wndFormatBar.InsertButton(
				CHexEditFontSizeCombo(IDC_FONTSIZE, 
					-1 /*CImageHash::GetImageOfCommand(IDC_FONTSIZE, FALSE)*/,
					WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWN, 50),
				index);
#endif

			CMenu menu;
			menu.LoadMenu(IDR_MENUBUTTON);

#if 0  // No longer there (as drop-menus)
			m_wndFormatBar.ReplaceButton(ID_DISPLAY_DROPDOWN,
				CMFCToolBarMenuButton (-1, *menu.GetSubMenu(0),
					CImageHash::GetImageOfCommand(ID_DISPLAY_DROPDOWN), _T("Show Area")));
			m_wndFormatBar.ReplaceButton(ID_CHARSET_DROPDOWN,
				CMFCToolBarMenuButton (-1, *menu.GetSubMenu(1),
					CImageHash::GetImageOfCommand(ID_CHARSET_DROPDOWN), _T("Char Set")));
			m_wndFormatBar.ReplaceButton(ID_CONTROL_DROPDOWN,
				CMFCToolBarMenuButton (-1, *menu.GetSubMenu(2),
					CImageHash::GetImageOfCommand(ID_CONTROL_DROPDOWN), _T("Ctrl Chars")));
#endif
			m_wndFormatBar.ReplaceButton(ID_HIGHLIGHT_MENU,
				CMFCToolBarMenuButton(ID_HIGHLIGHT, *menu.GetSubMenu(3),
													  -1 /*CImageHash::GetImageOfCommand(ID_HIGHLIGHT)*/ ));

		}
		m_wndFormatBar.ReplaceButton(ID_SCHEME,
					  CMFCToolBarComboBoxButton(::IsUs() ? ID_SCHEME_US : ID_SCHEME, 
												-1,
												CBS_DROPDOWNLIST | CBS_AUTOHSCROLL | WS_VSCROLL | WS_TABSTOP,
												120));
		break;
	case IDR_NAVBAR:
		{
			m_wndNavBar.ReplaceButton(ID_JUMP_HEX, CHexComboButton());
			m_wndNavBar.ReplaceButton(ID_JUMP_DEC, CDecComboButton());
			m_wndNavBar.ReplaceButton(ID_BOOKMARKS, CBookmarksComboButton());

			CMenu menu;
			menu.LoadMenu(IDR_MENUBUTTON);
			m_wndNavBar.ReplaceButton(ID_MARK_MENU,
				CMFCToolBarMenuButton(ID_GOTO_MARK, *menu.GetSubMenu(4),
													  -1 /*CImageHash::GetImageOfCommand(ID_GOTO_MARK)*/ ));
			m_wndNavBar.ReplaceButton(ID_BOOKMARKS_MENU,
				CMFCToolBarMenuButton(ID_BOOKMARKS_EDIT, *menu.GetSubMenu(5),
													  -1 /*CImageHash::GetImageOfCommand(ID_BOOKMARKS_EDIT)*/ ));
			m_wndNavBar.ReplaceButton(ID_NAV_BACK,
				CMFCToolBarMenuButton(ID_NAV_BACK, *menu.GetSubMenu(6),
													  -1 /*CImageHash::GetImageOfCommand(ID_NAV_BACK)*/ ));
			m_wndNavBar.ReplaceButton(ID_NAV_FORW,
				CMFCToolBarMenuButton(ID_NAV_FORW, *menu.GetSubMenu(7),
													  -1 /*CImageHash::GetImageOfCommand(ID_NAV_FORW)*/ ));
		}
		break;
	case IDR_MAINFRAME:
		/* nothing here */
		break;
	default:
		ASSERT(0);
	}

	return 0;
}

LRESULT CMainFrame::OnHelpCustomizeToolbars(WPARAM wp, LPARAM lp)
{
//  CMFCToolBarsCustomizeDialog* pDlg = (CMFCToolBarsCustomizeDialog*)lp;
//  ASSERT_VALID (pDlg);
	DWORD help_id[] =
	{
		HIDD_CUST_COMMANDS,
		HIDD_CUST_TOOLBARS,
		HIDD_CUST_TOOLS,
		HIDD_CUST_KEYBOARD,
		HIDD_CUST_MENU,
		HIDD_CUST_MOUSE,
		HIDD_CUST_OPTIONS
	};

	if (!::HtmlHelp(m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, help_id[(int)wp]))
		AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);

	return 0;
}

void CMainFrame::OnClosePopupMenu(CMFCPopupMenu *pMenuPopup)
{
	bool found = false;
	while (!popup_menu_.empty() && !found)
	{
		found = popup_menu_.back() == pMenuPopup;
		popup_menu_.pop_back();
	}
	CMDIFrameWndEx::OnClosePopupMenu(pMenuPopup);
	menu_tip_.Hide();
}

BOOL CMainFrame::OnShowPopupMenu (CMFCPopupMenu *pMenuPopup)
{
	CMDIFrameWndEx::OnShowPopupMenu(pMenuPopup);

	if (pMenuPopup == NULL)
	{
		return TRUE;
	}
	popup_menu_.push_back(pMenuPopup);      // used for displaying menu item tips
	//TRACE("OPEN POPUP %p %x\n", pMenuPopup, pMenuPopup->m_hWnd);

	// Now we scan the menu for any "dummy" items that are placeholders for dynamic menu changes

	// All compare diffs navigation - only shown for hex view when there is a history of diffs
	int idx = pMenuPopup->GetMenuBar()->CommandToIndex(ID_COMP_ALL_DUMMY);
	if (idx >=0)
	{
		// Get active view and check if it is hex view
		ASSERT(AfxGetMainWnd() != NULL);
		CMDIChildWnd * pwind;
		CView * pview;

		if ((pwind = ((CMainFrame *)AfxGetMainWnd())->MDIGetActive()) != NULL &&
			(pview = pwind->GetActiveView()) != NULL &&
			pview->IsKindOf(RUNTIME_CLASS(CHexEditView)) )
		{
			// Add the 4 "all diffs" commands to the menu
			pMenuPopup->InsertItem(CMFCToolBarMenuButton(ID_COMP_ALL_FIRST, NULL, -1, "All First Difference"), idx+1);
			pMenuPopup->InsertItem(CMFCToolBarMenuButton(ID_COMP_ALL_PREV,  NULL, -1, "All Previous Difference"), idx+2);
			pMenuPopup->InsertItem(CMFCToolBarMenuButton(ID_COMP_ALL_NEXT,  NULL, -1, "All Next Difference"), idx+3);
			pMenuPopup->InsertItem(CMFCToolBarMenuButton(ID_COMP_ALL_LAST,  NULL, -1, "All Last Difference"), idx+4);
		}

		// Remove the dummy command from the menu
		pMenuPopup->RemoveItem(idx);
	}

	// Nav point lists forward/backward
	if (pMenuPopup->GetMenuBar()->CommandToIndex(ID_NAV_BACK_DUMMY) >= 0)
	{
		if (CMFCToolBar::IsCustomizeMode ())
			return FALSE;

		pMenuPopup->RemoveAllItems ();
		theApp.navman_.AddItems(pMenuPopup, false, ID_NAV_BACK_FIRST, NAV_RESERVED);
	}
	if (pMenuPopup->GetMenuBar()->CommandToIndex(ID_NAV_FORW_DUMMY) >= 0)
	{
		if (CMFCToolBar::IsCustomizeMode ())
			return FALSE;

		pMenuPopup->RemoveAllItems ();
		theApp.navman_.AddItems(pMenuPopup, true, ID_NAV_FORW_FIRST, NAV_RESERVED);
	}

	// 2 lists of templates - according to Windows extension (preceded by underscore) and others
	if (pMenuPopup->GetMenuBar()->CommandToIndex(ID_DFFD_OPEN_TYPE_DUMMY) >= 0)
	{
		if (CMFCToolBar::IsCustomizeMode ())
			return FALSE;

		pMenuPopup->RemoveAllItems ();

		int count = 0;
		for (int ii = 0; ii < theApp.xml_file_name_.size() && ii < DFFD_RESERVED; ++ii)
		{
			if (theApp.xml_file_name_[ii].CompareNoCase("default") != 0 &&
				theApp.xml_file_name_[ii].CompareNoCase("_windows_types") != 0 &&
				theApp.xml_file_name_[ii].CompareNoCase("_common_types") != 0 &&
				theApp.xml_file_name_[ii].CompareNoCase("_custom_types") != 0 &&
				theApp.xml_file_name_[ii].CompareNoCase("_standard_types") != 0 &&
				theApp.xml_file_name_[ii][0] == '_')
			{
				// Get type name based on the file extension
				CString ss = "." + theApp.xml_file_name_[ii].Mid(1);
				SHFILEINFO sfi;
				VERIFY(SHGetFileInfo(ss, FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi),
									SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME ));
				CString sext = sfi.szTypeName + CString(" (") + ss + _T(")");  // Store file type (+ ext) for display in file page

				pMenuPopup->InsertItem(CMFCToolBarMenuButton(ID_DFFD_OPEN_FIRST + ii, NULL, -1, sext));
				++count;
			}
		}
		if (count == 0)
		{
			CMFCToolBarMenuButton mb(ID_DFFD_OPEN_TYPE_DUMMY, NULL, -1, "No File Type Templates Found");
			mb.SetStyle(mb.m_nStyle | TBBS_DISABLED);
			pMenuPopup->InsertItem(mb);
		}
	}
	if (pMenuPopup->GetMenuBar()->CommandToIndex(ID_DFFD_OPEN_OTHER_DUMMY) >= 0)
	{
		if (CMFCToolBar::IsCustomizeMode ())
			return FALSE;

		pMenuPopup->RemoveAllItems ();

		int count = 0;
		for (int ii = 0; ii < theApp.xml_file_name_.size() && ii < DFFD_RESERVED; ++ii)
		{
			if (theApp.xml_file_name_[ii].CompareNoCase("default") != 0 &&
				theApp.xml_file_name_[ii][0] != '_')
			{
				pMenuPopup->InsertItem(CMFCToolBarMenuButton(ID_DFFD_OPEN_FIRST + ii, NULL, -1, theApp.xml_file_name_[ii]));
				++count;
			}
		}
		if (count == 0)
		{
			CMFCToolBarMenuButton mb(ID_DFFD_OPEN_TYPE_DUMMY, NULL, -1, "No Other Templates Found");
			mb.SetStyle(mb.m_nStyle | TBBS_DISABLED);
			pMenuPopup->InsertItem(mb);
		}
	}

	return TRUE;
}

void CMainFrame::OnSearchCombo()
{
	// We can't search if there is no file open
	if (GetView() == NULL)
		return;

	// Find the search combo control that is being used
	CMFCToolBarComboBoxButton* pFindCombo = NULL;
	CObList listButtons;
	if (CMFCToolBar::GetCommandButtons(ID_SEARCH_COMBO, listButtons) > 0)
	{
		for (POSITION posCombo = listButtons.GetHeadPosition();	posCombo != NULL;  )
		{
			CMFCToolBarComboBoxButton* pCombo = DYNAMIC_DOWNCAST(CMFCToolBarComboBoxButton, listButtons.GetNext(posCombo));

			if (pCombo != NULL && pCombo->GetEditCtrl()->GetSafeHwnd() == ::GetFocus())
			{
				pFindCombo = pCombo;
				break;
			}
		}
	}
	if (pFindCombo == NULL || !CMFCToolBar::IsLastCommandFromButton(pFindCombo))
		return;

	CString strFindText = pFindCombo->GetText();

	if (!strFindText.IsEmpty())
	{
		// Add to (or move to top of) combo list
		CComboBox* pCombo = pFindCombo->GetComboBox();
		const int nCount = pCombo->GetCount();
		CString strCmpText;

		// Check if it's already there and remove it if so, since it is added again at top
		for (int ind = 0; ind < nCount; ++ind)
		{
			pCombo->GetLBText(ind, strCmpText);

			if (strCmpText.GetLength() == strFindText.GetLength() &&
				strCmpText == strFindText)
			{
				pCombo->DeleteString(ind);
				break;
			}
		}

		pCombo->InsertString(0,strFindText);
		pCombo->SetCurSel(0);

		// Start the search
		SendMessage(WM_COMMAND, ID_FIND_NEXT);
		SetFocus();
	}
}

void CMainFrame::OnUpdateSearchCombo(CCmdUI* pCmdUI)
{
	if (GetView() == NULL || pCmdUI->m_pOther == NULL)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	CObList listButtons;

	// Find the search tool that is being updated
	if (CMFCToolBar::GetCommandButtons(ID_SEARCH_COMBO, listButtons) > 0)
	{
		for (POSITION posCombo = listButtons.GetHeadPosition(); posCombo != NULL; )
		{
			CFindComboButton* pCombo = 
				DYNAMIC_DOWNCAST(CFindComboButton, listButtons.GetNext(posCombo));
			ASSERT(pCombo != NULL);

			CFindComboBox *pfcb = DYNAMIC_DOWNCAST(CFindComboBox, pCombo->GetComboBox());
			ASSERT(pfcb != NULL && ::IsWindow(pfcb->GetSafeHwnd()));
			if (pfcb != NULL && ::IsWindow(pfcb->GetSafeHwnd()) && pCmdUI->m_pOther->m_hWnd == pfcb->GetSafeHwnd())
			{
				// If combo is not in drop down state and current combo strings are wrong then ...
				if (!pfcb->GetDroppedState() && ComboNeedsUpdate(search_hist_, pfcb))
				{
					// ... fix up items in the drop down list
					//CString strCurr;
					//pfcb->GetWindowText(strCurr);
					//DWORD sel = pfcb->GetEditSel();
					int max_str = 0;                // Max width of all the strings added so far
					CClientDC dc(pfcb);
					int nSave = dc.SaveDC();
					dc.SelectObject(pfcb->GetFont());

					pfcb->ResetContent();
					for (std::vector<CString>::iterator ps = search_hist_.begin();
						 ps != search_hist_.end(); ++ps)
					{
						max_str = __max(max_str, dc.GetTextExtent(*ps).cx);

						// Add the string to the list
						pfcb->InsertString(0, *ps);
					}
					//pfcb->SetWindowText(strCurr);
					//pfcb->SetEditSel(LOWORD(sel), HIWORD(sel));

					// Add space for margin and possible scrollbar
					max_str += dc.GetTextExtent("0").cx + ::GetSystemMetrics(SM_CXVSCROLL);
					pfcb->SetDroppedWidth(__min(max_str, 780));

					dc.RestoreDC(nSave);
				}

				// Fix up the edit control
				CString strCurr;
				CEdit *pedit = pCombo->GetEditCtrl();
				ASSERT(pedit != NULL);
				pedit->GetWindowText(strCurr);
				if (strCurr != current_search_string_)
					pedit->SetWindowText(current_search_string_);
				break;
			}
		}
	}

	pCmdUI->Enable(TRUE);
}

void CMainFrame::OnHexCombo()
{
	// We can't search if there is no file open
	if (GetView() == NULL)
		return;

	// Find the hex jump tool that was just activated
	CMFCToolBarComboBoxButton* pHexCombo = NULL;
	CObList listButtons;
	if (CMFCToolBar::GetCommandButtons(ID_JUMP_HEX_COMBO, listButtons) > 0)
	{
		for (POSITION posCombo = listButtons.GetHeadPosition(); pHexCombo == NULL && posCombo != NULL; )
		{
			CMFCToolBarComboBoxButton* pCombo = DYNAMIC_DOWNCAST(CMFCToolBarComboBoxButton, listButtons.GetNext(posCombo));

			if (pCombo != NULL && pCombo->GetEditCtrl()->GetSafeHwnd() == ::GetFocus())
			{
				pHexCombo = pCombo;
				break;
			}
		}
	}
	if (pHexCombo == NULL || !CMFCToolBar::IsLastCommandFromButton(pHexCombo))
		return;

	CString ss = pHexCombo->GetText();
	if (!ss.IsEmpty())
	{
		// Add to (or move to top of) combo list
		CComboBox* pCombo = pHexCombo->GetComboBox();
		const int nCount = pCombo->GetCount();
		CString strCmpText;

		for (int ind = 0; ind < nCount; ++ind)
		{
			pCombo->GetLBText(ind, strCmpText);

			if (strCmpText.GetLength() == ss.GetLength() && strCmpText == ss)
			{
				pCombo->DeleteString(ind);
				break;
			}
		}

		pCombo->InsertString(0, ss);
		pCombo->SetCurSel(0);

		// Force the jump
		AddHexHistory(ss);
		SendMessage(WM_COMMAND, ID_JUMP_HEX_START);
		SetFocus();
	}
}

void CMainFrame::OnUpdateHexCombo(CCmdUI* pCmdUI)
{
	// We can't do anything without a file open or a control
	if (GetView() == NULL || pCmdUI->m_pOther == NULL)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	CObList listButtons;

	// Find the hex jump tool that is being updated
	if (CMFCToolBar::GetCommandButtons(ID_JUMP_HEX_COMBO, listButtons) > 0)
	{
		for (POSITION posCombo = listButtons.GetHeadPosition(); posCombo != NULL; )
		{
			CHexComboButton* pbut = DYNAMIC_DOWNCAST(CHexComboButton, listButtons.GetNext(posCombo));
			ASSERT(pbut != NULL);

			CHexComboBox *phcb = DYNAMIC_DOWNCAST(CHexComboBox, pbut->GetComboBox());
			ASSERT(phcb != NULL && ::IsWindow(phcb->GetSafeHwnd()));
			if (phcb != NULL && pCmdUI->m_pOther->m_hWnd == phcb->GetSafeHwnd())
			{
				// If combo is not in drop down state and current combo strings are wrong then ...
				if (!phcb->GetDroppedState() && ComboNeedsUpdate(hex_hist_, phcb))
				{
					// ... fix up items in the drop down list (including setting drop list width from longest string)
					int max_str = 0;                // Max width of all the strings added so far
					CClientDC dc(phcb);
					int nSave = dc.SaveDC();
					dc.SelectObject(phcb->GetFont());

					phcb->ResetContent();
					for (std::vector<CString>::iterator ps = hex_hist_.begin();
						 ps != hex_hist_.end(); ++ps)
					{
						max_str = __max(max_str, dc.GetTextExtent(*ps).cx);

						// Add the string to the list
						phcb->InsertString(0, *ps);
					}

					// Add space for margin and possible scrollbar
					max_str += dc.GetTextExtent("0").cx + ::GetSystemMetrics(SM_CXVSCROLL);
					phcb->SetDroppedWidth(__min(max_str, 780));

					dc.RestoreDC(nSave);
				}

				// Fix up the edit control text (if needs it)
				CString strCurr;
				CHexEditControl * pedit = DYNAMIC_DOWNCAST(CHexEditControl, pbut->GetEditCtrl());
				ASSERT(pedit != NULL && pedit->IsKindOf(RUNTIME_CLASS(CHexEditControl)));
				pedit->GetWindowText(strCurr);

				int ac;
				CJumpExpr::value_t vv(-1);
				if (!strCurr.IsEmpty())
					vv = expr_.evaluate(strCurr, 0 /*unused*/, ac /*unused*/, 16 /*hex int*/);

				if (strCurr != current_hex_address_ && vv.typ == CJumpExpr::TYPE_INT && vv.int64 != current_address_)
				{
					char buf[22];
					if (theApp.hex_ucase_)
						sprintf(buf, "%I64X", __int64(current_address_));
					else
						sprintf(buf, "%I64x", __int64(current_address_));
					pedit->SetWindowText(buf);
					pedit->add_spaces();
				}
				break;
			}
		} // for
	}
	pCmdUI->Enable(TRUE);
}

void CMainFrame::OnDecCombo()
{
	// We can't search if there is no file open
	if (GetView() == NULL)
		return;

	// Find the dec jump combo control that was just activated
	CMFCToolBarComboBoxButton* pDecCombo = NULL;
	CObList listButtons;
	if (CMFCToolBar::GetCommandButtons(ID_JUMP_DEC_COMBO, listButtons) > 0)
	{
		for (POSITION posCombo = listButtons.GetHeadPosition(); posCombo != NULL; )
		{
			CMFCToolBarComboBoxButton* pCombo = DYNAMIC_DOWNCAST(CMFCToolBarComboBoxButton, listButtons.GetNext(posCombo));

			if (pCombo != NULL && pCombo->GetEditCtrl()->GetSafeHwnd() == ::GetFocus())
			{
				pDecCombo = pCombo;
				break;
			}
		}
	}
	if (pDecCombo == NULL || !CMFCToolBar::IsLastCommandFromButton(pDecCombo))
		return;

	CString ss = pDecCombo->GetText();
	if (!ss.IsEmpty())
	{
		// Add to (or move to top of) combo list
		CComboBox* pCombo = pDecCombo->GetComboBox();
		const int nCount = pCombo->GetCount();
		CString strCmpText;

		// Check if already there and remove it if so
		for (int ind = 0; ind < nCount; ++ind)
		{
			pCombo->GetLBText(ind, strCmpText);

			if (strCmpText.GetLength() == ss.GetLength() && strCmpText == ss)
			{
				pCombo->DeleteString(ind);
				break;
			}
		}

		pCombo->InsertString(0, ss);
		pCombo->SetCurSel(0);

		// Force the jump
		AddDecHistory(ss);
		SendMessage(WM_COMMAND, ID_JUMP_DEC_START);
		SetFocus();
	}
}

void CMainFrame::OnUpdateDecCombo(CCmdUI* pCmdUI)
{
	// We can't do anything without a file open or a control
	if (GetView() == NULL || pCmdUI->m_pOther == NULL)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	CObList listButtons;

	// Find the hex jump tool that is being updated
	if (CMFCToolBar::GetCommandButtons(ID_JUMP_DEC_COMBO, listButtons) > 0)
	{
		for (POSITION posCombo = listButtons.GetHeadPosition(); posCombo != NULL; )
		{
			CDecComboButton* pbut = DYNAMIC_DOWNCAST(CDecComboButton, listButtons.GetNext(posCombo));
			ASSERT(pbut != NULL);

			CDecComboBox *pdcb = DYNAMIC_DOWNCAST(CDecComboBox, pbut->GetComboBox());
			ASSERT(pdcb != NULL && ::IsWindow(pdcb->GetSafeHwnd()));
			if (pdcb != NULL && pCmdUI->m_pOther->m_hWnd == pdcb->GetSafeHwnd())
			{
				// If combo is not in drop down state and current combo strings are wrong then ...
				if (!pdcb->GetDroppedState() && ComboNeedsUpdate(dec_hist_, pdcb))
				{
					// ... fix up items in the drop down list (including setting drop list width from longest string)
					int max_str = 0;                // Max width of all the strings added so far
					CClientDC dc(pdcb);
					int nSave = dc.SaveDC();
					dc.SelectObject(pdcb->GetFont());

					pdcb->ResetContent();
					for (std::vector<CString>::iterator ps = dec_hist_.begin();
						 ps != dec_hist_.end(); ++ps)
					{
						max_str = __max(max_str, dc.GetTextExtent(*ps).cx);

						// Add the string to the list
						pdcb->InsertString(0, *ps);
					}

					// Add space for margin and possible scrollbar
					max_str += dc.GetTextExtent("0").cx + ::GetSystemMetrics(SM_CXVSCROLL);
					pdcb->SetDroppedWidth(__min(max_str, 780));

					dc.RestoreDC(nSave);
				}

				// Fix up the edit control text (if needs it)
				CString strCurr;
				CDecEditControl * pedit = DYNAMIC_DOWNCAST(CDecEditControl, pbut->GetEditCtrl());
				ASSERT(pedit != NULL && pedit->IsKindOf(RUNTIME_CLASS(CDecEditControl)));
				pedit->GetWindowText(strCurr);

				int ac;
				CJumpExpr::value_t vv(-1);
				if (!strCurr.IsEmpty())
					vv = expr_.evaluate(strCurr, 0 /*unused*/, ac /*unused*/, 10 /*decimal int*/);

				if (strCurr != current_dec_address_ && vv.typ == CJumpExpr::TYPE_INT && vv.int64 != current_address_)
				{
					char buf[22];
					sprintf(buf, "%I64d", __int64(current_address_));
					pedit->SetWindowText(buf);
					pedit->add_commas();
				}
				break;
			}
		} // for
	}
	pCmdUI->Enable(TRUE);
}

// We need a case-insensitive search that works the same as CBS_SORT
struct case_insensitive_greater : binary_function<CString, CString, bool>
{
	bool operator()(const CString &s1, const CString &s2) const
	{
		return s1.CompareNoCase(s2) > 0;
	}
};

void CMainFrame::OnUpdateBookmarksCombo(CCmdUI* pCmdUI)
{
	CHexEditView *pview = GetView();
	if (pview == NULL || pCmdUI->m_pOther == NULL)
	{
		// If there is no file open or we don't know the control that we are updating we can't do anything
		pCmdUI->Enable(FALSE);
		return;
	}
	CHexEditDoc *pdoc = (CHexEditDoc *)pview->GetDocument();  // we need the current doc later to get its list of bookmarks

	CObList listButtons;

	// Find the bookmarks tool that is being updated
	if (CMFCToolBar::GetCommandButtons(ID_BOOKMARKS_COMBO, listButtons) > 0)
	{
		for (POSITION posCombo = listButtons.GetHeadPosition(); posCombo != NULL; )
		{
			// Get the toolbar "control"
			CBookmarksComboButton * pCombo = 
				DYNAMIC_DOWNCAST(CBookmarksComboButton, listButtons.GetNext(posCombo));
			ASSERT(pCombo != NULL);

			// Get the associated combo box
			CBookmarksComboBox * pbcb = DYNAMIC_DOWNCAST(CBookmarksComboBox, pCombo->GetComboBox());
			ASSERT(pbcb != NULL && ::IsWindow(pbcb->GetSafeHwnd()));

			// Check that the combo box is the one that is being updated
			if (pbcb != NULL && ::IsWindow(pbcb->GetSafeHwnd()) && pCmdUI->m_pOther->m_hWnd == pbcb->GetSafeHwnd())
			{
				// Handle special case of file with no bookmarks
				ASSERT(pview != NULL && pdoc != NULL);
				if (pdoc->bm_index_.empty())
				{
					CString ss;
					//if (pbcb->GetCount() > 0) pbcb->GetLBText(0, ss);
					if (pCombo->GetCount() > 0)
						ss = pCombo->GetText();

					if (pCombo->GetCount() != 1 || ss != CString("No bookmarks"))
					{
						pCombo->RemoveAllItems();
						pCombo->AddItem("No bookmarks");
						pCombo->SelectItem(0, FALSE);
						pCombo->NotifyCommand(CBN_SELENDOK);  // this is needed so text is updated
					}
					pCmdUI->Enable(FALSE);
					return;
				}

				CBookmarkList *pbl = theApp.GetBookmarkList();

				// Get index of the closest bookmark above the current position in the view
				FILE_ADDRESS diff;  // unused
				int current_bm = pview->ClosestBookmark(diff);  // closest bookmark or -1
				CString strCurrent;                             // name of closest bookmark
				if (current_bm > -1)
					strCurrent = pbl->name_[current_bm];

				// Get vector of names of all bookmarks in the current file
				std::vector<CString> bm_names;
				for (int ii = 0; ii < (int)pdoc->bm_index_.size(); ++ii)
				{
					ASSERT(pdoc->bm_index_[ii] < (int)pbl->name_.size());
					bm_names.push_back(pbl->name_[pdoc->bm_index_[ii]]);
				}

				// We must sort since the combo box sorts the bookmarks alphabetically (CBS_SORT)
				sort(bm_names.begin(), bm_names.end(), case_insensitive_greater());

				// Fix up the drop down list
				if (!pbcb->GetDroppedState() && ComboNeedsUpdate(bm_names, pbcb))
				{
					int max_str = 0;                // Max width of all the strings added so far
					CClientDC dc(pbcb);
					int nSave = dc.SaveDC();
					dc.SelectObject(pbcb->GetFont());

					// Add all the bookmarks in this document to the list
					pCombo->RemoveAllItems();
					for (int ii = 0; ii < (int)pdoc->bm_index_.size(); ++ii)
					{
						ASSERT(pdoc->bm_index_[ii] < (int)pbl->name_.size());
						max_str = __max(max_str, dc.GetTextExtent(pbl->name_[pdoc->bm_index_[ii]]).cx);
						pCombo->AddItem(pbl->name_[pdoc->bm_index_[ii]] /*, pdoc->bm_index_[ii] */);
					}

					// Add space for margin and possible scrollbar
					max_str += dc.GetTextExtent("0").cx + ::GetSystemMetrics(SM_CXVSCROLL);
					pbcb->SetDroppedWidth(__min(max_str, 640));

					dc.RestoreDC(nSave);

					// Set the current text to an invalid bookmark name (a space) - this is necessary as
					// the current text will be the last name added above but is not displayed in the
					// toolbar (only in the hidden combo box).  We need to do this so pCombo->GetText()
					// does not compare equal with strCurrent below and the toolbar text is updated.
					pCombo->SetText(" ");
				}

				// If the closest bookmark is wrong (and combo is not in dropped state) ...
				if (!pbcb->GetDroppedState() && pCombo->GetText() != strCurrent)
				{
					// Update the text with the closest bookmark
					if (current_bm == -1)
					{
						// Deselect any combo box item (shows blank in toolbar)
						pCombo->SelectItem(-1, FALSE);
						pCombo->NotifyCommand(CBN_SELENDOK);  // this is needed so the previous text is removed
					}
					else
					{
						// Search through the sorted combo list to find the associated entry
						for (int ii = 0; ii < pbcb->GetCount(); ++ii)
						{
							CString ss;
							pbcb->GetLBText(ii, ss);
							if (ss == strCurrent)
							{
								// Found it so show it and that's all we need to do
								pCombo->SelectItem(ii, FALSE);
								pCombo->NotifyCommand(CBN_SELENDOK);  // needed to make sure the text is displayed
								break;
							}
						}
					}
				}
			}
		}
	}

	pCmdUI->Enable(TRUE);
}

BOOL CMainFrame::ComboNeedsUpdate(const std::vector<CString> &vs, CComboBox *pp)
{
	int num_elts = pp->GetCount();

	if (num_elts != vs.size())
		return TRUE;

	CString ss;
	int ii;
	for (ii = 0; ii < num_elts; ++ii)
	{
		pp->GetLBText(ii, ss);
		if (vs[num_elts-ii-1] != ss)
			break;
	}

	// Check if list box is different in any way
	return ii != num_elts;
}

void CMainFrame::OnApplicationLook(UINT id)
{
	CWaitCursor wait;

	theApp.m_nAppLook = id;

	switch (theApp.m_nAppLook)
	{
	case ID_VIEW_APPLOOK_WIN_2000:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManager));
		break;

	case ID_VIEW_APPLOOK_OFF_XP:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOfficeXP));
		break;

	case ID_VIEW_APPLOOK_WIN_XP:
		CMFCVisualManagerWindows::m_b3DTabsXPTheme = TRUE;
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
		break;

	case ID_VIEW_APPLOOK_OFF_2003:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2003));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case ID_VIEW_APPLOOK_VS_2005:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2005));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	default:
		switch (theApp.m_nAppLook)
		{
		case ID_VIEW_APPLOOK_OFF_2007_BLUE:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_LunaBlue);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_BLACK:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_ObsidianBlack);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_SILVER:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Silver);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_AQUA:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Aqua);
			break;
		}

		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));
		CDockingManager::SetDockingMode(DT_SMART);
	}

	RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);

	theApp.WriteInt(_T("ApplicationLook"), theApp.m_nAppLook);
}

void CMainFrame::OnUpdateApplicationLook(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(theApp.m_nAppLook == pCmdUI->m_nID);
}

// Pass -1 to turn off progress, or a value from 0 to 100 to update the progress.
void CMainFrame::Progress(int value)
{
	if (value < 0)
	{
		if (progress_on_)
		{
			// Turn of progress
			m_wndStatusBar.EnablePaneProgressBar(0, -1);  // disable progress bar (pane 0)
			progress_on_ = false;
		}
	}
	else
	{
		if (!progress_on_)
		{
			// Turn on progress
			m_wndStatusBar.EnablePaneProgressBar(0);  // Turn on progress in left pane (pane 0)
			progress_on_ = true;
		}

		// Update progress
		m_wndStatusBar.SetPaneProgress(0, value < 100 ? value : 100);
	}
}

// Just for testing things (invoked with Ctrl+Shift+T)
void CMainFrame::OnTest()
{
//    m_wndSplitter.Flip();
}

/////////////////////////////////////////////////////////////////////////////
// CJumpExpr - override of expr_eval that stores values for whole program

// find_symbol just looks up a bookmark and returns the value as TYPE_INT.
// Most of the parameters are not relevant (used for template expressions):
// sym is the name of the bookmark.  All other params are ignored.

CJumpExpr::value_t CJumpExpr::find_symbol(const char *sym, value_t parent, size_t index, int *pac,
	__int64 &sym_size, __int64 &sym_address, CString &sym_str)
{
	value_t retval;
	CHexEditView *pview;

	retval.typ = TYPE_NONE;             // Default to symbol not found
	retval.error = false;
	retval.int64 = 0;

	sym_address = 0;                    // Put something here
	sym_size = 8;                       // bookmark = address, so why not 8 bytes

	if (parent.typ == TYPE_NONE &&
		(pview = GetView()) != NULL)
	{
		ASSERT(pview->IsKindOf(RUNTIME_CLASS(CHexEditView)));
		CHexEditDoc *pdoc = pview->GetDocument();
		ASSERT(pdoc != NULL && pdoc->IsKindOf(RUNTIME_CLASS(CHexEditDoc)));

		// Get the global bookmark list and check all the docs bookmarks
		CBookmarkList *pbl = theApp.GetBookmarkList();
		for (int ii = 0; ii < (int)pdoc->bm_index_.size(); ++ii)
		{
			if (sym == pbl->name_[pdoc->bm_index_[ii]])
			{
				retval.typ = TYPE_INT;
				retval.int64 = pdoc->bm_posn_[ii];
				break;
			}
		}
	}

	return retval;
}

bool CJumpExpr::LoadVars()
{
	bool retval = true;

	CString vars = theApp.GetProfileString("Calculator", "Vars");
	CString ss;

	for (const char *pp = vars; *pp != '\0'; ++pp)
	{
		const char *pp2;

		// Get next var name
		pp2 = strchr(pp, '=');
		if (pp2 == NULL)
		{
			retval = false;
			if ((pp = strchr(pp, ';')) != NULL)
				continue;
			else
				return false;
		}
		CString name(pp, pp2 - pp);
		if (pp2 - pp >
			strspn(name, "abcdefghijklmonpqrstuvwxyz"
						 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
						 "0123456789_$[]"))             // [] are used in "array element" names
		{
			// Invalid var name
			retval = false;
			if ((pp = strchr(pp, ';')) != NULL)
				continue;
			else
				return false;
		}
		name.MakeUpper();
		pp = pp2 + 1;      // Move pp after '='

		// Get the value
		value_t tmp;
		switch (*pp)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '-':
			tmp.typ = TYPE_INT;
			tmp.int64 = ::strtoi64(pp, 10, &pp2);
			break;

		case 'R':
			++pp;
			tmp = value_t(strtod(pp, (char **)&pp2));
			break;

		case 'D':
			++pp;
			tmp.typ = TYPE_DATE;
			tmp.date = strtod(pp, (char **)&pp2);
			break;

		case '\"':
			// Find end of string (unescaped double quote)
			ss.Empty();
			for (;;)
			{
				pp2 = pp + 1;
				pp = strchr(pp2, '"');
				if (pp == NULL)
					return false;           // there should be a terminating double-quote character
				if (*(pp-1) == '\\')
					ss += CString(pp2, pp - pp2 - 1) + "\"";  // Escaped double-quote
				else
				{
					ss += CString(pp2, pp - pp2);  // End of string
					break;
				}
			}
			pp2 = pp + 1;
			tmp = value_t(ss);
			break;

		case 'T':
			if (strncmp(pp, "TRUE", 4) != 0)
			{
				retval = false;
				if ((pp = strchr(pp, ';')) != NULL)
					continue;
				else
					return false;
			}
			tmp = value_t(true);
			pp2 = pp + 4;
			break;
		case 'F':
			if (strncmp(pp, "FALSE", 5) != 0)
			{
				retval = false;
				if ((pp = strchr(pp, ';')) != NULL)
					continue;
				else
					return false;
			}
			tmp = value_t(false);
			pp2 = pp + 5;
			break;

		default:
			retval = false;
			if ((pp = strchr(pp, ';')) != NULL)
				continue;
			else
				return false;
		}
		var_[name] = tmp;
		var_changed_ = clock();

		// Make sure we see terminator (;)
		if (*pp2 != ';')
		{
			retval = false;
			if ((pp2 = strchr(pp2, ';')) == NULL)
				return false;
		}
		pp = pp2;
	}
	return retval;
}

// TBD: TODO save in a Unicode string since a string var is Unicode (if UNICODE_TYPE_STRING defined)
void CJumpExpr::SaveVars()
{
	CString vars;               // All vars as text, separated by semicolon
	char buf[22];               // For formating values as text
	CString ss;

	for (std::map<CString, value_t>::const_iterator pp = var_.begin();
		 pp != var_.end(); ++pp)
	{
		if (pp->second.typ > TYPE_NONE && pp->second.typ <= TYPE_STRING)
			vars += pp->first + CString("=");
		switch (pp->second.typ)
		{
		case TYPE_NONE:
			// Just ignore these
			break;
		case TYPE_BOOLEAN:
			vars += pp->second.boolean ? "TRUE;" : "FALSE;";
			break;
		case TYPE_INT:
			sprintf(buf, "%I64d;", __int64(pp->second.int64));
			vars += buf;
			break;
		case TYPE_REAL:
			sprintf(buf, "R%.14g;", double(pp->second.real64));
			vars += buf;
			break;
		case TYPE_DATE:
			sprintf(buf, "D%g;", double(pp->second.date));
			vars += buf;
			break;
		case TYPE_STRING:
			ss = *pp->second.pstr;      // get copy of string
			ss.Replace("\"", "\\\"");   // escape double quotes
			vars += "\"" + ss + "\";";
			break;
		default:
			ASSERT(0);
		}
	}

	theApp.WriteProfileString("Calculator", "Vars", vars);
}

// Get the names of all variables of a certain type
vector<CString> CJumpExpr::GetVarNames(CJumpExpr::type_t typ)
{
	vector<CString> retval;

	for (std::map<CString, value_t>::const_iterator pp = var_.begin();
		 pp != var_.end(); ++pp)
	{
		if (typ == TYPE_NONE || typ == pp->second.typ)
			retval.push_back(pp->first);
	}

	return retval;
}
