#ifndef _INC_COMMCTRL_TASKDIALOGS
#define _INC_COMMCTRL_TASKDIALOGS

#include <windows.h>
//#include <commctrl.h>

#ifndef WINCOMMCTRLAPI_NO_IMPORT
#pragma comment (lib, "MLtaskdialogs.lib")
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define TDM_FIRST						(CCM_LAST + 100U)
#define TDN_FIRST						(0U - 1800U)

typedef DWORD MLTASKDIALOG_FLAGS;
typedef UINT MLTASKDIALOG_COMMON_BUTTON_FLAGS;
typedef UINT MLTASKDIALOG_ELEMENTS;
typedef UINT MLTASKDIALOG_ICON_ELEMENTS;

//
// TASKDIALOG_COMMON_BUTTON_FLAGS
//
#define MLCBF_OK_BUTTON					0x0100
#define MLCBF_CANCEL_BUTTON				0x0200
#define MLCBF_RETRY_BUTTON				0x0400
#define MLCBF_YES_BUTTON				0x0800
#define MLCBF_NO_BUTTON					0x1000
#define MLCBF_CLOSE_BUTTON				0x2000
#define MLCBF_HELP_BUTTON				0x8000

#ifndef TD_WARNING_ICON

//
// TASKDIALOG_FLAGS
//
#define TDF_ENABLE_HYPERLINKS			0x00000001
#define TDF_USE_HICON_MAIN				0x00000002
#define TDF_USE_HICON_FOOTER			0x00000004
#define TDF_ALLOW_DIALOG_CANCELLATION	0x00000008		
#define TDF_USE_COMMAND_LINKS			0x00000010
#define TDF_USE_COMMAND_LINKS_NO_ICON	0x00000020
#define TDF_EXPAND_FOOTER_AREA			0x00000040
#define TDF_EXPANDED_BY_DEFAULT			0x00000080
#define TDF_VERIFICATION_FLAG_CHECKED	0x00000100
#define TDF_SHOW_PROGRESS_BAR			0x00000200
#define TDF_SHOW_MARQUEE_PROGRESS_BAR	0X00000400
#define TDF_CALLBACK_TIMER				0x00000800
#define TDF_POSITION_RELATIVE_TO_WINDOW	0x00001000
#define TDF_RTL_LAYOUT					0x00002000
#define TDF_NO_DEFAULT_RADIO_BUTTON		0x00004000
#define TDF_CAN_BE_MINIMIZED			0x00008000

//
// Messages
//
#define TDM_CLICK_BUTTON						(TDM_FIRST +  1)

#define TaskDialog_ClickButton(hWndTD, uID)	\
	(void)SNDMSG((hWndTD), TDM_CLICK_BUTTON, (WPARAM)(UINT)(uID), 0L)

#define TDM_CLICK_RADIO_BUTTON					(TDM_FIRST +  2)

#define TaskDialog_ClickRadioButton(hWndTD, uID)	\
	(void)SNDMSG((hWndTD), TDM_CLICK_RADIO_BUTTON, (WPARAM)(UINT)(uID), 0L)

#define TDM_CLICK_VERIFICATION					(TDM_FIRST +  3)

#define TaskDialog_ClickVerification(hWndTD, bState, bFocus)	\
	(void)SNDMSG((hWndTD), TDM_CLICK_VERIFICATION, (WPARAM)(BOOL)(bState), (LPARAM)(BOOL)(bFocus))

#define TDM_ENABLE_BUTTON						(TDM_FIRST +  4)

#define TaskDialog_EnableButton(hWndTD, uID, bEnabled)	\
	(void)SNDMSG((hWndTD), TDM_ENABLE_BUTTON, (WPARAM)(UINT)(uID), (LPARAM)(BOOL)(bEnabled))

#define TDM_ENABLE_RADIO_BUTTON					(TDM_FIRST +  5)

#define TaskDialog_EnableRadioButton(hWndTD, uID, bEnabled)	\
	(void)SNDMSG((hWndTD), TDM_ENABLE_RADIO_BUTTON, (WPARAM)(UINT)(uID), (LPARAM)(BOOL)(bEnabled))

#define TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE	(TDM_FIRST +  6)

#define TDM_SET_PROGRESS_BAR_MARQUEE			(TDM_FIRST +  7)

#define TaskDialog_SetProgressBarMarquee(hWndTD, marquee, speed)	\
	(void)SNDMSG((hWndTD), TDM_SET_PROGRESS_BAR_MARQUEE, (WPARAM)(BOOL)(marquee), (LPARAM)(UINT)(speed))

#define TDM_SET_PROGRESS_BAR_POS				(TDM_FIRST +  8)

#define TaskDialog_SetProgressBarPos(hWndTD, pos)	\
	(void)SNDMSG((hWndTD), TDM_SET_PROGRESS_BAR_POS, (WPARAM)(UINT)(pos), 0L)

