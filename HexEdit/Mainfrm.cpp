// MainFrm.cpp : implementation of the CMainFrame class
//
// Copyright (c) 2004 by Andrew W. Phillips.
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
#include "Boyer.h"
#include "SystemSound.h"
#include "Misc.h"
#include "BCGMisc.h"
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

IMPLEMENT_SERIAL(CHexEditFontCombo, CBCGToolbarFontCombo, 1)  // see BCGMisc.h
IMPLEMENT_SERIAL(CHexEditFontSizeCombo, CBCGToolbarFontSizeCombo, 1)  // see BCGMisc.h

static const int max_search_hist = 48;
static const int max_replace_hist = 16;
static const int max_jump_hist = 16;

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CBCGMDIFrameWnd)

// static UINT WM_FINDREPLACE = ::RegisterWindowMessage(FINDMSGSTRING);

BEGIN_MESSAGE_MAP(CMainFrame, CBCGMDIFrameWnd)
        ON_WM_INITMENU()
        //{{AFX_MSG_MAP(CMainFrame)
        ON_WM_CREATE()
        ON_WM_CLOSE()
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
        ON_COMMAND(ID_VIEW_VIEWBAR, OnViewViewbar)
        ON_UPDATE_COMMAND_UI(ID_VIEW_VIEWBAR, OnUpdateViewViewbar)
        ON_COMMAND(ID_VIEW_EDITBAR, OnViewEditbar)
        ON_UPDATE_COMMAND_UI(ID_VIEW_EDITBAR, OnUpdateViewEditbar)
        ON_COMMAND(ID_VIEW_FORMATBAR, OnViewFormatbar)
        ON_UPDATE_COMMAND_UI(ID_VIEW_FORMATBAR, OnUpdateViewFormatbar)
        ON_COMMAND(ID_VIEW_NAVBAR, OnViewNavbar)
        ON_UPDATE_COMMAND_UI(ID_VIEW_NAVBAR, OnUpdateViewNavbar)
        ON_COMMAND(ID_VIEW_TOOLBAR, OnViewToolbar)
        ON_UPDATE_COMMAND_UI(ID_VIEW_TOOLBAR, OnUpdateViewToolbar)

        ON_COMMAND(ID_VIEW_CALCULATOR, OnViewCalculator)
        ON_UPDATE_COMMAND_UI(ID_VIEW_CALCULATOR, OnUpdateViewCalculator)
        ON_COMMAND(ID_VIEW_BOOKMARKS, OnViewBookmarks)
        ON_UPDATE_COMMAND_UI(ID_VIEW_BOOKMARKS, OnUpdateViewBookmarks)
        ON_COMMAND(ID_VIEW_FIND, OnViewFind)
        ON_UPDATE_COMMAND_UI(ID_VIEW_FIND, OnUpdateViewFind)
#ifdef EXPLORER_WND
        ON_COMMAND(ID_VIEW_EXPL, OnViewExpl)
        ON_UPDATE_COMMAND_UI(ID_VIEW_EXPL, OnUpdateViewExpl)
#endif
        ON_COMMAND(ID_VIEW_PROPERTIES, OnViewProperties)
        ON_UPDATE_COMMAND_UI(ID_VIEW_PROPERTIES, OnUpdateViewProperties)

        ON_COMMAND_EX(ID_VIEW_STATUS_BAR, OnBarCheck)
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

//        ON_REGISTERED_MESSAGE(WM_FINDREPLACE, OnFindDlgMess)
//        ON_MESSAGE(WM_USER, OnReturn)

        // BCG stuff
        ON_REGISTERED_MESSAGE(BCGM_RESETTOOLBAR, OnToolbarReset)
        ON_REGISTERED_MESSAGE(BCGM_RESETMENU, OnMenuReset)
        ON_REGISTERED_MESSAGE(BCGM_TOOLBARMENU, OnToolbarContextMenu)
        ON_COMMAND_EX_RANGE(ID_VIEW_USER_TOOLBAR1, ID_VIEW_USER_TOOLBAR10, OnToolsViewUserToolbar)
        ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_USER_TOOLBAR1, ID_VIEW_USER_TOOLBAR10, OnUpdateToolsViewUserToolbar)
        ON_REGISTERED_MESSAGE(BCGM_CUSTOMIZEHELP, OnHelpCustomizeToolbars)
        ON_COMMAND(ID_WINDOW_MANAGER, ShowWindowsDialog)

        // When vertically docked the combobox reverts to a button and sends this command when clicked
        ON_COMMAND(ID_SEARCH_COMBO, OnEditFind)
        ON_UPDATE_COMMAND_UI(ID_SEARCH_COMBO, OnUpdateSearchCombo)
        ON_COMMAND(ID_JUMP_HEX_COMBO, OnEditGotoHex)
        ON_UPDATE_COMMAND_UI(ID_JUMP_HEX_COMBO, OnUpdateHexCombo)
        ON_COMMAND(ID_JUMP_DEC_COMBO, OnEditGotoDec)
        ON_UPDATE_COMMAND_UI(ID_JUMP_DEC_COMBO, OnUpdateDecCombo)
        ON_COMMAND(ID_SCHEME_COMBO, OnOptionsScheme)
        ON_UPDATE_COMMAND_UI(ID_SCHEME_COMBO, OnUpdateSchemeCombo)
        ON_COMMAND(ID_SCHEME_COMBO_US, OnOptionsScheme)
        ON_UPDATE_COMMAND_UI(ID_SCHEME_COMBO_US, OnUpdateSchemeCombo)
        ON_COMMAND(ID_BOOKMARKS_COMBO, OnBookmarks)
        ON_UPDATE_COMMAND_UI(ID_BOOKMARKS_COMBO, OnUpdateBookmarksCombo)

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
        ON_COMMAND(ID_DIALOGS_DOCKABLE, OnDockableToggle)
        ON_UPDATE_COMMAND_UI(ID_DIALOGS_DOCKABLE, OnUpdateDockableToggle)

        ON_COMMAND(ID_NAV_BACK, OnNavigateBackwards)
        ON_UPDATE_COMMAND_UI(ID_NAV_BACK, OnUpdateNavigateBackwards)
        ON_COMMAND_RANGE(ID_NAV_BACK_FIRST, ID_NAV_BACK_FIRST+NAV_RESERVED-1, OnNavBack)
        ON_COMMAND(ID_NAV_FORW, OnNavigateForwards)
        ON_UPDATE_COMMAND_UI(ID_NAV_FORW, OnUpdateNavigateForwards)
        ON_COMMAND_RANGE(ID_NAV_FORW_FIRST, ID_NAV_FORW_FIRST+NAV_RESERVED-1, OnNavForw)

        ON_COMMAND(ID_TEST, OnTest)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
    //pfind_ = NULL;
    //pprop_ = NULL;
    //pcalc_ = NULL;
    preview_page_ = -1;
//    timer_id_ = 0;
#ifdef DRAW_BACKGROUND
	CString filename;
	if (::GetDataPath(filename))
		filename += FILENAME_BACKGROUND;
	else
		filename = GetExePath() + FILENAME_BACKGROUND;

	CString bgfile = theApp.GetProfileString("MainFrame", "BackgroundFileName", filename);
	if (!m_background.LoadImage(bgfile))
		m_background.LoadImage(GetExePath() + FILENAME_BACKGROUND);
	m_background_pos = theApp.GetProfileInt("MainFrame", "BackgroundPosition", 4); // dv = bottom-right
