// UserTool.cpp : subclass BCG user tools class to do command line substitutions for tools
//
// Copyright (c) 2015 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#include "stdafx.h"
#include "HexEdit.h"
#include "UserTool.h"

#include "HexEditDoc.h"
#include "HexEditView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_SERIAL(CHexEditUserTool, CUserTool, 1)

/////////////////////////////////////////////////////////////////////////////
// CHexEditUserTool

CHexEditUserTool::CHexEditUserTool()
{
}

CHexEditUserTool::~CHexEditUserTool()
{
}

BOOL CHexEditUserTool::Invoke()
{
	if (m_strCommand.IsEmpty ())
	{
		TRACE(_T("Empty command in user-defined tool: %d\n"), m_uiCmdId);
		return FALSE;
	}

	// Added by AWPhillips - translate parameters in command line args
	CString strOut, strArgs = m_strArguments;
	int pos;

	while ((pos = strArgs.Find("$(")) != -1)
	{
		strOut += strArgs.Left(pos);
		strArgs = strArgs.Mid(pos+2);
		if ((pos = strArgs.Find(")")) == -1)
		{
			strOut += "$(";
			break;
		}
		CString strParam = strArgs.Left(pos);

		if (strParam == "FilePath")
		{
			CHexEditView *pView = GetView();
			if (pView != NULL)
			{
				CHexEditDoc *pDoc = (CHexEditDoc *)(pView->GetDocument());
				if (pDoc != NULL && pDoc->pfile1_ != NULL)
					strOut += pDoc->pfile1_->GetFilePath();
			}
		}
		/* Add other subst. parameters here */

		strArgs = strArgs.Mid(pos+1);
	}
	strOut += strArgs;

	if (::ShellExecute (AfxGetMainWnd()->GetSafeHwnd (), NULL, m_strCommand,
		strOut, m_strInitialDirectory, 
		SW_SHOWNORMAL) < (HINSTANCE) 32)
	{
		TRACE(_T("Can't invoke command: %s\n"), m_strCommand);
		return FALSE;
	}

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CHexEditUserTool message handlers