#define TDM_SET_PROGRESS_BAR_RANGE				(TDM_FIRST +  9)

#define TaskDialog_SetProgressBarRange(hWndTD, mini, maxi)	\
	(void)SNDMSG((hWndTD), TDM_SET_PROGRESS_BAR_RANGE, (WPARAM)0, MAKELPARAM((mini), (maxi)))

#define PBST_NORMAL	0
#define PBST_PAUSED 1
#define PBST_ERROR	2

#define TDM_SET_PROGRESS_BAR_STATE				(TDM_FIRST + 10)


#define TDM_SET_MARQUEE_PROGRESS_BAR			TDM_SET_PROGRESS_BAR_MARQUEE
#define TDM_SET_ELEMENT_TEXT					TDM_UPDATE_ELEMENT_TEXT

//
// Notifications
//
#define TDN_DIALOG_CONSTRUCTED					(TDN_FIRST -  1)
#define TDN_CREATED								(TDN_FIRST -  2)
#define TDN_DESTROYED							(TDN_FIRST -  3)

#define TDN_BUTTON_CLICKED						(TDN_FIRST -  4)
#define TDN_RADIO_BUTTON_CLICKED				(TDN_FIRST -  5)
#define TDN_VERIFICATION_CLICKED				(TDN_FIRST -  6)
#define TDN_HYPERLINK_CLICKED					(TDN_FIRST -  7)
#define TDN_EXPANDO_BUTTON_CLICKED				(TDN_FIRST -  8)

#define TDN_HELP								(TDN_FIRST -  9)
#define TDN_TIMER								(TDN_FIRST - 10)
#define TDN_NAVIGATED							(TDN_FIRST - 11)

#endif // ifndef TD_WARNING_ICON

#define TaskDialog_SetProgressBarState(hWndTD, state)	\
	(void)SNDMSG((hWndTD), TDM_SET_PROGRESS_BAR_STATE, (WPARAM)state, 0L)


#define MLTDE_MAIN_INSTRUCTION		1U
#define MLTDE_CONTENT					2U
#define MLTDE_EXPANDED_INFORMATION	3U
#define MLTDE_FOOTER					4U

#define TDM_UPDATE_ELEMENT_TEXTA				(TDM_FIRST + 11)
#define TDM_UPDATE_ELEMENT_TEXTW				(TDM_FIRST + 51)

#ifdef UNICODE
#define TDM_UPDATE_ELEMENT_TEXT					TDM_UPDATE_ELEMENT_TEXTW
#else
#define TDM_UPDATE_ELEMENT_TEXT					TDM_UPDATE_ELEMENT_TEXTA
#endif

#define TaskDialog_UpdateElementText(hWndTD, tde, text)	\
	(void)SNDMSG((hWndTD), TDM_UPDATE_ELEMENT_TEXT, (WPARAM)(TASKDIALOG_ELEMENTS)(tde), (LPARAM)(LPCTSTR)(text))


#define TDIE_ICON_MAIN				1U
#define TDIE_ICON_FOOTER			2U

#define TDM_UPDATE_ICON							(TDM_FIRST + 12)

#define TaskDialog_UpdateIcon(hWndTD, tdie, res)	\
	(void)SNDMSG((hWndTD), TDM_UPDATE_ICON, (WPARAM)(MLTASKDIALOG_ICON_ELEMENTS)(tdie), (LPARAM)res)

#define TDM_NAVIGATEA							(TDM_FIRST + 13)
#define TDM_NAVIGATEW							(TDM_FIRST + 53)

#ifdef UNICODE
#define TDM_NAVIGATE							TDM_NAVIGATEW
#else
#define TDM_NAVIGATE							TDM_NAVIGATEA
#endif

#define TaskDialog_Navigate(hWndTD, task_dialog)	\
	(void)SNDMSG((hWndTD), TDM_NAVIGATE, (WPARAM)0, (LPARAM)(const TASKDIALOGCONFIG*)(task_dialog))

#define MLTD_ERROR_ICON		(IDI_ERROR)
#define MLTD_WARNING_ICON		(IDI_WARNING)
#define MLTD_INFORMATION_ICON	(IDI_INFORMATION)

//
// Callback
//
typedef HRESULT (WINAPI* PFTASKDIALOGCALLBACK)(HWND handle,
                           UINT notification,
                           WPARAM wParam,
                           LPARAM lParam,
                           LONG_PTR data);

//
// Structures
//
typedef struct tagTASKDIALOG_BUTTONW {
    int nButtonID;
    PCWSTR pszButtonText;
} TASKDIALOG_BUTTONW;

typedef struct tagTASKDIALOG_BUTTONA {
    int nButtonID;
    PCSTR pszButtonText;
} TASKDIALOG_BUTTONA;