#endif
	m_search_image.LoadBitmap(IDB_SEARCH);
	OccurrencesWidth = ValuesWidth = AddrHexWidth = AddrDecWidth = FileLengthWidth = -999;
    bg_progress_enabled_ = false;
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
        CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

        if (CBCGMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
                return -1;

        //menu_tip_.SetParent(this);

        EnableDocking(CBRS_ALIGN_ANY);

        // Create BCG menu bar
        if (!m_wndMenuBar.Create(this))
        {
                TRACE0("Failed to create a menu bar\n");
                return -1;      // fail to create
        }
        m_wndMenuBar.SetBarStyle(m_wndMenuBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
        m_wndMenuBar.EnableDocking(CBRS_ALIGN_ANY);
        DockControlBar(&m_wndMenuBar);

        // Create Tool bar
        if (!m_wndBar1.Create(this, dwDefaultToolbarStyle, IDR_STDBAR) || 
            !m_wndBar1.LoadToolBar(IDR_STDBAR))
        {
            TRACE0("Failed to create Standard Toolbar\n");
            return -1;      // fail to create
        }
        m_wndBar1.SetBarStyle(m_wndBar1.GetBarStyle() |
            CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
        m_wndBar1.SetWindowText("Standard Toolbar");
        m_wndBar1.EnableDocking(CBRS_ALIGN_ANY);
//        m_wndBar1.EnableCustomizeButton(TRUE, ID_CUSTOMIZE, _T("Customize..."));
//        m_wndBar1.EnableTextLabels();

        // Create "edit" bar
        if (!m_wndBar2.Create(this, dwDefaultToolbarStyle, IDR_EDITBAR) ||
            !m_wndBar2.LoadToolBar(IDR_EDITBAR))
        {
            TRACE0("Failed to create Edit Bar\n");
            return -1;      // fail to create
        }
        m_wndBar2.SetBarStyle(m_wndBar2.GetBarStyle() |
            CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
        m_wndBar2.SetWindowText("Edit Bar");
        m_wndBar2.EnableDocking(CBRS_ALIGN_ANY);

        // Create Format bar
        if (!m_wndBar3.Create(this, dwDefaultToolbarStyle, IDR_FORMATBAR) ||
            !m_wndBar3.LoadToolBar(IDR_FORMATBAR))
        {
            TRACE0("Failed to create Format Bar\n");
            return -1;      // fail to create
        }
        m_wndBar3.SetBarStyle(m_wndBar3.GetBarStyle() |
            CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
        m_wndBar3.SetWindowText("Format Bar");
        m_wndBar3.EnableDocking(CBRS_ALIGN_ANY);
        m_wndBar3.ShowWindow(SW_HIDE);

        // Create Navigation bar
        if (!m_wndBar4.Create(this, dwDefaultToolbarStyle, IDR_NAVBAR) ||
            !m_wndBar4.LoadToolBar(IDR_NAVBAR))
        {
            TRACE0("Failed to create Nav Bar\n");
            return -1;      // fail to create
        }
        m_wndBar4.SetBarStyle(m_wndBar4.GetBarStyle() |
            CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
        m_wndBar4.SetWindowText("Navigation Bar");
        m_wndBar4.EnableDocking(CBRS_ALIGN_ANY);
        m_wndBar4.ShowWindow(SW_HIDE);

        // Create Tool bar
        if (!m_wndBarOld.Create(this, dwDefaultToolbarStyle, IDR_MAINFRAME) || 
            !m_wndBarOld.LoadToolBar(IDR_MAINFRAME))
        {
            TRACE0("Failed to create Tool Bar\n");
            return -1;      // fail to create
        }
        m_wndBarOld.SetBarStyle(m_wndBarOld.GetBarStyle() |
            CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
        m_wndBarOld.SetWindowText("Old Tool Bar");
        m_wndBarOld.EnableDocking(CBRS_ALIGN_ANY);
        m_wndBarOld.ShowWindow(SW_HIDE);
//        m_wndBar1.EnableCustomizeButton(TRUE, ID_CUSTOMIZE, _T("Customize..."));
//        m_wndBar1.EnableTextLabels();

		dockable_ = (BOOL)theApp.GetProfileInt("MainFrame", "DockableDialogs", 0) > 0 ? TRUE : FALSE;

        if (!m_wndCalc.Create(this) ||
			!m_wndBookmarks.Create(this) ||
			!m_wndFind.Create(this) ||
#ifdef EXPLORER_WND
			!m_wndExpl.Create(this) ||
#endif
            !m_wndProp.Create(this) )
        {
            TRACE0("Failed to create bookmarks/find/calc/prop window\n");
            return -1;
        }

        CSize tmp_size;

        m_wndCalc.SetWindowText("Calculator");
        m_wndCalc.EnableRollUp();
        m_wndCalc.EnableDocking(dockable_ ? CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT : 0);
        m_wndCalc.SetCaptionStyle(TRUE);
        tmp_size = m_wndCalc.m_sizeInitial;
		ASSERT(tmp_size.cx > -1 && tmp_size.cy > -1);
		tmp_size.cx = tmp_size.cx*3/2; tmp_size.cy = tmp_size.cy*3/2;
        m_wndCalc.SetMaxSize(tmp_size);
		tmp_size.cx = tmp_size.cx*2/3;
		tmp_size.cy = tmp_size.cy/2;
        m_wndCalc.SetMinSize(tmp_size);

        m_wndBookmarks.SetWindowText("Bookmarks");
        m_wndBookmarks.EnableRollUp();
        m_wndBookmarks.EnableDocking(dockable_ ? CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT : 0);
        m_wndBookmarks.SetCaptionStyle(TRUE);

        tmp_size = m_wndBookmarks.m_sizeInitial;
		ASSERT(tmp_size.cx > -1 && tmp_size.cy > -1);
        tmp_size.cx /= 2;
        m_wndBookmarks.SetMinSize(tmp_size);

        m_wndFind.SetWindowText("Find");
        m_wndFind.EnableRollUp();
        m_wndFind.EnableDocking(dockable_ ? CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT : 0);
        m_wndFind.SetCaptionStyle(TRUE);

#ifdef EXPLORER_WND
        // Set up the window
        m_wndExpl.SetWindowText("HexEdit Explorer");
        m_wndExpl.EnableRollUp();
        m_wndExpl.EnableDocking(dockable_ ? CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT : 0);
        m_wndExpl.SetCaptionStyle(TRUE);
#endif

        m_wndProp.SetWindowText("Properties");
        m_wndProp.EnableRollUp();
        m_wndProp.EnableDocking(dockable_ ? CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT : 0);
        m_wndProp.SetCaptionStyle(TRUE);

        DockControlBar(&m_wndBar1);
        DockControlBar(&m_wndBar2);
        DockControlBar(&m_wndBar3);
        DockControlBar(&m_wndBar4);
        DockControlBar(&m_wndBarOld);

        // Get extra command images (without creating a toolbar)
        CBCGToolBar::AddToolBarForImageCollection(IDR_MISC);

#if 0
        // Find user toolbar images file -  check old locn 1st (mac dir)
        ASSERT(aa->mac_dir_.Right(1) == "\\");
        m_strImagesFileName = aa->mac_dir_ + "ToolbarImages.bmp";

        if (_access(m_strImagesFileName, 0) == -1)
        {
            // Not found so try HexEdit part of user's Application Data area
            if (::GetDataPath(m_strImagesFileName))
            {
                // Create HexEdit folder within that
                m_strImagesFileName += "ToolbarImages.bmp";
            }
        }
#endif
		CString strDefault = ::GetExePath() + "DefaultToolbarImages.bmp";
		::GetDataPath(m_strImagesFileName);

		if (!m_strImagesFileName.IsEmpty())
		{
			m_strImagesFileName += "ToolbarImages.bmp";

			// Check if the file is there and create it if not
			CFileFind ff;
			if (!ff.FindFile(m_strImagesFileName))
			{
				::MakeSureDirectoryPathExists(m_strImagesFileName);

				if (!::CopyFile(strDefault, m_strImagesFileName, TRUE))
				{
					if (GetLastError() == ERROR_PATH_NOT_FOUND)
					{
						AfxMessageBox("The HexEdit Application Data folder is invalid");
					    m_strImagesFileName = strDefault;
					}
					else
					{
						ASSERT(GetLastError() == ERROR_FILE_NOT_FOUND);
						AfxMessageBox("The default toolbar images file was not found in the HexEdit folder.");
					}
				}
			}
		}
		else
		{
			AfxMessageBox("The HexEdit Application Data folder is not found");
			m_strImagesFileName = strDefault;
		}

        // Open images files and allow user to use/edit toolbar images
        VERIFY(m_UserImages.Load(m_strImagesFileName));
        if (m_UserImages.IsValid())
            CBCGToolBar::SetUserImages (&m_UserImages);

        CBCGToolBar::EnableQuickCustomization();
        InitUserToobars(NULL,
                        ID_VIEW_USER_TOOLBAR1,
                        ID_VIEW_USER_TOOLBAR10);
        EnableWindowsDialog (ID_WINDOW_MANAGER, _T("Windows..."), TRUE);

        if (!m_wndStatusBar.Create(this) ||
            !m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(*indicators)))
        {
                TRACE0("Failed to create status bar\n");
                return -1;      // fail to create
        }
		m_wndStatusBar.SetToolTips();
		if (HBITMAP(m_search_image) != 0)
			m_wndStatusBar.SetPaneIcon(1, HBITMAP(m_search_image), RGB(255,255,255));
        for (int pane = 1; pane < sizeof(indicators)/sizeof(*indicators)-3; ++pane)
            m_wndStatusBar.SetPaneText(pane, "");   // clear out dummy text

        // Load search strings into mainframe edit bar
        LoadSearchHistory(aa);
        LoadJumpHistory(aa);
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

    BOOL retval = CBCGMDIFrameWnd::PreCreateWindow(cs);

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

    if (aa->mditabs_)
    {
        EnableMDITabs(TRUE, aa->tabicons_,
            aa->tabsbottom_ ? CBCGTabWnd::LOCATION_BOTTOM : CBCGTabWnd::LOCATION_TOP);
    }

    return retval;
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) 
{
    CWnd *pwnd = CWnd::FromHandlePermanent(pMsg->hwnd);  // xxx use FromHandle?

    if (pwnd != NULL && 
        (pwnd->IsKindOf(RUNTIME_CLASS(CSearchEditControl)) ||
         pwnd->IsKindOf(RUNTIME_CLASS(CHexEditControl)) ||
         pwnd->IsKindOf(RUNTIME_CLASS(CDecEditControl)) ||
		 pwnd->IsKindOf(RUNTIME_CLASS(CInPlaceEdit))  ) )
	{
        return FALSE;
	}

    return CBCGMDIFrameWnd::PreTranslateMessage(pMsg);
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

	// The following were move here from HexEditApp:ExitInstance as they require
	// access to the main windows (AfxGetMainWnd() returns NULL in ExitInstance)

    // Clear histories if the option to clear on exit is on
    if (aa->clear_on_exit_)
        aa->ClearHist(aa->clear_hist_, aa->clear_recent_file_list_, aa->clear_bookmarks_);

	if (aa->delete_all_settings_)
		remove(m_strImagesFileName);

	if (!aa->is_nt_)
	{
        if (!m_wndProp.IsFloating())
            m_wndProp.ToggleDocking();
        ShowControlBar(&m_wndProp, FALSE, FALSE); // hide it otherwise BCG/MFC/Windows gets confused on next open
	}

    CBCGMDIFrameWnd::OnClose();
}

// Handles control menu commands and system buttons (Minimize etc)
void CMainFrame::OnSysCommand(UINT nID, LONG lParam)
{
    CBCGMDIFrameWnd::OnSysCommand(nID, lParam);

    nID &= 0xFFF0;
    if (nID == SC_MINIMIZE || nID == SC_RESTORE || nID == SC_MAXIMIZE)
        theApp.SaveToMacro(km_mainsys, nID);
}

// Given a popup menu and an ID of an item within the menu return the
// rectangle of the item in screen coordinates
CRect CMainFrame::item_rect(CBCGPopupMenu *pm, UINT id)
{
    CRect rct;
    pm->GetMenuBar()->GetWindowRect(&rct);

    rct.top += ::GetSystemMetrics(SM_CYEDGE) + 1;
    for (int ii = 0; ii < pm->GetMenuItemCount(); ++ii)
    {
        CRect item_rct;
        pm->GetMenuItem(ii)->GetImageRect(item_rct);
        if (::GetMenuItemID(pm->GetMenu(), ii) == id)
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
    if (id == -1)
        id = last_id_;
    else
        last_id_ = id;
    if (popup_menu_.empty())
    {
        menu_tip_.Hide();
        return;
    }

	CHexEditView *pview = GetView();

    menu_tip_.Hide();
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
			menu_tip_.AddString("#hqb S*hn bxy .", text_col, &CPoint(rct.Width(), rct.top));
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
	TRACE("WM_MENUSELECT %d, %x, %d\n", nItemID, nFlags, int(hMenu));
	if ((nFlags & 0xFFFF) == 0xFFFF || (nFlags & MF_POPUP) != 0 || (nFlags & MF_SEPARATOR) != 0)
        menu_tip_.Hide();
	else
        show_tip(nItemID);

    CBCGMDIFrameWnd::OnMenuSelect(nItemID, nFlags, hMenu);
}


// We could draw into the MDI client area here (eg HexEdit logo).
BOOL CMainFrame::OnEraseMDIClientBackground(CDC* pDC)
{
#ifdef DRAW_BACKGROUND
	if (HBITMAP(m_background) == 0 || m_background_pos == 0)
	    return FALSE;  // No bitmap so let Windows draw the background

    CBrush backBrush;

	// Get rectangle to be drawn into
    CRect rct;
    m_wndClientArea.GetClientRect(rct);
    // pDC->FillSolidRect(rct, ::GetSysColor(COLOR_APPWORKSPACE));

	CSize siz;
	BITMAP bi;
	m_background.GetBitmap(&bi);
	siz.cx = bi.bmWidth;
	siz.cy = bi.bmHeight;

	CPoint point;
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
			CDC dcTmp;
			dcTmp.CreateCompatibleDC(pDC);
			dcTmp.SelectObject(&m_background);
			pDC->StretchBlt(0, 0, rct.Width(), rct.Height(), 
			        &dcTmp, 0, 0, siz.cx, siz.cy, SRCCOPY);
			dcTmp.DeleteDC();
		}
		goto no_fill;
	case 7:  // tile
		for (point.x = 0; point.x < rct.Width(); point.x += siz.cx)
			for (point.y = 0; point.y < rct.Height(); point.y += siz.cy)
				pDC->DrawState(point, siz, &m_background, DST_BITMAP | DSS_NORMAL);
		goto no_fill;
	}

	// Create background brush using top left pixel of bitmap
	{
		CDC dcTmp;
		dcTmp.CreateCompatibleDC(pDC);
		dcTmp.SelectObject(&m_background);

		// Use top left pixel as background colour
		backBrush.CreateSolidBrush(::GetPixel(dcTmp, 0, 0));  // get colour from (0,0) pixel
		backBrush.UnrealizeObject();
	} // dcTmp destroyed here

	pDC->FillRect(rct, &backBrush);
	pDC->DrawState(point, siz, &m_background, DST_BITMAP | DSS_NORMAL);
no_fill:
    return TRUE;
#else
    return FALSE;  // Let Windows draw the background
#endif
}

//void CMainFrame::OnTimer(UINT nIDEvent) 
//{
//    if (nIDEvent == timer_id_)
//        UpdateBGSearchProgress();
//}

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
    
    CBCGMDIFrameWnd::RecalcLayout(bNotify);
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
#ifndef DIALOG_BAR
    aa->WriteProfileInt("MainFrame", "ShowFindDlg", int(m_wndFind.visible_));
    aa->WriteProfileInt("MainFrame", "ShowPropDlg", int(m_wndProp.visible_));
#else
    aa->WriteProfileInt("MainFrame", "DockableDialogs", int(dockable_));
#endif
}

void CMainFrame::show_calc()
{
    m_wndCalc.SetWindowText("Calculator");
    ShowControlBar(&m_wndCalc, TRUE, FALSE);
	m_wndCalc.Unroll();

    // Make sure controls and the displayed calc value are up to date
    //m_wndCalc.UpdateData(FALSE);
    //m_wndCalc.edit_.Put();
    m_wndCalc.update_controls();
    m_wndCalc.ShowBinop();
    //m_wndCalc.FixFileButtons();
}

void CMainFrame::move_dlgbar(CBCGDialogBar &bar, const CRect &rct)
{
	// We don't need to move it if hidden or docked
	if ((bar.GetStyle() & WS_VISIBLE) == 0 || !bar.IsFloating())
		return;

	// Get height of screen (needed later)
    int scr_height = GetSystemMetrics(SM_CYSCREEN);
    int scr_width  = GetSystemMetrics(SM_CXSCREEN);

	// Get rect of the window we may have to move
	CRect wnd_rct;
    ASSERT(bar.GetParent() != NULL && bar.GetParent()->GetParent() != NULL);
	bar.GetParent()->GetParent()->GetWindowRect(&wnd_rct);

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

    // Now move it where we (finally) decided
    bar.GetParent()->GetParent()->MoveWindow(&new_rct);

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

void CMainFrame::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    if (pWnd->GetSafeHwnd () == m_wndClientArea.GetMDITabs().GetSafeHwnd())
    {
        const CBCGTabWnd& wndTab = m_wndClientArea.GetMDITabs();
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
    return CBCGMDIFrameWnd::OnHelpInfo(pHelpInfo);
}

void CMainFrame::OnContextHelp()
{
    CBCGMDIFrameWnd::OnContextHelp();
}

void CMainFrame::OnHelpFinder()
{
    CBCGMDIFrameWnd::OnHelpFinder();
    ((CHexEditApp *)AfxGetApp())->SaveToMacro(km_topics);
}

void CMainFrame::OnHelpKeyboardMap()
{
    CBCGKeyMapDlg dlg(this, TRUE);
    dlg.DoModal();
}

void CMainFrame::OnHelpTute(UINT nID)
{
    // Display help for this page
    if (!::HtmlHelp(m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, 0x20000+nID))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

void CMainFrame::OnHelp()
{
    CBCGMDIFrameWnd::OnHelp();
    ((CHexEditApp *)AfxGetApp())->SaveToMacro(km_help);
}

void CMainFrame::HtmlHelp(DWORD_PTR dwData, UINT nCmd)
{
	if (dwData == AFX_HIDD_FILEOPEN || dwData == AFX_HIDD_FILESAVE)
		dwData = hid_last_file_dialog;
	CBCGMDIFrameWnd::HtmlHelp(dwData, nCmd);
}

// This is here just so we can intercept calls in the debugger
LRESULT CMainFrame::OnCommandHelp(WPARAM wParam, LPARAM lParam)
{
    return CBCGMDIFrameWnd::OnCommandHelp(wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
        CBCGMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
        CBCGMDIFrameWnd::Dump(dc);
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

void CMainFrame::OnDockableToggle()
{
    dockable_ = !dockable_;

    if (!dockable_)
    {
        // Make sure the windows aren't docked
        if (!m_wndCalc.IsFloating())
            m_wndCalc.ToggleDocking();
        if (!m_wndBookmarks.IsFloating())
            m_wndBookmarks.ToggleDocking();
        if (!m_wndFind.IsFloating())
            m_wndFind.ToggleDocking();
        if (!m_wndProp.IsFloating())
            m_wndProp.ToggleDocking();

        // Prevent dockability
        m_wndCalc.m_dwDockStyle = 0;
        m_wndBookmarks.m_dwDockStyle = 0;
        m_wndFind.m_dwDockStyle = 0;
        m_wndProp.m_dwDockStyle = 0;

#ifdef EXPLORER_WND
        if (!m_wndExpl.IsFloating())
            m_wndExpl.ToggleDocking();
        m_wndExpl.m_dwDockStyle = 0;
#endif
    }
    else
    {
        // Allow dockability on left or right
        m_wndCalc.m_dwDockStyle = CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT;
        m_wndBookmarks.m_dwDockStyle = CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT;
        m_wndFind.m_dwDockStyle = CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT;
#ifdef EXPLORER_WND
        m_wndExpl.m_dwDockStyle = CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT;
#endif
        m_wndProp.m_dwDockStyle = CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT;
    }
}

void CMainFrame::OnUpdateDockableToggle(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(dockable_);
}

void CMainFrame::OnWindowNew()
{
    // Store options for the active view so that the new view will get the same ones
    if (GetView() != NULL)
        GetView()->StoreOptions();

    CBCGMDIFrameWnd::OnWindowNew();

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
    BOOL retval = CBCGMDIFrameWnd::OnMDIWindowCmd(nID);

    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
    if (retval)
        aa->SaveToMacro(km_win_cmd, nID);
    else
        aa->mac_error_ = 20;
    return retval;
}

void CMainFrame::OnUpdateViewViewbar(CCmdUI* pCmdUI) 
{
    pCmdUI->SetCheck((m_wndBar1.GetStyle() & WS_VISIBLE) != 0);
}

void CMainFrame::OnViewViewbar() 
{
    ShowControlBar(&m_wndBar1, (m_wndBar1.GetStyle() & WS_VISIBLE) == 0, FALSE);
    ((CHexEditApp *)AfxGetApp())->SaveToMacro(km_toolbar, 1);
}

void CMainFrame::OnUpdateViewEditbar(CCmdUI* pCmdUI) 
{
    pCmdUI->SetCheck((m_wndBar2.GetStyle() & WS_VISIBLE) != 0);
}

void CMainFrame::OnViewEditbar() 
{
    ShowControlBar(&m_wndBar2, (m_wndBar2.GetStyle() & WS_VISIBLE) == 0, FALSE);
    ((CHexEditApp *)AfxGetApp())->SaveToMacro(km_toolbar, 2);
}

void CMainFrame::OnUpdateViewFormatbar(CCmdUI* pCmdUI) 
{
    pCmdUI->SetCheck((m_wndBar3.GetStyle() & WS_VISIBLE) != 0);
}

void CMainFrame::OnViewFormatbar() 
{
    ShowControlBar(&m_wndBar3, (m_wndBar3.GetStyle() & WS_VISIBLE) == 0, FALSE);
    ((CHexEditApp *)AfxGetApp())->SaveToMacro(km_toolbar, 4);
}

void CMainFrame::OnUpdateViewNavbar(CCmdUI* pCmdUI) 
{
    pCmdUI->SetCheck((m_wndBar4.GetStyle() & WS_VISIBLE) != 0);
}

void CMainFrame::OnViewNavbar() 
{
    ShowControlBar(&m_wndBar4, (m_wndBar4.GetStyle() & WS_VISIBLE) == 0, FALSE);
    ((CHexEditApp *)AfxGetApp())->SaveToMacro(km_toolbar, 5);
}

void CMainFrame::OnUpdateViewToolbar(CCmdUI* pCmdUI) 
{
    pCmdUI->SetCheck((m_wndBarOld.GetStyle() & WS_VISIBLE) != 0);
}

void CMainFrame::OnViewToolbar() 
{
    ShowControlBar(&m_wndBarOld, (m_wndBarOld.GetStyle() & WS_VISIBLE) == 0, FALSE);
    ((CHexEditApp *)AfxGetApp())->SaveToMacro(km_toolbar, 3);
}

void CMainFrame::OnUpdateViewCalculator(CCmdUI* pCmdUI) 
{
    pCmdUI->SetCheck((m_wndCalc.GetStyle() & WS_VISIBLE) != 0);
}

void CMainFrame::OnViewCalculator() 
{
    ShowControlBar(&m_wndCalc, (m_wndCalc.GetStyle() & WS_VISIBLE) == 0, FALSE);
    theApp.SaveToMacro(km_toolbar, 10);
}

void CMainFrame::OnUpdateViewBookmarks(CCmdUI* pCmdUI) 
{
    pCmdUI->SetCheck((m_wndBookmarks.GetStyle() & WS_VISIBLE) != 0);
}

void CMainFrame::OnViewBookmarks() 
{
    ShowControlBar(&m_wndBookmarks, (m_wndBookmarks.GetStyle() & WS_VISIBLE) == 0, FALSE);
    theApp.SaveToMacro(km_toolbar, 11);
}

void CMainFrame::OnUpdateViewFind(CCmdUI* pCmdUI) 
{
    pCmdUI->SetCheck((m_wndFind.GetStyle() & WS_VISIBLE) != 0);
}

void CMainFrame::OnViewFind() 
{
    ShowControlBar(&m_wndFind, (m_wndFind.GetStyle() & WS_VISIBLE) == 0, FALSE);
    theApp.SaveToMacro(km_toolbar, 12);
}

#ifdef EXPLORER_WND
void CMainFrame::OnUpdateViewExpl(CCmdUI* pCmdUI) 
{
    pCmdUI->SetCheck((m_wndExpl.GetStyle() & WS_VISIBLE) != 0);
}

void CMainFrame::OnViewExpl() 
{
    ShowControlBar(&m_wndExpl, (m_wndExpl.GetStyle() & WS_VISIBLE) == 0, FALSE);
    theApp.SaveToMacro(km_toolbar, 14);
}
#endif

void CMainFrame::OnUpdateViewProperties(CCmdUI* pCmdUI) 
{
    pCmdUI->SetCheck((m_wndProp.GetStyle() & WS_VISIBLE) != 0);
}

void CMainFrame::OnViewProperties() 
{
    ShowControlBar(&m_wndProp, (m_wndProp.GetStyle() & WS_VISIBLE) == 0, FALSE);
    theApp.SaveToMacro(km_toolbar, 13);
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
            COLORREF text_col = RGB(0,0,0);
            int hue, luminance, saturation;

            get_hls(::GetSearchCol(), hue, luminance, saturation);
            if (hue != -1)
            {
                if (luminance > 50) luminance = 1; else luminance = 99;
                text_col  = get_rgb((hue+50)%100, luminance, 99);
            }
            if (!bg_progress_enabled_)
            {
                m_wndStatusBar.EnablePaneProgressBar(index, 100, TRUE, ::GetSearchCol(), -1, text_col);
                bg_progress_enabled_ = true;
            }
            m_wndStatusBar.SetPaneProgress(index, pview->GetDocument()->SearchProgress(ii));
            return TRUE;
        }
    }
    return FALSE;
}

void CMainFrame::OnUpdateOccurrences(CCmdUI *pCmdUI)
{
    CBCGStatusBar *psb = (CBCGStatusBar *)pCmdUI->m_pOther;
    ASSERT_KINDOF(CBCGStatusBar, psb);
    ASSERT_VALID(psb);
    int index = psb->CommandToIndex(ID_INDICATOR_OCCURRENCES);

    CHexEditView *pview = GetView();
	int pane_width = 80;

    int ii;
    if (pview != NULL)
    {
        if ((ii = pview->GetDocument()->SearchOccurrences()) > -1)
        {
            psb->EnablePaneProgressBar(index, -1);  // turn off progress bar so we can show the text
            bg_progress_enabled_ = false;

            CString ss;
            ss.Format("%ld ", long(ii));
            AddCommas(ss);

		    // Work out pane width
            CClientDC dc(psb);
		    dc.SelectObject(psb->GetFont());
            pane_width = max(dc.GetTextExtent(ss, ss.GetLength()).cx + 2, 35) + 20; // allow 20 for icon
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
            bg_progress_enabled_ = false;
            pCmdUI->SetText("OFF");
            pCmdUI->Enable();
        }
        else
            pCmdUI->Enable(FALSE);
    }
    else
    {
        psb->EnablePaneProgressBar(index, -1);      // turn off progress bar so we can show the text
        bg_progress_enabled_ = false;
        pCmdUI->SetText("");
        pCmdUI->Enable(FALSE);
    }

	if (pane_width != OccurrencesWidth)
	{
		psb->SetPaneWidth(index, pane_width);
		OccurrencesWidth = pane_width;
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
        CBCGStatusBar *psb = (CBCGStatusBar *)pCmdUI->m_pOther;
        ASSERT_KINDOF(CBCGStatusBar, psb);
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

#if 0 // Colours now handled by CBCGStatusBar
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
        CBCGStatusBar *psb = (CBCGStatusBar *)pCmdUI->m_pOther;
        ASSERT_KINDOF(CBCGStatusBar, psb);
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

#if 0 // Colours now handled by CBCGStatusBar
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
        CBCGStatusBar *psb = (CBCGStatusBar *)pCmdUI->m_pOther;
        ASSERT_KINDOF(CBCGStatusBar, psb);
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
        CBCGStatusBar *psb = (CBCGStatusBar *)pCmdUI->m_pOther;
        ASSERT_KINDOF(CBCGStatusBar, psb);
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
    CBCGStatusBar *psb = (CBCGStatusBar *)pCmdUI->m_pOther;
    ASSERT_KINDOF(CBCGStatusBar, psb);
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
    CString ss;                         // Profile string
    CString entry;                      // "Search1" etc
    int ii;

    hex_hist_.clear();
    for (ii = max_jump_hist; ii > 0; ii--)
    {
        entry.Format("HexJump%d", ii);
        ss = aa->GetProfileString("History", entry);
        if (!ss.IsEmpty())
            hex_hist_.push_back(ss);
    }

    dec_hist_.clear();
    for (ii = max_jump_hist; ii > 0; ii--)
    {
        entry.Format("DecJump%d", ii);
        ss = aa->GetProfileString("History", entry);
        if (!ss.IsEmpty())
            dec_hist_.push_back(ss);
    }

	hex_hist_changed_ = dec_hist_changed_ = clock();
}

// Save hex/dec histories to registry
void CMainFrame::SaveJumpHistory(CHexEditApp *aa)
{
    CString ss;                         // Profile string
    CString entry;                      // "HexJump1" etc
    int ii;

    int last = hex_hist_.size();
    for (ii = 1; ii <= max_jump_hist; ++ii)
    {
        entry.Format("HexJump%d", ii);

        // Note this clears any empty trailing entries OR
        // it clears all entries we are clearing the histories on exit
        aa->WriteProfileString("History", 
                               entry, 
                               last < ii || (aa->clear_on_exit_ && aa->clear_hist_) ? (LPCTSTR)NULL : (LPCTSTR)hex_hist_[last-ii]);
    }

    last = dec_hist_.size();
    for (ii = 1; ii <= max_jump_hist; ++ii)
    {
        entry.Format("DecJump%d", ii);

        // Note this clears any trailing entries OR
        // it clears all entries we are clearing the histories on exit
        aa->WriteProfileString("History", 
                               entry, 
                               last < ii || (aa->clear_on_exit_ && aa->clear_hist_) ? (LPCTSTR)NULL : (LPCTSTR)dec_hist_[last-ii]);
    }
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

// Retrieve search/replace histories from registry
void CMainFrame::LoadSearchHistory(CHexEditApp *aa)
{
    CString ss;                         // Profile string
    CString entry;                      // "Search1" etc
    int ii;

    search_hist_.clear();
    for (ii = max_search_hist; ii > 0; ii--)
    {
        entry.Format("Search%d", ii);
        ss = aa->GetProfileString("History", entry);
        if (!ss.IsEmpty())
            search_hist_.push_back(ss);
    }

    replace_hist_.clear();
    for (ii = max_replace_hist; ii > 0; ii--)
    {
        entry.Format("Replace%d", ii);
        ss = aa->GetProfileString("History", entry);
        if (!ss.IsEmpty())
            replace_hist_.push_back(ss);
    }
}

// Save search/replace histories to registry
void CMainFrame::SaveSearchHistory(CHexEditApp *aa)
{
    CString ss;                         // Profile string
    CString entry;                      // "Search1" etc
    int ii;

    int last = search_hist_.size();
    for (ii = 1; ii <= max_search_hist; ++ii)
    {
        entry.Format("Search%d", ii);

        // Note this clears any trailing entries OR
        // it clears all entries we are clearing the search history on exit
        aa->WriteProfileString("History", 
                               entry, 
                               last < ii || (aa->clear_on_exit_ && aa->clear_hist_) ? (LPCTSTR)NULL : (LPCTSTR)search_hist_[last-ii]);
    }

    last = replace_hist_.size();
    for (ii = 1; ii <= max_replace_hist; ++ii)
    {
        entry.Format("Replace%d", ii);

        // Note this clears any trailing entries OR
        // it clears all entries we are clearing the replace history on exit
        aa->WriteProfileString("History", 
                               entry, 
                               last < ii || (aa->clear_on_exit_ && aa->clear_hist_) ? (LPCTSTR)NULL : (LPCTSTR)replace_hist_[last-ii]);
    }
}

void CMainFrame::OnFindNext()
{
    AddSearchHistory(current_search_string_);

    if (DoFind() && theApp.recording_)
        theApp.SaveToMacro(km_find_next, m_wndFind.m_pSheet->GetOptions());
}

void CMainFrame::OnSearchForw()
{
    m_wndFind.m_pSheet->SetDirn(CFindSheet::DIRN_DOWN);
    CFindSheet::scope_t scope = m_wndFind.m_pSheet->GetScope();
    if (scope != CFindSheet::SCOPE_EOF && scope != CFindSheet::SCOPE_ALL)
        m_wndFind.m_pSheet->SetScope(CFindSheet::SCOPE_EOF);

    AddSearchHistory(current_search_string_);

    if (DoFind() && theApp.recording_)
        theApp.SaveToMacro(km_find_forw, m_wndFind.m_pSheet->GetOptions());
}

void CMainFrame::OnSearchBack()
{
    m_wndFind.m_pSheet->SetDirn(CFindSheet::DIRN_UP);
    CFindSheet::scope_t scope = m_wndFind.m_pSheet->GetScope();
    if (scope != CFindSheet::SCOPE_EOF && scope != CFindSheet::SCOPE_ALL)
        m_wndFind.m_pSheet->SetScope(CFindSheet::SCOPE_EOF);

    AddSearchHistory(current_search_string_);

    if (DoFind() && theApp.recording_)
        theApp.SaveToMacro(km_find_back, m_wndFind.m_pSheet->GetOptions());
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
        ::HMessageBox("There is no file open to search.");
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
        ::HMessageBox("Nothing selected to search for!");
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

        m_wndFind.m_pSheet->SetCharSet(CFindSheet::RB_CHARSET_EBCDIC);
        m_wndFind.m_pSheet->NewText((char *)buf);
    }
    else if (pview->CharMode())
    {
        buf[end-start] = '\0';
        // Check if all chars are normal ASCII text and set search text
        for (unsigned char *pp = buf; pp < buf+end-start; ++pp)
            if (*pp <= '\r')
                break;
        if (pp == buf+end-start)
            do_hex = false;             // No invalid chars found so do text search
        ASSERT(buf[end-start] == '\0'); // Check for buffer overrun

        m_wndFind.m_pSheet->SetCharSet(CFindSheet::RB_CHARSET_ASCII);
        m_wndFind.m_pSheet->NewText((char *)buf);
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

        m_wndFind.m_pSheet->NewHex((char *)tbuf);

        delete[] tbuf;
    }

    // Must be a forward search and fix scope
    m_wndFind.m_pSheet->SetDirn(CFindSheet::DIRN_DOWN);
    CFindSheet::scope_t scope = m_wndFind.m_pSheet->GetScope();
    if (scope != CFindSheet::SCOPE_EOF && scope != CFindSheet::SCOPE_ALL)
        m_wndFind.m_pSheet->SetScope(CFindSheet::SCOPE_EOF);

    AddSearchHistory(current_search_string_);

    if (DoFind() && theApp.recording_)
        theApp.SaveToMacro(km_find_sel, m_wndFind.m_pSheet->GetOptions());

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
        ::HMessageBox("There is no file open to search.");
        theApp.mac_error_ = 10;
        return FALSE;
    }

    CHexEditDoc *pdoc = pview->GetDocument();

    // Get current find options
    const unsigned char *ss;
    const unsigned char *mask = NULL;
    size_t length;

    m_wndFind.m_pSheet->GetSearch(&ss, &mask, &length);

    CFindSheet::dirn_t dirn = m_wndFind.m_pSheet->GetDirn();
    CFindSheet::scope_t scope = m_wndFind.m_pSheet->GetScope();
    BOOL wholeword = m_wndFind.m_pSheet->GetWholeWord();
    BOOL icase = !m_wndFind.m_pSheet->GetMatchCase();
    int tt = int(m_wndFind.m_pSheet->GetCharSet()) + 1;       // 1 = ASCII, 2 = Unicode, 3 = EBCDIC
    ASSERT(tt == 1 || tt == 2 || tt == 3);
    int alignment = m_wndFind.m_pSheet->GetAlignment();
	int offset    = m_wndFind.m_pSheet->GetOffset();
	bool align_rel = m_wndFind.m_pSheet->AlignRel();
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
                    ::HMessageBox(not_found_mess(FALSE, icase, tt, wholeword, alignment));
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
                if (::HMessageBox(mess, MB_YESNO) == IDYES)
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
                        ::HMessageBox(not_found_mess(FALSE, icase, tt, wholeword, alignment));
                    }
                    else
                    {
                        pview->MoveToAddress(found_addr, found_addr + length);
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
                ::HMessageBox(not_found_mess(FALSE, icase, tt, wholeword, alignment));
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
                m_wndFind.m_pSheet->SetScope(CFindSheet::SCOPE_EOF);
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
                    ::HMessageBox(not_found_mess(TRUE, icase, tt, wholeword, alignment));
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
                if (::HMessageBox(mess, MB_YESNO) == IDYES)
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
                        ::HMessageBox(not_found_mess(TRUE, icase, tt, wholeword, alignment));
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
                ::HMessageBox(not_found_mess(TRUE, icase, tt, wholeword, alignment));
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
                m_wndFind.m_pSheet->SetScope(CFindSheet::SCOPE_EOF);
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
        ::HMessageBox("There is no file open to replace in.");
        theApp.mac_error_ = 2;
        return;
    }

    CHexEditDoc *pdoc = pview->GetDocument();

    // Get current find options
    const unsigned char *ss;
    const unsigned char *mask = NULL;
    size_t length;

    m_wndFind.m_pSheet->GetSearch(&ss, &mask, &length);

    CFindSheet::dirn_t dirn = m_wndFind.m_pSheet->GetDirn();
    CFindSheet::scope_t scope = m_wndFind.m_pSheet->GetScope();
    BOOL wholeword = m_wndFind.m_pSheet->GetWholeWord();
    BOOL icase = !m_wndFind.m_pSheet->GetMatchCase();
    int tt = int(m_wndFind.m_pSheet->GetCharSet()) + 1;       // 1 = ASCII, 2 = Unicode, 3 = EBCDIC
    ASSERT(tt == 1 || tt == 2 || tt == 3);
    int alignment = m_wndFind.m_pSheet->GetAlignment();
	int offset    = m_wndFind.m_pSheet->GetOffset();
	bool align_rel = m_wndFind.m_pSheet->AlignRel();
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
            m_wndFind.m_pSheet->GetReplace(&pp, &replen);
            pview->do_replace(start, start+length, pp, replen);
        }
    }

    DoFind();

    AddSearchHistory(current_search_string_);
    AddReplaceHistory(current_replace_string_);

    if (theApp.recording_)
        theApp.SaveToMacro(km_replace, m_wndFind.m_pSheet->GetOptions());
}

void CMainFrame::OnReplaceAll()
{
    // Get current view thence (first) document to search
    CHexEditView *pview = GetView();

    if (pview == NULL)
    {
        ::HMessageBox("There is no file open to search.");
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

    m_wndFind.m_pSheet->GetSearch(&ss, &mask, &length);

    CFindSheet::dirn_t dirn = m_wndFind.m_pSheet->GetDirn();
    CFindSheet::scope_t scope = m_wndFind.m_pSheet->GetScope();
    BOOL wholeword = m_wndFind.m_pSheet->GetWholeWord();
    BOOL icase = !m_wndFind.m_pSheet->GetMatchCase();
    int tt = int(m_wndFind.m_pSheet->GetCharSet()) + 1;       // 1 = ASCII, 2 = Unicode, 3 = EBCDIC
    ASSERT(tt == 1 || tt == 2 || tt == 3);
    int alignment = m_wndFind.m_pSheet->GetAlignment();
	int offset    = m_wndFind.m_pSheet->GetOffset();
	bool align_rel = m_wndFind.m_pSheet->AlignRel();

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
    m_wndFind.m_pSheet->GetReplace(&pp, &replen);

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
                theApp.CheckBGSearchFinished();
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
                if (::HMessageBox(mess, MB_YESNO) == IDYES)
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
        theApp.SaveToMacro(km_replace_all, m_wndFind.m_pSheet->GetOptions());

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

    ::HMessageBox(mess);
}

void CMainFrame::OnBookmarkAll()
{
    if (m_wndFind.m_pSheet->bookmark_prefix_.IsEmpty())
    {
        ::HMessageBox("Bookmark All requires a bookmark prefix.");
        theApp.mac_error_ = 2;
        return;
    }
    // Get current view thence (first) document to search
    CHexEditView *pview = GetView();

    if (pview == NULL)
    {
        ::HMessageBox("There is no file open to search.");
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

    if ((next_number = pbl->GetSetLast(m_wndFind.m_pSheet->bookmark_prefix_, count)) > 0)
    {
        CBookmarkFind dlg;

        dlg.mess_.Format("%d bookmarks with the prefix \"%s\"\r"
                         "already exist.  Do you want to overwrite or\r"
                         "append to this set of bookmarks?",
                         count, (const char *)m_wndFind.m_pSheet->bookmark_prefix_);

        switch (dlg.DoModal())
        {
        case IDC_BM_FIND_OVERWRITE:
            pbl->RemoveSet(m_wndFind.m_pSheet->bookmark_prefix_);  // xxx does not remove doc bookmarks or dlg entries
            next_number = 0;
            break;
        case IDC_BM_FIND_APPEND:
            // nothing required here (next_number points to the next bookmark set number to use)
            break;
        case IDCANCEL:
            return;
        default:
            ASSERT(0);
            break;
        }
    }

    long start_number = next_number;
    int file_count = 1;

    // Get current find options
    const unsigned char *ss;
    const unsigned char *mask = NULL;
    size_t length;

    m_wndFind.m_pSheet->GetSearch(&ss, &mask, &length);

    CFindSheet::dirn_t dirn = m_wndFind.m_pSheet->GetDirn();
    CFindSheet::scope_t scope = m_wndFind.m_pSheet->GetScope();
    BOOL wholeword = m_wndFind.m_pSheet->GetWholeWord();
    BOOL icase = !m_wndFind.m_pSheet->GetMatchCase();
    int tt = int(m_wndFind.m_pSheet->GetCharSet()) + 1;       // 1 = ASCII, 2 = Unicode, 3 = EBCDIC
    ASSERT(tt == 1 || tt == 2 || tt == 3);
    int alignment = m_wndFind.m_pSheet->GetAlignment();
	int offset    = m_wndFind.m_pSheet->GetOffset();
	bool align_rel = m_wndFind.m_pSheet->AlignRel();

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

                if (::HMessageBox(mess, MB_YESNO) == IDYES)
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
            bm_name.Format("%s%05ld", (const char *)m_wndFind.m_pSheet->bookmark_prefix_, long(next_number));
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
//                pdoc2->AddBookmark(ii, found_addr);
//                ((CMainFrame *)AfxGetMainWnd())->m_wndBookmarks.UpdateBookmark(ii);
            }

            curr = found_addr + 1;              // Continue the search from the next byte
        }
    }

    pview->show_pos();                          // Restore display of orig address
    AddSearchHistory(current_search_string_);

    if (theApp.recording_)
        theApp.SaveToMacro(km_bookmark_all, m_wndFind.m_pSheet->GetOptions());

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

        ss.Format("Set one bookmark: %s%05ld", (const char *)m_wndFind.m_pSheet->bookmark_prefix_, long(start_number));
        mess += ss;
    }
    else
    {
        CString ss;

        ss.Format("Set %d bookmarks.\n\nFirst bookmark: %s%05ld\nLast bookmark: %s%05ld",
                  next_number - start_number,
                  (const char *)m_wndFind.m_pSheet->bookmark_prefix_, (long)start_number,
                  (const char *)m_wndFind.m_pSheet->bookmark_prefix_, (long)next_number-1);
        mess += ss;
    }
    ::HMessageBox(mess);
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
        MSG msg;

        // Do any redrawing, but nothing else
        while (::PeekMessage(&msg, NULL, WM_PAINT, WM_PAINT, PM_NOREMOVE))
        {
            if (::GetMessage(&msg, NULL, WM_PAINT, WM_PAINT))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
            }
        }

        // Check if key has been pressed
        if (::PeekMessage(&msg, NULL, WM_KEYDOWN, WM_KEYDOWN, PM_NOREMOVE))
        {
            VERIFY(GetMessage(&msg, NULL, WM_KEYDOWN, WM_KEYDOWN) > 0);
            if (msg.wParam != 'y' &&  // avoid accidental multiple presses of Y key from aborting
                ::HMessageBox("Abort search?", MB_YESNO) == IDYES)
            {
                StatusBarText("Search aborted");
                theApp.mac_error_ = 10;
//                        pview->show_pos();
                return -2;
            }
        }

        return bg_next;
    }

    size_t buf_len = size_t(min(end_addr-start_addr, FILE_ADDRESS(search_buf_len + length - 1)));

