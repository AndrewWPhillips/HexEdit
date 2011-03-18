#include "StdAfx.h"
#include "TaskDialog.h"

// This is taken from M. Labelle's partial task dialog implementation - see http://www.codeproject.com/KB/dialog/taskdialogs.aspx

//////////////////////////////////////////////////////////////////////////////
// CSimpleTaskDialog construction / destruction

IMPLEMENT_DYNAMIC(CSimpleTaskDialog, CWnd)

CSimpleTaskDialog::CSimpleTaskDialog(LPCTSTR instruction, LPCTSTR content, LPCTSTR title, MLTASKDIALOG_COMMON_BUTTON_FLAGS buttons, LPCTSTR icon)
{
	::ZeroMemory(&m_config, sizeof(TASKDIALOGCONFIG));
	m_config.cbSize = sizeof(TASKDIALOGCONFIG);
	m_config.hInstance = ::AfxGetResourceHandle();
	m_config.dwFlags = TDF_POSITION_RELATIVE_TO_WINDOW;
	m_config.dwCommonButtons = buttons == 0 ? MLCBF_OK_BUTTON : buttons;

	if (title == NULL || title[0] == 0)
		SetWindowTitle(_T("HexEdit"));
	else
		SetWindowTitle(title);
	SetMainInstruction(instruction);
	SetContent(content);

	SetIcon(icon);
}

CSimpleTaskDialog::~CSimpleTaskDialog()
{
}

//////////////////////////////////////////////////////////////////////////////
// CSimpleTaskDialog operations

INT_PTR CSimpleTaskDialog::DoModal(CWnd* pParentWnd)
{
	m_config.hwndParent = (pParentWnd ? pParentWnd->GetSafeHwnd() : 0);
	if (SUCCEEDED(::AltTaskDialog(
					  m_config.hwndParent
					, m_config.hInstance
					, m_config.pszWindowTitle
					, m_config.pszMainInstruction
					, m_config.pszContent
					, m_config.dwCommonButtons
					, m_config.pszMainIcon
					, &m_button)))
		return (INT_PTR) m_button;
	return 0;
}

//////////////////////////////////////////////////////////////////////////////
// CTaskDialog construction / destruction

IMPLEMENT_DYNAMIC(CTaskDialog, CSimpleTaskDialog)

CTaskDialog::CTaskDialog(LPCTSTR instruction, LPCTSTR content, LPCTSTR title, MLTASKDIALOG_COMMON_BUTTON_FLAGS buttons, LPCTSTR icon)
	: CSimpleTaskDialog(instruction, content, title, buttons, icon)
	, m_radiobutton(0)
	, m_verification(FALSE)
{
	m_config.pfCallback = TaskDialogCallbackProc;
	m_config.lpCallbackData = reinterpret_cast<LPARAM>(this);
}

void CTaskDialog::SetWindowTitle(LPCTSTR title)
{
	if (!::IsWindow(GetSafeHwnd())) {
		CSimpleTaskDialog::SetWindowTitle(title);
		return ;
	}

	if (IS_INTRESOURCE(title)) {
		TCHAR szWindowTitle[1024] = { _T('\0') };

#pragma warning (push)
#pragma warning (disable: 4311) // truncation warning

		if (::LoadStringW(::AfxGetResourceHandle(), reinterpret_cast<UINT>(title), (LPWSTR)(LPCWSTR)CStringW(szWindowTitle), sizeof(szWindowTitle)/sizeof(TCHAR) - sizeof(TCHAR)))
			::SetWindowText(GetSafeHwnd(), szWindowTitle);

#pragma warning (pop)

	}

	else
		::SetWindowText(GetSafeHwnd(), title);
}

void CTaskDialog::SetIcon(LPCTSTR icon)
{
	if (!::IsWindow(GetSafeHwnd())) {
		CSimpleTaskDialog::SetIcon(icon);
		return ;
	}

	UpdateIcon(TDIE_ICON_MAIN, icon);
}

