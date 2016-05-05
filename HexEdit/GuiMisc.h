// Misc.h - various global functions
//
// Copyright (c) 2016 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#ifndef GUI_MISC_INCLUDED_
#define GUI_MISC_INCLUDED_   1

int TaskMessageBox(LPCTSTR mess, LPCTSTR content = _T(""), UINT nType = MB_OK, LPCTSTR icon = 0);
int AvoidableTaskDialog(int id, LPCTSTR content = NULL, LPCTSTR expanded = NULL, LPCTSTR title = NULL,
	                    TASKDIALOG_COMMON_BUTTON_FLAGS buttons = 0, LPCTSTR icon = 0,
						const TASKDIALOG_BUTTON * custom = NULL, int count = 0);
bool AbortKeyPress();
void BrowseWeb(UINT id);
CString get_menu_text(CMenu *pmenu, int id);
void StringToClipboard(const char * str);

// Multiple monitor
bool OutsideMonitor(CRect);
CRect MonitorMouse();
CRect MonitorRect(CRect);
bool NeedsFix(CRect &rect);

#endif // GUI_MISC_INCLUDED_
