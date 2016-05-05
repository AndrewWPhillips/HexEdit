// GuiMisc.cpp : miscellaneous user-interface routines
//
// Copyright (c) 2016 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#include <StdAfx.h>
#include <Windows.h>
#include <Commctrl.h>
#include "GuiMisc.h"

int AvoidableTaskDialog(int id, LPCTSTR content/*=0*/, LPCTSTR expanded/*=0*/, LPCTSTR title/*=0*/,
                        TASKDIALOG_COMMON_BUTTON_FLAGS buttons/*=0*/, LPCTSTR icon/*=0*/)
{
	CString strId;
	strId.Format("%d", id);

	int retval;
	if ((retval = theApp.GetProfileInt("DialogDontAskValues", strId, 0)) > 0)
		return retval;

#undef TASKDIALOGCONFIG                      // qqq remove later
	CStringW strTitle, strContent, strExpanded;  // Used to create Unicode strings (must not be destroyed before call to TaskDialogIndirect())
	TASKDIALOGCONFIG config = { 0 };

	config.cbSize = sizeof(config);
	config.hwndParent = AfxGetMainWnd()->m_hWnd;
	config.hInstance = ::GetModuleHandle(NULL);
	//config.dwFlags = TDF_SIZE_TO_CONTENT;
	config.dwCommonButtons = buttons;
	if (title != NULL)
	{
		strTitle = title;
		config.pszWindowTitle = strTitle;
	}
	if (icon != NULL)
		config.pszMainIcon = (PCWSTR)icon;
	else
		config.pszMainIcon = TD_WARNING_ICON;
	config.pszMainInstruction = MAKEINTRESOURCEW(id);
	if (content != NULL)
	{
		strContent = content;
		config.pszContent = strContent;
	}
	if (buttons == 0 || buttons == TDCBF_OK_BUTTON)
		config.pszVerificationText = L"Don't show this message again.";  // It's just informational if only showing the OK button
	else
		config.pszVerificationText = L"Don't ask this question again.";  // Multiple options implies a question

	if (expanded != NULL)
	{
		strExpanded = expanded;
		config.pszExpandedInformation = strExpanded;
		config.pszExpandedControlText = L"Less information";
		config.pszCollapsedControlText = L"More information";
	}
/*
	config.pszFooter = L"This is the footer";

	const TASKDIALOG_BUTTON buttons[] = {
		{ IDOK, L"That's OK" }
	};
	config.pButtons = buttons;
	config.cButtons = ARRAYSIZE(buttons);
*/

	int button = 0;
	BOOL dont_ask = FALSE;
	if (TaskDialogIndirect(&config, &button, NULL, &dont_ask) == S_OK)
	{
		retval = button;
		if (dont_ask)
			theApp.WriteProfileInt("DialogDontAskValues", strId, retval);
	}
	return retval;
}

// Check if a lengthy operation should be aborted.
// Updates display and checks for user pressing Escape key/space bar.
bool AbortKeyPress()
{
	MSG msg;

	while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		// Handle paint, close, etc events but nothing that does anything (like WM_COMMAND)
		if (msg.message < WM_KEYFIRST)
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
		else if (msg.message == WM_KEYDOWN && (msg.wParam == VK_ESCAPE || msg.wParam == VK_SPACE))
			return true;
	}

	return false;
}

// OPen web page given string resource containing the URL
void BrowseWeb(UINT id)
{
	CString str;
	VERIFY(str.LoadString(id));

	::ShellExecute(AfxGetMainWnd()->m_hWnd, _T("open"), str, NULL, NULL, SW_SHOWNORMAL);
}

// When a menu item is selected we only get an id which is usually a command ID
// but for the menu created with make_var_menu_tree the id is only a unique number.
// What we really want is the menu text which contains a variable name.
// Given a menu ptr and an id the menu is searched and the menu item text for
// that id is returned or an empty string if it is not found.
CString get_menu_text(CMenu *pmenu, int id)
{
	CString retval;

	// Check menu items first
	if (pmenu->GetMenuString(id, retval, MF_BYCOMMAND) > 0)
		return retval;

	// Check ancestor menus
	int item_count = pmenu->GetMenuItemCount();
	for (int ii = 0; ii < item_count; ++ii)
	{
		CMenu *psub = pmenu->GetSubMenu(ii);
		if (psub != NULL && psub->GetMenuString(id, retval, MF_BYCOMMAND) > 0)
			return retval;
	}

	return CString("");
}

