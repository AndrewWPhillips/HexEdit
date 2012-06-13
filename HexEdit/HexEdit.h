// HexEdit.h : main header file for the HEXEDIT application
//
// Copyright (c) 1998-2012 by Andrew W. Phillips.
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

#if !defined(HEXEDIT_H__INCLUDED_)
#define HEXEDIT_H__INCLUDED_

#include <io.h>

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

// IMPORTANT: Increment this when we release a new version
//#define INTERNAL_VERSION 4              // version 2.5
//#define INTERNAL_VERSION 5              // version 2.6
//#define INTERNAL_VERSION 6              // version 3.0
//#define INTERNAL_VERSION 7              // version 3.1
//#define INTERNAL_VERSION 8              // version 3.2
//#define INTERNAL_VERSION 9              // version 3.3
//#define INTERNAL_VERSION 10             // version 3.4
//#define INTERNAL_VERSION 11             // version 3.5
//#define INTERNAL_VERSION 12             // version 4.0 and 3.6
#define INTERNAL_VERSION 13             // version 4.1

// may need to adjust this depending on how many versions there were in last 2 years
#define UPGRADE_DIFF 4   // diff between current version and version before which an upgrade is invalid

typedef __int64 FILE_ADDRESS;

#ifndef OFN_DONTADDTORECENT
#define OFN_DONTADDTORECENT          0x02000000
#endif

#if 0 // ndef SM_CMONITORS
// It seems there's a bug in winuser.h (assumed Win 98 was WIN_VER == 5 not 4.10)
// which causes the following multiple monitor stuff not to be conditionally compiled in
#define SM_CMONITORS            80
#define SM_XVIRTUALSCREEN       76
#define SM_YVIRTUALSCREEN       77
#define SM_CXVIRTUALSCREEN      78
#define SM_CYVIRTUALSCREEN      79
#define SM_SAMEDISPLAYFORMAT    81
#endif

// Conditional compilation flags - always to be used
#define BOOST_CRC        1  // Use Boost CRC code rather than rather than dubious code I got somewhere
#define BOOST_RAND       1  // Use Boost PRNGs rather than my own one
#define USE_OWN_PRINTDLG 1  // Replace the standard print dialog with our own derived dialog
#define INPLACE_MOVE 1      // Writes all changes to the file in place - even when bytes inserted/deleted (so temp file is not required)
#define SYS_SOUNDS      1   // Use system sounds - make an option for system sounds vs internal spkr

#define SHOW_CODE_PAGE  1
#define PROP_INFO       1   // Display info (Summary) page in properties dialog
#define TIME64_T        1   // Show 64 bit time_t in date page - this needs new compiler (VS 2002 or later)
#define SHADED_TOOLBARS 1   // When this is enabled we need to get rid of "Old Tool Bar" menu items

#define USE_FREE_IMAGE  1   // When this is enabled we need to get rid of EnBitmap.cpp from the project
#define FILE_PREVIEW    1   // Preview capture and display in file open dialog

// Flags for stuff in development
//#define AUTO_COMPLETE_SEARCH 1  // Use history for auto-complete in search tool - needs refinements/testing
//#define REFRESH_OFF     1   // Turn off display refresh when replacing all - doesn't seem to save much time so leave off for now

#define INTERNAL_ALGORITHM  "Internal Encryption Algorithm"

#define DEFAULT_MACRO_NAME " - Current Macro - "

#define FILENAME_BOOKMARKS   _T("Bookmarks")
#define FILENAME_RECENTFILES _T("RecentFiles")
#define FILENAME_BACKGROUND  _T("Backgrnd.bmp")
#define DIRNAME_PREVIEW      _T("PreviewThumbnails\\")
#define FILENAME_ABOUTBG     _T("About.jpg")
#define FILENAME_SPLASH      _T("Splash.bmp")
#define FILENAME_DTD         _T("BinaryFileFormat.DTD")
#define FILENAME_DEFTEMPLATE _T("Default.xml")

#include <vector>           // For vector of keys that stores keystroke macro
#include <afxmt.h>          // For MFC IPC (CCriticalSection)

#include "resource.h"       // main symbols

// Currently supported character sets
enum { CHARSET_ASCII = 0, CHARSET_ANSI = 1, CHARSET_OEM = 3, CHARSET_EBCDIC = 4, CHARSET_UCODE_EVEN, CHARSET_UCODE_ODD, CHARSET_CODEPAGE };

// Fonts required for different char sets
enum font_t {
	FONT_ANSI,     // Used for CHARSET_ASCII, ANSI and EBCDIC
	FONT_OEM,      // Used for CHARSET_OEM
	FONT_UCODE,    // Used for CHARSET_UCODE_EVEN, CHARSET_UCODE_ODD, CHARSET_CODEPAGE
};

// This is used to store all display flags in one int
// Note: only add to end since this struct is written to files
// We are limited to 32 bits and if we run out of bits then we can use:
//   - not_used_now - spare bit
//   - mark_char - not really that important
//   - vert_display - should at some stage be combined with hex_area/char_area
struct display_bits
{
	unsigned int hex_area: 1;   // Display hex area?
	unsigned int char_area: 1;  // Display char area?
	unsigned int control: 2;    // How control chars are displayed