//    ASSERT(length > 0 && length <= buf_len);

    // Warn of simple problems
    if (length == 0)
    {
        ::HMessageBox("Empty search sequence");
        return -2;
    }

    if (bg_next == -3)
    {
        // New search so signal to stop any current bg search
        pdoc->StopSearch();
    }

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
    bool progress_on = false;           // Are we showing progress in the status bar?
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
                MSG msg;

                // Do any redrawing, but nothing else
                while (::PeekMessage(&msg, NULL, WM_PAINT, WM_PAINT, PM_REMOVE)) // xxx fix others like this
                {
                    if (::GetMessage(&msg, NULL, WM_PAINT, WM_PAINT))
                    {
                        ::TranslateMessage(&msg);
                        ::DispatchMessage(&msg);
                    }
                }

                // Check if Escape has been pressed
                if (::PeekMessage(&msg, NULL, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE))
                {
                    // Windows does not like to miss key down events (need to match key up events)
                    ::TranslateMessage(&msg);
                    ::DispatchMessage(&msg);

                    // Remove any characters resulting from keypresses (so they are not inserted into the active file)
                    while (::PeekMessage(&msg, NULL, WM_CHAR, WM_CHAR, PM_REMOVE))
                        ;

                    if (msg.wParam != 'y' &&  // avoid accidental multiple presses of Y key from aborting
                        ::HMessageBox("Abort search?", MB_YESNO) == IDYES)
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
                        if (progress_on)
                            m_wndStatusBar.EnablePaneProgressBar(0, -1);
                        StatusBarText("Search aborted");
                        theApp.mac_error_ = 10;
//                        pview->show_pos();
                        return -2;
                    }
                }

                // Show search progress
                SetAddress(next_show);
                if (next_show >= slow_show)
                {
                    if (!progress_on)
                    {
                        progress_on = true;
                        m_wndStatusBar.EnablePaneProgressBar(0, 100);
                    }
                    m_wndStatusBar.SetPaneProgress(0, long(((next_show-start_addr)*100)/(end_addr-start_addr)));
                    show_inc = 0x1000000;
                }
                theApp.OnIdle(0);         // Force display of updated address
                next_show += show_inc;
            }

            // xxx step through this whole word code!
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
                MSG msg;

                // Do any redrawing, but nothing else
                while (::PeekMessage(&msg, NULL, WM_PAINT, WM_PAINT, PM_NOREMOVE))
                {
                    if (::GetMessage(&msg, NULL, WM_PAINT, WM_PAINT))
                    {
                        ::TranslateMessage(&msg);
                        ::DispatchMessage(&msg);
                    }
                }

                // Check if Escape has been pressed
                if (::PeekMessage(&msg, NULL, WM_KEYDOWN, WM_KEYDOWN, PM_NOREMOVE))
                {
                    VERIFY(GetMessage(&msg, NULL, WM_KEYDOWN, WM_KEYDOWN) > 0);
                    if (msg.wParam != 'y' &&  // avoid accidental multiple presses of Y key from aborting
                        ::HMessageBox("Abort search?", MB_YESNO) == IDYES)
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
                        if (progress_on)
                            m_wndStatusBar.EnablePaneProgressBar(0, -1);
                        StatusBarText("Search aborted");
                        theApp.mac_error_ = 10;
//                        pview->show_pos();
                        return -2;
                    }
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

                if (progress_on)
                    m_wndStatusBar.EnablePaneProgressBar(0, -1);
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
    if (progress_on)
        m_wndStatusBar.EnablePaneProgressBar(0, -1);
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
#if 0 // This is not nec since search_back is not called from ReplaceAll or BookmarkAll
        // Found
        MSG msg;

        // Do any redrawing, but nothing else
        while (::PeekMessage(&msg, NULL, WM_PAINT, WM_PAINT, PM_NOREMOVE))
        {
            if (::GetMessage(&msg, NULL, WM_PAINT, WM_PAINT))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
            }
        }

        // Check if Escape has been pressed
        if (::PeekMessage(&msg, NULL, WM_KEYDOWN, WM_KEYDOWN, PM_NOREMOVE))
        {
            VERIFY(GetMessage(&msg, NULL, WM_KEYDOWN, WM_KEYDOWN) > 0);
            if (msg.wParam != 'y' &&  // avoid accidental multiple presses of Y key from aborting
                ::HMessageBox("Abort search?", MB_YESNO) == IDYES)
            {
                StatusBarText("Search aborted");
                theApp.mac_error_ = 10;
//                        pview->show_pos();
                return -2;
            }
        }
