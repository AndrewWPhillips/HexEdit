#pragma once

#include "HexEdit.h"
#include "TaskDialog.h"

// This derives from CTaskDialog solely to add the ability to have an "avoidable dialog".
// If the user turns on the "Don't ask again" checkbox then subsequent use of this dialog returns 
// the same value without any user interaction and without any visible difference to the callee.
// The returns values are stored in the registry - once hidden by the user a dialogs will not be 
// seen again until the relevant regitry entry is removed.
class CAvoidableDialog : public CTaskDialog
{
public:
	CAvoidableDialog(int id, LPCTSTR content = _T(""), LPCTSTR title = _T(""), MLTASKDIALOG_COMMON_BUTTON_FLAGS buttons = 0, LPCTSTR icon = 0) :
	  CTaskDialog(MAKEINTRESOURCE(id), content, title, buttons,
		          icon != 0 ? icon : (buttons == 0 || buttons == MLCBF_OK_BUTTON ? MAKEINTRESOURCE(IDI_EXCLAMATIONMARK) : MAKEINTRESOURCE(IDI_QUESTIONMARK)))
	{
		id_.Format("%d", id);
		dont_ask_ = false;
		if (buttons == 0 || buttons == MLCBF_OK_BUTTON)
			SetVerificationText("Don't show this message again.");  // It's just informational if only showing the OK button
		else
			SetVerificationText("Don't ask this question again.");  // Multiple options implies a question
	}

	int DoModal(CWnd* pParentWnd = CWnd::GetActiveWindow())
	{
		int retval;

		if ((retval = theApp.GetProfileInt("DialogDontAskValues", id_, -1)) == -1)
		{
			retval = CTaskDialog::DoModal(pParentWnd);
			if (dont_ask_)
				theApp.WriteProfileInt("DialogDontAskValues", id_, retval);
		}

		return retval;
	}

	static int Show(int id, LPCTSTR content = _T(""), LPCTSTR title = _T(""), MLTASKDIALOG_COMMON_BUTTON_FLAGS buttons = 0, LPCTSTR icon = 0)
	{
		CAvoidableDialog dlg(id, content, title, buttons, icon);
		return dlg.DoModal(AfxGetMainWnd());
	}

private:
	CString id_;
	bool dont_ask_;

	void OnVerificationClicked(BOOL checked)
	{
		dont_ask_ = checked != 0;
	}
};