	// The following 3 bit flags have been combined into a single 3-bit char set
	// to allow for more Unicode and byte page char sets in the future.
	// There were 4 char sets corresponding to these bit patterns in the past:
	//        graphic     oem    graphic
	// ASCII     0         0       0    - uses current ANSI font only showing chars 32-126
	// ANSI      0         0       1    - ANSI font showing extra (graphic etc) chars
	// OEM/IBM   0         1       x    - uses current OEM font showing chars for bytes 0-255
	// EBCDIC    1         x       x    - basic EBCDIC translation into ASCII (ANSI font)
	// When combined the 3 bits give these values:
	// 0 = ASCII, 1 = ANSI, 2/3 = OEM/IBM, 4+ = EBCDIC (cf enum above)

#if 0
	unsigned int graphic: 1;    // Are graphic chars displayed?
	unsigned int oem: 1;        // Are we showing OEM graphics?
	unsigned int ebcdic: 1;     // Are we displaying as EBCDIC
#else
	unsigned int char_set: 3;   // refer to enum above (CHARSET_*)
#endif

	unsigned int autofit: 1;    // Autofit mode is on?
	// --- 8
	unsigned int dec_addr: 1;   // Replaced by sep. hex_addr and decimal_addr flags (below) - can be reused
	unsigned int edit_char: 1;  // Caret is in char area (or hex)?
	unsigned int mark_char: 1;  // Mark is in char area (or hex)?

	unsigned int overtype: 1;       // Are we in overtype mode?
	unsigned int readonly: 1;

	unsigned int unused1: 1;        // Was ruler - made into a global option

	unsigned int hide_highlight: 1; // Hide display of highlights
	unsigned int hide_bookmarks: 1; // Hide display of bookmarks
	// --- 16
	unsigned int auto_sync_dffd: 1;     // Automatically sync DFFD view and main view selections

	unsigned int hide_replace: 1;   // Hide replacements (change tracking)
	unsigned int hide_insert: 1;    // Hide insertions (change tracking)
	unsigned int hide_delete: 1;    // Hide deletions (change tracking)
	unsigned int delete_count: 1;   // Show count of deletions (up to 9) instead of *

	unsigned int vert_display: 1;   // Show vertical display instead of hex/char areas

	unsigned int big_endian: 1;     // Operations on the file are big-endian?

	unsigned int borders: 1;        // Display sector borders
	// --- 24
	// The user can now display 3 things independently in the address area: hex address, dec address, line #
	// Note that this subsumes the dec_addr flag (toggles hex/dec address) which can later be recycled
	unsigned int line_nums: 1;        // Show line numbers?
	unsigned int hex_addr: 1;         // Show hex addresses?
	unsigned int decimal_addr: 1;     // Show decimal addresses?
	unsigned int addrbase1: 1;        // Addresses start at 1 not zero

	unsigned int auto_sync_aerial: 1; // Automatically sync aerial view and main view selections
	unsigned int auto_sync_comp: 1;   // Automatically sync compare and hex view selections
	unsigned int auto_scroll_comp: 1; // Automatically scroll compare/hex views when other scrolled

	// Returns font required for display: currently ANSI unless displaying char area and OEM char set selected
	font_t FontRequired()
	{
		if (!char_area && !vert_display)
			return FONT_ANSI;                  // only showing hex
		else if (char_set == CHARSET_OEM)
			return FONT_OEM;                   // showing oem chars
		else if (char_set == CHARSET_CODEPAGE)
			return FONT_UCODE;                 // showing multi-byte chars (codepage/Unicode)
		else
			return FONT_ANSI;                  // showing ASCII, ANSI, or translated EBCDIC chars
	}  
};

#include "crypto.h"
#include "HexEditMacro.h"
#include "NavManager.h"     // For CNavManager that handles nav points
#include "Options.h"        // For C*Page classes used below
#include "Scheme.h"         // For colour schemes
#include "SpecialList.h"    // For volume/device list (Open Special etc)
#include "timer.h"

enum { COLOUR_OPTIONS_PAGE, MACRO_OPTIONS_PAGE, PRINTER_OPTIONS_PAGE, FILTER_OPTIONS_PAGE, WIN_OPTIONS_PAGE, BACKGROUND_OPTIONS_PAGE, };

class CHexEditDoc;
class CHexFileList;
class CBookmarkList;
class CSpecialList;

/////////////////////////////////////////////////////////////////////////////
// CHexEditApp:
// See HexEdit.cpp for the implementation of this class
//

class CHexEditView;         // Declare this so we can store a ptr to out view class (pview_)
class boyer;

extern DWORD hid_last_file_dialog;

#ifndef NO_SECURITY
extern int quick_check_called;
extern int get_security_called;
extern int dummy_to_confuse;
extern int add_security_called;
extern int new_check_called;
#endif

class CHexEditApp : public CWinAppEx
{
public:
		CHexEditApp();
		virtual ~CHexEditApp();

// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CHexEditApp)
	public:
		virtual BOOL InitInstance();
		virtual int ExitInstance();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnIdle(LONG lCount);
	virtual void WinHelp(DWORD dwData, UINT nCmd = HELP_CONTEXT);
	virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName);
	//}}AFX_VIRTUAL

	UINT  m_nAppLook;

	virtual void PreLoadState();
	virtual void OnAppContextHelp(CWnd* pWndControl, const DWORD dwHelpIDArray[]);

	// Allow creating of a document from a text string
	bool no_ask_insert_;                // Turn this on to prevent OnFileNew from prompting with insertion options
	void FileFromString(LPCTSTR str);   

// Command handlers
protected:
	afx_msg void OnFileNew();
	afx_msg void OnFileOpen();
	afx_msg BOOL OnOpenRecentFile(UINT nID);
	afx_msg void OnFilePrintSetup();
	afx_msg void OnAppExit();
