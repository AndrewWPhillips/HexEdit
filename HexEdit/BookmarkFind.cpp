// BookmarkFind.cpp : implementation file
//
// Asks if user wants to overwrite or append to set of bookmarks with the same
// prefix when using the bookmark all option of the Find dialog.
//

#include "stdafx.h"
#include "hexedit.h"
#include "BookmarkFind.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBookmarkFind dialog


CBookmarkFind::CBookmarkFind(CWnd* pParent /*=NULL*/)
	: CDialog(CBookmarkFind::IDD, pParent)
{
	//{{AFX_DATA_INIT(CBookmarkFind)
	mess_ = _T("");
	//}}AFX_DATA_INIT
}


void CBookmarkFind::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBookmarkFind)
	DDX_Text(pDX, IDC_BM_FIND_MESS, mess_);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBookmarkFind, CDialog)
	//{{AFX_MSG_MAP(CBookmarkFind)
	ON_BN_CLICKED(IDC_BM_FIND_APPEND, OnAppend)
	ON_BN_CLICKED(IDC_BM_FIND_OVERWRITE, OnOverwrite)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBookmarkFind message handlers

BOOL CBookmarkFind::OnInitDialog() 
{
	CDialog::OnInitDialog();

	return TRUE;
}

void CBookmarkFind::OnAppend() 
{
	EndDialog(IDC_BM_FIND_APPEND);
}

void CBookmarkFind::OnOverwrite() 
{
	EndDialog(IDC_BM_FIND_OVERWRITE);
}
