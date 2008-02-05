// HexEdit.cpp : Defines the class behaviors for the application.
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

// Note: Removed Stingray stuff: SECWorkSheet, SECWorkBook, SECDialogBar, SECStatusBar
//       SetTabIcon(), SetWorkbookMode(), m_hIcon, secall.h, secres.h, secres.rc

#include "stdafx.h"
#include <locale.h>

#include <afxadv.h>     // for CRecentFileList
#include <io.h>         // for _access()

#include <bcghelpids.h>     // For help on customize dlg

// #include <afxhtml.h>    // for CHtmlView

#include <MAPI.h>       // for MAPI constants

#include <shlwapi.h>    // for SHDeleteKey

#define COMPILE_MULTIMON_STUBS 1   // Had to remove this to link with BCG static lib
#include <MultiMon.h>   // for multiple monitor support
#undef COMPILE_MULTIMON_STUBS

#include "HexEdit.h"

#include <HtmlHelp.h>

#include "EmailDlg.h"
#include "HexFileList.h"
#include "RecentFileDlg.h"
#include "BookmarkDlg.h"
#include "Bookmark.h"
#include "BookmarkDlg.h"
#include "ClearHistDlg.h"
#include "boyer.h"
#include "SystemSound.h"

#include "MainFrm.h"
#include "ChildFrm.h"
#include "HexEditDoc.h"
#include "HexEditView.h"
#include "DataFormatView.h"
#include "TabView.h"
#include "UserTool.h"   // For CHexEditUserTool
#include "Dialog.h"
#include "OpenSpecialDlg.h"
#include "Register.h"   // For About dialog
#include "Algorithm.h"  // For encruption algorithm selection
#include "CompressDlg.h" // For compression settings dialog
#include "Password.h"   // For encryption password dialog
#include "Misc.h"
#include "SpecialList.h"    // For volume/device list (Open Special etc)


// The following is not in a public header
extern BOOL AFXAPI AfxFullPath(LPTSTR lpszPathOut, LPCTSTR lpszFileIn);

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// The following is the subkey of HKEY_CLASSES_ROOT that is used to enable the
// "Open with HexEdit" shell shortcut menu option.
static const char *HEXEDIT_SUBKEY = "*\\shell\\HexEdit";
static const char *HEXEDIT_SUBSUBKEY  = "*\\shell\\HexEdit\\command";

#ifndef NO_SECURITY
#include "security.h"
int quick_check_called = 0;
int get_security_called = 0;
int dummy_to_confuse = 0;
int add_security_called = 0;
int new_check_called = 0;
#endif
extern DWORD hid_last_file_dialog = HIDD_FILE_OPEN;

/////////////////////////////////////////////////////////////////////////////
// CHexEditDocManager

class CHexEditDocManager : public CDocManager
{
public:
	// We have to do it this way as CWinApp::DoPromptFileName is not virtual for some reason
	virtual BOOL DoPromptFileName(CString& fileName, UINT nIDSTitle,
			DWORD lFlags, BOOL bOpenFileDialog, CDocTemplate* pTemplate)
	{
		hid_last_file_dialog = HIDD_FILE_SAVE;
		return CDocManager::DoPromptFileName(fileName, nIDSTitle, lFlags | OFN_SHOWHELP | OFN_ENABLESIZING, bOpenFileDialog, pTemplate);
	}
};

/////////////////////////////////////////////////////////////////////////////
// CHexEditApp
const char *CHexEditApp::szHexEditClassName = "HexEditMDIFrame";

#ifdef _DEBUG
const int CHexEditApp::security_version_ = 6; // This is changed for testing of handling of versions (registration etc)
#else
const int CHexEditApp::security_version_ = INTERNAL_VERSION;
#endif

BEGIN_MESSAGE_MAP(CHexEditApp, CWinApp)
        ON_COMMAND(CG_IDS_TIPOFTHEDAY, ShowTipOfTheDay)
        //{{AFX_MSG_MAP(CHexEditApp)
        ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
        ON_COMMAND(ID_OPTIONS, OnOptions)
        ON_COMMAND(ID_OPTIONS2, OnOptions2)
        ON_COMMAND(ID_RECORD, OnMacroRecord)
        ON_COMMAND(ID_PLAY, OnMacroPlay)
        ON_UPDATE_COMMAND_UI(ID_PLAY, OnUpdateMacroPlay)
        ON_UPDATE_COMMAND_UI(ID_RECORD, OnUpdateMacroRecord)
        ON_COMMAND(ID_PROPERTIES, OnProperties)
        ON_COMMAND(ID_MULTI_PLAY, OnMultiPlay)
        ON_COMMAND(ID_HELP_EMAIL, OnHelpEmail)
        ON_UPDATE_COMMAND_UI(ID_HELP_EMAIL, OnUpdateHelpEmail)
        ON_COMMAND(ID_HELP_WEB, OnHelpWeb)
        ON_UPDATE_COMMAND_UI(ID_HELP_WEB, OnUpdateHelpWeb)
        ON_COMMAND(ID_WEB_PAGE, OnWebPage)
        ON_COMMAND(ID_ENCRYPT_ALG, OnEncryptAlg)
        ON_COMMAND(ID_ENCRYPT_CLEAR, OnEncryptClear)
        ON_UPDATE_COMMAND_UI(ID_ENCRYPT_CLEAR, OnUpdateEncryptClear)
        ON_COMMAND(ID_ENCRYPT_PASSWORD, OnEncryptPassword)
        ON_COMMAND(ID_MACRO_MESSAGE, OnMacroMessage)
        ON_UPDATE_COMMAND_UI(ID_MACRO_MESSAGE, OnUpdateMacroMessage)
        ON_UPDATE_COMMAND_UI(ID_MULTI_PLAY, OnUpdateMultiPlay)
        ON_COMMAND(ID_RECENT_FILES, OnRecentFiles)
        ON_COMMAND(ID_BOOKMARKS_EDIT, OnBookmarksEdit)
        ON_COMMAND(ID_TAB_ICONS, OnTabIcons)
        ON_COMMAND(ID_TABS_AT_BOTTOM, OnTabsAtBottom)
        ON_UPDATE_COMMAND_UI(ID_TAB_ICONS, OnUpdateTabIcons)
        ON_UPDATE_COMMAND_UI(ID_TABS_AT_BOTTOM, OnUpdateTabsAtBottom)
	ON_COMMAND(ID_FILE_OPEN_SPECIAL, OnFileOpenSpecial)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPEN_SPECIAL, OnUpdateFileOpenSpecial)
	//}}AFX_MSG_MAP
        ON_COMMAND(ID_ZLIB_SETTINGS, OnCompressionSettings)
        ON_COMMAND(ID_HELP_FORUM, OnHelpWebForum)
        ON_COMMAND(ID_HELP_FAQ, OnHelpWebFaq)
        ON_COMMAND(ID_HELP_HOMEPAGE, OnHelpWebHome)
        ON_COMMAND(ID_HELP_REGISTER, OnHelpWebReg)
        ON_UPDATE_COMMAND_UI(ID_HELP_FORUM, OnUpdateHelpWeb)
        ON_UPDATE_COMMAND_UI(ID_HELP_FAQ, OnUpdateHelpWeb)
        ON_UPDATE_COMMAND_UI(ID_HELP_HOMEPAGE, OnUpdateHelpWeb)
        ON_UPDATE_COMMAND_UI(ID_HELP_REGISTER, OnUpdateHelpWeb)

		// Repair commands
        ON_COMMAND(ID_REPAIR_DIALOGBARS, OnRepairDialogbars)
        ON_COMMAND(ID_REPAIR_CUST, OnRepairCust)
        ON_COMMAND(ID_REPAIR_SETTINGS, OnRepairSettings)
        ON_COMMAND(ID_REPAIR_ALL, OnRepairAll)
//        ON_COMMAND(ID_REPAIR_CHECK, OnRepairCheck)

		ON_COMMAND(ID_FILE_SAVE_ALL, OnFileSaveAll)
		ON_COMMAND(ID_FILE_CLOSE_ALL, OnFileCloseAll)

        // Standard file based document commands
        ON_COMMAND(ID_FILE_NEW, OnFileNew)
        ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
        ON_COMMAND_EX_RANGE(ID_FILE_MRU_FILE1, ID_FILE_MRU_FILE16, OnOpenRecentFile)
        ON_COMMAND(ID_APP_EXIT, OnAppExit)
        // Standard print setup command
        ON_COMMAND(ID_FILE_PRINT_SETUP, OnFilePrintSetup)
        // Message sent to mainthread
        ON_THREAD_MESSAGE(WM_USER+12, OnBGSearchFinished)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHexEditApp construction

#ifdef _DEBUG
// Memory allocation hook function used for debugging memory allocation exceptions
#if _MSC_VER <= 1100
    static int alloc_hook( int allocType, void *userData, size_t size, int blockType, 
        long requestNumber, const char *filename, int lineNumber)
#else
    // For some reason this declaration has changed slightly (filename is now uchar)
    static int alloc_hook( int allocType, void *userData, size_t size, int blockType, 
        long requestNumber, const unsigned char *filename, int lineNumber)
#endif
{
    BOOL retval = TRUE;         // TRUE = proceed normally, FALSE = alloc error
    // Change retval to 0 (in debugger) before returning to test error handling
    switch (allocType)
    {
    case _HOOK_ALLOC:           // malloc/calloc <= C++ new
        return retval;
    case _HOOK_REALLOC:         // realloc
        return retval;
    case _HOOK_FREE:            // free <= C++ delete & delete[]
        return retval;
    }
    ASSERT(0);
    return TRUE;
}
#endif