#endif
        return bg_next;
    }

    size_t buf_len = size_t(min(end_addr-start_addr, FILE_ADDRESS(search_buf_len + length - 1)));

//    ASSERT(length > 0 && length <= buf_len);

    // Warn of simple problems
    if (length == 0)
    {
        ::HMessageBox("Empty search sequence");
        return -2;
    }

    if (bg_next == -3)
    {
        // New search so signal to stop any current bg search
        pdoc->StopSearch();
    }

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
                MSG msg;

                // Do any redrawing, but nothing else
                while (::PeekMessage(&msg, NULL, WM_PAINT, WM_PAINT, PM_NOREMOVE))
                {
                    if (::GetMessage(&msg, NULL, WM_PAINT, WM_PAINT))
                    {
                        ::TranslateMessage(&msg);
                        ::DispatchMessage(&msg);
                    }
                }

                // Check if Escape has been pressed
                if (::PeekMessage(&msg, NULL, WM_KEYDOWN, WM_KEYDOWN, PM_NOREMOVE))
                {
                    VERIFY(GetMessage(&msg, NULL, WM_KEYDOWN, WM_KEYDOWN) > 0);
                    if (msg.wParam != 'y' &&  // avoid accidental multiple presses of Y key from aborting
                        ::HMessageBox("Abort search?", MB_YESNO) == IDYES)
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
//                        pview->show_pos();
                        return -2;
                    }
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

                // xxx step through this whole word code!
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

                // xxx step through this whole word code!
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
#if 0 // This is not nec since search_back is not called from ReplaceAll or BookmarkAll
                // If we find lots of occurrences then the Esc key check in 
                // above is never done so do it here too.
                MSG msg;

                // Do any redrawing, but nothing else
                while (::PeekMessage(&msg, NULL, WM_PAINT, WM_PAINT, PM_NOREMOVE))
                {
                    if (::GetMessage(&msg, NULL, WM_PAINT, WM_PAINT))
                    {
                        ::TranslateMessage(&msg);
                        ::DispatchMessage(&msg);
                    }
                }

                // Check if Escape has been pressed
                if (::PeekMessage(&msg, NULL, WM_KEYDOWN, WM_KEYDOWN, PM_NOREMOVE))
                {
                    VERIFY(GetMessage(&msg, NULL, WM_KEYDOWN, WM_KEYDOWN) > 0);
                    if (msg.wParam != 'y' &&  // avoid accidental multiple presses of Y key from aborting
                        ::HMessageBox("Abort search?", MB_YESNO) == IDYES)
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
//                        pview->show_pos();
                        return -2;
                    }
                }