// Put a string on the Windows Clipboard
void StringToClipboard(const char * str)
{
	if (!::OpenClipboard(AfxGetMainWnd()->m_hWnd))
	{
		TRACE("Could not open clipboard\n");
		return;
	}

	if (!::EmptyClipboard())
	{
		TRACE("Could not empty clipboard\n");
		::CloseClipboard();
		return;
	}

	size_t len = strlen(str);
	ASSERT(str != NULL && len < 10000);

	HANDLE hh = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, strlen(str) + 1);
	if (hh == NULL)
	{
		TRACE("GlobalAlloc failed\n");
		::CloseClipboard();
		return;
	}

	unsigned char * pp = reinterpret_cast<unsigned char *>(::GlobalLock(hh));
	if (pp == NULL)
	{
		TRACE("GlobalLock failed\n");
		::GlobalFree(hh);
		::CloseClipboard();
		return;
	}

	memcpy(pp, str, len + 1);

	if (::SetClipboardData(CF_TEXT, hh) == NULL)
	{
		TRACE("SetClipboardData failed\n");
		::GlobalFree(hh);
	}
	else
		::GlobalUnlock(hh);   // release (but don't free) memory now ownded by clipboard

	::CloseClipboard();
}

//-----------------------------------------------------------------------------
// Multiple monitor handling

// Gets rect of the monitor which contains the mouse
CRect MonitorMouse()
{
	CPoint pt;
	GetCursorPos(&pt);

	CRect rect(pt.x, pt.y, pt.x + 1, pt.y + 1);
	return MonitorRect(rect);
}

// Gets rect of monitor which contains most of rect.
// In non-multimon environment it just returns the rect of the
// screen work area (excludes "always on top" docked windows).
CRect MonitorRect(CRect rect)
{
	CRect cont_rect;

	if (theApp.mult_monitor_)
	{
		// Use rect of containing monitor as "container"
		HMONITOR hh = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi;
		mi.cbSize = sizeof(mi);
		if (hh != 0 && GetMonitorInfo(hh, &mi))
			cont_rect = mi.rcWork;  // work area of nearest monitor
		else
		{
			// Shouldn't happen but if it does use the whole virtual screen
			ASSERT(0);
			cont_rect = CRect(::GetSystemMetrics(SM_XVIRTUALSCREEN),
				::GetSystemMetrics(SM_YVIRTUALSCREEN),
				::GetSystemMetrics(SM_XVIRTUALSCREEN) + ::GetSystemMetrics(SM_CXVIRTUALSCREEN),
				::GetSystemMetrics(SM_YVIRTUALSCREEN) + ::GetSystemMetrics(SM_CYVIRTUALSCREEN));
		}
	}
	else if (!::SystemParametersInfo(SPI_GETWORKAREA, 0, &cont_rect, 0))
	{
		// I don't know if this will ever happen since the Windows documentation
		// is pathetic and does not say when or why SystemParametersInfo might fail.
		cont_rect = CRect(0, 0, ::GetSystemMetrics(SM_CXFULLSCREEN),
			::GetSystemMetrics(SM_CYFULLSCREEN));
	}

	return cont_rect;
}

// Returns true if rect is wholly or partially off the screen.
// In multimon environment it also returns true if rect extends over more than one monitor.
bool OutsideMonitor(CRect rect)
{
	CRect cont_rect = MonitorRect(rect);

	return rect.left   < cont_rect.left ||
		rect.right  > cont_rect.right ||
		rect.top    < cont_rect.top ||
		rect.bottom > cont_rect.bottom;
}

// Check if most of window is off all monitors.  If so it returns true and
// adjusts the parameter (rect) so the rect is fully within the closest monitor.
bool NeedsFix(CRect &rect)
{
	CRect cont_rect = MonitorRect(rect);
	CRect small_rect(rect);
	small_rect.DeflateRect(rect.Width() / 4, rect.Height() / 4);
	CRect tmp;
	if (!tmp.IntersectRect(cont_rect, small_rect))
	{
		// Fix the rect
#if _MSC_VER >= 1300
		if (rect.right > cont_rect.right)
			rect.MoveToX(rect.left - (rect.right - cont_rect.right));
		if (rect.bottom > cont_rect.bottom)
			rect.MoveToY(rect.top - (rect.bottom - cont_rect.bottom));
		if (rect.left < cont_rect.left)
			rect.MoveToX(cont_rect.left);
		if (rect.top < cont_rect.top)
			rect.MoveToY(cont_rect.top);
#endif
		return true;
	}
	else
		return false;  // its OK
}
