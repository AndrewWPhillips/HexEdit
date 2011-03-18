#ifndef _INC_AFXTASK
#define _INC_AFXTASK

// This is taken from M. Labelle's partial task dialog implementation - see http://www.codeproject.com/KB/dialog/taskdialogs.aspx
// It requires an external library (commctrl_taskdialogs.h, commctrl_taskdialogs.lib, commctrl_taskdialogs.dll).

#include <MLtaskdialogs.h>

class CSimpleTaskDialog : public CWnd
{
// construction / destruction
public:

	DECLARE_DYNAMIC(CSimpleTaskDialog)

	CSimpleTaskDialog(LPCTSTR instruction = _T(""), LPCTSTR content = _T(""), LPCTSTR title = NULL, MLTASKDIALOG_COMMON_BUTTON_FLAGS buttons = 0, LPCTSTR icon = 0);
	virtual ~CSimpleTaskDialog();

// attributes
public:

	virtual int GetButton(void) const;

	virtual void SetWindowTitle(LPCTSTR title);
	virtual void SetMainInstruction(LPCTSTR instruction);
	virtual void SetContent(LPCTSTR content);
	virtual void SetIcon(LPCTSTR icon);

// operations
public:

	virtual INT_PTR DoModal(CWnd* pParentWnd = CWnd::GetActiveWindow());

// data members
protected:

	TASKDIALOGCONFIG m_config;
	int m_button;

};

class CTaskDialog : public CSimpleTaskDialog
{
// construction / destruction
public:

	DECLARE_DYNAMIC(CTaskDialog)

	CTaskDialog(LPCTSTR instruction = _T(""), LPCTSTR content = _T(""), LPCTSTR title = NULL, MLTASKDIALOG_COMMON_BUTTON_FLAGS buttons = 0, LPCTSTR icon = 0);

// attributes
public:

	int GetRadioButton(void) const;
	BOOL IsVerificationChecked(void) const;

	void SetFlags(MLTASKDIALOG_FLAGS dwFlags, MLTASKDIALOG_FLAGS dwExMax);

	void SetCommonButtons(MLTASKDIALOG_COMMON_BUTTON_FLAGS buttons);

	void SetWindowTitle(LPCTSTR title);
	void SetMainInstruction(LPCTSTR instruction);
	void SetContent(LPCTSTR content);

	void SetIcon(LPCTSTR icon);
	void SetIcon(HICON hIcon);

	void AddButton(UINT nButtonID, LPCTSTR pszButtonText = 0);
	void AddRadioButton(UINT nButtonID, LPCTSTR pszButtonText = 0);

	void SetDefaultButton(UINT uID);
	void SetDefaultRadioButton(UINT uID);

	void SetVerificationText(LPCTSTR verification, BOOL bChecked = FALSE);

	void SetFooterIcon(HICON icon);
	void SetFooterIcon(LPCTSTR pszIcon);

	void SetFooter(LPCTSTR content);

// operations
public:

	INT_PTR DoModal(CWnd* pParentWnd = CWnd::GetActiveWindow());

	void ClickButton(UINT uID);
	void ClickRadioButton(UINT uID);
	void ClickVerification(BOOL bState, BOOL bFocus = FALSE);

	void EnableButton(UINT uID, BOOL bEnabled = TRUE);
	void EnableRadioButton(UINT uID, BOOL bEnabled = TRUE);

	void SetProgressBarMarquee(BOOL bMarquee, UINT nSpeed);
	void SetProgressBarPosition(UINT nPos);
	void SetProgressBarRange(UINT nMinRange, UINT nMaxRange);
	void SetProgressBarState(UINT nState);

	BOOL UpdateElementText(MLTASKDIALOG_ELEMENTS te, LPCTSTR string);

	void UpdateIcon(MLTASKDIALOG_ICON_ELEMENTS tie, LPCTSTR icon);
	void UpdateIcon(MLTASKDIALOG_ICON_ELEMENTS tie, HICON hIcon);

	void Navigate(const TASKDIALOGCONFIG* task_dialog);
	void Navigate(const CTaskDialog& task_dialog);

// overrides
public:

	virtual void OnDialogConstructed(void);
	virtual void OnCreated(void);
	virtual void OnDestroyed(void);
	virtual void OnRadioButtonClicked(UINT uID);
	virtual BOOL OnButtonClicked(UINT uID);
	virtual void OnVerificationClicked(BOOL bChecked);
	virtual void OnHyperLinkClicked(LPCWSTR wszHREF);
	virtual void OnHelp(void);
	virtual BOOL OnTimer(DWORD dwTickCount);
	virtual void OnNavigated(void);

// implementation
private:

	static HRESULT __stdcall TaskDialogCallbackProc(HWND hWnd, UINT uCode, WPARAM wParam, LPARAM lParam, LONG_PTR data);

// data members
private:

	int m_radiobutton;
	BOOL m_verification;

	CArray<TASKDIALOG_BUTTON> m_buttons;
	CArray<TASKDIALOG_BUTTON> m_radioButtons;

};

// Replacement for AfxMessageBox that uses CTaskDialog
// Parameters:
//   mess = short message at top
//   content = longer description (may be empty)
//   title = window title (defaults to "HexEdit" if empty)
//   buttons = combination of MLCBF_*_BUTTON where * = OK/CANCEL/YES/NO/CLOSE/RETRY
//     for equivalents to AfxMessageBox flags use:
//       MB_OK = MLCBF_OK_BUTTON
//       MB_OKCANCEL = MLCBF_OK_BUTTON | MLCBF_CANCEL_BUTTON
//       MB_YESNO = MLCBF_YES_BUTTON | MLCBF_NO_BUTTON
//       MB_YESNOCANCEL = MLCBF_YES_BUTTON | MLCBF_NO_BUTTON | MLCBF_CANCEL_BUTTON
//       etc
//   icon = ID of icon - if 0 is exclamation mark (for only OK button shown) or question mark
//  TBD: implement a "help ID" parameter
inline int TaskMessageBox(LPCTSTR mess, LPCTSTR content = _T(""), LPCTSTR title = _T(""), MLTASKDIALOG_COMMON_BUTTON_FLAGS buttons = 0, LPCTSTR icon = 0)
{
	CTaskDialog dlg(mess, content, title, buttons, icon);
	return dlg.DoModal(AfxGetMainWnd());
}

//////////////////////////////////////////////////////////////////////////////
// CSimpleTaskDialog attributes

inline void CSimpleTaskDialog::SetWindowTitle(LPCTSTR title)
	{ m_config.pszWindowTitle = title; }
inline void CSimpleTaskDialog::SetMainInstruction(LPCTSTR instruction)
	{ m_config.pszMainInstruction = instruction; }
inline void CSimpleTaskDialog::SetContent(LPCTSTR content)
	{ m_config.pszContent = content; }
inline void CSimpleTaskDialog::SetIcon(LPCTSTR icon)
	{
		m_config.dwFlags &= ~TDF_USE_HICON_MAIN;
		m_config.pszMainIcon = icon;
	}
inline int CSimpleTaskDialog::GetButton(void) const
	{ return m_button; }

//////////////////////////////////////////////////////////////////////////////
// CTaskDialog attributes

inline int CTaskDialog::GetRadioButton(void) const
	{ return m_radiobutton; }
inline BOOL CTaskDialog::IsVerificationChecked(void) const
	{ return m_verification; }

inline void CTaskDialog::SetFlags(MLTASKDIALOG_FLAGS dwFlags, MLTASKDIALOG_FLAGS dwExMax = 0)
	{
		ATLASSERT(!::IsWindow(GetSafeHwnd()));
		m_config.dwFlags &= ~dwExMax;
		m_config.dwFlags |= dwFlags;
	}
inline void CTaskDialog::SetCommonButtons(MLTASKDIALOG_COMMON_BUTTON_FLAGS buttons)
	{
		ASSERT(!::IsWindow(GetSafeHwnd()));
		m_config.dwCommonButtons = buttons;
	}
inline void CTaskDialog::SetDefaultButton(UINT uID)
	{
		ASSERT(!::IsWindow(GetSafeHwnd()));
		m_config.iDefaultButton = uID;
	}
inline void CTaskDialog::SetDefaultRadioButton(UINT nButtonID)
	{ m_config.nDefaultRadioButton = nButtonID; }


//////////////////////////////////////////////////////////////////////////////
// CTaskDialog operations

inline void CTaskDialog::ClickButton(UINT uID)
{
	ASSERT(::IsWindow(GetSafeHwnd()));
	SendMessage(TDM_CLICK_BUTTON, (WPARAM) uID);
}