//    void LoadStdProfileSettings(UINT nMaxMRU = _AFX_MRU_COUNT);

	afx_msg void OnRepairFiles();
	afx_msg void OnRepairDialogbars();
	afx_msg void OnRepairCust();
	afx_msg void OnRepairSettings();
	afx_msg void OnRepairAll();

public:
// Implementation
	void UpdateAllViews();      // just redraw all views

	static const char *szHexEditClassName; // Class name of mainframe
	static UINT wm_hexedit;     // Message for communicating between different HexEdit instances (to open files)
	HWND hwnd_1st_;             // Handle of previous instance mainfarm window or NULL if no previous instance

	CHexFileList *GetFileList() const { return (CHexFileList *)m_pRecentFileList; }
	CBookmarkList *m_pbookmark_list;
	CBookmarkList *GetBookmarkList() const { return m_pbookmark_list; }

	CSpecialList *m_pspecial_list;
	CSpecialList *GetSpecialList()
	{
		if (m_pspecial_list == NULL)
			m_pspecial_list = new CSpecialList(0);
		return m_pspecial_list;
	}

	void display_options(int display_page = -1, BOOL must_show_page = FALSE);
	void LoadOptions();
	void SaveOptions();         // Save global options to INI/registry
	void LoadSchemes();
	void SaveSchemes();
	void GetXMLFileList();
//    bool GetColours(const char *, const char *, const char *, const char *, partn &retval);
//    void SetColours(const char *, const char *, const char *, const char *, const partn &v);
	// Get global options and pu them into the property sheet
	void get_options(struct OptValues &val);

	// Set global options from values in options dialog
	// Note: Info tips, Colours and Filters pages are handled differently
	// They handle their own update in the OnOK members.
	void set_options(struct OptValues &val);
//    void set_general(struct OptValues &val);         // System options page
//    void set_sysdisplay(struct OptValues &val);      // Global Display page
//    void set_printer(struct OptValues &val);         // printer page
//    void set_macro(struct OptValues &val);           // macro page
	timer set_options_timer;    // See description of the use of this in set_options()

	// Set active window options