CHexEditApp::CHexEditApp() : CBCGWorkspace(TRUE), default_scheme_(""),
                             default_ascii_scheme_(ASCII_NAME), default_ansi_scheme_(ANSI_NAME),
                             default_oem_scheme_(OEM_NAME), default_ebcdic_scheme_(EBCDIC_NAME)
{
    // Add a memory allocation hook for debugging purposes
    // (Does nothing in release version.)
    _CrtSetAllocHook(&alloc_hook);
    recording_ = FALSE;
	macro_version_ = INTERNAL_VERSION;
    highlight_ = FALSE;
    playing_ = 0;
    pv_ = pview_ = NULL;
    refresh_off_ = false;
    open_plf_ = open_oem_plf_ = NULL;
    last_cb_size_ = 0;
#ifndef NDEBUG
    // Make default capacity for mac_ vector small to force reallocation sooner.
    // This increases likelihood of catching bugs related to reallocation.
    mac_.reserve(2);
#else
    // Pre-allocate room for 128 elts for initial speed
    mac_.reserve(128);
#endif
    security_type_ = -1;                // Init so we can detect if code has been bypassed
    security_rand_ = 0;

    open_disp_state_ = -1;

    // Set up the default colour scheme
    default_scheme_.AddRange("Other", -1, "0:255");
    default_scheme_.mark_col_ = -1;
    default_scheme_.hex_addr_col_ = -1;
    default_scheme_.dec_addr_col_ = -1;

    default_ascii_scheme_.AddRange("ASCII text", -1, "32:126");
    default_ascii_scheme_.AddRange("Special (TAB,LF,CR)", RGB(0,128,0), "9,10,13");
    default_ascii_scheme_.AddRange("Other", RGB(255,0,0), "0:255");
    default_ascii_scheme_.bg_col_ = RGB(255, 253, 244);
    default_ascii_scheme_.addr_bg_col_ = RGB(224, 240, 224);

    default_ansi_scheme_.AddRange("ASCII text", -1, "32:126");
    default_ansi_scheme_.AddRange("Special (TAB,LF,CR)", RGB(0,128,0), "9,10,13");
    default_ansi_scheme_.AddRange("ANSI text", RGB(0,0,128), "130:140,145:156,159:255");
    default_ansi_scheme_.AddRange("Other", RGB(255,0,0), "0:255");

    default_oem_scheme_.AddRange("ASCII text", -1, "32:126");
    default_oem_scheme_.AddRange("Other", RGB(255,0,0), "0:255");
    default_oem_scheme_.bg_col_ = RGB(255, 248, 255);
    default_oem_scheme_.addr_bg_col_ = RGB(240, 224, 240);

    default_ebcdic_scheme_.AddRange("EBCDIC text", -1, "64,74:80,90:97,106:111,121:127,"
                                    "129:137,145:153,161:169,192:201,208:217,224,226:233,240:249");
    default_ebcdic_scheme_.AddRange("Control", RGB(0,0,128), "0:7,10:34"
                                    "36:39,42:43,45:47,50,52:55,59:61,63");
    default_ebcdic_scheme_.AddRange("Unassigned", RGB(255,0,0), "0:255");
    default_ebcdic_scheme_.bg_col_ = RGB(240, 248, 255);
    default_ebcdic_scheme_.addr_bg_col_ = RGB(192, 224, 240);

    pboyer_ = NULL;

    algorithm_ = 0;   // Default to built-in encryption

    p_last_page_ = NULL;
    p_general = NULL;
    p_sysdisplay = NULL;
    p_windisplay = NULL;
//    p_partitions = NULL;  // no longer used when schemes added
    p_colours = NULL;
    p_macro = NULL;
    p_printer = NULL;
    p_filters = NULL;
	p_tips = NULL;

    m_pbookmark_list = NULL;
    open_file_shared_ = FALSE;
    open_current_readonly_ = -1;

	delete_reg_settings_ = FALSE;
	delete_all_settings_ = FALSE;

	// MFC 7.1 has special HTML help mode (but this stuffed our use of HTML help)
	SetHelpMode(afxHTMLHelp);
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CHexEditApp object

CHexEditApp theApp;

UINT CHexEditApp::wm_hexedit = ::RegisterWindowMessage("HexEditOpenMessage");

/////////////////////////////////////////////////////////////////////////////
// CHexEditApp initialization

BOOL CHexEditApp::InitInstance()
{
        if (!AfxOleInit())              // For BCG and COM (calls CoInitialize())
        {
            AfxMessageBox("OLE initialization failed.  Make sure that the OLE libraries are the correct version.");
            return FALSE;
        }
        InitCommonControls();

		// Override the document manager so we can make save dialog resizeable
		if (m_pDocManager != NULL) delete m_pDocManager;
		m_pDocManager = new CHexEditDocManager;

        // Get HexEdit version info
        DWORD dummy;                                        // Version functions take parameters that do nothing?!
        size_t vi_size;                                     // Size of all version info
        void *buf;                                          // Buffer to hold all version info
        void *p_version;                                    // Holds ptr to product version string in buffer
        UINT len;                                           // Length of product version string

        if ((vi_size = ::GetFileVersionInfoSize(__argv[0], &dummy)) > 0 &&
            (buf = malloc(vi_size+1)) != NULL &&
            ::GetFileVersionInfo(__argv[0], dummy, vi_size, buf) &&
            ::VerQueryValue(buf, "\\StringFileInfo\\040904B0\\ProductVersion",
                            &p_version, &len) )
        {
            char *endptr;
            version_ = short(strtol((char *)p_version, &endptr, 10) * 100);
            if (*endptr != '\0')
            {
                ++endptr;
                version_ += short(strtol(endptr, &endptr, 10));

                // Check if a beta version
				if (*endptr != '\0')
				{
					++endptr;
					beta_ = (short)strtol(endptr, &endptr, 10);

					if (*endptr != '\0')
					{
						++endptr;
						revision_ = (short)strtol(endptr, &endptr, 10);
					}
                }
            }
        }
        // Seed the random number generators
        unsigned int seed = ::GetTickCount();
        srand(seed);                    // Seed compiler PRNG (simple one)

#ifndef NO_SECURITY
		// This must be done after getting version info since theApp.version_ is written to file
        GetMystery();                   // Get mystery info for later security checks
#endif
        rand_good_seed(seed);           // Seed our own PRNG

        // Getting OS version info
        OSVERSIONINFO osvi;
        osvi.dwOSVersionInfoSize = sizeof(osvi);
        GetVersionEx(&osvi);
        is_nt_ = osvi.dwPlatformId == VER_PLATFORM_WIN32_NT;
        is_xp_ = osvi.dwPlatformId == VER_PLATFORM_WIN32_NT && osvi.dwMajorVersion >= 5 && osvi.dwMinorVersion >= 1;

        //win95_ = !(osvi.dwPlatformId == VER_PLATFORM_WIN32_NT && osvi.dwMajorVersion >= 4) &&   // NT4 and later
        //         !(osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS && osvi.dwMajorVersion == 4 && osvi.dwMinorVersion >= 10);  // Win98 + later
        win95_ = osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS && osvi.dwMajorVersion <= 4 && osvi.dwMinorVersion < 10;

        // Determine if multiple monitor supported (Win 98 or NT >= 5.0)
        // Note that Windows 95 is 4.00 and Windows 98 is 4.10
        mult_monitor_ = 
            (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS &&
                 osvi.dwMajorVersion == 4 &&
                 osvi.dwMinorVersion >= 10  ||
                 osvi.dwMajorVersion >= 5) && ::GetSystemMetrics(SM_CMONITORS) > 1;
//        mult_monitor_ = osvi.dwMajorVersion >= 5;

        // We must do this after getting version info as it relies on is_nt_
        m_pspecial_list = new CSpecialList();

        // Check for hexedit.chm file  (USE_HTML_HELP)
        htmlhelp_file_ = m_pszHelpFilePath;
        htmlhelp_file_.MakeUpper();
        if (htmlhelp_file_.Right(4) == ".HLP")
        {
            htmlhelp_file_ = htmlhelp_file_.Left(htmlhelp_file_.GetLength() - 4) + ".CHM";
            if (_access(htmlhelp_file_, 0) == -1)
                htmlhelp_file_.Empty();
        }

        // Work out if there is a previous instance running
        hwnd_1st_ = ::FindWindow(szHexEditClassName, NULL);

        InitConversions();                   // Read EBCDIC conversion table etc and validate conversions

        // Work out if there is a HEXEDIT.INI file in the windows directory
        char ininame[MAX_PATH + 13];
        if (GetWindowsDirectory(ininame, MAX_PATH) != 0)
        {
            size_t len = strlen(ininame);
            if (ininame[len-1] != '\\')
                strcat(ininame, "\\");
            strcat(ininame, "HexEdit.INI");
        }
        else
            ininame[0] = '\0';

        if (_access(ininame, 0) == 0)
        {
            if (AfxMessageBox("HexEdit.INI is no longer used.\n"
                              "Settings are stored in the registry.\n\n"
                              "Do you want to remove the file to\n"
                              "avoid seeing this message again?", MB_YESNO) == IDYES)
            {
                remove(ininame);
            }
        }

		// Note: if this is changed you also need to change the registry string
		// at the end of ExitInstance (see delete_reg_settings_).
        SetRegistryKey("ECSoftware");      // Always use registry

        // Set the locale to the native environment -- hopefully the MSC run-time
        // code does something suitable.  (This is currently just for thousands sep.)
        setlocale(LC_ALL, "");      // Set to native locale

		// Get decimal point characters, thousands separator and grouping
		struct lconv *plconv = localeconv();

		// Set defaults
		dec_sep_char_ = ','; dec_group_ = 3; dec_point_ = '.';

		// Work out thousands separator
		if (strlen(plconv->thousands_sep) == 1)
			dec_sep_char_ = *plconv->thousands_sep;

		// Work out thousands grouping
		if (strlen(plconv->grouping) != 1)
		{
			// Rarely used option of no grouping
			switch (GetProfileInt("Options", "AllowNoDigitGrouping", -1))
			{
			case -1:
				if (AfxMessageBox("You have digit grouping for large numbers turned\n"
					              "off or are using an unsupported grouping/format.\n"
					              "(See Regional Settings in the Control Panel.)\n\n"
								  "Numbers will be displayed without grouping, eg:\n"
								  "\n2489754937\n\n"
								  "Do you want the default digit grouping instead? eg:\n"
								  "\n2,489,754,937\n\n", MB_YESNO) == IDNO)
				{
					WriteProfileInt("Options", "AllowNoDigitGrouping", 1);
					dec_group_ = 9999;  // a big number so that grouping is not done
				}
				else
				{
					// Remember for next time so we don't ask every time HexEdit is run
					WriteProfileInt("Options", "AllowNoDigitGrouping", 0);
				}
				break;
			case 1:
				dec_group_ = 9999;  // a big number so that grouping is not done
				break;
			case 0:
				break; // Nothing required - just use default settings above
			default:
				ASSERT(0);
			}
		}
		else
			dec_group_ = *plconv->grouping;

		// Work out decimal point
		if (strlen(plconv->decimal_point) == 1)
			dec_point_ = *plconv->decimal_point;

        // Work out if we appear to be in US for spelling changes
        is_us_ = strnicmp("English_United States", ::setlocale(LC_COLLATE, NULL), 20) == 0;

        // The following are for BCG init
        SetRegistryBase(_T("Settings"));
        CBCGVisualManager::SetDefaultManager(RUNTIME_CLASS(CBCGWinXPVisualManager));
        CBCGButton::EnableWinXPTheme();
        VERIFY(InitMouseManager());
        VERIFY(InitContextMenuManager());
        VERIFY(InitKeyboardManager());
        VERIFY(InitShellManager());
        VERIFY(InitSkinManager());
        GetSkinManager ()->EnableSkinsDownload (_T("http://www.bcgsoft.com/Skins"));
		//CBCGPopupMenu::EnableMenuSound(FALSE);

		// These are commands that do not appear on a toolbar but have an icon
        static int dv_id[] =
        {
            ID_FILE_NEW,
		    ID_FILE_OPEN,
            ID_FILE_CLOSE,
            ID_FILE_SAVE,
            ID_FILE_PRINT,
            ID_EXPORT_HEX_TEXT,
            ID_IMPORT_HEX_TEXT,
            ID_APP_EXIT,
            ID_EDIT_UNDO,
            ID_EDIT_COPY,
            ID_EDIT_PASTE,
            ID_EDIT_CUT,
            ID_COPY_HEX,
            ID_BOOKMARKS_EDIT,
            ID_EDIT_FIND,
            ID_EDIT_GOTO,
            ID_PASTE_ASCII,
            ID_MARK,
            ID_AUTOFIT,
            ID_ADDR_TOGGLE,
            ID_DISPLAY_HEX,
            ID_DISPLAY_BOTH,
            ID_CHARSET_ASCII,
            ID_CONTROL_NONE,
            ID_PROPERTIES,
            ID_CRC32,
            ID_ENCRYPT_ENCRYPT,
            ID_ENCRYPT_DECRYPT,
            ID_RAND_BYTE,
            ID_FLIP_16BIT,
            ID_ASSIGN_BYTE,
            ID_NEG_BYTE,
            ID_REV_BYTE,
            ID_INC_BYTE,
            ID_DEC_BYTE,
            ID_ADD_BYTE,
            ID_SUBTRACT_BYTE,
            ID_SUBTRACT_X_BYTE,
            ID_MUL_BYTE,
            ID_DIV_BYTE,
            ID_DIV_X_BYTE,
            ID_MOD_BYTE,
            ID_MOD_X_BYTE,
            ID_GTR_BYTE,
            ID_LESS_BYTE,
            ID_AND_BYTE,
            ID_OR_BYTE,
            ID_XOR_BYTE,
            ID_ROL_BYTE,
            ID_ROR_BYTE,
            ID_LSL_BYTE,
            ID_LSR_BYTE,
            ID_ASR_BYTE,
            ID_INVERT,
            ID_CALCULATOR,
            ID_CUSTOMIZE,
            ID_OPTIONS,
            ID_RECORD,
            ID_PLAY,
            ID_WINDOW_NEW,
            ID_WINDOW_NEXT,
            ID_HELP_FINDER,
            ID_CONTEXT_HELP,
            ID_HELP_TUTE1,
            ID_REPAIR_DIALOGBARS,
            ID_HELP_HOMEPAGE,
            ID_APP_ABOUT,
            ID_HIGHLIGHT,
            ID_DIALOGS_DOCKABLE,
        };

		CBCGMenuBar::SetRecentlyUsedMenus(FALSE);
        for (int ii = 0; ii < sizeof(dv_id)/sizeof(*dv_id); ++ii)
            CBCGToolBar::AddBasicCommand(dv_id[ii]);

        // If there is no current skin setting and we have skins then default to the first
		// NOTE: the following seems to cause HexEdit to crash when built with VS 2003. There
		// seems to be some incompatibilty with the skin DLLs and BCG 6.2 DLL when built
		// with VS 2003. The workaround is to delete all skins DLLs.
        CBCGRegistry reg(FALSE, TRUE);
        if (!reg.Open(GetRegSectionPath() + _T("Skin")))
        {
            for (int ii = 0; ii < GetSkinManager()->GetSkinsCount(); ++ii)
                if (strnicmp(GetSkinManager()->GetSkinName(ii), "Gradient Bar", 12) == 0)
                {
                    GetSkinManager()->SetActiveSkin(ii);
                    break;
                }
        }

        // Enable BCG tools menu handling
        // (see CMainFrame::LoadFrame for default tools setup)
        EnableUserTools(ID_TOOLS_ENTRY, ID_TOOL1, ID_TOOL9,
            RUNTIME_CLASS (CHexEditUserTool));

#ifdef SYS_SOUNDS
        // Make sure sound registry settings are present
        CSystemSound::Add(_T("Invalid Character"),
                          CSystemSound::Get(_T(".Default"), _T(".Default"), _T(".Default")));
        CSystemSound::Add(_T("Macro Finished"),
                          CSystemSound::Get(_T("SystemAsterisk"), _T(".Default"), _T(".Default")));
        CSystemSound::Add(_T("Macro Error"),
                          CSystemSound::Get(_T(".Default"), _T(".Default"), _T(".Default")));
        CSystemSound::Add(_T("Calculator Error"),
                          CSystemSound::Get(_T(".Default"), _T(".Default"), _T(".Default")));
        CSystemSound::Add(_T("Comparison Difference Found"));
        CSystemSound::Add(_T("Comparison Difference Not Found"),
                          CSystemSound::Get(_T("SystemAsterisk"), _T(".Default"), _T(".Default")));
        CSystemSound::Add(_T("Search Text Found"));
        CSystemSound::Add(_T("Search Text Not Found"),
                          CSystemSound::Get(_T("SystemAsterisk"), _T(".Default"), _T(".Default")));
        CSystemSound::Add(_T("Background Search Finished"));
#endif

        LoadOptions();

        // Construct property pages for later use in options dialog
        // Note: Had to put this here since before the constructor (being
        // called in the app constructor) was trying to reference resources
        // before that was possible.
        p_general = new CGeneralPage;
        p_sysdisplay = new CSysDisplayPage;
        p_windisplay = new CWindowPage;
//        p_partitions = new CPartitionsPage;  // no longer used when schemes added
        p_colours = new CColourSchemes;
        p_macro = new CMacroPage;
        p_printer = new CPrintPage;
        p_filters = new CFiltersPage;
		p_tips = new CTipsPage;

        // Enable help button for all pages (must be done after construction)
        p_general->m_psp.dwFlags |= PSP_HASHELP;
        p_sysdisplay->m_psp.dwFlags |= PSP_HASHELP;
        p_windisplay->m_psp.dwFlags |= PSP_HASHELP;
//        p_partitions->m_psp.dwFlags |= PSP_HASHELP;  // no longer used when schemes added
        p_colours->m_psp.dwFlags |= PSP_HASHELP;
        p_macro->m_psp.dwFlags |= PSP_HASHELP;
        p_printer->m_psp.dwFlags |= PSP_HASHELP;
        p_filters->m_psp.dwFlags |= PSP_HASHELP;
		p_tips->m_psp.dwFlags |= PSP_HASHELP;

        LoadStdProfileSettings(0);  // Load standard INI file options (including MRU)

        GetXMLFileList();

        // Register the application's document templates.  Document templates
        //  serve as the connection between documents, frame windows and views.

//        CMultiDocTemplate* pDocTemplate;
        // Made pDocTemplate a member variable so I can use it to get all documents of the app.
        // A better way may have been to get the CDocTemplate from m_pDocManger->m_templateList?
        m_pDocTemplate = new CMultiDocTemplate(
                IDR_HEXEDTYPE,
                RUNTIME_CLASS(CHexEditDoc),
                RUNTIME_CLASS(CChildFrame), // custom MDI child frame
                RUNTIME_CLASS(CHexEditView));
        AddDocTemplate(m_pDocTemplate);

        // We must do this before we create the mainframe so that we have bookmarks for the bookmarks dlg
        m_pbookmark_list = new CBookmarkList(FILENAME_BOOKMARKS);
        m_pbookmark_list->ReadList();

        // create main MDI Frame window
        CMainFrame* pMainFrame = new CMainFrame;
        m_pMainWnd = pMainFrame;  // Need this before LoadFrame as calc constructor get main window
        if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
        {
                return FALSE;
        }
        m_pMainWnd->DragAcceptFiles();

//        CHexEditFontCombo::SaveCharSet();

		// NOTE: the name "RecentFiles" is also used at end of ExitInstance
        m_pRecentFileList = new CHexFileList(0, FILENAME_RECENTFILES, recent_files_);
        m_pRecentFileList->ReadList();

        // Make sure previous instance is not minimised before opening files in it
        if (hwnd_1st_ != 0 && one_only_)
        {
            ::BringWindowToTop(hwnd_1st_);
            WINDOWPLACEMENT wp;
            wp.length = sizeof(wp);
            if (::GetWindowPlacement(hwnd_1st_, &wp) &&
                (wp.showCmd == SW_MINIMIZE || wp.showCmd == SW_SHOWMINIMIZED))
            {
                ::ShowWindow(hwnd_1st_, SW_RESTORE);
            }
        }

        // This used to be after the command line parsing but was moved here so that
        // when files are opened the size of the main window is known so that they
        // are opened in sensible sizes and positions.
        pMainFrame->ShowWindow(m_nCmdShow);
        m_pMainWnd->SetFocus();
//          pMainFrame->ShowWindow(SW_SHOWMAXIMIZED);

        // Parse command line for standard shell commands, DDE, file open
        // Note: CCommandLineInfo class was overridden with the CCommandLineParser
        // class - this can open as many files as specified on the command line
        // but does this in CCommandLineParse::ParseParam rather than storing
        // the file name for CWinApp::ProcessShellCommands (and sets
        // m_nShellCommand to FileNothing).
        CCommandLineParser cmdInfo;
        ParseCommandLine(cmdInfo);

        // Don't create empty document by default
        if (cmdInfo.m_nShellCommand == CCommandLineInfo::FileNew)
            cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;

        // Dispatch commands specified on the command line
        if (!ProcessShellCommand(cmdInfo))
                return FALSE;

        // If only one copy allowed terminate now
        if (hwnd_1st_ != 0 && one_only_)
            return FALSE;

#ifdef _DEBUG
        // This avoids all sorts of confusion when testing/debugging
		// NOTE: we get a very odd crash in here when the skins DLLs are
		// incompatible - seesm to happen when we build with VS 2003.
        CBCGToolBar::ResetAll();
#endif

        switch (GetProfileInt("MainFrame", "ShowCalcDlg", 0))
		{
		case -1:
			// nothing here - it's now up to BCG control bar stuff to hide or show the calc
			break;
		case 0:
			// First run or hidden in pre-3.0 release: hide calc
            pMainFrame->FloatControlBar(&pMainFrame->m_wndCalc,
				CPoint(GetProfileInt("Window-Settings", "CalcX", 0), GetProfileInt("Window-Settings", "CalcY", 0)));
            pMainFrame->ShowControlBar(&pMainFrame->m_wndCalc, FALSE, FALSE);
			goto fix_reg_calc;
		case 1:
			// Shown in pre-3.0 release: show calc
            pMainFrame->FloatControlBar(&pMainFrame->m_wndCalc,
				CPoint(GetProfileInt("Window-Settings", "CalcX", 0), GetProfileInt("Window-Settings", "CalcY", 0)));
            pMainFrame->ShowControlBar(&pMainFrame->m_wndCalc, TRUE, FALSE);
		fix_reg_calc:
            WriteProfileInt("MainFrame", "ShowCalcDlg", -1);  // bypass this code next time
			WriteProfileString("Window-Settings", "CalcX", NULL);
			WriteProfileString("Window-Settings", "CalcY", NULL);
			break;
		}

        switch (GetProfileInt("MainFrame", "ShowBookmarkDlg", 0))
		{
		case -1:
			// nothing here - it's now up to BCG control bar stuff to hide or show the dlg
			break;
		case 0:
			// First run or hidden in pre-3.0 release: hide dlg
            pMainFrame->FloatControlBar(&pMainFrame->m_wndBookmarks,
				CPoint(GetProfileInt("Window-Settings", "BookmarksDlgX", 0), GetProfileInt("Window-Settings", "BookmarksDlgY", 0)));
            pMainFrame->ShowControlBar(&pMainFrame->m_wndBookmarks, FALSE, FALSE);
			goto fix_reg_bookmarks;
		case 1:
			// Shown in pre-3.0 release: show dlg
            pMainFrame->FloatControlBar(&pMainFrame->m_wndBookmarks,
				CPoint(GetProfileInt("Window-Settings", "BookmarksDlgX", 0), GetProfileInt("Window-Settings", "BookmarksDlgY", 0)));
            pMainFrame->ShowControlBar(&pMainFrame->m_wndBookmarks, TRUE, FALSE);
		fix_reg_bookmarks:
            WriteProfileInt("MainFrame", "ShowBookmarkDlg", -1);  // signal to bypass this code next time
			// Remove old reg settings (now handled by control bar reg settings)
			WriteProfileString("Window-Settings", "BookmarksDlgX", NULL);
			WriteProfileString("Window-Settings", "BookmarksDlgY", NULL);
			WriteProfileString("Window-Settings", "BookmarksDlgWidth", NULL);
			WriteProfileString("Window-Settings", "BookmarksDlgHeight", NULL);
			break;
		}

        switch (GetProfileInt("MainFrame", "ShowFindDlg", 0))
		{
		case -1:
			// nothing here - it's now up to BCG control bar stuff to hide or show the find dlg
			break;
		case 0:
			// First run or hidden in pre-3.0 release: hide find dlg
            pMainFrame->FloatControlBar(&pMainFrame->m_wndFind,
				CPoint(GetProfileInt("Window-Settings", "FindX", 0), GetProfileInt("Window-Settings", "FindY", 0)));
            pMainFrame->ShowControlBar(&pMainFrame->m_wndFind, FALSE, FALSE);
			goto fix_reg_find;
		case 1:
			// Shown in pre-3.0 release: show find dlg
            pMainFrame->FloatControlBar(&pMainFrame->m_wndFind,
				CPoint(GetProfileInt("Window-Settings", "FindX", 0), GetProfileInt("Window-Settings", "FindY", 0)));
            pMainFrame->ShowControlBar(&pMainFrame->m_wndFind, TRUE, FALSE);
		fix_reg_find:
            WriteProfileInt("MainFrame", "ShowFindDlg", -1);  // bypass this code next time
			WriteProfileString("Window-Settings", "FindX", NULL);
			WriteProfileString("Window-Settings", "FindY", NULL);
			break;
		}

#ifdef EXPLORER_WND
        if (GetProfileInt("MainFrame", "ShowExplorerDlg", 0) != -1)
		{
			pMainFrame->FloatControlBar(&pMainFrame->m_wndExpl, CPoint(0,0));
			pMainFrame->ShowControlBar(&pMainFrame->m_wndExpl, FALSE, FALSE);
            WriteProfileInt("MainFrame", "ShowExplorerDlg", -1);  // only the first time
		}
#endif

        switch (GetProfileInt("MainFrame", "ShowPropDlg", 0))
		{
		case -1:
			// nothing here - it's now up to BCG control bar stuff to hide or show the prop dlg
			break;
		case 0:
			// First run or hidden in pre-3.0 release: hide prop dlg
            pMainFrame->FloatControlBar(&pMainFrame->m_wndProp,
				CPoint(GetProfileInt("Window-Settings", "PropX", 0), GetProfileInt("Window-Settings", "PropY", 0)));
            pMainFrame->ShowControlBar(&pMainFrame->m_wndProp, FALSE, FALSE);
			goto fix_reg_prop;
		case 1:
			// Shown in pre-3.0 release: show prop dlg
            pMainFrame->FloatControlBar(&pMainFrame->m_wndProp,
				CPoint(GetProfileInt("Window-Settings", "PropX", 0), GetProfileInt("Window-Settings", "PropY", 0)));
            pMainFrame->ShowControlBar(&pMainFrame->m_wndProp, TRUE, FALSE);
		fix_reg_prop:
            WriteProfileInt("MainFrame", "ShowPropDlg", -1);  // bypass this code next time
			WriteProfileString("Window-Settings", "PropX", NULL);
			WriteProfileString("Window-Settings", "PropY", NULL);
			break;
		}

		pMainFrame->RecalcLayout();

#ifndef NO_SECURITY
        CString ss;
        switch (GetSecurity())
        {
        default:
            ASSERT(0);
            /* fall through */
        case 0:
            ::HMessageBox("HexEdit has not been installed on this machine");
            return FALSE;
        case 1:
            ss = "Unfortunately, your trial period has expired.";
            break;
        case 2:
            ss.Format("Your trial period expires in %ld days.", long(days_left_));
            break;
        case 4:  // really old licence
            ss = "Unfortunately, you are not licensed to run this version.";
            break;
        case 5:  // licence from 2 or more versions ago
            ss = "You need to upgrade your license to run this version.";
            break;

        case 3:  // temp licence
        case 6:  // full licence
            /* nothing needed here */
            break;
        }

        if (!ss.IsEmpty())
        {
            CStartup dlg;
            dlg.text_ = ss;
            dlg.DoModal();
        }
#endif
        pMainFrame->UpdateWindow();

        // CG: This line inserted by 'Tip of the Day' component.
        ShowTipAtStartup();

#ifndef DIALOG_BAR
        if (GetProfileInt("MainFrame", "ShowPropDlg", 0) == 1)
        {
            ASSERT(pMainFrame->m_wndProp.m_hWnd != 0);
            // pMainFrame->pprop_ = new CPropSheet(prop_page_);
            pMainFrame->m_wndProp.ShowWindow(SW_SHOWNORMAL);
            pMainFrame->m_wndProp.visible_ = TRUE;

            // Set focus back to the active view (if there is a view open)
            CHexEditView *pview = GetView();    // The active view (or NULL if none)
            if (pview != NULL)
                pview->SetFocus();
        }
        else
        {
            ASSERT(pMainFrame->m_wndProp.m_hWnd != 0);
            pMainFrame->m_wndProp.ShowWindow(SW_HIDE);
            pMainFrame->m_wndProp.visible_ = FALSE;
        }
#endif

        RunAutoExec();

        return TRUE;
}

void CHexEditApp::OnAppExit()
{
    SaveToMacro(km_exit);
    CWinApp::OnAppExit();
}

// Called on 1st run after upgrade to a new version
void CHexEditApp::OnNewVersion(int old_ver, int new_ver)
{
    if (old_ver == 200)
    {
        // Version 2.0 used BCG 5.3 which did not support resource smart update
        CBCGToolBar::ResetAll();
    }
    else if (old_ver == 210)
    {
        // We need to reset the Edit Bar otherwise the edit controls (Find/Jump tools) don't work properly
        CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
        mm->m_wndBar2.RestoreOriginalstate();
    }
}

void CHexEditApp::OnFileNew() 
{
    CWinApp::OnFileNew();
}

void CHexEditApp::OnFileOpen() 
{
	DWORD flags = OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST | OFN_SHOWHELP;
	if (no_recent_add_)
		flags |= OFN_DONTADDTORECENT;
    CFileOpenDialog dlgFile(NULL, flags, GetCurrentFilters());
    //CHexFileDialog dlgFile("FileOpenDlg", TRUE, NULL, NULL,
    //                       flags, GetCurrentFilters());

    // Set up buffer where selected file names are stored & default to none
    char all_files[16384];
    all_files[0] = all_files[1] = '\0';
    dlgFile.m_ofn.lpstrFile = all_files;
    dlgFile.m_ofn.nMaxFile = sizeof(all_files)-1;

    // Set up the title of the dialog
    CString title;
    VERIFY(title.LoadString(AFX_IDS_OPENFILE));
    dlgFile.m_ofn.lpstrTitle = title;

    // Change the initial directory
    if (!dir_open_.IsEmpty())
        dlgFile.m_ofn.lpstrInitialDir = dir_open_;

    if (dlgFile.DoModal() != IDOK)
    {
        mac_error_ = 2;
        return;
    }
    //open_file_readonly_ = (dlgFile.m_ofn.Flags & OFN_READONLY) != 0;

    // For some absolutely ridiculous reason if the user only selects one file
    // the full filename is just returned rather than as specified for more than
    // one file with OFN_ALLOWMULTISELECT.  So we need special code to handle this.
    if (dlgFile.m_ofn.nFileOffset < strlen(all_files))
    {
        ASSERT(all_files[dlgFile.m_ofn.nFileOffset-1] == '\\');
        all_files[strlen(all_files)+1] = '\0';      // Ensure double null ended
        all_files[dlgFile.m_ofn.nFileOffset-1] = '\0';
    }

    // Get directory name as first part of files buffer
    CString dir_name(all_files);
    dir_open_ = dir_name;

    // Get file names separated by nul char ('\0') stop on 2 nul chars
    for (const char *pp = all_files + strlen(all_files) + 1; *pp != '\0';
                     pp = pp + strlen(pp) + 1)
    {
        CString filename;
        if (dir_name[dir_name.GetLength()-1] == '\\')
            filename = dir_name + pp;
        else
            filename= dir_name + "\\" + pp;

        // Store this here so the document can find out if it has to open read-only
        // Note; this is done for each file as it is cleared after each use in file_open
	    ASSERT(open_current_readonly_ == -1);
        open_current_readonly_ = (dlgFile.m_ofn.Flags & OFN_READONLY) != 0;

        CHexEditDoc *pdoc;
        if ((pdoc = (CHexEditDoc*)(OpenDocumentFile(filename))) != NULL)
        {
            // Completed OK so store in macro if recording
            if (recording_ && mac_.size() > 0 && (mac_.back()).ktype == km_focus)
            {
                // We don't want focus change recorded (see CHexEditView::OnSetFocus)
                mac_.pop_back();
            }
            SaveToMacro(km_open, filename);
        }
        else
            mac_error_ = 20;
    }

    // BG search finished message may be lost if modeless dlg running
    CheckBGSearchFinished();
}

CDocument* CHexEditApp::OpenDocumentFile(LPCTSTR lpszFileName) 
{
    CDocument *retval = CWinApp::OpenDocumentFile(lpszFileName);

    // Get file extension and change "." to "_" and make macro filename
    ASSERT(mac_dir_.Right(1) == "\\");
    CString mac_filename = lpszFileName;
    if (mac_filename.ReverseFind('.') == -1)
        mac_filename = mac_dir_ + "_.hem";               // Filename without extension
    else
        mac_filename = mac_dir_ + CString("_") + mac_filename.Mid(mac_filename.ReverseFind('.')+1) + ".hem";

    std::vector<key_macro> mac;
    CString comment;
    int halt_lev;
    long plays;
	int version;  // Version of HexEdit in which the macro was recorded

    if (::_access(mac_filename, 0) == 0 &&
        macro_load(mac_filename, &mac, comment, halt_lev, plays, version))
    {
        ((CMainFrame *)AfxGetMainWnd())->StatusBarText(comment);
        macro_play(plays, &mac, halt_lev);
    }

    return retval;
}


BOOL CHexEditApp::OnOpenRecentFile(UINT nID)
{
    // I had to completely override OnOpenRecentFile (copying most of the code from
    // CWinApp::OnOpenRecentFile) since it does not return FALSE if the file is not
    // found -- in fact it always returns TRUE.  I'd say this is an MFC bug.
    ASSERT(m_pRecentFileList != NULL);

    ASSERT(nID >= ID_FILE_MRU_FILE1);
    ASSERT(nID < ID_FILE_MRU_FILE1 + (UINT)m_pRecentFileList->GetSize());
    int nIndex = nID - ID_FILE_MRU_FILE1;
    ASSERT((*m_pRecentFileList)[nIndex].GetLength() != 0);

    TRACE2("MRU: open file (%d) '%s'.\n", (nIndex) + 1,
                    (LPCTSTR)(*m_pRecentFileList)[nIndex]);

    // Save file name now since its index will be zero after it's opened
    CString file_name = (*m_pRecentFileList)[nIndex];

    ASSERT(open_current_readonly_ == -1);
    if (OpenDocumentFile((*m_pRecentFileList)[nIndex]) == NULL)
    {
        m_pRecentFileList->Remove(nIndex);
        mac_error_ = 10;                        // User has been told that file could not be found
        return FALSE;
    }
    else
    {
        // Completed OK so store in macro if recording
        if (recording_ && mac_.size() > 0 && (mac_.back()).ktype == km_focus)
        {
            // We don't want focus change recorded (see CHexEditView::OnSetFocus)
            mac_.pop_back();
        }
        SaveToMacro(km_open, file_name);
        return TRUE;
    }
    ASSERT(0);                                  // We shouldn't get here
}

void CHexEditApp::OnFilePrintSetup()
{
    CPrintDialog pd(TRUE);
    DoPrintDialog(&pd);
    SaveToMacro(km_print_setup);

    // BG search finished message may be lost if modeless dlg running
    CheckBGSearchFinished();
}

void CHexEditApp::OnFileSaveAll() 
{
	m_pDocManager->SaveAllModified();
}

void CHexEditApp::OnFileCloseAll() 
{
    // For each document, allow the user to save it if modified, then close it
    POSITION posn = m_pDocTemplate->GetFirstDocPosition();
    while (posn != NULL)
    {
        CHexEditDoc *pdoc = dynamic_cast<CHexEditDoc *>(m_pDocTemplate->GetNextDoc(posn));
        ASSERT(pdoc != NULL);
        if (!pdoc->SaveModified())      // save/no save return true, cancel/error returns false
            return;
        pdoc->OnCloseDocument();
    }
}

void CHexEditApp::OnFileOpenSpecial() 
{
	COpenSpecialDlg dlg;

    if (dlg.DoModal() == IDOK)
    {
        // Store this here so the document can find out if it has to open read-only
	    ASSERT(open_current_readonly_ == -1);
        open_current_readonly_ = dlg.ReadOnly();

        CHexEditDoc *pdoc;
        if ((pdoc = (CHexEditDoc*)(OpenDocumentFile(dlg.SelectedDevice()))) != NULL)
        {
            // Completed OK so store in macro if recording
            if (recording_ && mac_.size() > 0 && (mac_.back()).ktype == km_focus)
            {
                // We don't want focus change recorded (see CHexEditView::OnSetFocus)
                mac_.pop_back();
            }
            SaveToMacro(km_open, dlg.SelectedDevice());
        }
        else
            mac_error_ = 20;
    }
    else
        mac_error_ = 2;         // User cancelled out of dialog is a minor error
}

void CHexEditApp::OnUpdateFileOpenSpecial(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable();
}

void CHexEditApp::OnRepairDialogbars()
{
	if (AfxMessageBox("This restores all modeless dialogs so they are\n"
                      "visible, undocked and unrolled.  They include:\n"
                      "* Calculator\n"
                      "* Properties dialog\n"
                      "* Bookmarks dialog\n"
                      "* Find dialog\n"
                      "\nYou may need to do this if:\n"
                      "* a modeless dialog cannot be made visible\n"
                      "* modeless dialogs are docked improperly\n"
                      "* a modeless dialog cannot be unrolled/undocked\n"
                      "\nDo you want to continue?",
                      MB_YESNO|MB_DEFBUTTON2) != IDYES)
		return;

    CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
    ASSERT(mm != NULL);

    // Restore initial (nice looking) window size for calculator
    mm->m_wndCalc.m_szFloat = mm->m_wndCalc.m_sizeInitial;

	// Fix and make sure they are visible
    mm->m_wndCalc.FixAndFloat(TRUE);
	mm->m_wndBookmarks.FixAndFloat(TRUE);
	mm->m_wndFind.FixAndFloat(TRUE);
	mm->m_wndProp.FixAndFloat(TRUE);

#ifdef EXPLORER_WND
	mm->m_wndExpl.FixAndFloat(TRUE);
#endif

	mm->RecalcLayout();
}

void CHexEditApp::OnRepairCust()
{
	if (AfxMessageBox("You may need to repair customizations if:\n"
                      "* standard toolbars are missing\n"
                      "* toolbar buttons or menu items are missing\n"
                      "* toolbar or menu icons are incorrect\n"
                      "* standard keystrokes do not work\n"
                      "* you cannot assign custom keystrokes to a command\n\n"
                      "You will lose all customizations including:\n"
                      "* changes to standard toolbars\n"
                      "* changes to all menus including context (popup) menus\n"
                      "* keyboard customizations\n"
                      "\nDo you want to continue?",
                      MB_YESNO|MB_DEFBUTTON2|MB_ICONSTOP) != IDYES)
		return;

    GetContextMenuManager()->ResetState();
    GetKeyboardManager()->ResetAll();
    CBCGToolBar::ResetAll();
}

void CHexEditApp::OnRepairSettings()
{
	if (AfxMessageBox("All customizations and changes to\n"
                      "settings will be removed.\n"
                      "(All registry entries will be removed.)\n"
                      "\nTo do this HexEdit must close.\n"
                      "\nDo you want to continue?",
                      MB_YESNO|MB_DEFBUTTON2|MB_ICONSTOP) != IDYES)
		return;

	// Signal deletion of all registry settings
	delete_reg_settings_ = TRUE;

    CWinApp::OnAppExit();
}

void CHexEditApp::OnRepairAll()
{
	if (AfxMessageBox("All customizations, registry settings,\n"
                      "file settings etc will be removed, including:\n"
                      "* toolbar, menu and keyboard customizations\n"
                      "* settings made in the Options dialog\n"
                      "* previously opened files settings (columns etc)\n"
                      "* recent file list, bookmarks, highlights etc\n"
                      "* ALL REGISTRATION INFORMATION WILL BE LOST\n\n"
                      "When complete you will need to restart HexEdit\n"
                      "and re-enter your activation code.\n"
                      "\nAre you absolutely sure you want to continue?",
                      MB_YESNO|MB_DEFBUTTON2|MB_ICONSTOP) != IDYES)
		return;

	// Signal deletion of all registry settings and settings files
	delete_all_settings_ = TRUE;

    CWinApp::OnAppExit();
}

void CHexEditApp::OnBGSearchFinished(WPARAM wParam, LPARAM lParam)
{
    ASSERT(bg_search_);

    // Make sure doc is valid (may have been closed before we got the message)
    POSITION posn = m_pDocTemplate->GetFirstDocPosition();
    while (posn != NULL)
    {
        CHexEditDoc *pdoc = dynamic_cast<CHexEditDoc *>(m_pDocTemplate->GetNextDoc(posn));
        ASSERT(pdoc != NULL);
        if (pdoc == (void *)lParam)
        {
            pdoc->BGSearchFinished();
            return;
        }
    }
}

// Call BGSearchFinished for all documents to make sure all views have been updated
void CHexEditApp::CheckBGSearchFinished()
{
    if (!bg_search_) return;

    POSITION posn = m_pDocTemplate->GetFirstDocPosition();
    while (posn != NULL)
    {
        CHexEditDoc *pdoc = dynamic_cast<CHexEditDoc *>(m_pDocTemplate->GetNextDoc(posn));
        ASSERT(pdoc != NULL);
        pdoc->BGSearchFinished();
    }
}

void CHexEditApp::OnMacroRecord() 
{
    recording_ = !recording_;
    // Don't clear the last macro until we get the first key of the next
    if (recording_)
		no_keys_ = TRUE;            // Flag to say new macro started
	else
		macro_version_ = INTERNAL_VERSION;

#ifdef _DEBUG
    // Some commands call other commands as part of there task.  This can acc-
    // identally result in extra commands being unintentionally recorded. To
    // help detect this we display a trace message (in debug version) if there
    // is more than one entry in the macro vector - to detect problems we must
    // record a macro with just one command & check the debug window when run.
    if (!recording_ && mac_.size() > 1)
        TRACE1("Macro size is %ld\n", long(mac_.size()));
#endif
    // If invoked from toolbar make sure focus returns to view
    CHexEditView *pview = GetView();    // The active view (or NULL if none)
    if (pview != NULL && pview != pview->GetFocus())
        pview->SetFocus();

    CHECK_SECURITY(206);
}

void CHexEditApp::OnUpdateMacroRecord(CCmdUI* pCmdUI) 
{
	if (recording_)
		pCmdUI->SetText("Stop Recording");
	else
		pCmdUI->SetText("Record Macro");
    pCmdUI->SetCheck(recording_);
}

void CHexEditApp::OnMacroPlay() 
{
    ASSERT(!recording_);
    macro_play();

    CHECK_SECURITY(207);

    // Set focus to currently active view
    CHexEditView *pview = GetView();    // The active view (or NULL if none)
    if (pview != NULL && pview != pview->GetFocus())
        pview->SetFocus();
}

void CHexEditApp::OnUpdateMacroPlay(CCmdUI* pCmdUI) 
{
    // We can play the current macro if we're not recording it and it's not empty
    pCmdUI->Enable(!recording_ && mac_.size() > 0);
}

void CHexEditApp::OnMacroMessage() 
{
    ASSERT(recording_);
    if (recording_)
    {
        CMacroMessage dlg;

        if (dlg.DoModal() == IDOK)
            SaveToMacro(km_macro_message, dlg.message_);
    }
}

void CHexEditApp::OnUpdateMacroMessage(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable(recording_);
}

void CHexEditApp::OnMultiPlay() 
{
    CMultiplay dlg;
    dlg.plays_ = plays_;

    CHECK_SECURITY(208);
    if (dlg.DoModal() == IDOK)
    {
        if (dlg.macro_name_ == DEFAULT_MACRO_NAME)
        {
            ASSERT(!recording_);
            plays_ = dlg.plays_;
            macro_play(plays_);
        }
        else
        {
            play_macro_file(dlg.macro_name_, dlg.plays_);
        }
    }

    // BG search finished message may be lost if modeless dlg running
    CheckBGSearchFinished();

    // Set focus to currently active view
    CHexEditView *pview = GetView();    // The active view (or NULL if none)
    if (pview != NULL && pview != pview->GetFocus())
        pview->SetFocus();
}

void CHexEditApp::OnUpdateMultiPlay(CCmdUI* pCmdUI) 
{
    if (!recording_ && mac_.size() > 0)
    {
        pCmdUI->Enable(TRUE);
        return;
    }
    else
    {
        CFileFind ff;
        ASSERT(mac_dir_.Right(1) == "\\");
        BOOL bContinue = ff.FindFile(mac_dir_ + "*.hem");

        while (bContinue)
        {
            // At least one match - check them all
            bContinue = ff.FindNextFile();

            // Enable if there are macro file that do not start with an underscore OR
            // there are any macro file if we are recording.
            if (recording_ || ff.GetFileTitle().Left(1) != "_")
            {
                pCmdUI->Enable(TRUE);
                return;
            }
        }
    }
    pCmdUI->Enable(FALSE);
}

void CHexEditApp::play_macro_file(const CString &filename, int pp /*= -1*/) 
{
    std::vector<key_macro> tmp;
    CString comment;
    int halt_lev;
    long plays;
	int version;  // Version of HexEdit in which the macro was recorded

    ASSERT(mac_dir_.Right(1) == "\\");
    if (macro_load(mac_dir_ + filename + ".hem", &tmp, comment, halt_lev, plays, version))
    {
        BOOL saved_recording = recording_;
        recording_ = FALSE;
        macro_play(pp == -1 ? plays : pp, &tmp, halt_lev);
        recording_ = saved_recording;
        SaveToMacro(km_macro_play, filename);
    }
}

void CHexEditApp::RunAutoExec()
{
    ASSERT(mac_dir_.Right(1) == "\\");
    CString filename = mac_dir_ + "autoexec.hem"; 
    std::vector<key_macro> mac;
    CString comment;
    int halt_lev;
    long plays;
	int version;  // Version of HexEdit in which the macro was recorded

    if (::_access(filename, 0) == 0 &&
        macro_load(filename, &mac, comment, halt_lev, plays, version))
    {
        ((CMainFrame *)AfxGetMainWnd())->StatusBarText(comment);
        macro_play(plays, &mac, halt_lev);

        // Set focus to currently active view
        CHexEditView *pview = GetView();    // The active view (or NULL if none)
        if (pview != NULL && pview != pview->GetFocus())
            pview->SetFocus();
    }
}

void CHexEditApp::OnRecentFiles() 
{
    CRecentFileDlg dlg;
    dlg.DoModal();
}

void CHexEditApp::OnBookmarksEdit() 
{
    ASSERT(AfxGetMainWnd() != NULL);
    ((CMainFrame *)AfxGetMainWnd())->ShowControlBar(&((CMainFrame *)AfxGetMainWnd())->m_wndBookmarks, TRUE, FALSE);
	((CMainFrame *)AfxGetMainWnd())->m_wndBookmarks.Unroll();
    // Save to macros handled in DelayShow override
}

void CHexEditApp::OnTabIcons() 
{
    tabicons_ = !tabicons_;
    CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
    if (mm != NULL)
    {
        ASSERT_KINDOF(CMainFrame, mm);
        mm->EnableMDITabs(mditabs_, tabicons_,
            tabsbottom_ ? CBCGTabWnd::LOCATION_BOTTOM : CBCGTabWnd::LOCATION_TOP);
    }
}

void CHexEditApp::OnUpdateTabIcons(CCmdUI* pCmdUI) 
{
    pCmdUI->SetCheck(tabicons_);
}

void CHexEditApp::OnTabsAtBottom() 
{
    tabsbottom_ = !tabsbottom_;
    CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
    if (mm != NULL)
    {
        ASSERT_KINDOF(CMainFrame, mm);
        mm->EnableMDITabs(mditabs_, tabicons_,
            tabsbottom_ ? CBCGTabWnd::LOCATION_BOTTOM : CBCGTabWnd::LOCATION_TOP);
    }
}

void CHexEditApp::OnUpdateTabsAtBottom(CCmdUI* pCmdUI) 
{
    pCmdUI->SetCheck(tabsbottom_);
}

/////////////////////////////////////////////////////////////////////////////
// CHexEditApp commands

int CHexEditApp::ExitInstance()
{
    // Save "save on exit" option if it has changed
    if (save_exit_ != orig_save_exit_)
        WriteProfileInt("Options", "SaveExit", save_exit_ ? 1 : 0);

    // Save other options if saving on exit
    if (save_exit_)
        SaveOptions();

    // If we wrote something big to the clipboard ask the user if they want to delete it
    if (last_cb_size_ > 100000 && ::OpenClipboard(HWND(0)))
    {
        HANDLE hh;                              // Handle to clipboard memory
        char *pp;                               // Pointer to clipboard text
        size_t len;                             // Size of text on clipboard

        if ((hh = ::GetClipboardData(CF_TEXT)) != NULL &&
            (pp = reinterpret_cast<char *>(::GlobalLock(hh))) != NULL &&
            (len = strlen(pp)) == last_cb_size_)
        {
            if (AfxMessageBox("Leave data on clipboard?", MB_YESNO) != IDYES)
                ::EmptyClipboard();
        }

        ::CloseClipboard();
    }

    if (pboyer_ != NULL)
        delete pboyer_;

    delete p_general;
    delete p_sysdisplay;
    delete p_windisplay;
//    delete p_partitions;  // no longer used when schemes added
    delete p_colours;
    delete p_macro;
    delete p_printer;
    delete p_filters;
	delete p_tips;

    ::BCGCBCleanUp();

#ifndef NO_SECURITY
    // Check that recent activation used the correct code
    CheckSecurityActivated();
#endif

    if (m_pbookmark_list != NULL)
    {
        m_pbookmark_list->WriteList();
        delete m_pbookmark_list;
    }

    int retval = CWinApp::ExitInstance();

	if (delete_reg_settings_ || delete_all_settings_)
		::SHDeleteKey(HKEY_CURRENT_USER, "Software\\ECSoftware\\HexEdit");  // user settings

	if (delete_all_settings_)
	{
        CString data_path;
		::GetDataPath(data_path);
		if (!data_path.IsEmpty())
		{
			// NOTE: This needs to be updated when new data files added
			remove(data_path + FILENAME_RECENTFILES);
			remove(data_path + FILENAME_BOOKMARKS);
			remove(data_path + FILENAME_BACKGROUND);
		}

		::SHDeleteKey(HKEY_LOCAL_MACHINE, "Software\\ECSoftware\\HexEdit");  // machine settings
		::SHDeleteValue(HKEY_LOCAL_MACHINE, "Software\\ECSoftware", "Data");
		DeleteSecurityFiles();  // Note: FILENAME_BACKGROUND also deleted above
	}

	return retval;
}

BOOL CHexEditApp::PreTranslateMessage(MSG* pMsg) 
{
#ifndef NO_SECURITY
    extern int sec_init;
    extern int sec_type;

    if (pMsg->message == WM_KEYDOWN && 
        pMsg->wParam == 'S' &&
        ::GetKeyState(VK_CONTROL) < 0 &&
        ::GetKeyState(VK_SHIFT) < 0)
    {
        // Display info about which functions have been called so we can check on what
        // the crackers have been up to.
        CString ss;
        ss.Format("%d %d %d %d %d %d %d %d %d %d",
                  quick_check_called,
                  get_security_called,
                  dummy_to_confuse,
                  add_security_called,
                  new_check_called,
                  sec_init,
                  sec_type,
                  security_type_,
                  security_version_);
        AfxMessageBox(ss);
        CHECK_SECURITY(1);
    }
#endif

	// The following is necessary to allow controls in the calculator and bookmarks dialog to process
	// keystrokes normally (eg DEL and TAB keys).  If this is not done CWnd::WalkPreTranslateTree
	// is called which allows the mainframe window to process accelerator keys.  This problem was noticed
	// when DEL and TAB were turned into commands (with associated accelerators) but could have always
	// happened since 2.0 if the user assigned these keys, arrow keys etc to a command in Customize dialog.
	HWND hw = ::GetParent(pMsg->hwnd);
	if (m_pMainWnd != NULL && pMsg->message == WM_KEYDOWN &&
		(hw == ((CMainFrame*)m_pMainWnd)->m_wndCalc || 
#ifdef EXPLORER_WND
         hw == ((CMainFrame*)m_pMainWnd)->m_wndExpl ||
#endif
         hw == ((CMainFrame*)m_pMainWnd)->m_wndBookmarks) ) 
	{
		// Return 0 to allow processing (WM_KEYDOWN) but because we don't call base class version
		// (CWinApp::PreTranslateMessage) we avoid the key being absorbed by a keyboard accelerator.
		return FALSE;
	}

	// This allows a tilde to be inserted in char area (rather than being used for NOT command)
    CWnd *pwnd = CWnd::FromHandlePermanent(pMsg->hwnd);
	CHexEditView *pv;
	if (pMsg->message == WM_CHAR &&
		pwnd != NULL &&
		pwnd->IsKindOf(RUNTIME_CLASS(CHexEditView)) &&
		(pv = DYNAMIC_DOWNCAST(CHexEditView, pwnd)) != NULL &&
		pv->CharMode())
	{
		return FALSE;
	}

    return CWinApp::PreTranslateMessage(pMsg);
}

#ifndef NO_SECURITY
int CHexEditApp::NewCheck()
{
    extern int sec_init;
    extern int sec_type;

    ++new_check_called;

    static int count = 0;

    if (++count > 10 && (sec_init == -1 || sec_type < 1 || sec_type > 10 || security_type_ < 1))
    {
        ASSERT(0);      // Catch in debug
        return 0;
    }

    return 1;
}
#endif

void CHexEditApp::WinHelp(DWORD dwData, UINT nCmd) 
{
    switch(nCmd)
    {
    case HELP_CONTEXT:
        if (::HtmlHelp(m_pMainWnd->GetSafeHwnd(), htmlhelp_file_, HH_HELP_CONTEXT, dwData))
            return;
        break;
    case HELP_FINDER:
        if (::HtmlHelp(m_pMainWnd->GetSafeHwnd(), htmlhelp_file_, HH_DISPLAY_TOPIC, dwData))
            return;
        break;
    }
	ASSERT(0);
    ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

void CHexEditApp::OnAppContextHelp (CWnd* pWndControl, const DWORD dwHelpIDArray [])
{
    CBCGWorkspace::OnAppContextHelp(pWndControl, dwHelpIDArray);
}

BOOL CHexEditApp::OnIdle(LONG lCount) 
{
    ASSERT(AfxGetMainWnd() != NULL);
    CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
//    BOOL need_more = mm->UpdateBGSearchProgress();
    mm->m_wndFind.m_pSheet->SendMessage(WM_KICKIDLE);    // causes problem that we need to track down
    mm->m_wndBookmarks.SendMessage(WM_KICKIDLE);
	mm->m_wndCalc.SendMessage(WM_KICKIDLE);
	mm->m_wndProp.SendMessage(WM_KICKIDLE);
#ifdef EXPLORER_WND
	mm->m_wndExpl.SendMessage(WM_KICKIDLE);
#endif
    mm->UpdateBGSearchProgress();

	CHexEditView *pv;
	if (lCount == 1 && (pv = GetView()) != NULL)
		pv->check_error();

#ifndef NO_SECURITY
    if (lCount == 1 && !QuickCheck())
    {
        ASSERT(0);      // Catch in debug
        AfxAbort();
    }
#endif

    return CWinApp::OnIdle(lCount);
}

void CHexEditApp::PreLoadState()
{
    GetMouseManager()->AddView(IDR_CONTEXT_ADDRESS, "Address Area");
    GetMouseManager()->SetCommandForDblClick(IDR_CONTEXT_ADDRESS, ID_SELECT_LINE);
    GetMouseManager()->AddView(IDR_CONTEXT_HEX, "Hex Area");
    GetMouseManager()->SetCommandForDblClick(IDR_CONTEXT_HEX, ID_MARK);
    GetMouseManager()->AddView(IDR_CONTEXT_CHAR, "Character Area");
    GetMouseManager()->SetCommandForDblClick(IDR_CONTEXT_CHAR, ID_MARK);
    GetMouseManager()->AddView(IDR_CONTEXT_HIGHLIGHT, "Highlight");
    GetMouseManager()->SetCommandForDblClick(IDR_CONTEXT_HIGHLIGHT, ID_HIGHLIGHT_SELECT);
    GetMouseManager()->AddView(IDR_CONTEXT_BOOKMARKS, "Bookmark");
    GetMouseManager()->SetCommandForDblClick(IDR_CONTEXT_BOOKMARKS, ID_BOOKMARKS_EDIT);

    GetContextMenuManager()->AddMenu(_T("Address Area"), IDR_CONTEXT_ADDRESS);
    GetContextMenuManager()->AddMenu(_T("Hex Area"), IDR_CONTEXT_HEX);
    GetContextMenuManager()->AddMenu(_T("Character Area"), IDR_CONTEXT_CHAR);
    GetContextMenuManager()->AddMenu(_T("Highlight"), IDR_CONTEXT_HIGHLIGHT);
    GetContextMenuManager()->AddMenu(_T("Bookmarks"), IDR_CONTEXT_BOOKMARKS);
    GetContextMenuManager()->AddMenu(_T("Selection"), IDR_CONTEXT_SELECTION);
    GetContextMenuManager()->AddMenu(_T("Status Bar"), IDR_CONTEXT_STATBAR);
    GetContextMenuManager()->AddMenu(_T("Window Tabs"), IDR_CONTEXT_TABS);
}

// Starts bg searches in all documents except the active doc (passed in pp)
void CHexEditApp::StartSearches(CHexEditDoc *pp)
{
    POSITION posn = m_pDocTemplate->GetFirstDocPosition();
    while (posn != NULL)
    {
        CHexEditDoc *pdoc = dynamic_cast<CHexEditDoc *>(m_pDocTemplate->GetNextDoc(posn));
        ASSERT(pdoc != NULL);
        if (pdoc != pp)
		{
			CHexEditView *pview = pdoc->GetBestView();
			ASSERT(pview != NULL);
			pdoc->base_addr_ = theApp.align_rel_ ? pview->GetSearchBase() : 0;
            pdoc->StartSearch();
		}
    }
}

void CHexEditApp::NewSearch(const unsigned char *pat, const unsigned char *mask,
                            size_t len, BOOL icase, int tt, BOOL ww,
                            int aa, int offset, bool align_rel)
{
    CHECK_SECURITY(202);

    CSingleLock s2(&appdata_, TRUE);

	// Save search params for bg search to use
    if (pboyer_ != NULL) delete pboyer_;
    pboyer_ = new boyer(pat, len, mask);

    text_type_ = tt;
    icase_ = icase;
    wholeword_ = ww;
    alignment_ = aa;
	offset_ = offset;
	align_rel_ = align_rel;
}

// Get new encryption algorithm
void CHexEditApp::OnEncryptAlg() 
{
    CAlgorithm dlg;
    dlg.m_alg = algorithm_;

    if (dlg.DoModal() == IDOK)
    {
        // Clear password for old algorithm
        if (algorithm_ > 0)
            crypto_.SetPassword(algorithm_-1, NULL);

        // Get new algorithm and create key based on current password
        algorithm_ = dlg.m_alg;
        ASSERT(algorithm_ == 0 || algorithm_ - 1 < (int)crypto_.GetNum());

        if (algorithm_ > 0 && !password_.IsEmpty())
        {
            // Set key based on password
            crypto_.SetPassword(algorithm_-1, password_);
        }
        else if (algorithm_ > 0 && password_.IsEmpty())
        {
            // Clear password
            crypto_.SetPassword(algorithm_-1, NULL);
        }
        SaveToMacro(km_encrypt_alg, algorithm_==0 ? INTERNAL_ALGORITHM : crypto_.GetName(algorithm_-1));
    }
}

void CHexEditApp::set_alg(const char *pp)
{
    algorithm_ = 0;             // Default to internal in case it's not found

    if (strcmp(pp, INTERNAL_ALGORITHM) != 0)
    {
        for (int ii = 0; ii < (int)crypto_.GetNum(); ++ii)
        {
            if (strcmp(crypto_.GetName(ii), pp) == 0)
            {
                algorithm_ = ii+1;
                break;
            }
        }
        if (ii == crypto_.GetNum())
        {
            AfxMessageBox(CString("Encryption algorithm not found:\n") + pp);
            mac_error_ = 10;
            return;
        }

        //
        ASSERT(algorithm_ > 0);
        if (!password_.IsEmpty())
        {
            // Set key based on password
            crypto_.SetPassword(algorithm_-1, password_);
        }
        else if (algorithm_ > 0 && password_.IsEmpty())
        {
            // Clear password
            crypto_.SetPassword(algorithm_-1, NULL);
        }
    }
}

// Get new encryption password
void CHexEditApp::OnEncryptPassword() 
{
    CPassword dlg;
    dlg.m_password = password_;
    if (dlg.DoModal() == IDOK)
    {
        password_ = dlg.m_password;
        // Create encryption key with new password
        if (algorithm_ > 0)
        {
            ASSERT(algorithm_ - 1 < (int)crypto_.GetNum());
            crypto_.SetPassword(algorithm_-1, password_);
        }
        SaveToMacro(km_encrypt_password, (const char *)password_);
    }
}

void CHexEditApp::OnEncryptClear() 
{
    password_.Empty();  // Clear password
    if (algorithm_ > 0)
    {
        ASSERT(algorithm_ - 1 < (int)crypto_.GetNum());
        crypto_.SetPassword(algorithm_-1, NULL);
    }
    SaveToMacro(km_encrypt_password, "");
}

void CHexEditApp::set_password(const char *pp)
{
    password_ = pp;
    if (algorithm_ > 0)
    {
        ASSERT(algorithm_ - 1 < (int)crypto_.GetNum());
        if (password_.IsEmpty())
            crypto_.SetPassword(algorithm_-1, NULL);
        else
            crypto_.SetPassword(algorithm_-1, password_);
    }
}

void CHexEditApp::OnUpdateEncryptClear(CCmdUI* pCmdUI) 
{
    pCmdUI->SetCheck(password_.IsEmpty());   // Set check if password is clear
}

void CHexEditApp::OnCompressionSettings()
{
	CCompressDlg dlg;

	dlg.m_defaultLevel   = GetProfileInt("Options", "ZlibCompressionDVLevel", 1) ? TRUE : FALSE;
	dlg.m_defaultWindow  = GetProfileInt("Options", "ZlibCompressionDVWindow", 1) ? TRUE : FALSE;
	dlg.m_defaultMemory  = GetProfileInt("Options", "ZlibCompressionDVMemory", 1) ? TRUE : FALSE;
	dlg.m_level          = GetProfileInt("Options", "ZlibCompressionLevel", 7);
	dlg.m_window         = GetProfileInt("Options", "ZlibCompressionWindow", 15);
	dlg.m_memory         = GetProfileInt("Options", "ZlibCompressionMemory", 9);
	dlg.m_sync           = GetProfileInt("Options", "ZlibCompressionSync", 0);
	dlg.m_headerType     = GetProfileInt("Options", "ZlibCompressionHeaderType", 1);
	dlg.m_strategy       = GetProfileInt("Options", "ZlibCompressionStrategy", 0);

	if (dlg.DoModal() == IDOK)
	{
		WriteProfileInt("Options", "ZlibCompressionDVLevel", dlg.m_defaultLevel);
		WriteProfileInt("Options", "ZlibCompressionDVWindow", dlg.m_defaultWindow);
		WriteProfileInt("Options", "ZlibCompressionDVMemory", dlg.m_defaultMemory);
		WriteProfileInt("Options", "ZlibCompressionLevel", dlg.m_level);
		WriteProfileInt("Options", "ZlibCompressionWindow", dlg.m_window);
		WriteProfileInt("Options", "ZlibCompressionMemory", dlg.m_memory);
		WriteProfileInt("Options", "ZlibCompressionSync", dlg.m_sync);
		WriteProfileInt("Options", "ZlibCompressionHeaderType", dlg.m_headerType);
		WriteProfileInt("Options", "ZlibCompressionStrategy", dlg.m_strategy);

        SaveToMacro(km_compress);
	}
}

// Retrieve options from .INI file/registry
void CHexEditApp::LoadOptions()
{
    switch(GetProfileInt("Options", "SaveExit", 2))
    {
    case 0:
        orig_save_exit_ = save_exit_ = FALSE;
        break;
    case 2:
        // Option not found so make sure it is saved next time
        orig_save_exit_ = !(save_exit_ = TRUE);
        break;
    default:
        orig_save_exit_ = save_exit_ = TRUE;
    }
    open_restore_ = GetProfileInt("MainFrame", "Restore", 1) ? TRUE : FALSE;
    hex_ucase_ = GetProfileInt("Options", "UpperCaseHex", 1) ? TRUE : FALSE;
    nice_addr_ = GetProfileInt("Options", "NiceAddresses", 1) ? TRUE : FALSE;
    backup_        = (BOOL)GetProfileInt("Options", "CreateBackup",  0);
	backup_space_  = (BOOL)GetProfileInt("Options", "BackupIfSpace", 1);
	backup_size_   =       GetProfileInt("Options", "BackupIfLess",  0);  // 1 = 1KByte, 0 = always
	backup_prompt_ = (BOOL)GetProfileInt("Options", "BackupPrompt",  1);

    bg_search_ = GetProfileInt("Options", "BackgroundSearch", 1) ? TRUE : FALSE;
    one_only_ = GetProfileInt("Options", "OneInstanceOnly", 1) ? TRUE : FALSE;
    large_cursor_ = GetProfileInt("Options", "LargeCursor", 0) ? TRUE : FALSE;
    show_other_ = GetProfileInt("Options", "OtherAreaCursor", 1) ? TRUE : FALSE;

    clear_hist_ = GetProfileInt("Options", "ClearHist", 1) ? TRUE : FALSE;
    clear_recent_file_list_ = GetProfileInt("Options", "ClearRecentFileList", 1) ? TRUE : FALSE;
    clear_bookmarks_ = GetProfileInt("Options", "ClearBookmarks", 0) ? TRUE : FALSE;
    clear_on_exit_ = GetProfileInt("Options", "ClearOnExit", 0) ? TRUE : FALSE;
	no_recent_add_ = GetProfileInt("Options", "DontAddToRecent", 1) ? TRUE : FALSE;

    intelligent_undo_ = GetProfileInt("Options", "UndoIntelligent", 0) ? TRUE : FALSE;
    undo_limit_ = GetProfileInt("Options", "UndoMerge", 4);

    char buf[2];
    if (::GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IMEASURE, buf, 2) > 0 &&
        buf[0] == '0' &&
        GetProfileString("Printer", "LeftMargin").IsEmpty() )
    {
        print_units_ = prn_unit_t(GetProfileInt("Printer", "Units", 1));
    }
    else
    {
        print_units_ = prn_unit_t(GetProfileInt("Printer", "Units", 0));
    }
    left_margin_ = atof(GetProfileString("Printer", "LeftMargin",     print_units_ ? "0.3" : "0.1"));
    right_margin_ = atof(GetProfileString("Printer", "RightMargin",   print_units_ ? "0.3" : "0.1"));
    top_margin_ = atof(GetProfileString("Printer", "TopMargin",       print_units_ ? "1.1" : "0.4"));
    bottom_margin_ = atof(GetProfileString("Printer", "BottomMargin", print_units_ ? "0.8" : "0.3"));
    header_ = GetProfileString("Printer", "Header", "&A | | &N");
    footer_ = GetProfileString("Printer", "Footer", " | - &P - | ");
    header_edge_ = atof(GetProfileString("Printer", "HeaderEdge",     print_units_ ? "0.5" : "0.2"));
    footer_edge_ = atof(GetProfileString("Printer", "FooterEdge",     print_units_ ? "0.3" : "0.1"));
    print_box_ = GetProfileInt("Printer", "Border", 1) != 0 ? true : false;
    print_hdr_ = GetProfileInt("Printer", "Headings", 0) != 0 ? true : false;
    spacing_ = GetProfileInt("Printer", "LineSpacing", 0);

    int wt_flags = GetProfileInt("Options", "WindowTabs", 1);
    switch (wt_flags & 0x3)
    {
    case 1:
        mditabs_ = TRUE;
        tabsbottom_ = FALSE;
        break;
    case 2:
        mditabs_ = TRUE;
        tabsbottom_ = TRUE;
        break;
    default:
        mditabs_ = FALSE;
        tabsbottom_ = FALSE;
        break;
    }
    if ((wt_flags & 0x4) == 0)
        tabicons_ = TRUE;
    else
        tabicons_ = FALSE;

    tree_view_ = GetProfileInt("DataFormat", "TreeView", 0);
	if (tree_view_ > 2) tree_view_ = 0;
    //tree_edit_ = GetProfileInt("DataFormat", "Edit", 0);
    //tree_width_ = GetProfileInt("DataFormat", "TreeWidth", 247);
    max_fix_for_elts_ = GetProfileInt("DataFormat", "MaxFixForElts", 20);
    alt_data_bg_cols_ = GetProfileInt("DataFormat", "AltDataBgCols", 1) != 0 ? true : false;

    default_char_format_ = GetProfileString("DataFormat", "CharDefault", "'%c'");
    default_string_format_ = GetProfileString("DataFormat", "StringDefault", "\"%s\"");
    default_int_format_ = GetProfileString("DataFormat", "IntDefault", "%d");
    default_unsigned_format_ = GetProfileString("DataFormat", "UnsignedDefault", "%u");
    default_real_format_ = GetProfileString("DataFormat", "RealDefault", "%g");
    default_date_format_ = GetProfileString("DataFormat", "DateDefault", "%c");

    xml_dir_ = GetProfileString("DataFormat", "Folder");
    if (xml_dir_.IsEmpty())
    {
//        char fullname[_MAX_PATH];           // Full name of exe directory
//        char *end;                          // End of path of exe file

//        strncpy(fullname, __argv[0], sizeof(fullname)-1);
//        fullname[sizeof(fullname)-1] = '\0';
//        if ((end = strrchr(fullname, '\\')) == NULL && (end = strrchr(fullname, ':')) == NULL)
//            end = fullname;
//        else
//            ++end;
//        *end = '\0';

		// If templates in data folder use it else use hexedit .exe directory
		CString ss; ::GetDataPath(ss);
        CFileFind ff;
        if (!ss.IsEmpty() && ff.FindFile(ss + "*.xml"))
			xml_dir_ = ss;
		else
            xml_dir_ = GetExePath();
    }
//    if (xml_dir_.Right(1) != "\\")
//        xml_dir_ += "\\";

    mac_dir_ = GetProfileString("MacroOptions", "Folder");
    if (mac_dir_.IsEmpty())
    {
//        char fullname[_MAX_PATH];           // Full name of exe directory
//        char *end;                          // End of path of exe file

//        strncpy(fullname, m_pszHelpFilePath, sizeof(fullname)-1);
//        strncpy(fullname, __argv[0], sizeof(fullname)-1);
//        fullname[sizeof(fullname)-1] = '\0';
//        if ((end = strrchr(fullname, '\\')) == NULL && (end = strrchr(fullname, ':')) == NULL)
//            end = fullname;
//        else
//            ++end;
//        *end = '\0';

		// If macros in data folder use it else use hexedit .exe directory
		CString ss; ::GetDataPath(ss);
        CFileFind ff;
        if (!ss.IsEmpty() && ff.FindFile(ss + "*.hem"))
			mac_dir_ = ss;
		else
            mac_dir_ = GetExePath();
    }
//    if (mac_dir_.Right(1) != "\\")
//        mac_dir_ += "\\";

    refresh_ = GetProfileInt("MacroOptions", "Refresh", 1);
    num_secs_ = GetProfileInt("MacroOptions", "Seconds", 5);
    num_keys_ = GetProfileInt("MacroOptions", "Keys", 1);
    num_plays_ = GetProfileInt("MacroOptions", "Plays", 1);
    refresh_bars_ = GetProfileInt("MacroOptions", "StatusBarUpdate", 1) ? TRUE : FALSE;
    refresh_props_ = GetProfileInt("MacroOptions", "PropertiesUpdate", 0) ? TRUE : FALSE;
    halt_level_ = GetProfileInt("MacroOptions", "ErrorHaltLevel", 1);
    plays_ = GetProfileInt("MacroOptions", "NumPlays", 1);

    open_max_ = GetProfileInt("Options", "OpenMax", 0) ? TRUE : FALSE;
    open_disp_state_ = GetProfileInt("Options", "OpenDisplayOptions", -1);
	if (open_disp_state_ == -1)
	{
		open_disp_state_ = 0;  // all options off

		// We are now saving all state in open_disp_state_ but old registry entries may have been written
		open_display_.hex_area = GetProfileInt("Options", "OpenDisplayHex", 1) ? TRUE : FALSE;
		open_display_.char_area = GetProfileInt("Options", "OpenDisplayChar", 1) ? TRUE : FALSE;

		if (GetProfileInt("Options", "OpenEBCDIC", 0) != 0)
			open_display_.char_set = CHARSET_EBCDIC;
		else if (GetProfileInt("Options", "OpenGraphicChars", 0) == 0)  // no graphics means ASCII
			open_display_.char_set = CHARSET_ASCII;
		else if (GetProfileInt("Options", "OpenOemChars", 0) != 0)
			open_display_.char_set = CHARSET_OEM;
		else
			open_display_.char_set = CHARSET_ANSI;

		open_display_.control = GetProfileInt("Options", "OpenControlChars", 0);

		open_display_.autofit = GetProfileInt("Options", "OpenAutoFit", 0) ? TRUE : FALSE;
		open_display_.dec_addr = GetProfileInt("Options", "OpenDecimalAddresses", 0) ? TRUE : FALSE;
		open_display_.hide_highlight = GetProfileInt("Options", "OpenHideHighlight", 0) ? TRUE : FALSE;
		open_display_.hide_bookmarks = GetProfileInt("Options", "OpenHideBookmarks", 0) ? TRUE : FALSE;

		open_display_.hide_replace = GetProfileInt("Options", "OpenHideReplace", 0) ? TRUE : FALSE;
		open_display_.hide_insert = GetProfileInt("Options", "OpenHideInsert", 0) ? TRUE : FALSE;
		open_display_.hide_delete = GetProfileInt("Options", "OpenHideDelete", 0) ? TRUE : FALSE;
		open_display_.delete_count = GetProfileInt("Options", "OpenDeleteCount", 1) ? TRUE : FALSE;

		open_display_.readonly = GetProfileInt("Options", "OpenAllowMods", 0) ? FALSE : TRUE; // reverse of reg value!
		open_display_.overtype = GetProfileInt("Options", "OpenInsert", 0) ? FALSE : TRUE;    // reverse of reg value!

		open_display_.big_endian = GetProfileInt("Options", "OpenBigEndian", 0) ? TRUE : FALSE;

		// Save back now in new entry in case we crash (and are left with no registry settings)
		WriteProfileInt("Options", "OpenDisplayOptions", open_disp_state_);

		// Clear out reg entries to save reg space and to avoid confusing usre by leaving unused entries around
		WriteProfileString("Options", "OpenDisplayHex", NULL);
		WriteProfileString("Options", "OpenDisplayChar", NULL);
		WriteProfileString("Options", "OpenGraphicChars", NULL);
		WriteProfileString("Options", "OpenOemChars", NULL);
		WriteProfileString("Options", "OpenEBCDIC", NULL);
		WriteProfileString("Options", "OpenControlChars", NULL);

		WriteProfileString("Options", "OpenAutoFit", NULL);
		WriteProfileString("Options", "OpenDecimalAddresses", NULL);
		WriteProfileString("Options", "OpenHideHighlight", NULL);
		WriteProfileString("Options", "OpenHideBookmarks", NULL);

		WriteProfileString("Options", "OpenHideReplace", NULL);
		WriteProfileString("Options", "OpenHideInsert", NULL);
		WriteProfileString("Options", "OpenHideDelete", NULL);
		WriteProfileString("Options", "OpenDeleteCount", NULL);

		WriteProfileString("Options", "OpenAllowMods", NULL);
		WriteProfileString("Options", "OpenInsert", NULL);

		WriteProfileString("Options", "OpenBigEndian", NULL);
	}
	if (!open_display_.char_area) open_display_.hex_area = TRUE;  // We need to display one or the other (or both)
	if (!open_display_.hex_area) open_display_.edit_char = open_display_.mark_char = TRUE;

	// Make sure char_set values are valid
	// xxx this will be removed later when we support more char sets (Unicode and code page char sets)
	if (open_display_.char_set == 2)
		open_display_.char_set = CHARSET_ASCII;       // ASCII:  0/2 -> 0
	else if (open_display_.char_set > 4)
		open_display_.char_set = CHARSET_EBCDIC;      // EBCDIC: 4/5/6/7 -> 4

    CString strFont = GetProfileString("Options", "OpenFont", "Courier,16"); // Font info string (fields are comma sep.)
    CString strFace;                                            // Font FaceName from string
    CString strHeight;                                          // Font height as string
    AfxExtractSubString(strFace, strFont, 0, ',');
    AfxExtractSubString(strHeight, strFont, 1, ',');
    if (!strFace.IsEmpty())
    {
        open_plf_ = new LOGFONT;
        memset((void *)open_plf_, '\0', sizeof(*open_plf_));
        strncpy(open_plf_->lfFaceName, strFace, LF_FACESIZE-1);
        open_plf_->lfFaceName[LF_FACESIZE-1] = '\0';
        open_plf_->lfHeight = atol(strHeight);
        if (open_plf_->lfHeight < 2 || open_plf_->lfHeight > 100)
            open_plf_->lfHeight = 16;
        open_plf_->lfCharSet = ANSI_CHARSET;           // Only allow ANSI character set fonts
    }
    else
        open_plf_ = NULL;

    strFont = GetProfileString("Options", "OpenOemFont", "Terminal,18");     // Font info for oem font
    AfxExtractSubString(strFace, strFont, 0, ',');
    AfxExtractSubString(strHeight, strFont, 1, ',');
    if (!strFace.IsEmpty())
    {
        open_oem_plf_ = new LOGFONT;
        memset((void *)open_oem_plf_, '\0', sizeof(*open_oem_plf_));
        strncpy(open_oem_plf_->lfFaceName, strFace, LF_FACESIZE-1);
        open_oem_plf_->lfFaceName[LF_FACESIZE-1] = '\0';
        open_oem_plf_->lfHeight = atol(strHeight);
        if (open_oem_plf_->lfHeight < 2 || open_oem_plf_->lfHeight > 100)
            open_oem_plf_->lfHeight = 18;
        open_oem_plf_->lfCharSet = OEM_CHARSET;            // Only allow OEM/IBM character set fonts
    }
    else
        open_oem_plf_ = NULL;


    open_rowsize_ = GetProfileInt("Options", "OpenColumns", 16);
    if (open_rowsize_ < 4 || open_rowsize_ > CHexEditView::max_buf) open_rowsize_ = 4;
    open_group_by_ = GetProfileInt("Options", "OpenGrouping", 4);
    if (open_group_by_ < 2) open_group_by_ = 2;
    open_offset_ = GetProfileInt("Options", "OpenOffset", 0);
    if (open_offset_ < 0 || open_offset_ >= open_rowsize_) open_offset_ = 0;
    open_vertbuffer_ = GetProfileInt("Options", "OpenScrollZone", 0);
    if (open_vertbuffer_ < 0) open_vertbuffer_ = 0;

    open_keep_times_ = GetProfileInt("Options", "OpenKeepTimes", 0) ? TRUE : FALSE;

    LoadSchemes();

    // Always default back to little-endian for decimal & fp pages
    prop_dec_endian_ = FALSE;
    prop_fp_endian_ = FALSE;
    //prop_ibmfp_endian_ = TRUE;
    prop_date_endian_ = FALSE;

    password_mask_ = GetProfileInt("Options", "PasswordMask", 1) ? TRUE : FALSE;
    password_min_ = GetProfileInt("Options", "PasswordMinLength", 8);

#ifndef DIALOG_BAR
    // Restore more subtle things such as window placement etc
    find_x_ = GetProfileInt("Window-Settings", "FindX", -30000);
    find_y_ = GetProfileInt("Window-Settings", "FindY", -30000);
    prop_x_ = GetProfileInt("Window-Settings", "PropX", -30000);
    prop_y_ = GetProfileInt("Window-Settings", "PropY", -30000);
#endif

    // Last settings for property sheet
    prop_page_ = GetProfileInt("Property-Settings", "PropPage", 0);
    prop_dec_signed_ = GetProfileInt("Property-Settings", "DecFormat", 1);
    prop_fp_format_  = GetProfileInt("Property-Settings", "FPFormat", 1);
    //prop_ibmfp_format_  = GetProfileInt("Property-Settings", "IBMFPFormat", 1);
    prop_date_format_  = GetProfileInt("Property-Settings", "DateFormat", 0);

    // Restore default file dialog directories
    dir_open_ = GetProfileString("File-Settings", "DirOpen");
	//open_file_readonly_ = GetProfileInt("File-Settings", "OpenReadOnly", 0);
	open_file_shared_   = GetProfileInt("File-Settings", "OpenShareable", 0);
//    current_save_ = GetProfileString("File-Settings", "Save");
    current_write_ = GetProfileString("File-Settings", "Write");
    current_read_ = GetProfileString("File-Settings", "Read");
//    current_append_ = GetProfileString("File-Settings", "Append");
    current_export_ = GetProfileString("File-Settings", "Export");
    export_base_addr_ = GetProfileInt("File-Settings", "ExportBaseAddress", 0);
    export_line_len_ = GetProfileInt("File-Settings", "ExportLineLen", 32);
    current_import_ = GetProfileString("File-Settings", "Import");
    import_discon_ = GetProfileInt("File-Settings", "ImportDiscontiguous", 0);
    import_highlight_ = GetProfileInt("File-Settings", "ImportHighlight", 0);
    recent_files_ = GetProfileInt("File-Settings", "RecentFileList", 8);

    current_filters_ = GetProfileString("File-Settings", "Filters",
                        _T(
                           "All Files (*.*)|*.*|"
                           "Executable files (.exe;.dll;.ocx)|*.exe;*.dll;*.ocx|"
                           "Text files (.txt;.asc;read*)|>*.txt;*.asc;read*|"
                           "INI files (.ini)|>*.ini|"
                           "Batch files (.bat;.cmd)|>*.bat;*.cmd|"
                           "EBCDIC files (.cbl;.cob;.cpy;.ddl;.bms)|>*.cbl;*.cob;*.cpy;*.ddl;*.bms|"
                           "Bitmap image files (.bmp;.dib)|>*.bmp;*.dib|"
                           "Internet image files (.gif;.jpg;.jpeg;.png)|>*.gif;*.jpg;*.jpeg;*.png|"
                           "Windows image files (.ico;.cur;.wmf)|>*.ico;*.cur;*.wmf|"
                           "Other image files (.tif,.pcx)|>*.tif;*.pcx|"
                           "All image file formats|*.bmp;*.dib;*.gif;*.jpg;*.jpeg;*.png;*.ico;*.cur;*.wmf;*.tif;*.pcx|"
                           "Postscript files (.eps;.ps;.prn)|*.eps;*.ps;*.prn|"
                           "MS Word files (.doc;.dot;.rtf)|*.doc;*.dot;*.rtf|"
                           "Windows Write (.wri)|>*.wri|"
                           "Rich Text Format (.rtf)|>*.rtf|"
                           "MS Excel files (.xl*)|>*.xl*|"
                           "MS Access databases (.mdb;.mda;.mdw;.mde)|*.mdb;*.mdw;*.mde;*.mda|"
                           "Resource files (.rc;.rct;.res)|>*.rc;*.rct;*.res|"
                           "HTML files (.htm;.html;.?html;.asp;.css;.php;.php?)|>*.htm;*.html;*.?html;*.asp;*.css;*.php;*.php?|"
                           "XML files (.xml)|>*.xml|"
                        "|"));

	// Get settings for info tip window
	tip_transparency_ = GetProfileInt("Options", "InfoTipTransparency", 200);

	// get flags which say which hard-coded info is enabled (currently only option is bookmarks)
	int hard = GetProfileInt("Options", "InfoTipFlags", 0);  // if bit is zero option is in use
    tip_name_.push_back("Bookmarks");
    tip_on_.push_back( (hard&0x1) == 0);
    tip_expr_.push_back("");
    tip_format_.push_back("");

	ASSERT(tip_name_.size() == FIRST_USER_TIP);

	// Next get user specified info lines. Each line has 4 fields:
	//   1. name       - name shown to user - if preceded by '>' then this line is disabled (not shown)
	//   2. expression - evaluated expression involving special symbols such as "address", "byte", etc
	//   3. format     - how the result of the expression is display
	//   4. unused     - for future use (possibly colour and other options)
	// Symbols: address, sector, offset, byte, sbyte, word, uword, dword, udword, qword, uqword,
	//          ieee32, ieee64, ibm32, ibm64, time_t, time_t_80, time_t_1899, time_t_mins, time64_t
    CString ss = GetProfileString("Options", "InfoTipUser",
                    _T(
					">Address;address;;;"
					">Hex Address;address;hex;;"
					">Dec Address;address;dec;;"
					">From Mark;address - mark;;;"
					">From Mark;address - mark;hex;;"
					">From Mark;address - mark;dec;;"
					">Hex Sector;sector;hex;;"
					">Sector;sector;dec;;"
					">Hex Offset;offset;hex;;"
					">Offset;offset;dec;;"
					">ASCII char;byte;%c;;"
					">Bits     ;byte;bin;;"
					">High Bit ;(byte&0x80)!=0;OFF;;"
					">Dec Byte;byte;dec;;"
					">Sign Byte;sbyte;dec;;"
					">Octal Byte;byte;oct;;"
					">Word;word;dec;;"
					">Word Bits;uword;bin;;"
					">Unsigned Word;uword;dec;;"
					">Double Word;dword;dec;;"
					">Unsigned DWord;udword;dec;;"
					">Quad Word;qword;dec;;"
					">32 bit IEEE float;ieee32;%.7g;;"
					">64 bit IEEE float;ieee64;%.15g;;"
					">32 bit IBM float;ibm32;%.7g;;"
					">64 bit IBM float;ibm64;%.16g;;"
					">Date/time;time_t;%c;;"
//					">time_t MSC 5.1;time_t_80;%c;;"
//					">time_t MSC 7;time_t_1899;%c;;"
//					">time_t MINS;time_t_mins;%c;;"
// xxx TBD                       ">String;string;%s;;"
					">To EOF;eof - address;dec;;"
                    ";;"));


	for (int ii = 0; ; ii += 4)
    {
        CString name, expr, format;
        AfxExtractSubString(name,   ss, ii,   ';');
        AfxExtractSubString(expr,   ss, ii+1, ';');
        if (name.IsEmpty() && expr.IsEmpty())
            break;
        AfxExtractSubString(format, ss, ii+2, ';');
        bool on = true;
        if (name[0] == '>')
        {
            on = false;
            name = name.Mid(1);
        }
        tip_name_.push_back(name);
        tip_on_.push_back(on);
        tip_expr_.push_back(expr);
        tip_format_.push_back(format);
    }
}

// Save global options to .INI file/registry
void CHexEditApp::SaveOptions()
{
    CHECK_SECURITY(111);

    // Save general options
    WriteProfileInt("Options", "SaveExit", save_exit_ ? 1 : 0);
    WriteProfileInt("Options", "UpperCaseHex", hex_ucase_ ? 1 : 0);
    WriteProfileInt("Options", "NiceAddresses", nice_addr_ ? 1 : 0);
    WriteProfileInt("Options", "CreateBackup", backup_);
    WriteProfileInt("Options", "BackupIfSpace", int(backup_space_));
    WriteProfileInt("Options", "BackupIfLess", backup_size_);
    WriteProfileInt("Options", "BackupPrompt", int(backup_prompt_));
    WriteProfileInt("Options", "BackgroundSearch", bg_search_ ? 1 : 0);
    WriteProfileInt("Options", "OneInstanceOnly", one_only_ ? 1 : 0);
    WriteProfileInt("Options", "LargeCursor", large_cursor_ ? 1 : 0);
    WriteProfileInt("Options", "OtherAreaCursor", show_other_ ? 1 : 0);

    WriteProfileInt("Options", "ClearHist", clear_hist_ ? 1 : 0);
    WriteProfileInt("Options", "ClearRecentFileList", clear_recent_file_list_ ? 1 : 0);
    WriteProfileInt("Options", "ClearBookmarks", clear_bookmarks_ ? 1 : 0);
    WriteProfileInt("Options", "ClearOnExit", clear_on_exit_ ? 1 : 0);
    WriteProfileInt("Options", "DontAddToRecent", no_recent_add_ ? 1 : 0);

    WriteProfileInt("Options", "UndoIntelligent", intelligent_undo_ ? 1 : 0);
    WriteProfileInt("Options", "UndoMerge", undo_limit_);

    WriteProfileInt("Printer", "Units", int(print_units_));
    CString ss;
    ss.Format("%g", left_margin_);
    WriteProfileString("Printer", "LeftMargin", ss);
    ss.Format("%g", right_margin_);
    WriteProfileString("Printer", "RightMargin", ss);
    ss.Format("%g", top_margin_);
    WriteProfileString("Printer", "TopMargin", ss);
    ss.Format("%g", bottom_margin_);
    WriteProfileString("Printer", "BottomMargin", ss);
    WriteProfileString("Printer", "Header", header_);
    WriteProfileString("Printer", "Footer", footer_);
    ss.Format("%g", header_edge_);
    WriteProfileString("Printer", "HeaderEdge", ss);
    ss.Format("%g", footer_edge_);
    WriteProfileString("Printer", "FooterEdge", ss);
    WriteProfileInt("Printer", "Border", print_box_ ? 1 : 0);
    WriteProfileInt("Printer", "Headings", print_hdr_ ? 1 : 0);
    WriteProfileInt("Printer", "LineSpacing", spacing_);

    WriteProfileInt("Options", "WindowTabs", !mditabs_ ? 0 : 
                    (tabsbottom_ ? 2 : 1) | (!tabicons_ ? 4 : 0));
    CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
    if (mm != NULL)
        mm->SaveFrameOptions();

    // Save data format view options
    WriteProfileInt("DataFormat", "TreeView", tree_view_);
    //WriteProfileInt("DataFormat", "Edit", tree_edit_);
    //WriteProfileInt("DataFormat", "TreeWidth", tree_width_);
    WriteProfileInt("DataFormat", "MaxFixForElts", max_fix_for_elts_);
    WriteProfileInt("DataFormat", "AltDataBgCols", alt_data_bg_cols_ ? 1 : 0);
    ASSERT(xml_dir_.Right(1) == "\\");

    WriteProfileString("DataFormat", "CharDefault", default_char_format_);
    WriteProfileString("DataFormat", "StringDefault", default_string_format_);
    WriteProfileString("DataFormat", "IntDefault", default_int_format_);
    WriteProfileString("DataFormat", "UnsignedDefault", default_unsigned_format_);
    WriteProfileString("DataFormat", "RealDefault", default_real_format_);
    WriteProfileString("DataFormat", "DateDefault", default_date_format_);
    WriteProfileString("DataFormat", "Folder", xml_dir_);

    // Save macro options
    ASSERT(mac_dir_.Right(1) == "\\");
    WriteProfileString("MacroOptions", "Folder", mac_dir_);
    WriteProfileInt("MacroOptions", "Refresh", refresh_);
    WriteProfileInt("MacroOptions", "Seconds", num_secs_);
    WriteProfileInt("MacroOptions", "Keys", num_keys_);
    WriteProfileInt("MacroOptions", "Plays", num_plays_);
    WriteProfileInt("MacroOptions", "StatusBarUpdate", refresh_bars_ ? 1 : 0);
    WriteProfileInt("MacroOptions", "PropertiesUpdate", refresh_props_ ? 1 : 0);
    WriteProfileInt("MacroOptions", "ErrorHaltLevel", halt_level_);
    WriteProfileInt("MacroOptions", "NumPlays", plays_);

    // Save default window options
    WriteProfileInt("Options", "OpenMax", open_max_ ? 1 : 0);

	WriteProfileInt("Options", "OpenDisplayOptions", open_disp_state_);
    //WriteProfileInt("Options", "OpenDisplayHex", open_display_.hex_area ? 1 : 0);
    //WriteProfileInt("Options", "OpenDisplayChar", open_display_.char_area ? 1 : 0);
    //WriteProfileInt("Options", "OpenGraphicChars", open_display_.graphic ? 1 : 0);
    //WriteProfileInt("Options", "OpenOemChars", open_display_.oem ? 1 : 0);
    //WriteProfileInt("Options", "OpenEBCDIC", open_display_.ebcdic ? 1 : 0);
    //WriteProfileInt("Options", "OpenControlChars", open_display_.control);

    //WriteProfileInt("Options", "OpenAutoFit", open_display_.autofit ? 1 : 0);
    //WriteProfileInt("Options", "OpenDecimalAddresses", open_display_.dec_addr ? 1 : 0);
    //WriteProfileInt("Options", "OpenHideHighlight", open_display_.hide_highlight ? 1 : 0);
    //WriteProfileInt("Options", "OpenHideBookmarks", open_display_.hide_bookmarks ? 1 : 0);

    //WriteProfileInt("Options", "OpenHideReplace", open_display_.hide_replace ? 1 : 0);
    //WriteProfileInt("Options", "OpenHideInsert", open_display_.hide_insert ? 1 : 0);
    //WriteProfileInt("Options", "OpenHideDelete", open_display_.hide_delete ? 1 : 0);
    //WriteProfileInt("Options", "OpenDeleteCount", open_display_.delete_count ? 1 : 0);

    //WriteProfileInt("Options", "OpenAllowMods", open_display_.readonly ? 0 : 1); // reverse of reg value!
    //WriteProfileInt("Options", "OpenInsert", open_display_.overtype ? 0 : 1);    // reverse of reg value!

    //WriteProfileInt("Options", "OpenBigEndian", open_display_.big_endian ? 1 : 0);

    WriteProfileInt("Options", "OpenKeepTimes", open_keep_times_ ? 1 : 0);

    CString strFont;
    if (open_plf_ != NULL)
    {
        strFont.Format("%s,%ld", open_plf_->lfFaceName, open_plf_->lfHeight);
        WriteProfileString("Options", "OpenFont", strFont);
    }
    if (open_oem_plf_ != NULL)
    {
        strFont.Format("%s,%ld", open_oem_plf_->lfFaceName, open_oem_plf_->lfHeight);
        WriteProfileString("Options", "OpenOemFont", strFont);
    }

    WriteProfileInt("Options", "OpenColumns", open_rowsize_);
    WriteProfileInt("Options", "OpenGrouping", open_group_by_);
    WriteProfileInt("Options", "OpenOffset", open_offset_);
    WriteProfileInt("Options", "OpenScrollZone", open_vertbuffer_);

    // Encryption password options
    WriteProfileInt("Options", "PasswordMask", password_mask_ ? 1 : 0);
    if (password_min_ != 8)
        WriteProfileInt("Options", "PasswordMinLength", password_min_);

    SaveSchemes();

    // Save info about modeless dialogs (find and properties)
#ifndef DIALOG_BAR
    if (find_y_ != -30000)
    {
        WriteProfileInt("Window-Settings", "FindX", find_x_);
        WriteProfileInt("Window-Settings", "FindY", find_y_);
    }
    if (prop_y_ != -30000)
    {
        WriteProfileInt("Window-Settings", "PropX", prop_x_);
        WriteProfileInt("Window-Settings", "PropY", prop_y_);
    }
#endif
    WriteProfileInt("Property-Settings", "PropPage", prop_page_);
    WriteProfileInt("Property-Settings", "DecFormat", prop_dec_signed_);
    WriteProfileInt("Property-Settings", "FPFormat", prop_fp_format_);
    //WriteProfileInt("Property-Settings", "IBMFPFormat", prop_ibmfp_format_);
    WriteProfileInt("Property-Settings", "DateFormat", prop_date_format_);

    // Save directories for file dialogs
    WriteProfileString("File-Settings", "DirOpen", dir_open_);

	//WriteProfileInt("File-Settings", "OpenReadOnly",  open_file_readonly_);
	WriteProfileInt("File-Settings", "OpenShareable", open_file_shared_);
//    WriteProfileString("File-Settings", "Save", current_save_);
    WriteProfileString("File-Settings", "Write", current_write_);
    WriteProfileString("File-Settings", "Read", current_read_);

    WriteProfileInt("File-Settings", "RecentFileList", recent_files_);
//    WriteProfileString("File-Settings", "Append", current_append_);
    WriteProfileString("File-Settings", "Export", current_export_);
    WriteProfileInt("File-Settings", "ExportBaseAddress", export_base_addr_);
    WriteProfileInt("File-Settings", "ExportLineLen", export_line_len_);
    WriteProfileString("File-Settings", "Import", current_import_);
    WriteProfileInt("File-Settings", "ImportDiscontiguous", import_discon_);
    WriteProfileInt("File-Settings", "ImportHighlight", import_highlight_);

    WriteProfileString("File-Settings", "Filters", current_filters_);

	// Save tip (info) window options
	int hard = 0;
	CString soft;
	for (size_t ii = 0; ii < tip_name_.size(); ++ii)
	{
		if (ii < FIRST_USER_TIP)
		{
			if (!tip_on_[ii])
				hard |= (1<<ii);      // turn bit on to indicate this one is disabled
		}
		else
		{
			CString ss;
			ss.Format("%s%s;%s;%s;;",
				      tip_on_[ii] ? "" : ">",
					  tip_name_[ii],
					  tip_expr_[ii],
					  tip_format_[ii]);
			soft += ss;
		}
	}
	soft += ";;";  // mark end of list
    WriteProfileInt("Options", "InfoTipTransparency", tip_transparency_);
    WriteProfileInt("Options", "InfoTipFlags", hard);
	WriteProfileString("Options", "InfoTipUser", soft);
}

// Clears the various histories and lists (see also SaveSearchHistory)
void CHexEditApp::ClearHist(BOOL clear_search_hist, BOOL clear_recent_file_list, BOOL clear_bookmarks)
{
    CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
    CHexFileList *pfl = GetFileList();
    CBookmarkList *pbl = GetBookmarkList();
    ASSERT(mm != NULL && pfl != NULL && pbl != NULL);

    if (clear_search_hist && mm != NULL)
    {
		mm->hex_hist_.clear();
		mm->dec_hist_.clear();
        mm->search_hist_.clear();
        mm->replace_hist_.clear();

		mm->hex_hist_changed_ = mm->dec_hist_changed_ = clock();
    }

    if (clear_recent_file_list)
        pfl->ClearAll();

    if (clear_bookmarks)
        pbl->ClearAll();
}

void CHexEditApp::LoadSchemes()
{
    scheme_.clear();

    // Get the number of schemes
    int num_schemes = GetProfileInt("Options", "NumberOfSchemes", 0);

    // For each scheme
    for (int ii = 0; ii < num_schemes; ++ii)
    {
        CString strKey;

        strKey.Format("Scheme%d", ii+1);

        // get name, and normal colours (bg etc)
        CString scheme_name = GetProfileString(strKey, "Name", "");

        ASSERT(ii != 0 || scheme_name == ASCII_NAME);
        ASSERT(ii != 1 || scheme_name == ANSI_NAME);
        ASSERT(ii != 2 || scheme_name == OEM_NAME);
        ASSERT(ii != 3 || scheme_name == EBCDIC_NAME);

        CScheme scheme(scheme_name);
        scheme.bg_col_ = GetProfileInt(strKey, "BackgroundColour", -2);     // Use -2 here to detect new scheme
		bool is_new = (scheme.bg_col_ == -2);
		if (is_new) scheme.bg_col_ = -1;									// use default background for new scheme
        scheme.dec_addr_col_ = GetProfileInt(strKey, "DecAddressColour", -1);
        scheme.hex_addr_col_ = GetProfileInt(strKey, "HexAddressColour", -1);
        scheme.hi_col_ = GetProfileInt(strKey, "HiColour", -1);
        scheme.bm_col_ = GetProfileInt(strKey, "BookmarkColour", -1);
        scheme.mark_col_ = GetProfileInt(strKey, "MarkColour", -2);
		// If this is an old scheme but no has mark colour (-2) make it the same as
		// bookmark colour unless bookmark colour is default (-1) thence make it cyan.
		// (This should make the mark the same colour as in previous version for upgraders.)
		if (scheme.mark_col_ == -2) scheme.mark_col_ = is_new ? -1 : (scheme.bm_col_ == -1 ? RGB(0, 224, 224) : scheme.bm_col_);
        scheme.search_col_ = GetProfileInt(strKey, "SearchColour", -1);
#ifdef CHANGE_TRACKING
        scheme.trk_col_ = GetProfileInt(strKey, "ChangeTrackingColour", -1);
        scheme.addr_bg_col_ = GetProfileInt(strKey, "AddressBackgroundColour", -2);
		// Make address background same as normal background for upgraders.
		if (scheme.addr_bg_col_ == -2) scheme.addr_bg_col_ = is_new ? -1 : scheme.bg_col_;
        scheme.sector_col_ = GetProfileInt(strKey, "SectorColour", -1);
#endif

        // For all ranges
        for (int jj = 0; ; ++jj)
        {
            // Get name, colour and range
            CString name, range, ss;
            COLORREF col;

            ss.Format("Name%d", jj+1);
            name = GetProfileString(strKey, ss);
            if (name.IsEmpty())
                break;

            ss.Format("Colour%d", jj+1);
            col = (COLORREF)GetProfileInt(strKey, ss, 0);

            ss.Format("Range%d", jj+1);
            range = GetProfileString(strKey, ss, "0:255");

            scheme.AddRange(name, col, range);
        }

        scheme_.push_back(scheme);
    }

    if (ii < 1)
        scheme_.push_back(default_ascii_scheme_);
    if (ii < 2)
        scheme_.push_back(default_ansi_scheme_);
    if (ii < 3)
        scheme_.push_back(default_oem_scheme_);
    if (ii < 4)
        scheme_.push_back(default_ebcdic_scheme_);
    if (ii < 4)
    {
        // It seems that this is a new installation so add the plain and pretty schemes
        CScheme new_scheme(PLAIN_NAME);
        new_scheme.AddRange("ALL", -1, "0:255");
		// Restore these to "Automatic" values which are mainly greys
        new_scheme.mark_col_ = new_scheme.hex_addr_col_ = new_scheme.dec_addr_col_ = -1;
        // new_scheme.can_delete_ = TRUE;
        scheme_.push_back(new_scheme);

        CScheme new_scheme2(PRETTY_NAME);
		new_scheme2.AddRange("range1", RGB(200, 0, 0), "0:20");
		new_scheme2.AddRange("range2", RGB(200, 100, 0), "21:41");
		new_scheme2.AddRange("range3", RGB(200, 200, 0), "42:63");
		new_scheme2.AddRange("range4", RGB(100, 200, 0), "64:84");
		new_scheme2.AddRange("range5", RGB(0, 200, 0), "85:105");
		new_scheme2.AddRange("range6", RGB(0, 200, 100), "106:127");
		new_scheme2.AddRange("range7", RGB(0, 200, 200), "128:148");
		new_scheme2.AddRange("range8", RGB(0, 100, 200), "149:169");
		new_scheme2.AddRange("range9", RGB(0, 0, 200), "170:191");
		new_scheme2.AddRange("range10", RGB(100, 0, 200), "192:212");
		new_scheme2.AddRange("range11", RGB(200, 0, 200), "213:233");
		new_scheme2.AddRange("range12", RGB(200, 0, 100), "234:255");
        new_scheme2.AddRange("ALL", RGB(100, 100, 100), "0:255");
        scheme_.push_back(new_scheme2);
    }
}

void CHexEditApp::SaveSchemes()
{
    std::vector<CScheme>::const_iterator ps;
    int ii, jj;                         // Loop counters
    CString ss;                         // Temp reg key name

    // Save the number of schemes
    WriteProfileInt("Options", "NumberOfSchemes", scheme_.size());

    // Save each scheme
    for (ii = 0, ps = scheme_.begin(); ps != scheme_.end(); ++ii, ++ps)
    {
        CString strKey;

        strKey.Format("Scheme%d", ii+1);

        // Save name, and normal colours
        WriteProfileString(strKey, "Name", ps->name_);
        WriteProfileInt(strKey, "BackgroundColour", ps->bg_col_);
        WriteProfileInt(strKey, "DecAddressColour", ps->dec_addr_col_);
        WriteProfileInt(strKey, "HexAddressColour", ps->hex_addr_col_);
        WriteProfileInt(strKey, "HiColour", ps->hi_col_);
        WriteProfileInt(strKey, "BookmarkColour", ps->bm_col_);
        WriteProfileInt(strKey, "MarkColour", ps->mark_col_);
        WriteProfileInt(strKey, "SearchColour", ps->search_col_);
#ifdef CHANGE_TRACKING
        WriteProfileInt(strKey, "ChangeTrackingColour", ps->trk_col_);
        WriteProfileInt(strKey, "AddressBackgroundColour", ps->addr_bg_col_);
        WriteProfileInt(strKey, "SectorColour", ps->sector_col_);
#endif

        // Save each range
        for (jj = 0; jj < (int)ps->range_name_.size(); ++jj)
        {
            ss.Format("Name%d", jj+1);
            WriteProfileString(strKey, ss, ps->range_name_[jj]);

            ss.Format("Colour%d", jj+1);
            WriteProfileInt(strKey, ss, ps->range_col_[jj]);

            std::ostringstream strstr;
            strstr << ps->range_val_[jj];

            ss.Format("Range%d", jj+1);
            WriteProfileString(strKey, ss, strstr.str().c_str());
        }

        // Delete entry past last one so that any old values are not used
        ss.Format("Name%d", jj+1);
        WriteProfileString(strKey, ss, NULL);
    }
}

// Get name or description of all XML files for display in drop list
void CHexEditApp::GetXMLFileList() 
{
    CFileFind ff;

    xml_file_name_.clear();

    ASSERT(xml_dir_.Right(1) == "\\");
    BOOL bContinue = ff.FindFile(xml_dir_ + _T("*.XML"));

    while (bContinue)
    {
        // At least one match - check them all
        bContinue = ff.FindNextFile();

        xml_file_name_.push_back(ff.GetFileTitle());
    }
}

#if 0  // replaced by schemes
bool CHexEditApp::GetColours(const char *section, const char *key1, const char *key2,
                const char *key3, partn &retval)
{
    CString name = GetProfileString(section, key1, "");
    if (name.IsEmpty()) return false;

    COLORREF colour = (COLORREF)GetProfileInt(section, key2, 0);
    CString range = GetProfileString(section, key3, "0:255");

    retval = partn(name, colour, range);
    return true;
}

void CHexEditApp::SetColours(const char *section, const char *key1, const char *key2,
                const char *key3, const partn &v)
{
    // Write the range name and RGB value
    WriteProfileString(section, key1, v.name);
    WriteProfileInt(section, key2, v.col);

    // Convert the range itself to a string and write it
    std::ostringstream strstr;
    strstr << v.range;
    WriteProfileString(section, key3, strstr.str().c_str());
}
#endif

void CHexEditApp::OnProperties() 
{
    CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();

    // Save this so that we get it before the page activation "keystroke"
    // (km_prop_file, km_prop_char, etc)
    SaveToMacro(km_prop);
    CHECK_SECURITY(203);

#ifndef DIALOG_BAR
    mm->m_wndProp.ShowWindow(SW_SHOWNORMAL);
    mm->m_wndProp.visible_ = TRUE;
#else
    mm->ShowControlBar(&mm->m_wndProp, TRUE, FALSE);
    mm->m_wndProp.Unroll();
#endif
    mm->m_wndProp.m_pSheet->SetFocus();
    mm->m_wndProp.m_pSheet->UpdateWindow(); // Needed for when prop dlg opened in a macro
}

void CHexEditApp::OnOptions() 
{
    display_options();
}

void CHexEditApp::OnOptions2()
{
    display_options(p_windisplay);
}

void CHexEditApp::display_options(CPropertyPage *display_page /*=NULL*/, BOOL must_show_page /*=FALSE*/)
{
    // Construct property sheet + its pages
    COptSheet options_sheet(_T("HexEdit Options"));

    get_options();

    options_sheet.AddPage(p_general);
    options_sheet.AddPage(p_sysdisplay);
    if (GetView() != NULL)
    {
        options_sheet.AddPage(p_windisplay);
//        options_sheet.AddPage(p_partitions);  // no longer used when schemes added
    }
    options_sheet.AddPage(p_tips);
    options_sheet.AddPage(p_colours);
    options_sheet.AddPage(p_macro);
    options_sheet.AddPage(p_printer);
    options_sheet.AddPage(p_filters);

    if (must_show_page && display_page != NULL && options_sheet.GetPageIndex(display_page) != -1)
    {
        options_sheet.SetActivePage(display_page);
    }
    else if (p_last_page_ != NULL && options_sheet.GetPageIndex(p_last_page_) != -1)
    {
        options_sheet.SetActivePage(p_last_page_);
    }
    else if (display_page != NULL)
    {
        options_sheet.SetActivePage(display_page);
    }

    options_sheet.m_psh.dwFlags &= ~(PSH_HASHELP);      // Turn off help button

    CHECK_SECURITY(201);
    options_sheet.DoModal();

//    if (options_sheet.DoModal() == IDOK)
//      SaveToMacro(km_options, (long)display_page);
//    else
//      mac_error_ = 2;

    // BG search finished message may be lost if modeless dlg running
    CheckBGSearchFinished();
}

// Get current options into member variables for pages
void CHexEditApp::get_options()
{
    CHexEditView *pview;                // The active view (or NULL if none)
    char buf[_MAX_PATH + 3];            // Stores value of key (not used)
    long buf_size = sizeof(buf);        // Size of buffer and returned key value

    // Init dialog fields with option values
    p_general->shell_open_ = 
        RegQueryValue(HKEY_CLASSES_ROOT, HEXEDIT_SUBSUBKEY, buf, &buf_size) == ERROR_SUCCESS;
    p_general->one_only_ = one_only_;
    p_general->bg_search_ = bg_search_;
    p_general->save_exit_ = save_exit_;
    p_general->backup_ = backup_;
    p_general->backup_space_ = backup_space_;
    p_general->backup_if_size_ = backup_size_ > 0;
    p_general->backup_size_ = backup_size_;
    p_general->backup_prompt_ = backup_prompt_;

    p_general->address_specified_ = export_base_addr_ != -1;
    p_general->base_address_ = export_base_addr_ != -1 ? export_base_addr_ : 0;
    p_general->export_line_len_ = export_line_len_;
    p_general->recent_files_ = recent_files_;

    p_sysdisplay->open_restore_ = open_restore_;
    p_sysdisplay->hex_ucase_ = hex_ucase_;
	//p_sysdisplay->nice_addr_ = nice_addr_;
    p_sysdisplay->large_cursor_ = large_cursor_;
    p_sysdisplay->show_other_ = show_other_;
    p_sysdisplay->mditabs_ = mditabs_;
    p_sysdisplay->tabsbottom_ = tabsbottom_;

	p_sysdisplay->dffd_view_ = tree_view_;
    //p_sysdisplay->tree_edit_ = tree_edit_;
    p_sysdisplay->max_fix_for_elts_ = max_fix_for_elts_;
    p_sysdisplay->default_char_format_ = default_char_format_;
    p_sysdisplay->default_int_format_ = default_int_format_;
    p_sysdisplay->default_unsigned_format_ = default_unsigned_format_;
    p_sysdisplay->default_string_format_ = default_string_format_;
    p_sysdisplay->default_real_format_ = default_real_format_;
    p_sysdisplay->default_date_format_ = default_date_format_;

    if ((pview = GetView()) != NULL)
    {
        // Set up the Window Display page
        // Get the name of the active view window and save in window_name_
        ((CMDIChildWnd *)((CMainFrame *)AfxGetMainWnd())->MDIGetActive())->
            GetWindowText(p_windisplay->window_name_);

        p_windisplay->disp_state_ = pview->disp_state_;

        p_windisplay->cols_ = pview->rowsize_;
        p_windisplay->grouping_ = pview->group_by_;
//      p_windisplay->offset_ = pview->offset_;
        p_windisplay->offset_ = (pview->rowsize_ - pview->offset_)%pview->rowsize_;
        p_windisplay->vertbuffer_ = pview->GetVertBufferZone();

        // Get whether or not the window is currently maximized
        WINDOWPLACEMENT wp;

        // Get owner child frame (ancestor window)
        CWnd *cc = pview->GetParent();
        while (cc != NULL && !cc->IsKindOf(RUNTIME_CLASS(CChildFrame)))  // view may be in splitter(s)
            cc = cc->GetParent();
        ASSERT_KINDOF(CChildFrame, cc);

        cc->GetWindowPlacement(&wp);
        p_windisplay->maximize_ = wp.showCmd == SW_SHOWMAXIMIZED;

        p_windisplay->lf_ = pview->lf_;
        p_windisplay->oem_lf_ = pview->oem_lf_;
    }

    p_macro->refresh_ = refresh_;
    p_macro->num_secs_ = num_secs_;
    p_macro->num_keys_ = num_keys_;
    p_macro->num_plays_ = num_plays_;
    p_macro->refresh_bars_ = refresh_bars_;
    p_macro->refresh_props_ = refresh_props_;
    p_macro->halt_level_ = halt_level_;

    p_printer->units_ = int(print_units_);
    p_printer->footer_ = footer_;
    p_printer->header_ = header_;
    p_printer->border_ = print_box_;
    p_printer->headings_ = print_hdr_;
    p_printer->left_ = left_margin_;
    p_printer->right_ = right_margin_;
    p_printer->top_ = top_margin_;
    p_printer->bottom_ = bottom_margin_;
    p_printer->header_edge_ = header_edge_;
    p_printer->footer_edge_ = footer_edge_;
    p_printer->spacing_ = spacing_;
}

void CHexEditApp::set_general()
{
    char buf[_MAX_PATH + 3];            // Stores value of key (not used)
    long buf_size = sizeof(buf);        // Size of buffer and returned key value

    save_exit_ = p_general->save_exit_;

    if (p_general->shell_open_ != 
        (RegQueryValue(HKEY_CLASSES_ROOT, HEXEDIT_SUBSUBKEY, buf, &buf_size) == ERROR_SUCCESS))
    {
        // Option has been changed (turned on or off)
        if (p_general->shell_open_)
        {
			// Create the registry entries that allow "Open with HexEdit" on shortcut menus
			CString s1("Open with HexEdit");
			CString s2 = GetExePath() + "\"HexEdit.exe\"  \"%1\"";
            RegSetValue(HKEY_CLASSES_ROOT, HEXEDIT_SUBKEY, REG_SZ, s1, s1.GetLength());
            RegSetValue(HKEY_CLASSES_ROOT, HEXEDIT_SUBSUBKEY, REG_SZ, s2, s2.GetLength());
        }
        else
        {
            RegDeleteKey(HKEY_CLASSES_ROOT, HEXEDIT_SUBSUBKEY); // Delete subkey first (for NT)
            RegDeleteKey(HKEY_CLASSES_ROOT, HEXEDIT_SUBKEY);    // Delete registry entries
        }
    }
    if (bg_search_ != p_general->bg_search_)
    {
        if (p_general->bg_search_)
        {
            bg_search_ = TRUE;

            // Create bg search thread for all docs
            POSITION posn = m_pDocTemplate->GetFirstDocPosition();
            while (posn != NULL)
            {
                CHexEditDoc *pdoc = dynamic_cast<CHexEditDoc *>(m_pDocTemplate->GetNextDoc(posn));
                ASSERT(pdoc != NULL);
                pdoc->CreateThread();
                if (pboyer_ != NULL)        // If a search has already been done do bg search this file
                    pdoc->StartSearch();
            }

            // We don't need change thread priority for active doc here since
            // when active view regains focus this is done automatically.
        }
        else
        {
            // Kill bg searches for all docs
            POSITION posn = m_pDocTemplate->GetFirstDocPosition();
            while (posn != NULL)
            {
                CHexEditDoc *pdoc = dynamic_cast<CHexEditDoc *>(m_pDocTemplate->GetNextDoc(posn));
                ASSERT(pdoc != NULL);

                // Kill bg thread (and clean up data structures)
                pdoc->KillThread();

                // Signal all views to remove display of search strings
                CBGSearchHint bgsh(FALSE);
                pdoc->UpdateAllViews(NULL, 0, &bgsh);
            }

            // Change this after killing threads to avoid assertions
            bg_search_ = FALSE;
        }
    }

    one_only_ = p_general->one_only_;
    backup_ = p_general->backup_;
    backup_space_ = p_general->backup_space_;
    backup_size_ = p_general->backup_if_size_ ? p_general->backup_size_ : 0;
    backup_prompt_ = p_general->backup_prompt_;
    export_base_addr_ = p_general->address_specified_ ? p_general->base_address_ : -1;
    export_line_len_ = p_general->export_line_len_;
    if (recent_files_ != p_general->recent_files_)
    {
        recent_files_ = p_general->recent_files_;
        GetFileList()->ChangeSize(recent_files_);
    }
}

void CHexEditApp::set_sysdisplay()
{
    open_restore_ = p_sysdisplay->open_restore_;

    if (large_cursor_ != p_sysdisplay->large_cursor_)
    {
        large_cursor_ = p_sysdisplay->large_cursor_;

        // Change caret of all views
        CMainFrame *mm = dynamic_cast<CMainFrame *>(AfxGetMainWnd());
        CMDIChildWnd *nextc;    // Loops through all MDI child frames

        // Find all view windows
        for (nextc = dynamic_cast<CMDIChildWnd *>(mm->MDIGetActive());
             nextc != NULL;
             nextc = dynamic_cast<CMDIChildWnd *>(nextc->GetWindow(GW_HWNDNEXT)) )
        {
            CHexEditView *pview = dynamic_cast<CHexEditView *>(nextc->GetActiveView());
            // Note pview may be NULL if in print preview
            if (pview != NULL && pview->IsKindOf(RUNTIME_CLASS(CHexEditView)))
            {
                if (large_cursor_)
                    pview->BlockCaret();
                else
                    pview->LineCaret();
            }
        }
    }

    if (show_other_ != p_sysdisplay->show_other_)
    {
        show_other_ = p_sysdisplay->show_other_;

        // Invalidate all views to show/hide 2nd "cursor"
        CMainFrame *mm = dynamic_cast<CMainFrame *>(AfxGetMainWnd());
        CMDIChildWnd *nextc;    // Loops through all MDI child frames

        // Invalidate all view windows
        for (nextc = dynamic_cast<CMDIChildWnd *>(mm->MDIGetActive());
             nextc != NULL;
             nextc = dynamic_cast<CMDIChildWnd *>(nextc->GetWindow(GW_HWNDNEXT)) )
        {
            CHexEditView *pview = dynamic_cast<CHexEditView *>(nextc->GetActiveView());
            // Note pview may be NULL if in print preview
            if (pview != NULL && pview->IsKindOf(RUNTIME_CLASS(CHexEditView)))
            {
                pview->DoInvalidate();
            }
        }
    }

    if (hex_ucase_ != p_sysdisplay->hex_ucase_ /* || nice_addr_ != p_sysdisplay->nice_addr_ */)
    {
        hex_ucase_ = p_sysdisplay->hex_ucase_;
//        nice_addr_ = p_sysdisplay->nice_addr_;

        // Invalidate all views to change display of hex digits
        CMainFrame *mm = dynamic_cast<CMainFrame *>(AfxGetMainWnd());
        CMDIChildWnd *nextc;    // Loops through all MDI child frames

        // Invalidate all view windows
        for (nextc = dynamic_cast<CMDIChildWnd *>(mm->MDIGetActive());
             nextc != NULL;
             nextc = dynamic_cast<CMDIChildWnd *>(nextc->GetWindow(GW_HWNDNEXT)) )
        {
            CHexEditView *pview = dynamic_cast<CHexEditView *>(nextc->GetActiveView());
            // Note pview may be NULL if in print preview
            if (pview != NULL && pview->IsKindOf(RUNTIME_CLASS(CHexEditView)))
            {
                pview->recalc_display(); // xxx nec.?
                pview->DoInvalidate();
            }
        }

        // Fix up case of search strings (find tool, find dlg)
        CObList listButtons;
        if (CBCGToolBar::GetCommandButtons(ID_SEARCH_COMBO, listButtons) > 0)
        {
            for (POSITION posCombo = listButtons.GetHeadPosition (); 
                posCombo != NULL; )
            {
                CFindComboButton* pCombo = 
                    DYNAMIC_DOWNCAST(CFindComboButton, listButtons.GetNext(posCombo));
                ASSERT(pCombo != NULL);

                CSearchEditControl *pedit = pCombo->GetEdit();
                ASSERT(pedit != NULL);
                pedit->Redisplay();
            }
        }
        //if (mm->pfind_ != NULL)
            mm->m_wndFind.m_pSheet->Redisplay();

        // Fix up case of hex addresses
        if (CBCGToolBar::GetCommandButtons(ID_JUMP_HEX_COMBO, listButtons) > 0)
        {
            for (POSITION posCombo = listButtons.GetHeadPosition (); 
                posCombo != NULL; )
            {
                CHexComboButton* pCombo = 
                    DYNAMIC_DOWNCAST(CHexComboButton, listButtons.GetNext(posCombo));
                ASSERT(pCombo != NULL);

                CHexEditControl *pedit = pCombo->GetEdit();
                ASSERT(pedit != NULL);
                pedit->Redisplay();
            }
        }

        // Fix up case of calc value (in case it's hex)
        if (mm->m_wndCalc.m_hWnd != 0)
            mm->m_wndCalc.Redisplay();
    }

    if (mditabs_ != p_sysdisplay->mditabs_ ||
        (mditabs_ && tabsbottom_ != p_sysdisplay->tabsbottom_))
    {
        mditabs_ = p_sysdisplay->mditabs_;
        tabsbottom_ = p_sysdisplay->tabsbottom_;

        CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
        ASSERT_KINDOF(CMainFrame, mm);
        if (mm != NULL)
            mm->EnableMDITabs(mditabs_, tabicons_,
                tabsbottom_ ? CBCGTabWnd::LOCATION_BOTTOM : CBCGTabWnd::LOCATION_TOP);
    }

	tree_view_ = p_sysdisplay->dffd_view_;
    //tree_edit_ = p_sysdisplay->tree_edit_;
    max_fix_for_elts_ = p_sysdisplay->max_fix_for_elts_;
    default_char_format_ = p_sysdisplay->default_char_format_;
    default_int_format_ = p_sysdisplay->default_int_format_;
    default_unsigned_format_ = p_sysdisplay->default_unsigned_format_;
    default_string_format_ = p_sysdisplay->default_string_format_;
    default_real_format_ = p_sysdisplay->default_real_format_;
    default_date_format_ = p_sysdisplay->default_date_format_;
}

// Change macro options according to Macro Options page
void CHexEditApp::set_macro()
{
    // Set global options from the macro page values (possibly) changed
    // xxx save any of these option changes to recorded macro?
    refresh_ = p_macro->refresh_;
    num_secs_ = p_macro->num_secs_;
    num_keys_ = p_macro->num_keys_;
    num_plays_ = p_macro->num_plays_;
    refresh_bars_ = p_macro->refresh_bars_;
    refresh_props_ = p_macro->refresh_props_;
    halt_level_ = p_macro->halt_level_;
}

bool CHexEditApp::set_windisplay()
{
    bool one_done = false;              // Have we already made a change (saving undo info)
    // one_done is used to keep track of the changes made to the view so that
    // "previous_too" flag is set for all changes made at once except for the
    // first.  This allows all the changes made at the same time to be undone
    // with one undo operation, which will make more sense to the user.

    CHexEditView *pview = GetView();    // The active view
    ASSERT(pview != NULL);
    if (pview != NULL)
    {
        // Make the window maximized or not depending on setting chosen
        WINDOWPLACEMENT wp;

        // Get owner child frame (ancestor window)
        CWnd *cc = pview->GetParent();
        while (cc != NULL && !cc->IsKindOf(RUNTIME_CLASS(CChildFrame)))  // view may be in splitter(s)
            cc = cc->GetParent();
        ASSERT_KINDOF(CChildFrame, cc);

        cc->GetWindowPlacement(&wp);
        if (p_windisplay->maximize_ != (wp.showCmd == SW_SHOWMAXIMIZED))
        {
            wp.showCmd = p_windisplay->maximize_ ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL;
            cc->SetWindowPlacement(&wp);
        }

        // Make other (undoable) changes if any of the options have changed
#if 0
        bool change_required = (!p_windisplay->autofit_ && pview->rowsize_ != p_windisplay->cols_) ||
                               (pview->group_by_ != p_windisplay->grouping_) ||
                               (pview->real_offset_ != (p_windisplay->cols_ - p_windisplay->offset_)%p_windisplay->cols_) ||
                               pview->disp_state_ != p_windisplay->disp_state_ ||
                               ((!p_windisplay->display_.char_area || p_windisplay->display_.ebcdic ||
                                 !p_windisplay->display_.graphic || !p_windisplay->display_.oem) &&
                                memcmp(&pview->lf_, &p_windisplay->lf_, sizeof(LOGFONT)) != 0) ||
                               (p_windisplay->display_.char_area && !p_windisplay->display_.ebcdic &&
                                p_windisplay->display_.graphic && p_windisplay->display_.oem &&
                                memcmp(&pview->oem_lf_, &p_windisplay->oem_lf_, sizeof(LOGFONT)) != 0);
#else
        bool change_required = (!p_windisplay->autofit_ && pview->rowsize_ != p_windisplay->cols_) ||
                               (pview->group_by_ != p_windisplay->grouping_) ||
                               (pview->real_offset_ != (p_windisplay->cols_ - p_windisplay->offset_)%p_windisplay->cols_) ||
                               pview->disp_state_ != p_windisplay->disp_state_ ||
							   (p_windisplay->display_.FontRequired() == FONT_ANSI && memcmp(&pview->lf_, &p_windisplay->lf_, sizeof(LOGFONT)) != 0) ||
							   (p_windisplay->display_.FontRequired() == FONT_OEM  && memcmp(&pview->oem_lf_, &p_windisplay->oem_lf_, sizeof(LOGFONT)) != 0);
#endif
        if (change_required)
            pview->begin_change();

		// Id current scheme is std scheme matching charset change scheme to match new charset
        if (p_windisplay->display_.char_set != pview->display_.char_set)
		{
			if (pview->scheme_name_ == ANSI_NAME   && pview->display_.char_set == CHARSET_ANSI ||
				pview->scheme_name_ == ASCII_NAME  && pview->display_.char_set == CHARSET_ASCII ||
				pview->scheme_name_ == OEM_NAME    && pview->display_.char_set == CHARSET_OEM  ||
				pview->scheme_name_ == EBCDIC_NAME && pview->display_.char_set == CHARSET_EBCDIC )
			{
				if (p_windisplay->display_.char_set == CHARSET_ASCII)
                    pview->SetScheme(ASCII_NAME);
				else if (p_windisplay->display_.char_set == CHARSET_OEM)
                    pview->SetScheme(OEM_NAME);
				else if (p_windisplay->display_.char_set == CHARSET_EBCDIC)
                    pview->SetScheme(EBCDIC_NAME);
				else
                    pview->SetScheme(ANSI_NAME);
				if (one_done) pview->undo_.back().previous_too = true;
				one_done = true;
			}
		}
        if (p_windisplay->display_.autofit != pview->display_.autofit)
        {
            pview->do_autofit(p_windisplay->display_.autofit);
            if (one_done) pview->undo_.back().previous_too = true;
            one_done = true;
        }

        pview->disp_state_ = p_windisplay->disp_state_;

        if (change_required)
        {
            one_done = pview->make_change(one_done) != 0;    // save disp_state_ in undo vector
            SaveToMacro(km_display_state, pview->disp_state_);
        }

        if (p_windisplay->display_.vert_display && pview->GetVertBufferZone() < 2)
            pview->SetVertBufferZone(2);   // Make sure we can always see the other 2 rows at the same address

        // Do this after autofit changed so that rowsize_ is not messed up by old autofit value
        if (!p_windisplay->display_.autofit && pview->rowsize_ != p_windisplay->cols_)
        {
            pview->change_rowsize(p_windisplay->cols_);
            if (one_done) pview->undo_.back().previous_too = true;
            one_done = true;
        }
        if (pview->group_by_ != p_windisplay->grouping_)
        {
            pview->change_group_by(p_windisplay->grouping_);
            if (one_done) pview->undo_.back().previous_too = true;
            one_done = true;
        }
        if (pview->real_offset_ != (p_windisplay->cols_ - p_windisplay->offset_)%p_windisplay->cols_)
        {
            pview->change_offset((p_windisplay->cols_ - p_windisplay->offset_)%p_windisplay->cols_);
            if (one_done) pview->undo_.back().previous_too = true;
            one_done = true;
        }

        // Vert buffer zone is not an undioable operation
        if (p_windisplay->vertbuffer_ != pview->GetVertBufferZone())
            pview->SetVertBufferZone(p_windisplay->vertbuffer_);

        // Do this after disp_state_ change as restoring the correct font
        // (ANSI or OEM) relies on the state being correct.
#if 0
        if (( !p_windisplay->display_.char_area || p_windisplay->display_.ebcdic ||
              !p_windisplay->display_.graphic || !p_windisplay->display_.oem) &&
#else
        if (p_windisplay->display_.FontRequired() == FONT_ANSI &&
#endif
            memcmp(&pview->lf_, &p_windisplay->lf_, sizeof(LOGFONT)) != 0)
        {
            LOGFONT *prev_lf = new LOGFONT;
            *prev_lf = pview->lf_;
            pview->lf_ = p_windisplay->lf_;
            pview->undo_.push_back(view_undo(undo_font));      // Allow undo of font change
            pview->undo_.back().plf = prev_lf;
            if (one_done) pview->undo_.back().previous_too = true;
            one_done = true;
        }
#if 0
        else if (p_windisplay->display_.char_area && !p_windisplay->display_.ebcdic &&
                 p_windisplay->display_.graphic && p_windisplay->display_.oem &&
#else
        else if (p_windisplay->display_.FontRequired() == FONT_OEM &&
#endif
                 memcmp(&pview->oem_lf_, &p_windisplay->oem_lf_, sizeof(LOGFONT)) != 0)
        {
            LOGFONT *prev_lf = new LOGFONT;
            *prev_lf = pview->oem_lf_;
            pview->oem_lf_ = p_windisplay->oem_lf_;
            pview->undo_.push_back(view_undo(undo_font));      // Allow undo of font change
            pview->undo_.back().plf = prev_lf;
            if (one_done) pview->undo_.back().previous_too = true;
            one_done = true;
        }

        if (change_required)
        {
            pview->end_change();                // updates display
            AfxGetApp()->OnIdle(0);             // This forces buttons & status bar pane update when Apply button used
        }
    }
    return one_done;
}

bool CHexEditApp::set_schemes()
{
    scheme_ = p_colours->scheme_;

    // Update the colours for the current view
    CHexEditView *pview = GetView();    // The active view
    if (pview != NULL)
    {
        if (pview->scheme_name_ != p_colours->scheme_[p_colours->scheme_no_].name_)
        {
            pview->SetScheme(p_colours->scheme_[p_colours->scheme_no_].name_);
        }
        else
        {
            // Even if the scheme has not changed some of its colours may have
            pview->set_colours();
            pview->DoInvalidate();
        }
    }
    return true;
}

void CHexEditApp::set_printer()
{
    print_units_ = prn_unit_t(p_printer->units_);
    footer_ = p_printer->footer_;
    header_ = p_printer->header_;
    print_box_ = p_printer->border_ ? true : false;
    print_hdr_ = p_printer->headings_ ? true : false;
    left_margin_ = p_printer->left_;
    right_margin_ = p_printer->right_;
    top_margin_ = p_printer->top_;
    bottom_margin_ = p_printer->bottom_;
    header_edge_ = p_printer->header_edge_;
    footer_edge_ = p_printer->footer_edge_;
    spacing_ = p_printer->spacing_;
}

void CHexEditApp::set_filters()
{
}

CString CHexEditApp::GetCurrentFilters()
{
    CString retval;

    // Set up the grid cells
    for (int ii = 0; ; ++ii)
    {
        CString s1, s2;

        AfxExtractSubString(s1, current_filters_, ii*2, '|');

        AfxExtractSubString(s2, current_filters_, ii*2+1, '|');

        if (s1.IsEmpty() && s2.IsEmpty())
            break;

        if (s1.IsEmpty() || s2.IsEmpty() || s2[0] == '>')
            continue;

        retval += s1 + "|" + s2 + "|";
    }
    if (retval.IsEmpty())
        retval = "All Files (*.*)|*.*||";
    else
        retval += "|";

    return retval;
}

void CHexEditApp::ShowTipAtStartup(void)
{
        // CG: This function added by 'Tip of the Day' component.

        CCommandLineInfo cmdInfo;
        ParseCommandLine(cmdInfo);
        if (cmdInfo.m_bShowSplash /* && _access("HexEdit.tip", 0) == 0*/)
        {
                CTipDlg dlg;
                if (dlg.m_bStartup)
                        dlg.DoModal();
        }
}

void CHexEditApp::ShowTipOfTheDay(void)
{
        // CG: This function added by 'Tip of the Day' component.

        CTipDlg dlg;
        dlg.DoModal();

        // BG search finished message may be lost if modeless dlg running
        CheckBGSearchFinished();

        // This bit not added by the automatic "Tip of the Day" component
        SaveToMacro(km_tips);
}

void CHexEditApp::OnHelpEmail() 
{
    SendEmail();
}

void CHexEditApp::OnUpdateHelpEmail(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable(::GetProfileInt("MAIL", "MAPI", 0) == 1);
}

void CHexEditApp::OnWebPage() 
{
    // Go to hexedit web site
    ::BrowseWeb(IDS_WEB_ADDRESS);
}

BOOL CHexEditApp::backup(LPCTSTR filename, FILE_ADDRESS file_len) const
{
	BOOL retval = FALSE;

	if (backup_)
	{
		// Default to TRUE but if any test below fails set to FALSE
		retval = TRUE;

		if (backup_space_)
		{
			// First get the volumen name from the file name
			_TCHAR vol[128] = "";
			if (isalpha(filename[0]) && filename[1] == ':')
			{
				// Normal file name
				vol[0] = filename[0];
				strcpy(vol+1, _T(":\\"));
			}
			else if (filename[0] == '\\' && filename[1] == '\\')
			{
				// UNC file name (eg \\MyServer\MyShare\MyFile)
				// Find 4th backslash and copy everything up to it
				_TCHAR *pp = strchr(filename+2, '\\');      // 3rd
				if (pp != NULL) pp = strchr(pp+1, '\\');    // 4th

				if (pp != NULL && pp-filename + 1 < sizeof(vol)/sizeof(*vol))
				{
					// Copy just the volume part
					strncpy(vol, filename, pp-filename + 1);
					vol[pp-filename + 1] = '\0'; // terminate the string
				}
			}

			__int64 free_space = -1;
		    ULARGE_INTEGER bytes_avail, dummy1, dummy2;
			DWORD SectorsPerCluster;
			DWORD BytesPerSector;
			DWORD NumberOfFreeClusters;
			DWORD TotalNumberOfClusters;

			// Get free space on drive and make sure there is enough
		    if (::GetDiskFreeSpaceEx(vol, &bytes_avail, &dummy1, &dummy2))
			{
				free_space = bytes_avail.QuadPart;
			}
			else if (::GetDiskFreeSpace(vol,
					    &SectorsPerCluster,
					    &BytesPerSector,
					    &NumberOfFreeClusters,
					    &TotalNumberOfClusters) )
			{
				free_space = BytesPerSector * (__int64)SectorsPerCluster * NumberOfFreeClusters;
			}

			ASSERT(free_space != -1);   // The calls above should not fail?

			if (file_len >= free_space-65536)   // allow 64K leeway
				retval = FALSE;
		}

		// Make sure file length is less than max backup size (in Mbytes)
		if (retval && backup_size_ > 0 && file_len > FILE_ADDRESS(backup_size_)*1024)
		{
			retval = FALSE;
		}

		// If still backing up and prompt the user (if required)
		if (retval &&
			backup_prompt_ &&
			::HMessageBox("Do you want to create a backup file?", MB_YESNO) != IDYES)
		{
			retval = FALSE;
		}
	}

	return retval;
}

#if 0 // This dummy doc no longer needed since we no lomger use CHtmlView
// This is all needed I think just to get a dummy document class
class CDummyDoc : public CDocument
{
protected: // create from serialization only
    CDummyDoc() {}
    DECLARE_DYNCREATE(CDummyDoc)
public:
    virtual ~CDummyDoc() {}
protected:
    DECLARE_MESSAGE_MAP()
};

IMPLEMENT_DYNCREATE(CDummyDoc, CDocument)

BEGIN_MESSAGE_MAP(CDummyDoc, CDocument)
END_MESSAGE_MAP()
#endif

void CHexEditApp::OnHelpWeb()
{
#if 0  // Don't use HtmlView, just fire up browser instead
    static CMultiDocTemplate *pDocTemplate = NULL;
    if (pDocTemplate == NULL)
    {
        pDocTemplate = new CMultiDocTemplate(
                IDR_HEXEDTYPE,
                RUNTIME_CLASS(CDummyDoc),
                RUNTIME_CLASS(CChildFrame), // custom MDI child frame
                RUNTIME_CLASS(CHtmlView));
    }

    CDocument *pDoc = pDocTemplate->OpenDocumentFile(NULL);

    ASSERT(pDoc != NULL);
    if (pDoc == NULL) return;

    POSITION pos = pDoc->GetFirstViewPosition();
    if (pos != NULL)
    {
        CHtmlView *pView = (CHtmlView *)pDoc->GetNextView(pos);
        ASSERT_KINDOF(CHtmlView, pView);

        // Go to hexedit web site
        CString str;
        VERIFY(str.LoadString(IDS_WEB_HELP));
        pView->Navigate2(str);
    }
#else
    ::BrowseWeb(IDS_WEB_HELP);
#endif
}

void CHexEditApp::OnHelpWebForum()
{
    ::BrowseWeb(IDS_WEB_FORUMS);
}

void CHexEditApp::OnHelpWebFaq()
{
    ::BrowseWeb(IDS_WEB_FAQ);
}

void CHexEditApp::OnHelpWebHome()
{
    ::BrowseWeb(IDS_WEB_ADDRESS);
}

void CHexEditApp::OnHelpWebReg()
{
    ::BrowseWeb(IDS_WEB_REG_USER);
}

void CHexEditApp::OnUpdateHelpWeb(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable(TRUE);
}

// App command to run the dialog
void CHexEditApp::OnAppAbout()
{
        CAbout aboutDlg;

        aboutDlg.DoModal();

        // BG search finished message may be lost if modeless dlg running
        CheckBGSearchFinished();
}

/////////////////////////////////////////////////////////////////////////////
// CCommandLineParser member functions

void CCommandLineParser::ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast)
{
    if (non_std)
    {
        if (bFlag)
        {
            // If there are any flags set then assume this is a standard command line
            non_std = FALSE;
        }
        else
        {
            CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
		    ASSERT(aa->open_current_readonly_ == -1);

            if (aa->hwnd_1st_ != 0 && aa->one_only_)
            {
                // Get full path name as other instance may have different current directory
                char fullname[_MAX_PATH];
                AfxFullPath(fullname, pszParam);

                ATOM atom = ::GlobalAddAtom(fullname);
                ASSERT(atom != 0);
                if (atom == 0) return;
                DWORD dw = 0;
                ::SendMessageTimeout(aa->hwnd_1st_, aa->wm_hexedit, 0, (LPARAM)atom,
                                     SMTO_ABORTIFHUNG, 2000, &dw);
                ::GlobalDeleteAtom(atom);
                return;
            }
            else if (aa->OpenDocumentFile(pszParam))
            {
                m_nShellCommand = FileNothing;
                return;
            }
        }
    }

    // Do standard command line processing using base class
    CCommandLineInfo::ParseParam(pszParam, bFlag, bLast);
}

/////////////////////////////////////////////////////////////////////////////
// Global functions

// Return a ptr to the active view or NULL if none
CHexEditView *GetView()
{
    if (theApp.pv_ != NULL)
    {
        ASSERT(theApp.playing_ > 0);
		if (!IsWindow(theApp.pv_->m_hWnd) || !theApp.pv_->IsKindOf(RUNTIME_CLASS(CHexEditView)))
			return NULL;
		else
			return theApp.pv_;
    }

    CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();

    if (mm != NULL)
    {
        CMDIChildWnd *pwind = mm->MDIGetActive();

        // Ignore if there are no views open
        if (pwind != NULL)
        {
//            CHexEditView *pv = dynamic_cast<CHexEditView *>(pwind->GetActiveView());
            CView *pv = pwind->GetActiveView();
            if (pv != NULL)                         // May be NULL if print preview
            {
                if (pv->IsKindOf(RUNTIME_CLASS(CHexEditView)))
                    return (CHexEditView *)pv;
                else if (pv->IsKindOf(RUNTIME_CLASS(CDataFormatView)))
                    return ((CDataFormatView *)pv)->phev_;
                else if (pv->IsKindOf(RUNTIME_CLASS(CTabView)))
                {
					// Find the hex view (left-most tab)
					CTabView *ptv = (CTabView *)pv;
					ptv->SetActiveView(0);  // hex view is always left-most (index 0)
					ASSERT_KINDOF(CHexEditView, ptv->GetActiveView());
					return (CHexEditView *)ptv->GetActiveView();
                }
            }
        }
    }
    return NULL;
}

COLORREF GetDecAddrCol()
{
    CHexEditView *pv = GetView();
    if (pv == NULL)
        return theApp.scheme_[0].dec_addr_col_;
    else
        return pv->GetDecAddrCol();
}

COLORREF GetHexAddrCol()
{
    CHexEditView *pv = GetView();
    if (pv == NULL)
        return theApp.scheme_[0].hex_addr_col_;
    else
        return pv->GetHexAddrCol();
}

COLORREF GetSearchCol()
{
    CHexEditView *pv = GetView();
    if (pv == NULL)
        return theApp.scheme_[0].search_col_;
    else
        return pv->GetSearchCol();
}

int HMessageBox(LPCTSTR lpszText, UINT nType /*=MB_OK*/, UINT nIDHelp /*=0*/)
{
    int retval = AfxMessageBox(lpszText, nType, nIDHelp);

    // BG search finished message may be lost while modeless dlg running
    ((CHexEditApp *)AfxGetApp())->CheckBGSearchFinished();

    return retval;
}

int HMessageBox(UINT nIDPrompt, UINT nType /*=MB_OK*/, UINT nIDHelp /*=-1*/)
{
    int retval = AfxMessageBox(nIDPrompt, nType, nIDHelp);

    // BG search finished message may be lost while modeless dlg running
    ((CHexEditApp *)AfxGetApp())->CheckBGSearchFinished();

    return retval;
}

BOOL SendEmail(int def_type /*=0*/, const char *def_text /*=NULL*/, const char *def_name)
{
    BOOL retval = FALSE;
    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

    if (::GetProfileInt("MAIL", "MAPI", 0) != 1)
    {
        ::HMessageBox("MAPI not supported on this machine");
        return FALSE;                                 // MAPI mail not supported
    }

    HINSTANCE hmapi = ::LoadLibrary("MAPI32.DLL");
    if (hmapi == (HINSTANCE)0)
    {
        ::HMessageBox("MAPI32.DLL not found");
        return FALSE;
    }

    LPMAPISENDMAIL pf = (LPMAPISENDMAIL)GetProcAddress(hmapi, "MAPISendMail");
    if (pf == (LPMAPISENDMAIL)0)
    {
        ::HMessageBox("MAPI32.DLL failure");
        ::FreeLibrary(hmapi);
        return FALSE;
    }

    CEmailDlg dlg;
    dlg.type_ = def_type;
    dlg.text_ = def_text;
    dlg.name_ = def_name;
    if (def_type == 2)
    {
        dlg.to_ = "info@HexEdit.com";
        if (dlg.name_.IsEmpty())
            dlg.subject_ = "Single-machine license";
        else
            dlg.subject_ = "Single-user license";
    }

    // If bug report then default attachment file name to be the active file
    CHexEditView *pv = NULL;
    if (def_type == 0 &&
        (pv = GetView()) != NULL &&
        pv->GetDocument()->pfile1_ != NULL &&
        pv->GetDocument()->length() < 2000000 &&
        !pv->GetDocument()->IsDevice() )
    {
        dlg.attachment_ = pv->GetDocument()->pfile1_->GetFilePath();
    }

    while (dlg.DoModal() == IDOK)
    {
        // Send (or try to send) the mail
        CString subject, text;          // Where subject & body of mail is built

        // Set up the sender and receiver of the email
        MapiRecipDesc from, to;
        memset(&from, '\0', sizeof(from));
        from.ulRecipClass = MAPI_ORIG;
        from.lpszName = getenv("USERNAME");

        CString address = "SMTP:" + dlg.to_;
        memset(&to, '\0', sizeof(to));
        to.ulRecipClass = MAPI_TO;
        to.lpszName = "andrew";
        to.lpszAddress = address.GetBuffer(0);

        // Build the subject and message text from the info the user entered
        switch (dlg.type_)
        {
        case 0:
            subject = "HEXEDIT BUG: ";
            text = "TYPE: BUG REPORT\n";
            break;
        case 1:
            subject = "HEXEDIT REQ: ";
            text = "TYPE: ENHANCEMENT REQUEST\n";
            break;
        case 2:
            subject = "HEXEDIT BUY: ";
            text = "TYPE: REGISTRATION REQUEST\n";
            break;
        default:
            subject = "HEXEDIT OTH: ";
            text = "TYPE: OTHER\n";
        }
        subject += dlg.subject_;

        // Add the system information to the email
        text += "SYSTEM: ";
        if (dlg.systype_.CompareNoCase("This system") == 0)
        {
            OSVERSIONINFO osvi;
            osvi.dwOSVersionInfoSize = sizeof(osvi);
            GetVersionEx(&osvi);

            if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT &&
                osvi.dwMajorVersion == 3 && osvi.dwMinorVersion >= 50)
                text += "NT 3.5X";
            else if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT && osvi.dwMajorVersion == 4)
                text += "NT 4.0";
            else if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT && 
				osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
                text += "Windows 2000";
            else if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT && 
				osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
                text += "Windows XP";
            else if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT && 
				osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2)
                text += "Windows Server 2003";
            else if (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS &&
                osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
                text += "Windows 95";
            else if (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS &&
                osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
                text += "Windows 98";
            else if (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS &&
                osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
                text += "Windows ME";
            CString version_info;
            version_info.Format("  %ld.%ld (build %ld)", (long)osvi.dwMajorVersion,
                                (long)osvi.dwMinorVersion, (long)osvi.dwBuildNumber);
            text += version_info;
        }
        else
            text += dlg.systype_;

        // Add info about the version of HexEdit and user details
        text += "\nVERSION: ";
        text += dlg.version_;
        text += "\nNAME: ";
        text += dlg.name_;
        text += "\nEMAIL: ";
        text += dlg.address_;
        text += "\n";

        CString ss;
        if (aa->security_type_ == 6)
            ss.Format("REGISTERED TO: %s\n", aa->security_name_);
        else if (aa->security_type_ == 5)
            ss.Format("UPGRADEABLE (REGISTERED TO: %s)\n", aa->security_name_);
        else if (aa->security_type_ == 4)
            ss.Format("EARLIER VERSION (REGISTERED TO: %s)\n", aa->security_name_);
        else
            ss.Format("UNREGISTERED: Expires in %ld days\n", long(aa->days_left_));
        text += ss;

        text += dlg.text_;

        // Set up the email header info
        MapiMessage mm;
        memset(&mm, '\0', sizeof(mm));
        mm.lpszSubject = subject.GetBuffer(0);
        mm.lpszNoteText = text.GetBuffer(0);
        mm.lpOriginator = &from;
        mm.nRecipCount = 1;
        mm.lpRecips = &to;

        MapiFileDesc mfd;
        memset(&mfd, '\0', sizeof(mfd));

        bool file_closed = false;       // Did we have to close the active file (in order to attach it)?
        bool bg_search = false;         // Did we have to stop bg search before closing the file?
        if (dlg.attach_)
        {
            mm.nFileCount = 1;
            mm.lpFiles = &mfd;
            mfd.lpszPathName = (char *)(const char *)dlg.attachment_;

            if (pv != NULL &&
                pv->GetDocument()->pfile1_ != NULL &&
                pv->GetDocument()->pfile1_->GetFilePath().CompareNoCase(dlg.attachment_) == 0 &&
                !pv->GetDocument()->read_only())
            {
                // Cannot attach the current file if it is read-only so close it

                // First make sure there are no unsaved changes
                if (pv->GetDocument()->IsModified() &&
                    ::HMessageBox("Attached file has unsaved changes.\r"
                          "Save it now?", MB_YESNO) == IDYES)
                {
                    pv->GetDocument()->DoFileSave();
                }

                if (pv->GetDocument()->SearchOccurrences() == -2)
                {
                    pv->GetDocument()->StopSearch();
                    bg_search = true;
                }
                pv->GetDocument()->close_file();
                file_closed = true;
            }
        }

        ULONG result = pf(0, 0, &mm, MAPI_LOGON_UI, 0);

        // Reopen the active file if it was closed
        if (file_closed)
        {
            pv->GetDocument()->open_file(dlg.attachment_);

            // Restart background search if it was stopped
            if (bg_search)
                pv->GetDocument()->StartSearch();
        }

        if (result == 0)
        {
            retval = TRUE;
            break;
        }

        // For some reason 2 is returned (XP only) if attachment is currently open
        // in modeReadWrite (even if shareDenyRead not used)
        if ((result == 2 ||
             result == MAPI_E_ATTACHMENT_NOT_FOUND ||
             result == MAPI_E_ATTACHMENT_OPEN_FAILURE) &&
            ::HMessageBox("Error opening attachment.\r"
                          "Try again?", MB_YESNO) != IDYES)
        {
            break;
        }

        if (::HMessageBox("Error encountered creating email session\r"
                          "Try again?", MB_YESNO) != IDYES)
        {
            break;
        }
    }

    ::FreeLibrary(hmapi);

    // BG search finished message may be lost if modeless dlg running
    aa->CheckBGSearchFinished();

    return retval;
}