inline void CTaskDialog::ClickRadioButton(UINT uID)
{
	ASSERT(::IsWindow(GetSafeHwnd()));
	SendMessage(TDM_CLICK_RADIO_BUTTON, (WPARAM) uID);
}

inline void CTaskDialog::ClickVerification(BOOL bState, BOOL bFocus)
{
	ASSERT(::IsWindow(GetSafeHwnd()));
	SendMessage(TDM_CLICK_VERIFICATION, (WPARAM) bState, (LPARAM) bFocus);
}

inline void CTaskDialog::EnableButton(UINT uID, BOOL bEnabled)
{
	ASSERT(::IsWindow(GetSafeHwnd()));
	SendMessage(TDM_ENABLE_BUTTON, (WPARAM) uID, (LPARAM) bEnabled);
}

inline void CTaskDialog::EnableRadioButton(UINT uID, BOOL bEnabled)
{
	ASSERT(::IsWindow(GetSafeHwnd()));
	SendMessage(TDM_ENABLE_RADIO_BUTTON, (WPARAM) uID, (LPARAM) bEnabled);
}

inline void CTaskDialog::SetProgressBarMarquee(BOOL bMarquee, UINT nSpeed)
{
	ASSERT(::IsWindow(GetSafeHwnd()));
	SendMessage(TDM_SET_PROGRESS_BAR_MARQUEE, (WPARAM) bMarquee, (LPARAM) nSpeed);
}

inline void CTaskDialog::SetProgressBarPosition(UINT nPos)
{
	ASSERT(::IsWindow(GetSafeHwnd()));
	SendMessage(TDM_SET_PROGRESS_BAR_POS, (WPARAM) nPos);
}

inline void CTaskDialog::SetProgressBarRange(UINT nMinRange, UINT nMaxRange)
{
	ASSERT(::IsWindow(GetSafeHwnd()));
	SendMessage(TDM_SET_PROGRESS_BAR_RANGE, (WPARAM) 0, MAKELPARAM(nMinRange, nMaxRange));
}

inline void CTaskDialog::SetProgressBarState(UINT nState)
{
	ASSERT(::IsWindow(GetSafeHwnd()));
	SendMessage(TDM_SET_PROGRESS_BAR_STATE, (WPARAM) nState);
}

inline BOOL CTaskDialog::UpdateElementText(MLTASKDIALOG_ELEMENTS te, LPCTSTR string)
{
	ASSERT(::IsWindow(GetSafeHwnd()));
	return (BOOL) SendMessage(TDM_UPDATE_ELEMENT_TEXT, (WPARAM) te, (LPARAM) string);
}

inline void CTaskDialog::UpdateIcon(MLTASKDIALOG_ICON_ELEMENTS tie, LPCTSTR icon)
{
	ASSERT(::IsWindow(GetSafeHwnd()));
	ASSERT(
		((tie == TDIE_ICON_MAIN)   && (m_config.dwFlags & TDF_USE_HICON_MAIN) == 0) ||
		((tie == TDIE_ICON_FOOTER) && (m_config.dwFlags & TDF_USE_HICON_FOOTER) == 0)
		);
	SendMessage(TDM_UPDATE_ICON, (WPARAM) tie, (LPARAM) icon);
}

inline void CTaskDialog::UpdateIcon(MLTASKDIALOG_ICON_ELEMENTS tie, HICON hIcon)
{
	ASSERT(::IsWindow(GetSafeHwnd()));
	ASSERT(
		((tie == TDIE_ICON_MAIN)   && (m_config.dwFlags & TDF_USE_HICON_MAIN) == TDF_USE_HICON_MAIN) ||
		((tie == TDIE_ICON_FOOTER) && (m_config.dwFlags & TDF_USE_HICON_FOOTER) == TDF_USE_HICON_FOOTER)
		);
	SendMessage(TDM_UPDATE_ICON, (WPARAM) tie, (LPARAM) hIcon);
}

inline void CTaskDialog::Navigate(const TASKDIALOGCONFIG* task_dialog)
{
	// we remove the association between this object and its window handle in the permanent map
	// because the task dialog being navigated to will receive a TDN_DIALOG_CONSTRUCTED notification
	// and will attach to the same window handle
	ASSERT(::IsWindow(GetSafeHwnd()));
	::SendMessage(Detach(), TDM_NAVIGATE, (WPARAM) 0, (LPARAM) task_dialog);
}

#endif // _INC_AFXTASK


