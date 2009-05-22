#if !defined(AFX_HEXEDITSPLITTER_H__11F459C9_76E4_406C_B00C_D4BDA5B10D01__INCLUDED_)
#define AFX_HEXEDITSPLITTER_H__11F459C9_76E4_406C_B00C_D4BDA5B10D01__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// HexEditSplitter.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CHexEditSplitter window

class CHexEditSplitter : public CSplitterWnd
{
	DECLARE_DYNAMIC(CHexEditSplitter)

// Operations
public:
    BOOL InsColumn(int col, int width, int min_width, CRuntimeClass *pViewClass = NULL, CCreateContext *pContext = NULL);
    BOOL DelColumn(int col, BOOL del_views = FALSE);

	// Search all columns (row 0) for a view - returns the column number or -1 if not found
	int FindViewColumn(HWND) const;

	int GetMaxRows() { return m_nMaxRows; }
	int GetMaxCols() { return m_nMaxCols; }
	
	int ColWidth(int col)
	{
	    int cur, min;
	    GetColumnInfo(col, cur, min);
	    return cur;
	}
	int RowHeight(int row)
	{
	    int cur, min;
	    GetRowInfo(row, cur, min);
	    return cur;
	}

    // Can do rows similarly but not yet needed

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHexEditSplitter)
	//}}AFX_VIRTUAL


	// Generated message map functions
protected:
	//{{AFX_MSG(CHexEditSplitter)
	//}}AFX_MSG
	// afx_msg void OnSize(UINT nType, int cx, int cy);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HEXEDITSPLITTER_H__11F459C9_76E4_406C_B00C_D4BDA5B10D01__INCLUDED_)
