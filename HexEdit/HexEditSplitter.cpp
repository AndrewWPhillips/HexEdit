// HexEditSplitter.cpp : implementation file
//

#include "stdafx.h"
#include "hexedit.h"
#include "HexEditSplitter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHexEditSplitter

IMPLEMENT_DYNAMIC(CHexEditSplitter, CSplitterWnd)

BEGIN_MESSAGE_MAP(CHexEditSplitter, CSplitterWnd)
	//{{AFX_MSG_MAP(CHexEditSplitter)
	//}}AFX_MSG_MAP
//	ON_WM_SIZE()
END_MESSAGE_MAP()

//void CHexEditSplitter::OnSize(UINT nType, int cx, int cy)
//{
//    CSplitterWnd::OnSize(nType, cx, cy);
//}

BOOL CHexEditSplitter::InsColumn(int new_col, int width, int min_width, CRuntimeClass *pViewClass /*=NULL*/,
								 CCreateContext* pContext /*=NULL*/)
{
	// Make sure insert pos is a valid column and we don't already have max columns
    if (new_col < 0 || new_col > m_nCols || m_nCols >= m_nMaxCols)
        return FALSE;

	int col, row;

    ++m_nCols;
    for (col = m_nCols - 2; col >= new_col; col--)
	{
        for (row = 0; row < m_nRows; ++row)
			if (GetDlgItem(IdFromRowCol(row, col)) != NULL)
                GetPane(row, col)->SetDlgCtrlID(IdFromRowCol(row, col+1));
		m_pColInfo[col+1] = m_pColInfo[col];
	}

	if (pViewClass != NULL)
        for (row = 0; row < m_nRows; ++row)
        {
            int cur, min;
            this->GetRowInfo(row, cur, min);
			VERIFY(CreateView(row, new_col, pViewClass, CSize(width, cur), pContext));
		}
	//SetColumnInfo(new_col, width, min_width);   // this does not seem to do anything useful

	ASSERT(m_nCols <= m_nMaxCols);

    return TRUE;
}

BOOL CHexEditSplitter::DelColumn(int del_col, BOOL del_views /*=FALSE*/)
{
	// Make sure it is a valid column and we have at least 2 columns
    if (m_nCols < 2 || del_col < 0 || del_col >= m_nCols)
        return FALSE;

    // Make sure no deleted views are active
	int rowActive, colActive;
	if (GetActivePane(&rowActive, &colActive) != NULL && colActive == del_col)
	{
		if (++colActive >= m_nCols)
			colActive = 0;
		SetActivePane(rowActive, colActive);
	}

	int col, row;

	if (del_views)
        for (row = 0; row < m_nRows; ++row)
		{
			CWnd *pw = GetDlgItem(IdFromRowCol(row, del_col));
			if (pw != NULL)
				DeleteView(row, del_col);
		}
#ifdef _DEBUG
	else
        for (row = 0; row < m_nRows; ++row)
			ASSERT(GetDlgItem(IdFromRowCol(row, del_col)) == NULL);  // Make sure there is no view
#endif

    for (col = del_col+1; col < m_nCols; ++col)
	{
		for (row = 0; row < m_nRows; ++row)
			if (GetDlgItem(IdFromRowCol(row, col)) != NULL)
                GetPane(row, col)->SetDlgCtrlID(IdFromRowCol(row, col-1));
		m_pColInfo[col-1] = m_pColInfo[col];
	}
	m_pColInfo[col-1].nCurSize = -1; // Mark as not present

    m_nCols--;
	ASSERT(m_nCols < m_nMaxCols);

    return TRUE;
}

int CHexEditSplitter::FindViewColumn(HWND hWndView) const
{
	ASSERT_VALID (this);

	for (int ii = 0; ii < m_nCols; ii++)
	{
		if (GetPane(0, ii)->GetSafeHwnd() == hWndView)
			return ii;
	}

	return -1;
}


/////////////////////////////////////////////////////////////////////////////
// CHexEditSplitter message handlers