#endif 

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

// Create find modeless property sheet - if already exists then just activate it
void CMainFrame::OnEditFind() 
{
    if (GetView() != NULL)
        m_wndFind.ShowPage(0);
}

void CMainFrame::OnEditFind2() 
{
    if (GetView() != NULL)
        m_wndFind.ShowPage(1);
}

void CMainFrame::OnEditReplace() 
{
    if (GetView() != NULL)
        m_wndFind.ShowPage(4);
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

    // Macro recording now handled in DelayShow override
    //((CHexEditApp *)AfxGetApp())->SaveToMacro(km_calc_dlg);
}

void CMainFrame::OnOptionsScheme() 
{
    theApp.display_options(theApp.p_colours, TRUE);
}

void CMainFrame::OnBookmarks() 
{
    ShowControlBar(&m_wndBookmarks, TRUE, FALSE);
	m_wndBookmarks.Unroll();
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
        //if (m_wndCalc.m_hWnd == 0)
        //    m_wndCalc.Create();
        ASSERT(m_wndCalc.m_hWnd != 0);

        m_wndCalc.change_bits(64);

        switch (base_mode)
        {
        case 1:
            // Use base 10 addresses
            m_wndCalc.change_base(10);
            break;
        case 2:
            // Use base 16 addresses
            m_wndCalc.change_base(16);
            break;
        default:
            ASSERT(0);
            /* fall through */
        case 0:
            // Change base depending on how view is displaying addresses
            if (pview->DecAddresses())
                m_wndCalc.change_base(10);
            else
                m_wndCalc.change_base(16);
            break;
        }
        m_wndCalc.SetWindowText("Go To");
        ShowControlBar(&m_wndCalc, TRUE, FALSE);
        m_wndCalc.Unroll();

        // Make sure controls are up to date and put current address into it
        //m_wndCalc.UpdateData(FALSE);

        FILE_ADDRESS start, end;
        pview->GetSelAddr(start, end);
        m_wndCalc.Set(start);

        m_wndCalc.update_controls();
        m_wndCalc.ShowBinop();
        //m_wndCalc.FixFileButtons();

        m_wndCalc.SetFocus();

        ((CHexEditApp *)AfxGetApp())->SaveToMacro(km_goto);
    }
}