//    void set_windisplay(struct OptValues &val, CHexEditView *pview);  // window display page
//    void set_winedit(struct OptValues &val, CHexEditView *pview);     // window edit page

	CMultiDocTemplate* m_pDocTemplate;

	// Current search info
	CCriticalSection appdata_;  // Protects access to following data
	boyer *pboyer_;             // Ptr to current search pattern (NULL if none)
								// (Also stores search bytes and their length.)
	int text_type_;             // Type of text search (0=binary, 1=ascii, 2=Unicode, 3=EBCDIC)
	BOOL icase_;                // Indicates a case-insensitive search (only for text_type_ > 0)
	BOOL wholeword_;            // Match whole word only (only for text_type_ > 0)
	int alignment_;             // 1 means no alignment restrictions
	int offset_;                // 0 <= offset_ < alignment_
	bool align_rel_;            // Force alignment relative to current mark posn

	void StartSearches(CHexEditDoc *pp);
	void StopSearches();
	void NewSearch(const unsigned char *pat, const unsigned char *mask, size_t len,
				   BOOL icase, int tt, BOOL ww, int aa, int offset, bool align_rel);

	//{{AFX_MSG(CHexEditApp)
	afx_msg void OnAppAbout();
	afx_msg void OnOptions();
	afx_msg void OnOptions2();
	afx_msg void OnMacroRecord();
	afx_msg void OnMacroPlay();
	afx_msg void OnUpdateMacroPlay(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMacroRecord(CCmdUI* pCmdUI);
	afx_msg void OnProperties();
	afx_msg void OnMultiPlay();
	afx_msg void OnHelpEmail();
	afx_msg void OnUpdateHelpEmail(CCmdUI* pCmdUI);
	afx_msg void OnHelpWeb();
	afx_msg void OnUpdateHelpWeb(CCmdUI* pCmdUI);
	afx_msg void OnWebPage();
	afx_msg void OnEncryptAlg();
	afx_msg void OnEncryptClear();
	afx_msg void OnUpdateEncryptClear(CCmdUI* pCmdUI);
	afx_msg void OnEncryptPassword();
	afx_msg void OnMacroMessage();
	afx_msg void OnUpdateMacroMessage(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMultiPlay(CCmdUI* pCmdUI);
	afx_msg void OnRecentFiles();
	afx_msg void OnBookmarksEdit();
	afx_msg void OnTabIcons();
	afx_msg void OnTabsAtBottom();
	afx_msg void OnUpdateTabIcons(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTabsAtBottom(CCmdUI* pCmdUI);
	afx_msg void OnFileOpenSpecial();
	afx_msg void OnUpdateFileOpenSpecial(CCmdUI* pCmdUI);
	//}}AFX_MSG
	afx_msg void OnHelpWebForum();
	afx_msg void OnHelpWebHome();
	afx_msg void OnHelpWebReg();
	afx_msg void OnCompressionSettings();
	afx_msg void OnFileSaveAll();
	afx_msg void OnFileCloseAll();
	afx_msg void OnFileCloseOthers();
	DECLARE_MESSAGE_MAP()

public:
	short version_;             // Use 3 decimal digits as Maj.MinRel eg 320 means 3.2, 321 means 3.2A
	short beta_;                // Beta version (0 if not a beta)
	short revision_;            // This should be a unique build no (one day we will use SVN revision number)

	// USE_HTML_HELP
	CString htmlhelp_file_;
	BOOL HtmlHelpWmHelp(HWND hw, DWORD *id_pairs)
	{
		CWaitCursor wait;
		if (::HtmlHelp(hw, htmlhelp_file_+"::/CtlIdMap.txt", HH_TP_HELP_WM_HELP, (DWORD) (LPSTR) id_pairs) == HWND(0))
		{
			AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
			return FALSE;
		}
		return TRUE;
	}

	void HtmlHelpContextMenu(CWnd * pWnd, DWORD *id_pairs)
	{
		int id;
		// This never gets executed - Windows seems to use the parent window for the context menu of static controls
		//while (pWnd != NULL && (id = pWnd->GetDlgCtrlID()) == IDC_STATIC)
		//	pWnd = pWnd->GetNextWindow();

		// Ignore windows with no ID (probably dialog window)
		if (pWnd == NULL || (id = pWnd->GetDlgCtrlID()) < 1)
			return;

		CWaitCursor wait;
		if (::HtmlHelp((HWND)pWnd->GetSafeHwnd(),
					   htmlhelp_file_+"::/CtlIdMap.txt",
					   HH_TP_HELP_CONTEXTMENU, 
					   (DWORD) (LPSTR) id_pairs) == HWND(0))
			TRACE("Failed to launch help for ID %d ($%X)\n", id, id);
	}
#if 0
	BOOL HtmlHelpContextMenu(HWND hw, DWORD *id_pairs)
	{
		CWaitCursor wait;
		if(::HtmlHelp(hw, htmlhelp_file_+"::/CtlIdMap.txt", HH_TP_HELP_CONTEXTMENU, (DWORD) (LPSTR) id_pairs) == HWND(0))
		{
			AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
			return FALSE;
		}
		return TRUE;
	}
#endif

	BOOL is_nt_;                        // Are we running under NT family? (NT/W2K/XP and later)
	BOOL is_xp_;                        // Is it XP or later?
	BOOL is_vista_;                     // Is it Vista or later?
	BOOL is_win7_;                      // Windows 7 or Windows Server 2008 R2 or later?
	BOOL mult_monitor_;                 // Are we running on an OS with multiple monitor support?
	BOOL is_us_;                        // Are we in US?  (for spelling fixes)
	BOOL win95_;                        // Is it Windows 95? (not 98 or later and not NT or later)

	// Locale settings
	char dec_sep_char_;		            // Char to use as "thousands" separator for decimals
	int dec_group_;			            // Number of digits in "thousands" group
	char dec_point_;                    // Decimal point for floating point numbers

	// Stuff for keystroke macros
	void macro_play(long play_times=1, const std::vector<key_macro> *pmac=NULL, int halt_lev=-1);
	BOOL macro_save(const char *filename, const std::vector<key_macro> *pmac = NULL,
					const char *comment = NULL, int halt_lev = -1, long plays = 1, int version = 0);
	BOOL macro_load(const char *filename, std::vector<key_macro> *pmac,
					CString &comment, int &halt_lev, long &plays, int &version);
	void RunAutoExec();

	BOOL highlight_;                    // Is highlighting on?

	// The following are global options for aerial views
	unsigned int aerialview_;           // 0=none, 2=tabbed views, else splitter view width
	unsigned long aerial_disp_state_;
	bool auto_aerial_zoom_;
	unsigned int aerial_max_;           // Max size for bitmap in MBytes (min = 16)

	unsigned int compview_;             // How is compare view window shown: 0=none, 2=tabbed views, else splitter view width

	// The following are options for the binary file format tree view display
	unsigned int dffdview_;             // 0=none, 2=tabbed views, else splitter view width

	CString xml_dir_;                   // Where XML files (and DTD) are stored
	int max_fix_for_elts_;              // Max no of fixed sized elts in array to display
	bool alt_data_bg_cols_;             // Use alternate background colours for data lines of tree/grid
	CString default_char_format_, default_string_format_,
			default_int_format_, default_unsigned_format_, 
			default_real_format_, default_date_format_;


	// The following are options for keystroke macros
	CString mac_dir_;                   // Where macros are saved
	CString mac_comment_;               // Description of last loaded macro
	CString mac_filename_;              // Filename of last loaded macro

	BOOL recording_;                    // Are we recording a macro?
	BOOL no_keys_;                      // Have we recorded any keys yet?
	int playing_;                       // 0 if not playing else nest level
	CHexEditView *pv_;                  // Current view in playback (NULL if not playing)

	CHexEditView *pview_;               // Last view that got focus (used in macro recording)
	std::vector<key_macro> mac_;        // Stores the "keys" of the current macro
	int mac_error_;                     // Error level set during play of macro
	// mac_error_ = 0  No error
	// mac_error_ = 1  Warning
	// mac_error_ = 2  Ignorable error
	// mac_error_ = 5  Normal error - warn user
	// mac_error_ = 10 Serious error - user has already been informed
	// mac_error_ = 20 Unlikely system/internal error
	bool refresh_off_;                  // Disables refresh during playback
	int macro_version_;                 // Version of HexEdit in which macro was recorded (see INTERNAL_VERSION)

	__int64 last_fill_state_;           // Contains fill options (file/new)
	CString last_fill_str_;             // Depends on options (string, number, range etc)

	// Encryption
	void set_alg(const char *pp);
	void set_password(const char *pp);
	CCrypto crypto_;            // Handles all CryptoAPI stuff
	int algorithm_;             // Current encryption algorithm
	CString password_;          // Current encryption password

	// Clipboard
	static const char * bin_format_name;  // Name of custom binary clipboard format
	static const char * temp_format_name; // Name of custom format where data stored to temp file
	static const char * flag_format_name; // Custom format that has no data but just indicates that current text is hex text
	FILE_ADDRESS last_cb_size_; // Size of last thing we put on the clipboard
	DWORD last_cb_seq_;         // Clipboard seq no for last thing added
	CString last_cb_temp_file_; // Clipboard data is in temp file here
	void ClipBoardAdd(FILE_ADDRESS len, LPCTSTR temp_file = NULL)
	{
		if (!last_cb_temp_file_.IsEmpty())
		{
			CString tt(last_cb_temp_file_);
			last_cb_temp_file_.Empty();
			::remove(tt);
		}
		last_cb_size_ = len;
		last_cb_seq_ = ::GetClipboardSequenceNumber();
		last_cb_temp_file_ = temp_file;  // OK if temp_file is NULL!
	}

	void play_macro_file(const CString &filename, int pp = -1) ;

	// Save the entry in the macro vector if we are currently recording
	void SaveToMacro(enum km_type k, unsigned __int64 v64 = 0)
	{
		// Make sure that this is not called for special macro types
		ASSERT(k != km_find_text && k != km_replace_text && k != km_open &&
			   k != km_read_file && k != km_focus &&
			   k != km_import_motorola && k != km_import_intel && k != km_import_text &&
			   k != km_macro_message && k != km_macro_play &&
			   k != km_address_hex && k != km_address_dec &&
			   k != km_scheme && k != km_font &&
			   k != km_expression && k != km_user_str && 
			   k != km_encrypt_alg && k != km_encrypt_password &&
			   k != km_bookmarks_add && k != km_bookmarks_goto &&
			   k != km_new_str && k != km_insert_str &&
			   k != km_mouse && k != km_shift_mouse);

		if (recording_)
		{
			if (no_keys_)
			{
				// This is the first "keystroke" of the macro so
				// clear the previous macro (if any)
				no_keys_ = FALSE;
				mac_filename_.Empty();
				mac_comment_.Empty();
				mac_.clear();
			}
			mac_.push_back(key_macro(k, v64));
			TRACE("=== MACRO RECORD: %d %d\n", int(k), int(v64));
		}
	}
#if 0 // Remove this (replaced by above) due to ambiguous calls problems
	  // Note that this may cause portability problems since for some macro
	  // key types we assign to v64 of the union but get the value from v.
	void SaveToMacro(enum km_type k, long v)
	{
		ASSERT(k != km_find_text && k != km_replace_text && k != km_open &&
			   k != km_read_file && k != km_focus &&
			   k != km_import_motorola && k != km_import_intel && k != km_import_text &&
			   k != km_macro_message && k != km_macro_play &&
			   k != km_address_hex && k != km_address_dec &&
			   k != km_scheme && k != km_font &&
			   k != km_expression && k != km_user_str && 
			   k != km_encrypt_alg && k != km_encrypt_password &&
			   k != km_bookmarks_add && k != km_bookmarks_goto &&
			   k != km_new_str && k != km_insert_str &&
			   k != km_mouse && k != km_shift_mouse);
		if (recording_)
		{
			if (no_keys_)
			{
				// This is the first "keystroke" of the macro so
				// clear the previous macro (if any)
				no_keys_ = FALSE;
				mac_filename_.Empty();
				mac_comment_.Empty();
				mac_.clear();
			}
			mac_.push_back(key_macro(k,v));
		}
	}
#endif
	void SaveToMacro(enum km_type k, const CString &ss)
	{
		ASSERT(k == km_find_text || k == km_replace_text || k == km_open || 
			   k == km_macro_message || k == km_macro_play ||
			   k == km_address_hex || k == km_address_dec ||
			   k == km_encrypt_alg || k == km_encrypt_password ||
			   k == km_read_file || k == km_import_motorola ||
			   k == km_import_intel || k == km_import_text ||
			   k == km_bookmarks_add || k == km_bookmarks_goto ||
			   k == km_new_str || k == km_insert_str ||
			   k == km_focus || k == km_scheme ||
			   k == km_expression || k == km_user_str);
		if (recording_)
		{
			if (no_keys_)
			{
				// This is the first "keystroke" of the macro so
				// clear the previous macro (if any)
				no_keys_ = FALSE;
				mac_filename_.Empty();
				mac_comment_.Empty();
				mac_.clear();
			}
			mac_.push_back(key_macro(k,ss));
		}
	}
	void SaveToMacro(enum km_type k, LOGFONT *plf)
	{
		ASSERT(k == km_font);
		if (recording_)
		{
			if (no_keys_)
			{
				// This is the first "keystroke" of the macro so
				// clear the previous macro (if any)
				no_keys_ = FALSE;
				mac_filename_.Empty();
				mac_comment_.Empty();
				mac_.clear();
			}
			mac_.push_back(key_macro(k,plf));
		}
	}
	void SaveToMacro(enum km_type k, mouse_sel *pms)
	{
		ASSERT(k == km_mouse || k == km_shift_mouse);
		if (recording_)
		{
			if (no_keys_)
			{
				// This is the first "keystroke" of the macro so
				// clear the previous macro (if any)
				no_keys_ = FALSE;
				mac_filename_.Empty();
				mac_comment_.Empty();
				mac_.clear();
			}
			mac_.push_back(key_macro(k,pms));
		}
	}
	int FindXMLFile(const char *filename)
	{
		for (size_t ii = 0; ii < xml_file_name_.size(); ++ii)
			if (xml_file_name_[ii].CompareNoCase(filename) == 0)
			{
				if (_access(xml_dir_ + filename + CString(".xml"), 0) == -1)
					return -1;
				else
					return (int)ii;
			}

		return -1;
	}

	// Current values for open settings
	//BOOL open_file_readonly_;  // Removed as we always default to read-only flag off
	BOOL open_file_shared_;      // Current value of global setting
	// Temp flags for passing to CHexEditDocument::open_file
	BOOL open_current_readonly_;

	// Startup options
	BOOL save_exit_;                    // Save settings on exit?
	BOOL orig_save_exit_;               // Original value of save_exit_
	BOOL one_only_;                     // Only allow one instance of app to run
	BOOL open_restore_;                 // Restore main frame window on startup?
	BOOL special_list_scan_;            // Scan devices/volumes at startup
	BOOL splash_;                       // Display splash screen?
	BOOL tipofday_;                     // Show tip of the day at startup?
	BOOL run_autoexec_;                 // Run AutoExec.HEM at startup if it exists
	BOOL update_check_;                 // Check for updates?

	// Folder options
	int     open_locn_;                 // Default folder for open dialog (0=current doc folder, ... see enum FOLDER_LOCN)
	CString open_folder_;               // Folder to use if explicitly specified option chosen
	int     save_locn_;                 // Default folder for save as dialog (0=current doc folder, ...)
	CString save_folder_;               // Folder to use if explicitly specified option chosen

#ifdef FILE_PREVIEW
	BOOL thumbnail_;                    // Generate a preview file when file is saved
	int thumb_size_;                    // Size of thumbnail
	double thumb_zoom_;                 // How much to zoom the thumbnail when showing it
	enum THUMB_TYPE { THUMB_NONE, PNG, JPEG_GOOD, JPEG_AVERAGE, JPEG_BAD, THUMB_LAST };
	int thumb_type_;                    // Type of file to use
	//BOOL thumb_8bit_;                   // Thumbnail saved with 8bpp (rather than 24)
	BOOL thumb_frame_;                  // Thumbnail of child frame window (rather than just hex view)
	int cleanup_days_;                  // Number of days before thumbnail is deleted

	void CleanupPreviewFiles();         // Starts thread to remove thumbnails older than cleanup_days_
	UINT RunCleanupThread();            // function that runs in the background thread
	CWinThread * cleanup_thread_;       // cleanup is done in low-priority background thread
	bool thread_stop_;                  // Signal the thread to stop
#endif

	// History list options
	BOOL no_recent_add_;                // When a file is opened it is not added to "My Recent Documents" (OFN_DONTADDTORECENT)
	int max_search_hist_;               // dv = 48
	int max_replace_hist_;              // dv = 16
	int max_hex_jump_hist_;             // dv = 16
	int max_dec_jump_hist_;             // dv = 16
	int max_expl_dir_hist_;             // dv = 32
	int max_expl_filt_hist_;            // dv = 16
	BOOL clear_recent_file_list_;       // Clearing recent file list?
	BOOL clear_bookmarks_;              // Clearing bookmarks?
	BOOL clear_on_exit_;                // Clear the above on exit

	// Backup options
	BOOL backup_;                       // Create backups on save? (following conditions must also be satisfied)
	BOOL backup_space_;                 // Only if enough disk space.
	int backup_size_;                   // Only if the file is less than this many MBytes (0 = always)
	BOOL backup_prompt_;                // Only backup after prompting user.

	// Macro options
	int refresh_;                       // Refresh type (0=none,1=secs,2=keys,3=plays)
	long num_secs_;                     // Number of seconds before refresh (refresh_ == 1)
	long num_keys_;                     // Number of keys before refresh (refresh_ == 2)
	long num_plays_;                    // Number of plays before refresh (refresh_ == 3)
	int halt_level_;                    // When to halt (0=any warning/error,1=any error,2=major errors)
	BOOL refresh_props_;                // Refresh properties dialog when refreshing
	BOOL refresh_bars_;                 // Refresh status bar and tools on edit bar
	long plays_;                        // Default number of plays in Multiplay dlg

	BOOL bg_search_;                    // Do background searches?
	BOOL bg_stats_;                     // Calc file stats in background thread?
	  BOOL bg_stats_crc32_;             // Do CRC32 as well
	  BOOL bg_stats_md5_;               // Do MD5 as well
	  BOOL bg_stats_sha1_;              // Do SHA1 as well
	BOOL bg_exclude_network_;           // Don't do background search/stats for files on network drives
	BOOL bg_exclude_removeable_;        // Don't do background search/stats for files on removeable media
	BOOL bg_exclude_optical_;           // Don't do background search/stats for files on CD, DVD
	BOOL bg_exclude_device_;            // Don't do background search/stats for files on raw devices and volumes

	// Global display options
	BOOL mditabs_;                      // Show MDI tabs
	BOOL tabsbottom_;                   // Show MDI tabs at bottom (not top)
	BOOL tabicons_;                     // Show icons in MDI tabs
	BOOL large_cursor_;                 // Use large (block) cursor
	BOOL show_other_;                   // Show where the cursor would be in the other (hex/char) area
	BOOL nice_addr_;                    // Display nice looking addresses?
	BOOL sel_len_tip_, sel_len_div2_;   // Show selection len tip window when dragging or when SHIFT held down?
	BOOL scroll_past_ends_;             // Scrolling past end of doc allowed?
	int autoscroll_accel_;              // How quickly autoscroll increases as mouse is dragged past edge of window (0 = none, 10 = linear, 20 = squared etc)
	BOOL dlg_dock_, dlg_move_;          // Modeless dialogs: can dock? move out of the way when floating?
	BOOL reverse_zoom_;                 // Reverse Ctrl+wheel zoom direction

	int cont_char_;                     // Used for continuation bytes in MBCS (CHARSET_CODEPAGE)
	int invalid_char_;                  // Used for error bytes in MBCS (CHARSET_CODEPAGE)

	BOOL hex_ucase_;                    // Display hex in upper case?
	int k_abbrev_;                      // How are K/M/G abbreviated (0=1000/1000/1000, 1=1024/1000/1000, 2=1024/1024/1000, 3=1024/1024/1024)

	BOOL ruler_;                        // Show (hex and/or decimal) ruler?
	UINT ruler_hex_ticks_, ruler_dec_ticks_; // How often to show major ticks in the ruler(s)
	UINT ruler_hex_nums_, ruler_dec_nums_;   // How often to show digits in the rulers
	BOOL hl_caret_, hl_mouse_;          // Highlight caret or mouse in ruler/address area

	BOOL delete_reg_settings_;          // Delete all registry settings on exit (used in repair commands)
	BOOL delete_all_settings_;          // Delete settings files (recent file list etc), reg info. etc

	BOOL intelligent_undo_;             // Do op then reverse op does not change undo stack
	int undo_limit_;                    // How many bytes of consec. undo info can be merged before starting a new undo
	int cb_text_type_;                  // Says how Edit/Cut+Copy+Paste behave

	// Info (tip) window options
	// Note that the top entried are "hard-coded" and have empty tip_expr_ and tip_format_ values,
	// for example, currently entry 0 is for "Bookmarks" to allow the user to see the name of a
	// bookmark if the cursor hovers over one.  Other hard-coded entries will be added later.
	std::vector<CString> tip_name_;     // Name for the user to see in check-list
	std::vector<bool> tip_on_;          // Is this tip on (checkbox in check-list checked)
	std::vector<CString> tip_expr_;     // Expression (involving predefined variable names like "address")
	std::vector<CString> tip_format_;   // How expression is formated for display (eg "hex", "%x" etc)
	int tip_transparency_;              // How transparent to make the tip
	CSize tip_offset_;                  // Where to draw tip relative to mouse pointer
	//static const int FIRST_USER_TIP = 1;
#define FIRST_USER_TIP 1                // = no of hard-coded tip types - 1 at present since only hard-coded type is bookmarks

	// Printing options
	enum prn_unit_t {PRN_INCHES, PRN_CM } print_units_; // Units for margins/distances
	double left_margin_, right_margin_, top_margin_, bottom_margin_;
	double header_edge_, footer_edge_;  // How far the header/footer are from the top/bottom edge of the page
	bool print_box_, print_hdr_;        // Print a border around the text and column headings
	bool print_mark_, print_bookmarks_, print_highlights_, print_search_, print_change_, print_compare_, print_sectors_;
	int spacing_;                       // 0 = 1 line, 1 = 1.5 lines 2 = 2 lines
	CString watermark_, header_, first_header_, footer_, first_footer_;
	bool print_watermark_;              // Print light "watermark" behind the text
	bool diff_first_header_, diff_first_footer_; // Print different header/footer on the first page
	bool even_reverse_;					// Reverse header/footer sections for even pages

	// Encryption password settings
	BOOL password_mask_;                // Mask password while entering?
	int password_min_;                  // Minimum password length

	// Use member function to say if backups required in case we want to
	// do anything more complicated later
	BOOL backup(LPCTSTR file_name, FILE_ADDRESS file_len) const;

	// Default window states
	BOOL open_max_;                     // Open new windows maximized?
	union
	{
		DWORD open_disp_state_;
		struct display_bits open_display_;
	};
	int open_code_page_;

	LOGFONT *open_plf_;                 // Pointer to default font or NULL if none
	LOGFONT *open_oem_plf_;             // Pointer to default OEM font or NULL if none
	LOGFONT *open_mb_plf_;              // Pointer to default code page/Unicode font or NULL if none

	// All colour schemes available
	std::vector<CScheme> scheme_;

	std::vector<CString> xml_file_name_;    // Names of all XML files found

	// "factory" defaults for when these special schemes are reset
	CScheme default_scheme_,            // settings used for resetting a scheme
			default_ascii_scheme_, default_ansi_scheme_,
			default_oem_scheme_, default_ebcdic_scheme_,
			default_multi_scheme_;

	CString open_scheme_name_;          // Name of default scheme

	int open_rowsize_;                  // Default number of display columns
	int open_group_by_;                 // Default column grouping
	int open_offset_;                   // Display offset on open
	int open_vertbuffer_;

	BOOL open_keep_times_;              // Default to keeping file times?

	CString GetCurrentFilters();        // Return filter string without disabled filters
	CString current_filters_;           // String representing all file selection filters

	// Remembered user interface stuff (may even be saved to registry)
	CString last_open_folder_, last_save_folder_, last_both_folder_;  // Remember last used folders (see open_locn_, save_locn_)
	CString current_write_, current_read_; // Current files (defaults for file dlgs)
	CString current_import_, current_export_; // Current dits for import/export
	long export_base_addr_;             // Base address when exporting (-1 for file address)
	int export_line_len_;               // Bytes per line when exporting
	BOOL import_discon_;                // Allow discontiguous addresses when importing (and exporting too now)
	BOOL import_highlight_;             // Highlight changed bytes for import
	int recent_files_;                  // Number of recent files to put on menus

	int prop_page_;                     // Last active properties page
	UINT prop_dec_signed_;              // Display signed decimal values
	BOOL prop_dec_endian_;              // Display big-endian decimal values
	UINT prop_fp_format_;               // Which floating point format to display
	BOOL prop_fp_endian_;               // Display big-endian floating pt values
	//int prop_ibmfp_format_;             // Which IBM floting point format to display
	//BOOL prop_ibmfp_endian_;            // Display big-endian IBM fp values
	UINT prop_date_format_;             // Which date format to display
	BOOL prop_date_endian_;             // Display big-endian date values
	int calc_radix_;                    // Radix used in calculator
	int calc_bits_;                     // Number of bits used in calculator

	CNavManager navman_;				// Manages list of nav points for Navigate Back/Forward

#ifndef NO_SECURITY
	// Security info
	void AddSecurity(const char *name);
	void SaveSecurityFile();
	BOOL GetSecurityFile();
	bool CheckNewVersion();
	void OnNewVersion(int, int);         // Called when version number changes
	int NewCheck();
	void DeleteSecurityFiles();
	void CleanUp();                     // Delete security files and reg entries

	time_t init_date_;                  // When HexEdit was 1st run

	int security_type_;                 // 1 = not registered (trial expired)
										// 2 = not registered but still in 30 day trial
										// 3 = temp reg
										// 4 = no longer registered (more than 5 versions old)
										// 5 = no longer registered (upgradeable),
										// 6 = reg for current or most recent previous version
										// -1 = error
	int days_left_;                     // Days till expiry (if security_type_ == 1 or 2)
	CString security_name_;             // Name of registered user (if security_type_ == 6)
	short security_rand_;                // Random number used for checks

	// UPDATE THIS FOR EACH NEW VERSION
	static const int security_version_; // An incremented version number for the current release
	int security_licensed_version_;     // Version that current licence is for (may be < security_version_)
#endif

private:
	void refresh_display(bool do_all = false);
	void enable_carets();
	void disable_carets();

#ifndef NO_SECURITY
	// Security stuff
	void GetMystery();
	int GetMysteryFile(const char * filename);
	void SaveMyst(const char * filename);
	void SaveMyst2(const char * filename);
	bool GetMyst2FileName(CString & filename);
	int GetSecurity();
	void SaveTo(const char *filename, const void *pdata, size_t len);
	bool ReadFrom(const char *filename, void *pdata, size_t len);
	int QuickCheck();
	void CheckSecurityActivated();
#endif

	void OnNewUser();
	void CopyUserFiles();
	void ShowTipAtStartup(void);
	void ShowTipOfTheDay(void);
	void InitVersionInfo();
	void InitWorkspace();
	void InitConversions();
};

class CCommandLineParser : public CCommandLineInfo
{
public:
	CCommandLineParser() { non_std = TRUE; }

	virtual void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast);

private:
	BOOL non_std;
};

extern CHexEditApp theApp;
inline BOOL IsUs() { return theApp.is_us_; }  // Used for diff spelling

// These chars must correspond to the values in UCODE_CHAR_TYPE
static const wchar_t SPECIAL_CHARS[] = {L' ', 0x00B7, 0x2022, 0x2026, L'?', 0x00BF, 0x097A, 0xFFF9, 0xFFFD, 0 };

inline const wchar_t * ContChar()
{
	ASSERT(sizeof(SPECIAL_CHARS)/sizeof(*SPECIAL_CHARS) == UCODE_LAST + 1);
	return SPECIAL_CHARS + theApp.cont_char_;
}

inline const wchar_t * BadChar()
{
	ASSERT(sizeof(SPECIAL_CHARS)/sizeof(*SPECIAL_CHARS) == UCODE_LAST + 1);
	return SPECIAL_CHARS + theApp.invalid_char_;
}

extern unsigned char e2a_tab[256];
extern unsigned char a2e_tab[128];

extern unsigned char a2i_tab[128];
extern unsigned char i2a_tab[128];

extern CHexEditView *GetView();
extern COLORREF GetDecAddrCol();
extern COLORREF GetHexAddrCol();
extern COLORREF GetSearchCol();

extern BOOL SendEmail(int def_type = 0, const char *def_text = NULL, const char *def_name = NULL);
extern CString reg_code(int send, int flags = 0);
extern CString user_reg_code(int send, const char *name, int type = 6, int flags = 0, time_t tt = 0);

// This is in a macro to ensure that a simple function call cannot be disabled.
// This can be inserted in lots of places; a cracker can't be sure he found them all.
// Also making it hard to check is that the last check is only done occasionally:
// one in nn times (at random), where nn is the parameter passed to the macro.
// The idea is that CHECK_SECURITY is called all over the place making it hard for
// a cracker to find them all.  In often used places use a high value for the param.
// (nn) so that checks are done rarely.  For less often used places use a low value.
#ifdef _DEBUG
#define CHECK_SECURITY(nn) \
do { \
	if ((::GetTickCount()>>10)%(nn) == 0 && (((CHexEditApp*)AfxGetApp())->security_type_ < 1 || !theApp.NewCheck())) \
		TRACE("!!!!!!!!!!!!!!Security check: ABORT!!!!!!!!!!!!\n"); \
} while(0)
#else
#ifdef NO_SECURITY
#define CHECK_SECURITY(nn) do { } while(0)
#else
#define CHECK_SECURITY(nn) \
do { \
	if ((::GetTickCount()>>10)%(nn) == 0 && (((CHexEditApp*)AfxGetApp())->security_type_ < 1 || !theApp.NewCheck())) \
		AfxAbort(); \
} while(0)
#endif  // NO_SECURITY
#endif  // _DEBUG

#endif