void CTaskDialog::SetIcon(HICON hIcon)
{
	if (!::IsWindow(GetSafeHwnd())) {
		m_config.dwFlags |= TDF_USE_HICON_MAIN;
		m_config.hMainIcon = hIcon;
		return ;
	}

	UpdateIcon(TDIE_ICON_MAIN, hIcon);
}

void CTaskDialog::SetMainInstruction(LPCTSTR instruction)
{
	if (!::IsWindow(GetSafeHwnd())) {
		CSimpleTaskDialog::SetMainInstruction(instruction);
		return ;
	}

	UpdateElementText(MLTDE_MAIN_INSTRUCTION, instruction);
}

void CTaskDialog::SetContent(LPCTSTR content)
{
	if (!::IsWindow(GetSafeHwnd())) {
		CSimpleTaskDialog::SetContent(content);
		return ;
	}

	UpdateElementText(MLTDE_CONTENT, content);
}

void CTaskDialog::AddButton(UINT nButtonID, LPCTSTR pszButtonText)
{
	TASKDIALOG_BUTTON button = { nButtonID, pszButtonText ? pszButtonText : MAKEINTRESOURCE(nButtonID) };
	m_buttons.Add(button);

	m_config.pButtons = m_buttons.GetData();
	m_config.cButtons = static_cast<UINT>(m_buttons.GetSize());
}

void CTaskDialog::AddRadioButton(UINT nButtonID, LPCTSTR pszButtonText)
{
	TASKDIALOG_BUTTON button = { nButtonID, pszButtonText ? pszButtonText : MAKEINTRESOURCE(nButtonID) };
	m_radioButtons.Add(button);

	m_config.pRadioButtons = m_radioButtons.GetData();
	m_config.cRadioButtons = static_cast<UINT>(m_radioButtons.GetSize());
}

void CTaskDialog::SetVerificationText(LPCTSTR verification, BOOL bChecked)
{
	ATLASSERT(!::IsWindow(GetSafeHwnd()));
	m_config.pszVerificationText = verification;
	if (bChecked)
		m_config.dwFlags |= TDF_VERIFICATION_FLAG_CHECKED;
}

void CTaskDialog::SetFooterIcon(LPCTSTR icon)
{
	if (!::IsWindow(GetSafeHwnd())) {
		m_config.dwFlags &= ~TDF_USE_HICON_FOOTER;
		m_config.pszFooterIcon = icon;
		return ;
	}

	UpdateIcon(TDIE_ICON_FOOTER, icon);
}

void CTaskDialog::SetFooterIcon(HICON hIcon)
{
	if (!::IsWindow(GetSafeHwnd())) {
		m_config.dwFlags |= TDF_USE_HICON_FOOTER;
		m_config.hFooterIcon = hIcon;
		return ;
	}

	UpdateIcon(TDIE_ICON_FOOTER, hIcon);
}

void CTaskDialog::SetFooter(LPCTSTR footer)
{
	if (!::IsWindow(GetSafeHwnd()))
		m_config.pszFooter = footer;

	else
		UpdateElementText(MLTDE_FOOTER, footer);
}

//////////////////////////////////////////////////////////////////////////////
// CTaskDialog operations

INT_PTR CTaskDialog::DoModal(CWnd* pParentWnd)
{
	m_config.hwndParent = (pParentWnd ? pParentWnd->GetSafeHwnd() : 0);
	if (SUCCEEDED(::AltTaskDialogIndirect(&m_config, &m_button, &m_radiobutton, &m_verification)))
		return (INT_PTR) m_button;
	return 0;
}

void CTaskDialog::Navigate(const CTaskDialog& task_dialog)
{
	TASKDIALOGCONFIG task_dialog_config;
	::CopyMemory(&task_dialog_config, &task_dialog.m_config, sizeof(TASKDIALOGCONFIG));
	task_dialog_config.hwndParent = m_config.hwndParent;

	Navigate(&task_dialog_config);
}

//////////////////////////////////////////////////////////////////////////////
// CTaskDialog notification handlers overrides

void CTaskDialog::OnDialogConstructed(void)
	{ /* empty stub */ }