#ifdef UNICODE
#define TASKDIALOG_BUTTON	TASKDIALOG_BUTTONW
#else
#define TASKDIALOG_BUTTON	TASKDIALOG_BUTTONA
#endif

typedef struct tagTASKDIALOGCONFIGW
{
	UINT cbSize;
	HWND hwndParent;
	HINSTANCE hInstance;

	MLTASKDIALOG_FLAGS dwFlags;
	MLTASKDIALOG_COMMON_BUTTON_FLAGS dwCommonButtons;

	PCWSTR pszWindowTitle;

	union
	{
		HICON hMainIcon;
		PCWSTR pszMainIcon;
	} DUMMYUNIONNAME;

	PCWSTR pszMainInstruction;
	PCWSTR pszContent;

	UINT cButtons;
	const TASKDIALOG_BUTTONW* pButtons;
	int iDefaultButton;

	UINT cRadioButtons;
	const TASKDIALOG_BUTTONW* pRadioButtons;
	int nDefaultRadioButton;

	PCWSTR pszVerificationText;
	PCWSTR pszExpandedInformation;
	PCWSTR pszExpandedControlText;
	PCWSTR pszCollapsedControlText;

	union
	{
		HICON hFooterIcon;
		PCWSTR pszFooterIcon;
	} DUMMYUNIONNAME2;

	PCWSTR pszFooter;

	PFTASKDIALOGCALLBACK pfCallback;
	LONG_PTR lpCallbackData;

	UINT cxWidth;

} TASKDIALOGCONFIGW;

typedef struct tagTASKDIALOGCONFIGA
{
	UINT cbSize;
	HWND hwndParent;
	HINSTANCE hInstance;

	MLTASKDIALOG_FLAGS dwFlags;
	MLTASKDIALOG_COMMON_BUTTON_FLAGS dwCommonButtons;

	PCSTR pszWindowTitle;

	union
	{
		HICON hMainIcon;
		PCSTR pszMainIcon;
	} DUMMYUNIONNAME;

	PCSTR pszMainInstruction;
	PCSTR pszContent;

	UINT cButtons;
	const TASKDIALOG_BUTTONA* pButtons;
	int iDefaultButton;

	UINT cRadioButtons;
	const TASKDIALOG_BUTTONA* pRadioButtons;
	int nDefaultRadioButton;

	PCSTR pszVerificationText;
	PCSTR pszExpandedInformation;
	PCSTR pszExpandedControlText;
	PCSTR pszCollapsedControlText;

	union
	{
		HICON hFooterIcon;
		PCSTR pszFooterIcon;
	} DUMMYUNIONNAME2;

	PCSTR pszFooter;

	PFTASKDIALOGCALLBACK pfCallback;
	LONG_PTR lpCallbackData;

	UINT cxWidth;

} TASKDIALOGCONFIGA;

#ifdef UNICODE
#define TASKDIALOGCONFIG	TASKDIALOGCONFIGW
#else
#define TASKDIALOGCONFIG	TASKDIALOGCONFIGA
#endif

//
// Functions
//

WINCOMMCTRLAPI HRESULT WINAPI AltTaskDialogIndirectW(
	const TASKDIALOGCONFIGW* config,
	int* button,
	int* radioButton,
	BOOL* verificationChecked);

WINCOMMCTRLAPI HRESULT WINAPI AltTaskDialogIndirectA(
	const TASKDIALOGCONFIGA* config,
	int* button,
	int* radioButton,
	BOOL* verificationChecked);

#ifdef UNICODE
#define AltTaskDialogIndirect AltTaskDialogIndirectW
#else
#define AltTaskDialogIndirect AltTaskDialogIndirectA
#endif

WINCOMMCTRLAPI HRESULT WINAPI AltTaskDialogW(
	HWND hWndParent,
	HINSTANCE hInstance,
	PCWSTR pszWindowTitle,
	PCWSTR pszMainInstruction,
	PCWSTR pszContent,
	MLTASKDIALOG_COMMON_BUTTON_FLAGS dwCommonButtons,
	PCWSTR pszIcon,
	int *pnButton
);

WINCOMMCTRLAPI HRESULT WINAPI AltTaskDialogA(
	HWND hWndParent,
	HINSTANCE hInstance,
	PCSTR pszWindowTitle,
	PCSTR pszMainInstruction,
	PCSTR pszContent,
	MLTASKDIALOG_COMMON_BUTTON_FLAGS dwCommonButtons,
	PCSTR pszIcon,
	int *pnButton
);

#ifdef UNICODE
#define AltTaskDialog AltTaskDialogW
#else
#define AltTaskDialog AltTaskDialogA
#endif

#ifdef __cplusplus
}
#endif

#endif /* _INC_COMMCTRL_TASKDIALOGS */

