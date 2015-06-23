// SaveDffd.cpp : implements dialog that prompts for a modified template to be saved
//
// Copyright (c) 2015 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#include "stdafx.h"
#include "hexedit.h"
#include "SaveDffd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSaveDffd dialog


CSaveDffd::CSaveDffd(CWnd* pParent /*=NULL*/)
	: CDialog(CSaveDffd::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSaveDffd)
	message_ = _T("");
	//}}AFX_DATA_INIT
}


void CSaveDffd::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSaveDffd)
	DDX_Text(pDX, IDC_MESSAGE, message_);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSaveDffd, CDialog)
	//{{AFX_MSG_MAP(CSaveDffd)
	ON_BN_CLICKED(IDC_SAVEAS, OnSaveAs)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSaveDffd message handlers

void CSaveDffd::OnSaveAs()
{
	CDialog::EndDialog(IDYES);
}
