// UserTool.cpp : subclass BCG user tools class to do command line substitutions
//
// Copyright (c) 2000-2010 by Andrew W. Phillips.
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