void CMainFrame::OnCalcSel()
{
    CHexEditView *pview = GetView();
    if (pview != NULL)
    {
        __int64 sel_val = 0;                    // Value from file to add to the calc

        //if (m_wndCalc.m_hWnd == 0)
        //    m_wndCalc.Create();
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
            ::HMessageBox("Selection too big for calculator!");
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

//        if (pview->DecAddresses())
//            m_wndCalc.change_base(10);
//        else
//            m_wndCalc.change_base(16);

        m_wndCalc.SetWindowText("Calculator");
        ShowControlBar(&m_wndCalc, TRUE, FALSE);
        m_wndCalc.Unroll();

        // Make sure controls are up to date and put current address into it
        //m_wndCalc.UpdateData(FALSE);

        m_wndCalc.Set(sel_val);

        m_wndCalc.update_controls();
        m_wndCalc.ShowBinop();
        //m_wndCalc.FixFileButtons();

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
    CBCGContextMenuManager *pCMM = aa->GetContextMenuManager();

    pCMM->ShowPopupMenu(IDR_CONTEXT_STATBAR, point.x, point.y, this);
#endif
}

void CMainFrame::OnInitMenu(CMenu* pMenu)
{
    // Remove this?
    CBCGMDIFrameWnd::OnInitMenu(pMenu);
}

// Toggle display of status bar
BOOL CMainFrame::OnBarCheck(UINT nID)
{
    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
    if (CBCGMDIFrameWnd::OnBarCheck(nID))
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
    CBCGToolbarCustomize *pdlg = new CBCGToolbarCustomize(this, TRUE,
                        BCGCUSTOMIZE_MENU_SHADOWS | BCGCUSTOMIZE_TEXT_LABELS | 
                        BCGCUSTOMIZE_LOOK_2000 | BCGCUSTOMIZE_MENU_ANIMATIONS |
                        BCGCUSTOMIZE_CONTEXT_HELP  | BCGCUSTOMIZE_SELECT_SKINS);
    pdlg->EnableUserDefinedToolbars();

    // Allow user to select commands that don't appear on any menus
//    pdlg->AddMenu(IDR_MISC);
    CMenu menu;
    VERIFY(menu.LoadMenu(IDR_MISC));
    pdlg->AddMenuCommands(menu.GetSubMenu(0), FALSE, "Format commands");
    pdlg->AddMenuCommands(menu.GetSubMenu(1), FALSE, "Navigation commands");
    pdlg->AddMenuCommands(menu.GetSubMenu(2), FALSE, "Find commands");
    menu.DestroyMenu();

    pdlg->ReplaceButton(ID_JUMP_HEX, CHexComboButton ());
    pdlg->ReplaceButton(ID_JUMP_DEC, CDecComboButton ());
    pdlg->ReplaceButton(ID_SEARCH, CFindComboButton ());
    pdlg->ReplaceButton(ID_SCHEME, CSchemeComboButton ());
    pdlg->ReplaceButton(ID_BOOKMARKS, CBookmarksComboButton ());

    pdlg->ReplaceButton(IDC_FONTNAME,
        CHexEditFontCombo(IDC_FONTNAME, 
            CImageHash::GetImageOfCommand(IDC_FONTNAME, FALSE),
            RASTER_FONTTYPE | TRUETYPE_FONTTYPE | DEVICE_FONTTYPE,
            ANSI_CHARSET,
            WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST |
            CBS_AUTOHSCROLL | CBS_HASSTRINGS | CBS_OWNERDRAWFIXED, 175) );
    pdlg->ReplaceButton(IDC_FONTSIZE,
        CHexEditFontSizeCombo(IDC_FONTSIZE, 
            CImageHash::GetImageOfCommand(IDC_FONTSIZE, FALSE),
            WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWN, 50) );

//    pdlg->AddToolBar("Main", IDR_MAINFRAME);
//    pdlg->AddButton("User", CBCGToolbarButton(ID_MARK, 1, "User Tool 1", TRUE));
//    pdlg->AddButton("User", CBCGToolbarButton(ID_GOTO_MARK, 2, "User Tool 2", TRUE));
//    pdlg->SetUserCategory("User");

    menu.LoadMenu(IDR_MENUBUTTON);

    pdlg->ReplaceButton(ID_DISPLAY_DROPDOWN,
        CBCGToolbarMenuButton (-1, *menu.GetSubMenu(0), -1, _T("Show Area")));
    pdlg->ReplaceButton(ID_CHARSET_DROPDOWN,
        CBCGToolbarMenuButton (-1, *menu.GetSubMenu(1), -1, _T("Char Set")));
    pdlg->ReplaceButton(ID_CONTROL_DROPDOWN,
        CBCGToolbarMenuButton (-1, *menu.GetSubMenu(2), -1, _T("Ctrl Chars")));

    pdlg->ReplaceButton(ID_HIGHLIGHT_MENU,
        CBCGToolbarMenuButton(ID_HIGHLIGHT, *menu.GetSubMenu(3), CImageHash::GetImageOfCommand(ID_HIGHLIGHT)));
    pdlg->ReplaceButton(ID_MARK_MENU,
        CBCGToolbarMenuButton(ID_GOTO_MARK, *menu.GetSubMenu(4), CImageHash::GetImageOfCommand(ID_GOTO_MARK)));
    pdlg->ReplaceButton(ID_BOOKMARKS_MENU,
        CBCGToolbarMenuButton(ID_BOOKMARKS_EDIT, *menu.GetSubMenu(5), CImageHash::GetImageOfCommand(ID_BOOKMARKS_EDIT)));

    pdlg->ReplaceButton(ID_NAV_BACK,
        CBCGToolbarMenuButton (ID_NAV_BACK, *menu.GetSubMenu(6), -1, _T("Navigate Backward")));
    pdlg->ReplaceButton(ID_NAV_FORW,
        CBCGToolbarMenuButton (ID_NAV_FORW, *menu.GetSubMenu(7), -1, _T("Navigate Forward")));

    pdlg->Create();
}

BOOL CMainFrame::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext) 
{
    if (!CBCGMDIFrameWnd::LoadFrame(nIDResource, dwDefaultStyle, pParentWnd, pContext))
    {
        return FALSE;
    }

    if (m_wndCalc.m_pDockBar == NULL || m_wndCalc.IsHorzDocked())
        m_wndCalc.FixAndFloat();
    if (m_wndBookmarks.m_pDockBar == NULL || m_wndBookmarks.IsHorzDocked())
        m_wndBookmarks.FixAndFloat();
    if (m_wndFind.m_pDockBar == NULL || m_wndFind.IsHorzDocked())
        m_wndFind.FixAndFloat();
#ifdef EXPLORER_WND
    if (m_wndExpl.m_pDockBar == NULL || m_wndExpl.IsHorzDocked())
        m_wndExpl.FixAndFloat();
#endif
    if (m_wndProp.m_pDockBar == NULL || m_wndProp.IsHorzDocked())
        m_wndProp.FixAndFloat();

    // Add some tools for example....
    CBCGUserToolsManager* pUserToolsManager = theApp.GetUserToolsManager ();
    if (pUserToolsManager != NULL &&
        pUserToolsManager->GetUserTools().IsEmpty ())
    {
        CBCGUserTool* pTool = pUserToolsManager->CreateNewTool();
        pTool->m_strLabel = _T("Notepad");
        pTool->m_strArguments = _T("$(FilePath)");
        pTool->SetCommand(_T("notepad.exe"));
        pTool = pUserToolsManager->CreateNewTool();
        pTool->m_strLabel = _T("Windows Calculator");
        pTool->SetCommand(_T("calc.exe"));
    }
    
    return TRUE;
}

LRESULT CMainFrame::OnToolbarContextMenu(WPARAM,LPARAM lp)
{
    CPoint point(LOWORD(lp), HIWORD(lp));
    
    CMenu menu;
    VERIFY(menu.LoadMenu(IDR_CONTEXT_BARS));
    
    CMenu* pPopup = menu.GetSubMenu(0);
    ASSERT(pPopup != NULL);
    
    SetupToolbarMenu(*pPopup, ID_VIEW_USER_TOOLBAR1, ID_VIEW_USER_TOOLBAR10);
    
    CBCGPopupMenu* pPopupMenu = new CBCGPopupMenu;
    pPopupMenu->SetAutoDestroy(FALSE);
    pPopupMenu->Create(this, point.x, point.y, pPopup->GetSafeHmenu());
    
    return 0;
}