void CTaskDialog::OnCreated(void)
	{ /* empty stub */ }
void CTaskDialog::OnDestroyed(void)
	{ /* empty stub */ }
void CTaskDialog::OnRadioButtonClicked(UINT uID)
	{ /* empty stub */ }
BOOL CTaskDialog::OnButtonClicked(UINT uID)
	{ /* empty stub */ return /* should we close the dialog box */ FALSE; }
void CTaskDialog::OnVerificationClicked(BOOL bChecked)
	{ /* empty stub */ }
void CTaskDialog::OnHyperLinkClicked(LPCWSTR wszHREF)
	{ /* empty stub */ }
void CTaskDialog::OnHelp(void)
	{ /* empty stub */ }
BOOL CTaskDialog::OnTimer(DWORD dwTickCount)
	{ /* empty stub */ return /* should we reset the timer */ FALSE; }
void CTaskDialog::OnNavigated(void)
	{ /* empty stub */ }

//////////////////////////////////////////////////////////////////////////////
// CTaskDialog notification callback implementation

HRESULT __stdcall CTaskDialog::TaskDialogCallbackProc(HWND hWnd, UINT uCode, WPARAM wParam, LPARAM lParam, LONG_PTR data)
{
	CTaskDialog* pT = reinterpret_cast<CTaskDialog*>(data);

	ATLASSERT(::IsWindow(hWnd));
	ATLASSERT((uCode == TDN_DIALOG_CONSTRUCTED && !::IsWindow(pT->GetSafeHwnd())) || ::IsWindow(pT->GetSafeHwnd()));

	ATLTRACE(_T("TaskDialogCallbackProc(%s)\n"),
		(uCode == TDN_DIALOG_CONSTRUCTED ? _T("TDN_DIALOG_CONSTRUCTED") :
		(uCode == TDN_CREATED ? _T("TDN_CREATED") :
		(uCode == TDN_DESTROYED ? _T("TDN_DESTROYED") :
		(uCode == TDN_BUTTON_CLICKED ? _T("TDN_BUTTON_CLICKED") :
		(uCode == TDN_RADIO_BUTTON_CLICKED ? _T("TDN_RADIO_BUTTON_CLICKED") :
		(uCode == TDN_VERIFICATION_CLICKED ? _T("TDN_VERIFICATION_CLICKED") :
		(uCode == TDN_HYPERLINK_CLICKED ? _T("TDN_HYPERLINK_CLICKED") :
		(uCode == TDN_HELP ? _T("TDN_HELP") :
		(uCode == TDN_TIMER ? _T("TDN_TIMER") :
		(uCode == TDN_NAVIGATED ? _T("TDN_NAVIGATED") : _T("E_NOTIMPL")))))))))))
			);

	HRESULT hResult = S_OK;

	switch (uCode) {
		case TDN_DIALOG_CONSTRUCTED:
			pT->Attach(hWnd);
			pT->OnDialogConstructed();
			break;

		case TDN_CREATED:
			pT->OnCreated();
			break;

		case TDN_DESTROYED:
			pT->OnDestroyed();
			pT->Detach();
			break;

		case TDN_BUTTON_CLICKED:
			hResult = (HRESULT) pT->OnButtonClicked((UINT) wParam);
			break;

		case TDN_RADIO_BUTTON_CLICKED:
			pT->OnRadioButtonClicked((UINT) wParam);
			break;

		case TDN_VERIFICATION_CLICKED:
			pT->OnVerificationClicked((BOOL) wParam);
			break;

		case TDN_HYPERLINK_CLICKED:
			pT->OnHyperLinkClicked((LPCWSTR) lParam);
			break;

		case TDN_EXPANDO_BUTTON_CLICKED:
			ATLASSERT(E_NOTIMPL);
			hResult = E_NOTIMPL;
			break;

		case TDN_HELP:
			pT->OnHelp();
			break;

		case TDN_TIMER:
			hResult = (HRESULT) pT->OnTimer((DWORD) wParam);
			break;

		case TDN_NAVIGATED:
			pT->OnNavigated();
			break;
	}

	return hResult;
}