BOOL CMainFrame::OnToolsViewUserToolbar(UINT uiId)
{
    CBCGToolBar* pUserToolBar = GetUserBarByIndex(uiId - ID_VIEW_USER_TOOLBAR1);
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
    CBCGToolBar* pUserToolBar = GetUserBarByIndex(pCmdUI->m_nID - ID_VIEW_USER_TOOLBAR1);
    if (pUserToolBar == NULL)
    {
        pCmdUI->Enable(FALSE);
        return;
    }
    
    pCmdUI->Enable();
    pCmdUI->SetCheck(pUserToolBar->GetStyle() & WS_VISIBLE);
}

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
        
            m_wndBar1.ReplaceButton(ID_DISPLAY_DROPDOWN,
                CBCGToolbarMenuButton (-1, *menu.GetSubMenu(0),
                    CImageHash::GetImageOfCommand(ID_DISPLAY_DROPDOWN), _T("Show Area")));
            m_wndBar1.ReplaceButton(ID_CHARSET_DROPDOWN,
                CBCGToolbarMenuButton (-1, *menu.GetSubMenu(1),
                    CImageHash::GetImageOfCommand(ID_CHARSET_DROPDOWN), _T("Char Set")));
            m_wndBar1.ReplaceButton(ID_CONTROL_DROPDOWN,
                CBCGToolbarMenuButton (-1, *menu.GetSubMenu(2),
                    CImageHash::GetImageOfCommand(ID_CONTROL_DROPDOWN), _T("Ctrl Chars")));
        }
        m_wndBar1.ReplaceButton(ID_SCHEME, CSchemeComboButton());
        break;
    case IDR_EDITBAR:
        // Only req. because they're on default edit bar (customisation handled by BCG)
        m_wndBar2.ReplaceButton(ID_SEARCH, CFindComboButton());
        m_wndBar2.ReplaceButton(ID_JUMP_HEX, CHexComboButton());
        m_wndBar2.ReplaceButton(ID_JUMP_DEC, CDecComboButton());
        break;
    case IDR_FORMATBAR:
        {

            m_wndBar3.ReplaceButton(IDC_FONTNAME,
                CHexEditFontCombo(IDC_FONTNAME, 
                    CImageHash::GetImageOfCommand(IDC_FONTNAME, FALSE),
                    RASTER_FONTTYPE | TRUETYPE_FONTTYPE | DEVICE_FONTTYPE,
                    DEFAULT_CHARSET,
                    WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST |
                    CBS_AUTOHSCROLL | CBS_HASSTRINGS | CBS_OWNERDRAWFIXED, 175) );

#if 0  // We need to call InsertButton else the size does not get updated properly for some reason
            m_wndBar3.ReplaceButton(IDC_FONTSIZE,
                CHexEditFontSizeCombo(IDC_FONTSIZE, 
                    CImageHash::GetImageOfCommand(IDC_FONTSIZE, FALSE),
                    WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWN, 50) );
#else
            int index = m_wndBar3.CommandToIndex(IDC_FONTSIZE);
            if (index == -1 || index > m_wndBar3.GetCount()) index = m_wndBar3.GetCount();
            m_wndBar3.RemoveButton(index);
            m_wndBar3.InsertButton(
                CHexEditFontSizeCombo(IDC_FONTSIZE, 
                    CImageHash::GetImageOfCommand(IDC_FONTSIZE, FALSE),
                    WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWN, 50),
                index);
#endif

            CMenu menu;
            menu.LoadMenu(IDR_MENUBUTTON);

#if 0  // No longer there (as drop-menus)
            m_wndBar3.ReplaceButton(ID_DISPLAY_DROPDOWN,
                CBCGToolbarMenuButton (-1, *menu.GetSubMenu(0),
                    CImageHash::GetImageOfCommand(ID_DISPLAY_DROPDOWN), _T("Show Area")));
            m_wndBar3.ReplaceButton(ID_CHARSET_DROPDOWN,
                CBCGToolbarMenuButton (-1, *menu.GetSubMenu(1),
                    CImageHash::GetImageOfCommand(ID_CHARSET_DROPDOWN), _T("Char Set")));
            m_wndBar3.ReplaceButton(ID_CONTROL_DROPDOWN,
                CBCGToolbarMenuButton (-1, *menu.GetSubMenu(2),
                    CImageHash::GetImageOfCommand(ID_CONTROL_DROPDOWN), _T("Ctrl Chars")));
#endif
            m_wndBar3.ReplaceButton(ID_HIGHLIGHT_MENU,
                CBCGToolbarMenuButton(ID_HIGHLIGHT, *menu.GetSubMenu(3), CImageHash::GetImageOfCommand(ID_HIGHLIGHT)));

        }
        m_wndBar3.ReplaceButton(ID_SCHEME, CSchemeComboButton());
        break;
    case IDR_NAVBAR:
        {
            m_wndBar4.ReplaceButton(ID_JUMP_HEX, CHexComboButton());
            m_wndBar4.ReplaceButton(ID_JUMP_DEC, CDecComboButton());
            m_wndBar4.ReplaceButton(ID_BOOKMARKS, CBookmarksComboButton());

            CMenu menu;
            menu.LoadMenu(IDR_MENUBUTTON);
            m_wndBar4.ReplaceButton(ID_MARK_MENU,
                CBCGToolbarMenuButton(ID_GOTO_MARK, *menu.GetSubMenu(4), CImageHash::GetImageOfCommand(ID_GOTO_MARK)));
            m_wndBar4.ReplaceButton(ID_BOOKMARKS_MENU,
                CBCGToolbarMenuButton(ID_BOOKMARKS_EDIT, *menu.GetSubMenu(5), CImageHash::GetImageOfCommand(ID_BOOKMARKS_EDIT)));
            m_wndBar4.ReplaceButton(ID_NAV_BACK,
                CBCGToolbarMenuButton(ID_NAV_BACK, *menu.GetSubMenu(6), CImageHash::GetImageOfCommand(ID_NAV_BACK)));
            m_wndBar4.ReplaceButton(ID_NAV_FORW,
                CBCGToolbarMenuButton(ID_NAV_FORW, *menu.GetSubMenu(7), CImageHash::GetImageOfCommand(ID_NAV_FORW)));
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
//  CBCGToolbarCustomize* pDlg = (CBCGToolbarCustomize*)lp;
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
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);

    return 0;
}

void CMainFrame::OnClosePopupMenu(CBCGPopupMenu *pMenuPopup)
{
    bool found = false;
    while (!popup_menu_.empty() && !found)
    {
        found = popup_menu_.back() == pMenuPopup;
        popup_menu_.pop_back();
    }
    CBCGMDIFrameWnd::OnClosePopupMenu(pMenuPopup);
    show_tip();
}

BOOL CMainFrame::OnShowPopupMenu (CBCGPopupMenu *pMenuPopup)
{
    CBCGMDIFrameWnd::OnShowPopupMenu(pMenuPopup);

    if (pMenuPopup == NULL)
    {
        return TRUE;
    }
    popup_menu_.push_back(pMenuPopup);
    TRACE("OPEN POPUP %p %x\n", pMenuPopup, pMenuPopup->m_hWnd);
    
    if (pMenuPopup->GetMenuBar()->CommandToIndex(ID_VIEW_TOOLBARS) >= 0)
    {
        if (CBCGToolBar::IsCustomizeMode())
        {
            return FALSE;
        }
        
        pMenuPopup->RemoveAllItems();
        
        CMenu menu;
        VERIFY(menu.LoadMenu(IDR_CONTEXT_BARS));
        
        CMenu* pPopup = menu.GetSubMenu(0);
        ASSERT(pPopup != NULL);
        
        SetupToolbarMenu(*pPopup, ID_VIEW_USER_TOOLBAR1, ID_VIEW_USER_TOOLBAR10);
        pMenuPopup->GetMenuBar()->ImportFromMenu (*pPopup, TRUE);
    }

    if (pMenuPopup->GetMenuBar()->CommandToIndex(ID_NAV_BACK_DUMMY) >= 0)
    {
		if (CBCGToolBar::IsCustomizeMode ())
		{
			return FALSE;
		}

		pMenuPopup->RemoveAllItems ();
		theApp.navman_.AddItems(pMenuPopup, false, ID_NAV_BACK_FIRST, NAV_RESERVED);
	}
    if (pMenuPopup->GetMenuBar()->CommandToIndex(ID_NAV_FORW_DUMMY) >= 0)
    {
		if (CBCGToolBar::IsCustomizeMode ())
		{
			return FALSE;
		}

		pMenuPopup->RemoveAllItems ();
		theApp.navman_.AddItems(pMenuPopup, true, ID_NAV_FORW_FIRST, NAV_RESERVED);
	}

    if (pMenuPopup->GetMenuBar()->CommandToIndex(ID_DFFD_OPEN_TYPE_DUMMY) >= 0)
    {
		if (CBCGToolBar::IsCustomizeMode ())
		{
			return FALSE;
		}

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
				ss = sfi.szTypeName;          // Store file type for display in file page

                pMenuPopup->InsertItem(CBCGToolbarMenuButton(ID_DFFD_OPEN_FIRST + ii, NULL, -1, ss));
				++count;
			}
		}
		if (count == 0)
		{
			CBCGToolbarMenuButton mb(ID_DFFD_OPEN_TYPE_DUMMY, NULL, -1, "No File Type Templates Found");
			mb.SetStyle(mb.m_nStyle | TBBS_DISABLED);
            pMenuPopup->InsertItem(mb);
		}
	}
    if (pMenuPopup->GetMenuBar()->CommandToIndex(ID_DFFD_OPEN_OTHER_DUMMY) >= 0)
    {
		if (CBCGToolBar::IsCustomizeMode ())
		{
			return FALSE;
		}

		pMenuPopup->RemoveAllItems ();

		int count = 0;
		for (int ii = 0; ii < theApp.xml_file_name_.size() && ii < DFFD_RESERVED; ++ii)
		{
			if (theApp.xml_file_name_[ii].CompareNoCase("default") != 0 &&
				theApp.xml_file_name_[ii][0] != '_')
			{
                pMenuPopup->InsertItem(CBCGToolbarMenuButton(ID_DFFD_OPEN_FIRST + ii, NULL, -1, theApp.xml_file_name_[ii]));
				++count;
			}
		}
		if (count == 0)
		{
			CBCGToolbarMenuButton mb(ID_DFFD_OPEN_TYPE_DUMMY, NULL, -1, "No Other Templates Found");
			mb.SetStyle(mb.m_nStyle | TBBS_DISABLED);
            pMenuPopup->InsertItem(mb);
		}
	}

    return TRUE;
}

void CMainFrame::OnUpdateSearchCombo(CCmdUI* pCmdUI)
{
    if (GetView() == NULL)
    {
        pCmdUI->Enable(FALSE);
        return;
    }

//    TRACE3("OnUpdateSearchCombo ID %d index %d hwnd %x\n", pCmdUI->m_nID, pCmdUI->m_nIndex, pCmdUI->m_pOther->m_hWnd);
    if (pCmdUI->m_pOther != NULL && pCmdUI->m_pOther->GetDlgCtrlID() == ID_SEARCH_COMBO)
    {
        CFindComboBox *pp = static_cast<CFindComboBox *>(pCmdUI->m_pOther);
        ASSERT(::IsWindow(pp->GetSafeHwnd()));

        ASSERT(GetView() != NULL);
        CString strCurr;
        pp->GetWindowText(strCurr);

        pp->EnableWindow(TRUE);

        // Fix up the drop down list
        if (!pp->GetDroppedState() && ComboNeedsUpdate(search_hist_, pp))
        {
            DWORD sel = pp->GetEditSel();
            int max_str = 0;                // Max width of all the strings added so far
//                CDC *pDC = pp->GetDC();         // Drawing context of the combo box
//                ASSERT(pDC != NULL);
            CClientDC dc(pp);
            int nSave = dc.SaveDC();
            dc.SelectObject(pp->GetFont());

            pp->ResetContent();
            for (std::vector<CString>::iterator ps = search_hist_.begin();
                 ps != search_hist_.end(); ++ps)
            {
//                    CSize str_size = dc.GetTextExtent(*ps);
//                    if (str_size.cx > max_str) max_str = str_size.cx;
                max_str = __max(max_str, dc.GetTextExtent(*ps).cx);

                // Add the string to the list
                pp->InsertString(0, *ps);
            }
            pp->SetWindowText(strCurr);
            pp->SetEditSel(LOWORD(sel), HIWORD(sel));

            // Add space for margin and possible scrollbar
            max_str += dc.GetTextExtent("0").cx + ::GetSystemMetrics(SM_CXVSCROLL);
            pp->SetDroppedWidth(__min(max_str, 780));

            dc.RestoreDC(nSave);
        }

        if (strCurr != current_search_string_)
            pp->SetWindowText(current_search_string_);
    }
    pCmdUI->Enable(TRUE);
}

void CMainFrame::OnUpdateHexCombo(CCmdUI* pCmdUI)
{
    if (pCmdUI->m_pOther != NULL && pCmdUI->m_pOther->GetDlgCtrlID() == ID_JUMP_HEX_COMBO)
    {
        CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
        CHexComboBox *pp = static_cast<CHexComboBox *>(pCmdUI->m_pOther);
        ASSERT(::IsWindow(pp->GetSafeHwnd()));

        if (GetView() != NULL)
        {
            CString strCurr;
            pp->GetWindowText(strCurr);

            pp->EnableWindow(TRUE);

            // Fix up the drop down list
            if (!pp->GetDroppedState() && ComboNeedsUpdate(hex_hist_, pp))
            {
                DWORD sel = pp->GetEditSel();
                int max_str = 0;                // Max width of all the strings added so far
//                CDC *pDC = pp->GetDC();         // Drawing context of the combo box
                CClientDC dc(pp);
                int nSave = dc.SaveDC();
                dc.SelectObject(pp->GetFont());

                // Rebuild list
                pp->ResetContent();
                for (std::vector<CString>::iterator ps = hex_hist_.begin();
                     ps != hex_hist_.end(); ++ps)
                {
                    max_str = __max(max_str, dc.GetTextExtent(*ps).cx);

                    // Add the string to the list
                    pp->InsertString(0, *ps);
                }

                // Set the edit text back the way it was (removed by ResetContent)
                pp->SetWindowText(strCurr);
                pp->SetEditSel(LOWORD(sel), HIWORD(sel));

                // Set the drop list width based on the widest string
                max_str += dc.GetTextExtent("0").cx + ::GetSystemMetrics(SM_CXVSCROLL);
                pp->SetDroppedWidth(__min(max_str, 400));

                dc.RestoreDC(nSave);
            }

            int ac;
            CJumpExpr::value_t vv(-1);
            if (!strCurr.IsEmpty())
                vv = expr_.evaluate(strCurr, 0 /*unused*/, ac /*unused*/, 16 /*hex int*/);

            // Fix up the edit text (if different)
            if (strCurr != current_hex_address_ &&
                vv.typ == CJumpExpr::TYPE_INT && vv.int64 != current_address_)
            {
                CHexEditControl *pedit = (CHexEditControl *)pp->GetWindow(GW_CHILD);

                ASSERT_KINDOF(CHexEditControl, pedit);

                if (pedit->IsKindOf(RUNTIME_CLASS(CHexEditControl)))
                {
                    char buf[22];
                    if (aa->hex_ucase_)
                        sprintf(buf, "%I64X", __int64(current_address_));
                    else
                        sprintf(buf, "%I64x", __int64(current_address_));
                    pedit->SetWindowText(buf);
                    pedit->add_spaces();
                }
            }
        }
        else
            pp->EnableWindow(FALSE);
    }
}

void CMainFrame::OnUpdateDecCombo(CCmdUI* pCmdUI)
{
    if (pCmdUI->m_pOther != NULL && pCmdUI->m_pOther->GetDlgCtrlID() == ID_JUMP_DEC_COMBO)
    {
        CDecComboBox *pp = static_cast<CDecComboBox *>(pCmdUI->m_pOther);
        ASSERT(::IsWindow(pp->GetSafeHwnd()));

        if (GetView() != NULL)
        {
            CString strCurr;
            pp->GetWindowText(strCurr);

            pp->EnableWindow(TRUE);

            // Fix up the drop down list
            if (!pp->GetDroppedState() && ComboNeedsUpdate(dec_hist_, pp))
            {
                DWORD sel = pp->GetEditSel();
                int max_str = 0;                // Max width of all the strings added so far
//                CDC *pDC = pp->GetDC();         // Drawing context of the combo box
                CClientDC dc(pp);
                int nSave = dc.SaveDC();
                dc.SelectObject(pp->GetFont());

                pp->ResetContent();
                for (std::vector<CString>::iterator ps = dec_hist_.begin();
                     ps != dec_hist_.end(); ++ps)
                {
                    max_str = __max(max_str, dc.GetTextExtent(*ps).cx);

                    // Add the string to the list
                    pp->InsertString(0, *ps);
                }
                pp->SetWindowText(strCurr);
                pp->SetEditSel(LOWORD(sel), HIWORD(sel));

                // Add space for margin and possible scrollbar
                max_str += dc.GetTextExtent("0").cx + ::GetSystemMetrics(SM_CXVSCROLL);
                pp->SetDroppedWidth(__min(max_str, 400));

                dc.RestoreDC(nSave);
            }
            int ac;
            CJumpExpr::value_t vv(-1);
            if (!strCurr.IsEmpty())
                vv = expr_.evaluate(strCurr, 0 /*unused*/, ac /*unused*/, 10 /*dec int*/);

            if (strCurr != current_dec_address_ &&
                vv.typ == CJumpExpr::TYPE_INT && vv.int64 != current_address_)
            {
                CDecEditControl *pedit = (CDecEditControl *)pp->GetWindow(GW_CHILD);

                ASSERT_KINDOF(CDecEditControl, pedit);

                if (pedit->IsKindOf(RUNTIME_CLASS(CDecEditControl)))
                {
                    char buf[22];
                    sprintf(buf, "%I64d", __int64(current_address_));
                    pedit->SetWindowText(buf);
                    pedit->add_commas();
                }
            }
        }
        else
            pp->EnableWindow(FALSE);
    }
}

void CMainFrame::OnUpdateSchemeCombo(CCmdUI* pCmdUI)
{
    CHexEditView *pview = GetView();
    if (pview == NULL)
    {
        pCmdUI->Enable(FALSE);
        return;
    }

    if (pCmdUI->m_pOther != NULL && pCmdUI->m_pOther->GetDlgCtrlID() == (theApp.is_us_ ? ID_SCHEME_COMBO_US : ID_SCHEME_COMBO))
    {
        CSchemeComboBox *pp = static_cast<CSchemeComboBox *>(pCmdUI->m_pOther);
        ASSERT(::IsWindow(pp->GetSafeHwnd()));

        ASSERT(pview != NULL);
        pp->EnableWindow(TRUE);

        // Work out current scheme of active view and get vector of scheme names
        std::vector<CString> scheme_names;
        int current_scheme = -1;

        // Build list backwards as ComboNeedsUpdate() assumes top of list at bottom of vector
        for (int ii = theApp.scheme_.size(); ii > 0; ii--)
        {
            scheme_names.push_back(theApp.scheme_[ii-1].name_);
            if (theApp.scheme_[ii-1].name_ == pview->GetSchemeName())
                current_scheme = ii - 1;
        }

        // Fix up the drop down list
        if (!pp->GetDroppedState() && ComboNeedsUpdate(scheme_names, pp))
        {
            int max_str = 0;                // Max width of all the strings added so far
            CClientDC dc(pp);
            int nSave = dc.SaveDC();
            dc.SelectObject(pp->GetFont());

            pp->ResetContent();
            for (std::vector<CString>::reverse_iterator ps = scheme_names.rbegin();
                 ps != scheme_names.rend(); ++ps)
            {
                max_str = __max(max_str, dc.GetTextExtent(*ps).cx);

                // Add the string to the list
                pp->AddString(*ps);
            }
            // Add space for margin and possible scrollbar
            max_str += dc.GetTextExtent("0").cx + ::GetSystemMetrics(SM_CXVSCROLL);
            pp->SetDroppedWidth(__min(max_str, 640));

            dc.RestoreDC(nSave);
        }
        if (!pp->GetDroppedState() && pp->GetCurSel() != current_scheme)
            pp->SetCurSel(current_scheme);
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
    if (pview == NULL)
    {
        pCmdUI->Enable(FALSE);
        return;
    }
    CHexEditDoc *pdoc = (CHexEditDoc *)pview->GetDocument();

    if (pCmdUI->m_pOther != NULL && pCmdUI->m_pOther->GetDlgCtrlID() == ID_BOOKMARKS_COMBO)
    {
        CBookmarksComboBox *pp = static_cast<CBookmarksComboBox *>(pCmdUI->m_pOther);
        ASSERT(::IsWindow(pp->GetSafeHwnd()));

        ASSERT(pview != NULL && pdoc != NULL);
        if (pdoc->bm_index_.empty())
        {
            CString ss;
            if (pp->GetCount() > 0) pp->GetLBText(0, ss);

            if (pp->GetCount() != 1 || ss != CString("No bookmarks"))
            {
                pp->ResetContent();
                pp->SetCurSel(pp->AddString("No bookmarks"));
            }
            pp->EnableWindow(FALSE);
            return;
        }
        pp->EnableWindow(TRUE);

        // Get vector of bookmark names
        CBookmarkList *pbl = theApp.GetBookmarkList();
        std::vector<CString> bm_names;
        FILE_ADDRESS diff;
        int current_bm = pview->ClosestBookmark(diff);  // Get the closest bookmark to the cursor or -1

        // Build list of bookmarks in current doc
        for (int ii = 0; ii < (int)pdoc->bm_index_.size(); ++ii)
        {
            ASSERT(pdoc->bm_index_[ii] < (int)pbl->name_.size());
            bm_names.push_back(pbl->name_[pdoc->bm_index_[ii]]);
        }

        // We must sort since the combo box sorts the bookmarks alphabetically (CBS_SORT)
        sort(bm_names.begin(), bm_names.end(), case_insensitive_greater());

        // Fix up the drop down list
        if (!pp->GetDroppedState() && ComboNeedsUpdate(bm_names, pp))
        {
            int max_str = 0;                // Max width of all the strings added so far
            CClientDC dc(pp);
            int nSave = dc.SaveDC();
            dc.SelectObject(pp->GetFont());

            // Add all the bookmarks in this document to the list
            pp->ResetContent();
            for (int ii = 0; ii < (int)pdoc->bm_index_.size(); ++ii)
            {
                ASSERT(pdoc->bm_index_[ii] < (int)pbl->name_.size());
                max_str = __max(max_str, dc.GetTextExtent(pbl->name_[pdoc->bm_index_[ii]]).cx);
                int ind = pp->AddString(pbl->name_[pdoc->bm_index_[ii]]);
                ASSERT(ind != CB_ERR);

                // Store the bookmark list index in the list box too
                pp->SetItemData(ind, pdoc->bm_index_[ii]);
            }
            // Add space for margin and possible scrollbar
            max_str += dc.GetTextExtent("0").cx + ::GetSystemMetrics(SM_CXVSCROLL);
            pp->SetDroppedWidth(__min(max_str, 640));

            dc.RestoreDC(nSave);
        }
		else
		{
		    // Even though the strings may be the same an bookmark may have been moved and have a new index in the global list
            for (int ii = 0; ii < (int)pdoc->bm_index_.size(); ++ii)
            {
				int ind = pp->FindString(0, pbl->name_[pdoc->bm_index_[ii]]);
                if (ind != CB_ERR)
                    pp->SetItemData(ind, pdoc->bm_index_[ii]);
			}
		}

        if (!pp->GetDroppedState() && pp->GetItemData(pp->GetCurSel()) != current_bm)
        {
            if (current_bm == -1)
                pp->SetCurSel(-1);
            else
            {
                // Search through the sorted combo list to find the associated entry
                for (int ii = 0; ii < pp->GetCount(); ++ii)
                {
                    if (pp->GetItemData(ii) == current_bm)
                    {
                        // Found it so select it
                        pp->SetCurSel(ii);
                        break;
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
    CString vars = theApp.GetProfileString("Calculator", "Vars");
    CString ss;

	for (const char *pp = vars; *pp != '\0'; ++pp)
	{
		const char *pp2;

		// Get next var name
		pp2 = strchr(pp, '=');
		if (pp2 == NULL)
		{
			return false;
		}
		CString name(pp, pp2 - pp);
		if (pp2 - pp >
			strspn(name, "abcdefghijklmonpqrstuvwxyz"
                         "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                         "0123456789_$"))
		{
			return false;  // not a valid variable name
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
				return false;
            tmp = value_t(true);
			pp2 = pp + 4;
			break;
		case 'F':
			if (strncmp(pp, "FALSE", 5) != 0)
				return false;
            tmp = value_t(false);
			pp2 = pp + 5;
			break;

		default:
			return false;
		}
        var_[name] = tmp;
		var_changed_ = clock();

		// Make sure we see terminator (;)
		if (*pp2 != ';')
			return false;
		pp = pp2;
	}
	return true;
}

// xxx need to use Unicode string since a string var is Unicode (if UNICODE_TYPE_STRING defined)
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
